#pragma once

#include <coalpy.tasks/ITaskSystem.h>
#include <coalpy.core/HandleContainer.h>
#include "ThreadWorker.h"
#include <memory>
#include <set>
#include <shared_mutex>

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
    virtual void execute(Task task);
    virtual void execute(Task* tasks, int counts);
    virtual void cleanFinishedTasks();

protected:
    void onMessageLoop();
    void onScheduleTask(Task* t, int counts);

    enum class TaskState
    {
        Created,
        WaitingOnDeps,
        Scheduled,
        InQueue,
        Finished
    };

    struct TaskData
    {
        TaskDesc desc;
        TaskState state;
        void* data;
        std::set<Task> dependencies;
        std::set<Task> parents;
    };

    bool getTaskData(Task task, TaskData& outData);
    bool getTaskArgData(Task task, TaskDesc& outDesc, void*& outArgData);
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
