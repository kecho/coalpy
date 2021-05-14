#pragma once

#include <coalpy.core/GenericHandle.h>
#include <string>
#include <functional>

namespace coalpy
{

struct TaskSystemDesc;
struct TaskDesc;
struct TaskContext;
class ITaskSystem;

struct TaskSystemDesc
{
    int threadPoolSize = 8u;
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
    std::string name;
    int flags;
    TaskFn fn;
};

struct TaskContext
{
    Task task;
    void* data;
    ITaskSystem* ts;
};

}
