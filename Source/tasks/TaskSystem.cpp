#include "TaskSystem.h"
#include "ThreadQueue.h"
#include <thread>

namespace coalpy
{

enum class TaskScheduleMessageType
{
    Exit,
    RunJob,
    RunJobs
};

struct TaskScheduleMessage
{
    TaskScheduleMessageType type;
    Task task;
    std::vector<Task> tasks;
};

class TaskSchedulerQueue : public ThreadQueue<TaskScheduleMessage> {};

Task TaskSystem::createTask(const TaskDesc& taskDesc, void* taskData)
{
    Task outHandle;
    TaskData& data = m_taskTable.allocate(outHandle);
    data.desc = taskDesc;
    data.data = taskData;
    if ((taskDesc.flags & (int)TaskFlags::AutoStart) != 0)
        execute(outHandle);
    return outHandle;
}

TaskSystem::TaskSystem(const TaskSystemDesc& desc)
: m_schedulerQueue(std::make_unique<TaskSchedulerQueue>())
, m_nextWorker(0u)
{
}

TaskSystem::~TaskSystem()
{
    signalStop();
    join();
}

void TaskSystem::start()
{
    m_workers.resize(m_desc.threadPoolSize);
    m_schedulerThread = std::make_unique<std::thread>(
    [this]()
    {
        onMessage();
    });
}

void TaskSystem::signalStop()
{
    TaskScheduleMessage msg;
    msg.type = TaskScheduleMessageType::Exit;
    m_schedulerQueue->push(msg);
    for (auto& w : m_workers)
        w.signalStop();
}

void TaskSystem::join()
{
    if (!m_schedulerThread)
        return;

    m_schedulerThread->join();
    for (auto& w : m_workers)
        w.join();
}

void TaskSystem::execute(Task task)
{
    TaskScheduleMessage msg;
    msg.type = TaskScheduleMessageType::RunJob;
    msg.task = task;
    m_schedulerQueue->push(msg);
}

void TaskSystem::execute(Task* tasks, int counts)
{
    TaskScheduleMessage msg;
    msg.type = TaskScheduleMessageType::RunJobs;
    msg.tasks.assign(tasks, tasks + counts);
    m_schedulerQueue->push(msg);
}

void TaskSystem::depends(Task src, Task dst)
{
}

void TaskSystem::depends(Task src, Task* dsts, int counts)
{
}

void TaskSystem::onScheduleTask(Task* tasks, int counts)
{
    TaskContext ct = { nullptr, this };
    for (int i = 0; i < counts; ++i)
    {
        Task t = tasks[i];
        if (!m_taskTable.contains(t))
            continue;

        TaskData& td = m_taskTable[t];
        ct.data = td.data;
        m_workers[m_nextWorker].schedule(td.desc.fn, ct);
        m_nextWorker = (m_nextWorker + 1) % (int)m_workers.size();
    }
}

void TaskSystem::onMessage()
{
    bool active = true;
    while (active)
    {
        TaskScheduleMessage msg;
        m_schedulerQueue->waitPop(msg);
        switch (msg.type)
        {
        case TaskScheduleMessageType::RunJob:
            onScheduleTask(&msg.task, 1);
            break;
        case TaskScheduleMessageType::RunJobs:
            onScheduleTask(msg.tasks.data(), msg.tasks.size());
            break;
        case TaskScheduleMessageType::Exit:
        default:
            active = false;
            break;
        }
    }
}

ITaskSystem* ITaskSystem::create(const TaskSystemDesc& desc)
{
    return new TaskSystem(desc);
}

void ITaskSystem::wait(Task other)
{
}

void TaskUtil::yieldUntil(TaskPredFn fn)
{
}

void TaskUtil::wait(Task other)
{
}

}
