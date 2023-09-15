#include "TaskSystem.h"
#include <coalpy.tasks/ThreadQueue.h>
#include <coalpy.core/Assert.h>
#include <chrono>

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
        data.data = taskData;
        data.syncData = new SyncData;
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

    int aliveTasks = 0;
    m_taskTable.forEach([&aliveTasks](Task task, TaskData& data){
        aliveTasks += data.syncData == nullptr ? 0 : 1;
    });

    CPY_ASSERT_FMT(aliveTasks == 0, "%d still alive tasks detected. This will cause memory leaks.", aliveTasks);
}

void TaskSystem::start()
{
    std::unique_lock lock(m_stateMutex);

    CPY_ASSERT_MSG(m_schedulerThread == nullptr, "Task system cannot start, must call signalStop followed by join().");
    if (m_schedulerThread)
        return;

    m_workers.resize(m_desc.threadPoolSize);
    int nextId = 0;
    for (auto& w : m_workers)
    {
        w.setId(nextId++);
        w.start([this](Task t) { this->onTaskComplete(t); });
    }

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
    TaskScheduleMessage msg = {};
    msg.type = TaskScheduleMessageType::RunJob;
    msg.task = task;
    m_schedulerQueue->push(msg);
}

void TaskSystem::execute(Task* tasks, int counts)
{
    TaskScheduleMessage msg = {};
    msg.type = TaskScheduleMessageType::RunJobs;
    msg.tasks.assign(tasks, tasks + counts);
    m_schedulerQueue->push(msg);
}

void TaskSystem::depends(Task src, Task dst)
{
    std::unique_lock lock(m_stateMutex);

    bool hasSrcTask = m_taskTable.contains(src);
    bool hasDstTask = m_taskTable.contains(dst);
    CPY_ASSERT_MSG(hasSrcTask, "Src task must exist");
    CPY_ASSERT_MSG(hasDstTask, "Dst task must exist");
    if (!hasSrcTask || !hasDstTask)
        return;

    TaskData& srcTaskData = m_taskTable[src];
    TaskData& dstTaskData = m_taskTable[dst];
    srcTaskData.initialDependencies.insert(dst);
    srcTaskData.dependencies.insert(dst);
    dstTaskData.parents.insert(src);
}

void TaskSystem::depends(Task src, Task* dsts, int counts)
{
    std::unique_lock lock(m_stateMutex);

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

        srcTaskData.initialDependencies.insert(dstTask);
        srcTaskData.dependencies.insert(dstTask);
        dstTaskData.parents.insert(src);
    }
}

