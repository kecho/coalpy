#pragma once

#include <coalpy.tasks/ITaskSystem.h>
#include <coalpy.core/HandleContainer.h>
#include "ThreadWorker.h"
#include <memory>
#include <vector>

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
    

protected:
    void onMessage();
    void onScheduleTask(Task* t, int counts);

    struct TaskData
    {
        TaskDesc desc;
        void* data;
    };

    TaskSystemDesc m_desc;
    std::unique_ptr<TaskSchedulerQueue> m_schedulerQueue;
    std::unique_ptr<std::thread> m_schedulerThread;
    std::vector<ThreadWorker> m_workers;
    HandleContainer<Task, TaskData> m_taskTable;
    int m_nextWorker;
};

}
