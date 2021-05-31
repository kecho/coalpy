#pragma once

#include <coalpy.tasks/ITaskSystem.h>
#include <coalpy.core/HandleContainer.h>
#include "ThreadWorker.h"
#include <memory>
#include <set>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>

namespace std
{
class thread;
}

namespace coalpy
{

class TaskSchedulerQueue;

class TaskSystem : public ITaskSystem
{
public:
    TaskSystem(const TaskSystemDesc& desc);
    virtual ~TaskSystem();

    virtual Task createTask(const TaskDesc& taskDesc, void* taskData) override;

    virtual void start() override;
    virtual void signalStop() override;
    virtual void join() override;
    virtual void depends(Task src, Task dst) override;
    virtual void depends(Task src, Task* dsts, int counts) override;
    virtual void wait(Task other) override;
    virtual void execute(Task task) override;
    virtual void execute(Task* tasks, int counts) override;
    virtual void cleanFinishedTasks() override;
    virtual void cleanTaskTree(Task src) override;

    void getStats(Stats& outStats) override;

protected:
    void onMessageLoop();
    void runSingleJob(ThreadWorker& worker);
    void onScheduleTask(Task* t, int counts);
    void internalWait(Task other);
    void removeTask(Task t);
    bool isTaskFinished(Task t);

    enum class TaskState
    {
        Unscheduled,
        InWorker,
        Finished
    };

    struct SyncData
    {
        TaskState state = TaskState::Unscheduled;
        int workerId = -1;
        std::mutex m;
        std::condition_variable cv;
    };

    struct TaskData
    {
        TaskDesc desc;
        void* data = nullptr;
        std::set<Task> initialDependencies;
        std::set<Task> dependencies;
        std::set<Task> parents;
        SyncData* syncData = nullptr;

        TaskState state() { return syncData->state; }
    };

    void onTaskComplete(Task task);

    TaskSystemDesc m_desc;
    std::unique_ptr<TaskSchedulerQueue> m_schedulerQueue;
    std::unique_ptr<std::thread> m_schedulerThread;
    std::vector<ThreadWorker> m_workers;

    mutable std::shared_mutex m_stateMutex;
    HandleContainer<Task, TaskData> m_taskTable;

    mutable std::shared_mutex m_finishedTasksMutex;
    std::set<Task> m_finishedTasksList;

    int m_nextWorker;
};

}
