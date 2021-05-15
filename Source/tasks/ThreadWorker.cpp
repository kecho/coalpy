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
    RunJob,
    RunAuxLambda
};

struct ThreadWorkerMessage
{
    ThreadMessageType type;
    TaskFn fn;
    TaskBlockFn blockFn;
    TaskContext ctx;
};

class ThreadWorkerQueue : public ThreadQueue<ThreadWorkerMessage> {};

ThreadWorker::~ThreadWorker()
{
    signalStop();
    join();
    CPY_ASSERT(m_thread == nullptr);
    CPY_ASSERT(m_auxThread == nullptr);
    if (m_queue)
        delete m_queue;

    if (m_auxQueue)
        delete m_auxQueue;
}

void ThreadWorker::start(OnTaskCompleteFn onTaskCompleteFn)
{
    CPY_ASSERT_MSG(m_thread == nullptr, "system must call signalStop and then join to restart the thread worker.");
    if (m_thread)
        return;

    m_onTaskCompleteFn = onTaskCompleteFn;
    if (!m_queue)
        m_queue = new ThreadWorkerQueue;
    if (!m_auxQueue)
        m_auxQueue = new ThreadWorkerQueue;

    CPY_ASSERT(m_thread == nullptr && m_auxThread == nullptr);

    m_thread = new std::thread(
    [this](){
        this->run();
    });

    m_auxThread = new std::thread(
    [this](){
        this->auxLoop();
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

void ThreadWorker::auxLoop()
{
    bool active = true;
    while (active)
    {
        ThreadWorkerMessage msg;
        m_auxQueue->waitPop(msg);

        switch (msg.type)
        {
        case ThreadMessageType::RunAuxLambda:
            {
                CPY_ASSERT(msg.blockFn);
                msg.blockFn(); //this function, which is set internally, usually waits for responses.
                ThreadWorkerMessage response = { ThreadMessageType::Exit, nullptr, {} };
                m_queue->push(msg);
                break;
            }
        case ThreadMessageType::Exit:
        default:
            active = false;
        }
    }
}

void ThreadWorker::waitUntil(TaskBlockFn fn)
{
    ThreadWorkerMessage msg = { ThreadMessageType::RunAuxLambda, {}, fn, {} };
    m_auxQueue->push(msg);
    run(); //trap and start a new job in the stack until the aux thread is finished.
}

void ThreadWorker::signalStop()
{
    if (!m_thread)
        return;

    ThreadWorkerMessage exitMessage { ThreadMessageType::Exit, {}, {}, {} };
    m_queue->push(exitMessage);
    m_auxQueue->push(exitMessage);
}

void ThreadWorker::join()
{
    if (m_thread)
    {
        m_thread->join();
        delete m_thread;
        m_thread = nullptr;
    }

    if (m_auxThread)
    {
        m_auxThread->join();
        delete m_auxThread;
        m_auxThread = nullptr;
    }
}

void ThreadWorker::schedule(TaskFn fn, TaskContext& context)
{
    if (!m_thread)
        return;

    ThreadWorkerMessage runMessage { ThreadMessageType::RunJob, fn, {}, context };
    m_queue->push(runMessage);
}

}
