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
    void waitUntil(TaskBlockFn fn);

private:
    void run();
    void auxLoop();
    std::thread* m_thread = nullptr;
    ThreadWorkerQueue* m_queue = nullptr;
    std::thread* m_auxThread = nullptr;
    ThreadWorkerQueue* m_auxQueue = nullptr;
    OnTaskCompleteFn m_onTaskCompleteFn = nullptr;
};

}
