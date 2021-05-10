#pragma once
#include <coalpy.tasks/TaskDefs.h>

namespace coalpy
{

class ITaskSystem
{
public:
    static ITaskSystem* create(const TaskSystemDesc& desc);
    virtual ~ITaskSystem() {}
    virtual Task createTask(const TaskDesc& taskDesc) = 0;
    virtual void depends(Task src, Task dst) = 0;
    virtual void depends(Task src, Task* dsts, int counts) = 0;
    virtual void wait(Task other) = 0;
};

namespace TaskUtil
{
    void yieldUntil(TaskPredFn fn);
    void wait(Task other);
}

}
