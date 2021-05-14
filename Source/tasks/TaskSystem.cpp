#include "TaskSystem.h"
#include "ThreadQueue.h"
#include <coalpy.core/Assert.h>
#include <chrono>
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
    {
        std::unique_lock lock(m_stateMutex);
        TaskData& data = m_taskTable.allocate(outHandle);
        data.desc = taskDesc;
        data.state = TaskState::Created;
        data.data = taskData;
    }

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
    std::unique_lock lock(m_stateMutex);

    signalStop();
    join();
}

void TaskSystem::start()
{
    std::unique_lock lock(m_stateMutex);

    CPY_ASSERT_MSG(m_schedulerThread == nullptr, "Task system cannot start, must call signalStop followed by join().");
    if (m_schedulerThread)
        return;

    m_workers.resize(m_desc.threadPoolSize);
    for (auto& w : m_workers)
        w.start([this](Task t) { this->onTaskComplete(t); });

    m_schedulerThread = std::make_unique<std::thread>([this]() { onMessageLoop(); });
}

void TaskSystem::signalStop()
{
    TaskScheduleMessage msg;
    msg.type = TaskScheduleMessageType::Exit;
    m_schedulerQueue->push(msg);
}

void TaskSystem::join()
{
    if (!m_schedulerThread)
        return;

    m_schedulerThread->join();
    for (auto& w : m_workers)
        w.join();

    m_schedulerThread.release();
}

void TaskSystem::execute(Task task)
{
    TaskScheduleMessage msg;
    msg.type = TaskScheduleMessageType::RunJob;
    msg.task = task;
    {
        std::unique_lock lock(m_stateMutex);
        if (!m_taskTable.contains(task))
        {
            CPY_ERROR_MSG(false, "Could not schedule task!");
            return;
        }

        m_taskTable[task].state = TaskState::Scheduled;
    }
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
    std::shared_lock lock(m_stateMutex);

    bool hasSrcTask = m_taskTable.contains(src);
    bool hasDstTask = m_taskTable.contains(dst);
    CPY_ASSERT_MSG(hasSrcTask, "Src task must exist");
    CPY_ASSERT_MSG(hasDstTask, "Dst task must exist");
    if (!hasSrcTask || !hasDstTask)
        return;

    TaskData& srcTaskData = m_taskTable[src];
    TaskData& dstTaskData = m_taskTable[dst];
    srcTaskData.dependencies.insert(dst);
    dstTaskData.parents.insert(src);
}

void TaskSystem::depends(Task src, Task* dsts, int counts)
{
    std::shared_lock lock(m_stateMutex);

    bool hasSrcTask = m_taskTable.contains(src);
    CPY_ASSERT_MSG(hasSrcTask, "Src task must exist");
    if (!hasSrcTask)
        return;

    TaskData& srcTaskData = m_taskTable[src];
    for (int i = 0; i < counts; ++i)
    {
        Task dstTask = dsts[i];
        bool hasDstTask = m_taskTable.contains(dstTask);
        CPY_ASSERT_MSG(hasDstTask, "Dst task must exist");
        if (!hasDstTask)
            continue;
        
        TaskData& dstTaskData = m_taskTable[dstTask];

        srcTaskData.dependencies.insert(dstTask);
        dstTaskData.parents.insert(src);
    }
}

bool TaskSystem::getTaskData(Task task, TaskData& outData)
{
    std::shared_lock lock(m_stateMutex);
    if (!m_taskTable.contains(task))
        return false;

    outData = m_taskTable[task];
    return true;
}

bool TaskSystem::getTaskArgData(Task task, TaskDesc& outDesc, void*& outData)
{
    std::shared_lock lock(m_stateMutex);
    if (!m_taskTable.contains(task))
        return false;

    auto& td = m_taskTable[task];
    outDesc = td.desc;
    outData = td.data;
    return true;
}

void TaskSystem::onScheduleTask(Task* tasks, int counts)
{
    struct RunCommand
    {
        TaskContext context;
        TaskFn fn;
    };

    TaskContext initialContext = { Task(), nullptr, this };

    for (int i = 0; i < counts; ++i)
    {
        Task t = tasks[i];
        std::vector<RunCommand> runCommands;
        auto insertRunCommand = [&initialContext](std::vector<RunCommand>& outCmds, Task t, TaskData& taskData)
        {
                taskData.state = TaskState::InQueue;
                RunCommand coreCommand = { initialContext, nullptr };
                coreCommand.context.task = t;
                coreCommand.context.data = taskData.data;
                coreCommand.fn = taskData.desc.fn;
                outCmds.push_back(coreCommand);
        };

        {
            std::unique_lock lock(m_stateMutex);
            if (!m_taskTable.contains(t))
            {
                CPY_ERROR_MSG(false, "Missing task while scheduling it?");
                continue;
            }
        
            auto& taskData = m_taskTable[t];
            if (!taskData.dependencies.empty())
            {
                taskData.state = TaskState::WaitingOnDeps;
                for (auto dep : taskData.dependencies)
                {
                    auto& depData = m_taskTable[dep];
                    if (depData.state == TaskState::Created)
                        insertRunCommand(runCommands, dep, depData);
                }
            }
            else
                insertRunCommand(runCommands, t, taskData);
        }

        for (auto& runCmd : runCommands)
        {
            m_workers[m_nextWorker].schedule(runCmd.fn, runCmd.context);
            m_nextWorker = (m_nextWorker + 1) % (int)m_workers.size();
        }
    }
}

void TaskSystem::onMessageLoop()
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
            for (auto& w : m_workers)
                w.signalStop();
            active = false;
            break;
        }
    }
}

void TaskSystem::onTaskComplete(Task t)
{
    std::vector<Task> nextTasks;
    {
        std::unique_lock lock(m_stateMutex);
        TaskData& taskData = m_taskTable[t];
        for (auto p : taskData.parents)
        {
            auto& parentTask = m_taskTable[p];
            parentTask.dependencies.erase(t);
            if (parentTask.dependencies.empty())
                nextTasks.push_back(p);
        }
    }

    {
        std::unique_lock lock(m_finishedTasksMutex);
        m_finishedTasksList.insert(t);
    }

    execute(nextTasks.data(), (int)nextTasks.size());
}

void TaskSystem::cleanFinishedTasks()
{
    std::unique_lock lock(m_stateMutex);
    std::unique_lock lock2(m_finishedTasksMutex);

    for (auto t : m_finishedTasksList)
        m_taskTable.free(t);

    m_finishedTasksList.clear();
}

void TaskSystem::wait(Task other)
{
}

ITaskSystem* ITaskSystem::create(const TaskSystemDesc& desc)
{
    return new TaskSystem(desc);
}

void TaskUtil::sleep(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void TaskUtil::yieldUntil(TaskPredFn fn)
{
}

void TaskUtil::wait(Task other)
{
}

}
