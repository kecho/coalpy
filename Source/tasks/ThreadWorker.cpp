#include "ThreadWorker.h"
#include "ThreadQueue.h"
#include <coalpy.core/Assert.h>
#include <thread>

namespace coalpy
{

enum class ThreadMessageType
{
    Exit,
    Signal,
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
    signalStop();
    join();
    delete m_thread;
    delete m_queue;
}

void ThreadWorker::start(OnTaskCompleteFn onTaskCompleteFn)
{
    CPY_ASSERT_MSG(m_thread == nullptr, "system must call signalStop and then join to restart the thread worker.");
    if (m_thread)
        return;

    m_onTaskCompleteFn = onTaskCompleteFn;
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
    bool active = true;
    while (active)
    {
        ThreadWorkerMessage msg;
        m_queue->waitPop(msg);

        switch (msg.type)
        {
        case ThreadMessageType::RunJob:
        {
            if (msg.fn)
                msg.fn(msg.ctx);

            if (m_onTaskCompleteFn)
                m_onTaskCompleteFn(msg.ctx.task);
            break;
        }
        case ThreadMessageType::Exit:
        default:
            active = false;
        }
    }
}

void ThreadWorker::signalStop()
{
    if (!m_thread)
        return;

    ThreadWorkerMessage exitMessage { ThreadMessageType::Exit, {}, {} };
    m_queue->push(exitMessage);
}

void ThreadWorker::join()
{
    if (!m_thread)
        return;

    m_thread->join();
    delete m_thread;
    m_thread = nullptr;
}

void ThreadWorker::schedule(TaskFn fn, TaskContext& context)
{
    if (!m_thread)
        return;

    ThreadWorkerMessage runMessage { ThreadMessageType::RunJob, fn, context };
    m_queue->push(runMessage);
}

}
