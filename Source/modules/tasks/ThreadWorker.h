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
    ThreadWorker();
    ~ThreadWorker();
    
    void setId(int workerId) { m_workerId = workerId; }
    void start(OnTaskCompleteFn onTaskCompleteFn = nullptr);
    void schedule(TaskFn fn, TaskContext& payload);
    bool stealJob(TaskFn& fn, TaskContext& payload);
    void runInThread(TaskFn fn, TaskContext& payload);
    void signalStop();
    void join();
    int queueSize() const;
    void waitUntil(TaskBlockFn fn);
    static ThreadWorker* getLocalThreadWorker();
private:
    void run();
    void auxLoop();
    std::thread* m_thread = nullptr;
    ThreadWorkerQueue* m_queue = nullptr;
    std::thread* m_auxThread = nullptr;
    ThreadWorkerQueue* m_auxQueue = nullptr;
    OnTaskCompleteFn m_onTaskCompleteFn = nullptr;
    int m_activeDepth = 0;
    int m_workerId = -1;
};

}
