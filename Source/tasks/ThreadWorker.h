#pragma once

#include <coalpy.tasks/TaskDefs.h>
#include <memory>
#include <functional>

namespace std
{
    class thread;
}

namespace coalpy
{

class ThreadWorkerQueue;

using OnTaskCompleteFn = std::function<void(Task)>;

class ThreadWorker
{
public:
    ~ThreadWorker();
    void start(OnTaskCompleteFn onTaskCompleteFn = nullptr);
    void schedule(TaskFn fn, TaskContext& payload);
    void signalStop();
    void join();
    int queueSize() const;

private:
    void run();
    std::thread* m_thread = nullptr;
    OnTaskCompleteFn m_onTaskCompleteFn = nullptr;
    ThreadWorkerQueue* m_queue = nullptr;
};

}
