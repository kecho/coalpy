#pragma once

#include <coalpy.tasks/TaskDefs.h>

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
    void join();
    int queueSize() const;

private:
    void run();
    std::thread* m_thread = nullptr;
    ThreadWorkerQueue* m_queue = nullptr;
    bool m_finished = false;
};

}
