#pragma once

#include <coalpy.tasks/ITaskSystem.h>


namespace coalpy
{

class TaskSystem : public ITaskSystem
{
public:
    TaskSystem(const TaskSystemDesc& desc);
    virtual Task createTask(const TaskDesc& taskDesc) override;
    virtual void depends(Task src, Task dst) override;
    virtual void depends(Task src, Task* dsts, int counts) override;
    virtual void wait(Task other) override;

protected:
    TaskSystemDesc m_desc;
};

}
