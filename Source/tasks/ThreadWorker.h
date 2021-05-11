#pragma once

#include <coalpy.tasks/TaskDefs.h>
#include <memory>

namespace std
{
    class thread;
}

namespace coalpy
{

class ThreadWorkerQueue;

class ThreadWorker
{
public:
    ~ThreadWorker();
    void start();
    void schedule(TaskFn fn, TaskContext& payload);
    void signalStop();
    void join();
    int queueSize() const;

private:
    void run();
    std::thread* m_thread;
    ThreadWorkerQueue* m_queue;
};

}