void TaskSystem::onScheduleTask(Task* tasks, int counts)
{
    struct RunCommand
    {
        TaskContext context;
        TaskFn fn;
    };

    for (int i = 0; i < counts; ++i)
    {
        Task t = tasks[i];
        {
            std::unique_lock lock(m_stateMutex);
            std::vector<Task> childTasks;
            if (!m_taskTable.contains(t))
            {
                CPY_ERROR_MSG(false, "Missing task while scheduling it?");
                continue;
            }
        
            auto& taskData = m_taskTable[t];
            if (taskData.syncData->state != TaskState::Unscheduled)
                continue;

            if (!taskData.dependencies.empty())
            {
                taskData.syncData->state = TaskState::Unscheduled;
                for (auto dep : taskData.dependencies)
                {
                    if (m_taskTable[dep].syncData->state == TaskState::Unscheduled)
                        childTasks.push_back(dep);
                }
            }
            else
            {
                taskData.syncData->state = TaskState::InWorker;
                taskData.syncData->workerId = m_nextWorker;
                TaskContext context = { t, taskData.data, this };
                m_workers[m_nextWorker].schedule(taskData.desc.fn, context);
                m_nextWorker = (m_nextWorker + 1) % (int)m_workers.size();
            }
            execute(childTasks.data(), (int)childTasks.size());
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

void TaskSystem::runSingleJob(ThreadWorker& worker)
{
    TaskFn fn;
    TaskContext ctx;
    for (ThreadWorker& otherWorker : m_workers)
    {
        if (otherWorker.stealJob(fn, ctx))
        {
            worker.runInThread(fn, ctx);
            return;
        }
    }
}

void TaskSystem::onTaskComplete(Task t)
{
    std::vector<Task> nextTasks;
    SyncData* syncData = nullptr;
    {
        std::unique_lock lock(m_stateMutex);
        TaskData& taskData = m_taskTable[t];
        syncData = taskData.syncData;
        CPY_ERROR_MSG(syncData != nullptr, "Sync data not found when task has been completed.");
        syncData->state = TaskState::Finished;

        for (auto p : taskData.parents)
        {
            auto& parentTask = m_taskTable[p];
            parentTask.dependencies.erase(t);
            if (parentTask.dependencies.empty() && parentTask.syncData->state == TaskState::Unscheduled)
                nextTasks.push_back(p);
        }

        {
            std::unique_lock lock(m_finishedTasksMutex);
            m_finishedTasksList.insert(t);
        }

        syncData->cv.notify_all();
    }
    execute(nextTasks.data(), (int)nextTasks.size());
}

void TaskSystem::removeTask(Task t)
{
    auto& taskData = m_taskTable[t];
    for (auto p : taskData.parents)
    {
        if (!m_taskTable.contains(p))
            continue;
        auto& pdata = m_taskTable[p];
        pdata.initialDependencies.erase(t);
        pdata.dependencies.erase(t);
    }
    delete taskData.syncData;
    m_taskTable.free(t);
}

void TaskSystem::cleanFinishedTasks()
{
    CPY_ASSERT_MSG(ThreadWorker::getLocalThreadWorker() == nullptr, "cleanFinishedTasks cannot be called from a worker thread.");
    std::unique_lock lock(m_stateMutex);
    std::unique_lock lock2(m_finishedTasksMutex);

    for (auto t : m_finishedTasksList)
        removeTask(t);

    m_finishedTasksList.clear();
}

void TaskSystem::cleanTaskTree(Task src)
{
    std::unique_lock lock(m_stateMutex);
    std::unique_lock lock2(m_finishedTasksMutex);
    std::vector<Task> tasksToClean;
    std::set<Task> erasedTasks;
    tasksToClean.push_back(src);

    while (!tasksToClean.empty())
    {
        auto t = tasksToClean.back();
        tasksToClean.pop_back();
        if (erasedTasks.insert(t).second == false)
            continue;

        auto& tableData = m_taskTable[t];
        for (auto d : tableData.initialDependencies)
            tasksToClean.push_back(d);

        removeTask(t);
        m_finishedTasksList.erase(t);
    }
}

bool TaskSystem::isTaskFinished(Task t)
{
    std::shared_lock lock(m_stateMutex);
    SyncData* syncData = nullptr;
    if (!m_taskTable.contains(t))
    {
        CPY_ASSERT_MSG(false, "Task does not exist");
        return true;
    }
    
    syncData = m_taskTable[t].syncData;
    if (syncData == nullptr)
    {
        CPY_ASSERT_MSG(false, "Task does not exist");
        return true;
    }
    
    if (syncData->state == TaskState::Finished)
        return true;

    return false;
}

void TaskSystem::wait(Task other)
{
    ThreadWorker* worker = ThreadWorker::getLocalThreadWorker();
    if (worker != nullptr)
    {
        bool finished = false;
        while (!isTaskFinished(other))
            runSingleJob(*worker);
    }
    else
    {
        internalWait(other);
    }
}

void TaskSystem::internalWait(Task other)
{
    SyncData* syncData = nullptr;
    {
        std::shared_lock lock(m_stateMutex);
        if (!m_taskTable.contains(other))
        {
            CPY_ASSERT_MSG(false, "Cannot wait for task that does not exist");
            return;
        }

        syncData = m_taskTable[other].syncData;
        CPY_ERROR_MSG(syncData != nullptr, "Error, no sync data found during wait.");
    }

    {
        std::unique_lock lock(syncData->m);
        syncData->cv.wait(lock, [syncData]() { return syncData->state == TaskState::Finished; });
    }
}

void TaskSystem::getStats(ITaskSystem::Stats& outStats)
{
    std::shared_lock lock(m_stateMutex);
    outStats.numElements = m_taskTable.elementsCount();
}

void TaskSystem::yield()
{
    ThreadWorker* localWorker = ThreadWorker::getLocalThreadWorker();
    if (!localWorker)
        return;
    
    runSingleJob(*localWorker);
}


ITaskSystem* ITaskSystem::create(const TaskSystemDesc& desc)
{
    return new TaskSystem(desc);
}

void TaskUtil::sleepThread(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void TaskUtil::yieldUntil(TaskBlockFn fn)
{
    ThreadWorker* localWorker = ThreadWorker::getLocalThreadWorker();
    CPY_ASSERT_MSG(localWorker != nullptr, "Calling TaskUtil::yieldUntil in a non task context is illegal. This function must be called within a tasks callstack.");

    if (!localWorker)
        return;

    localWorker->waitUntil(fn);
}

}
