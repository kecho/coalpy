#include "TaskSystem.h"

namespace coalpy
{

Task TaskSystem::createTask(const TaskDesc& taskDesc)
{
    return Task();
}

TaskSystem::TaskSystem(const TaskSystemDesc& desc)
{
       
}

void TaskSystem::depends(Task src, Task dst)
{
}

void TaskSystem::depends(Task src, Task* dsts, int counts)
{
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
