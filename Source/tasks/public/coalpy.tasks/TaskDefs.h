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


using TaskBlockFn = std::function<void()>;
using TaskFn = std::function<void(TaskContext& ctx)>;
using Task = GenericHandle<unsigned int>;

struct TaskDesc
{
    TaskDesc() : name(""), flags(0), fn(nullptr) {}
    TaskDesc(TaskFn fn) : name(""), flags(0), fn(fn) {}
    TaskDesc(std::string nm, int flags, TaskFn fn) : name(nm), flags(flags), fn(fn) {}
    TaskDesc(std::string nm, TaskFn fn) : name(nm), flags(0), fn(fn) {}

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
