#include "ThreadWorker.h"
#include "ThreadQueue.h"
#include <thread>

namespace coalpy
{

enum class ThreadMessageType
{
    Exit,
    RunJob
};

struct ThreadWorkerMessage
{
    ThreadMessageType type;
    TaskFn fn;
    TaskContext ctx;
};

class ThreadWorkerQueue : public ThreadQueue<ThreadWorkerMessage> {};

ThreadWorker::~ThreadWorker()
{
    join();
    delete m_thread;
    delete m_queue;
}

void ThreadWorker::start()
{
    if (m_thread)
        return;

    m_queue = new ThreadWorkerQueue;

    m_thread = new std::thread(
    [this](){
        this->run();
    });
}

int ThreadWorker::queueSize() const
{
    if (!m_queue)
        return 0;
    return m_queue->size();
}

void ThreadWorker::run()
{
    while (!m_finished)
    {
        ThreadWorkerMessage msg;
        m_queue->waitPop(msg);

        switch (msg.type)
        {
        case ThreadMessageType::RunJob:
            //todo: run job here
            break;
        case ThreadMessageType::Exit:
        default:
            m_finished = true;
        }
    }
}

void ThreadWorker::join()
{
    if (!m_thread)
        return;

    ThreadWorkerMessage exitMessage { ThreadMessageType::Exit, {}, {} };
    m_queue->push(exitMessage);
    m_thread->join();
}

void ThreadWorker::schedule(TaskFn fn, TaskContext& context)
{
    if (m_thread)
        return;

    ThreadWorkerMessage runMessage { ThreadMessageType::RunJob, fn, context };
    m_queue->push(runMessage);
}

}
