#pragma once

#include <coalpy.core/GenericHandle.h>
#include <string>
#include <functional>

namespace coalpy
{

class ITaskSystem;

struct TaskSystemDesc
{
    int threadPoolSize = 8u;
};

struct TaskContext
{
    void* data;
    ITaskSystem* ts;
};

enum class TaskFlags : int
{
    AutoStart = 1 << 0
};

using TaskPredFn = std::function<bool()>;
using TaskFn = std::function<void(TaskContext& ctx)>;
using Task = GenericHandle<unsigned int>;

struct TaskDesc
{
    int flags;
    std::string name;
    TaskFn fn;
};

}
