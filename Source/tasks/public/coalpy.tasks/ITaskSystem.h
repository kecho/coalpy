#pragma once
#include <coalpy.tasks/TaskDefs.h>

namespace coalpy
{

class ITaskSystem
{
public:
    static ITaskSystem* create(const TaskSystemDesc& desc);
    virtual ~ITaskSystem() {}
    virtual void start() = 0;
    virtual void signalStop() = 0;
    virtual void join() = 0;
    virtual Task createTask(const TaskDesc& taskDesc, void* data = nullptr) = 0;
    virtual void depends(Task src, Task dst) = 0;
    virtual void depends(Task src, Task* dsts, int counts) = 0;
    virtual void wait(Task other) = 0;
    virtual void execute(Task task) = 0;
    virtual void execute(Task* tasks, int counts) = 0;
};

namespace TaskUtil
{
    void yieldUntil(TaskPredFn fn);
    void wait(Task other);
}

}
