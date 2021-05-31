#include "ThreadWorker.h"
#include <coalpy.tasks/ThreadQueue.h>
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
    ThreadMessageType type = ThreadMessageType::Exit;
    TaskFn fn = {};
    TaskBlockFn blockFn = {};
    TaskContext ctx = {};
    int targetStack = -1;
};

class ThreadWorkerQueue : public ThreadQueue<ThreadWorkerMessage>
{
public:
    void addInactiveMessage(ThreadWorkerMessage& msg)
    {
        m_inactiveMessages.push_back(msg);
    }

    void recoverInactiveMessages()
    {
        for (const auto& m : m_inactiveMessages)
        {
            push(m);
        }

        m_inactiveMessages.clear();
    }

private:
    std::vector<ThreadWorkerMessage> m_inactiveMessages;
};

thread_local ThreadWorker* t_localWorker = nullptr;

ThreadWorker::ThreadWorker()
{
    m_auxLoopActive = new std::atomic<bool>();
    *m_auxLoopActive = true;
}
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

    delete m_auxLoopActive;
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
        CPY_ASSERT(t_localWorker == nullptr);
        t_localWorker = this;
        m_activeDepth = 0;
        this->run();
        CPY_ASSERT(m_activeDepth == 0);
        t_localWorker = nullptr;
    });

    m_auxThread = new std::thread(
    [this](){
        CPY_ASSERT(t_localWorker == nullptr);
        t_localWorker = this;
        this->auxLoop();
        t_localWorker = nullptr;
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
            }
            break;
        case ThreadMessageType::Exit:
        default:
            {
                if (msg.targetStack == m_activeDepth || msg.targetStack < 0)
                {
                    active = false;
                }
                else
                {
                    m_queue->addInactiveMessage(msg);
                }
            }
        }
    }
}

void ThreadWorker::auxLoop()
{
    *m_auxLoopActive = true;
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

                //send a message to the main thread which is waiting, to wake up and exit the current stack frame (and resume previously asleep work)
                ThreadWorkerMessage response;
                response.type = ThreadMessageType::Exit;
                response.targetStack = msg.targetStack;
                m_queue->push(response);
                break;
            }
        case ThreadMessageType::Exit:
        default:
            active = false;
        }
    }
    *m_auxLoopActive = false;
}

void ThreadWorker::waitUntil(TaskBlockFn fn)
{
    ThreadWorkerMessage msg;
    msg.type = ThreadMessageType::RunAuxLambda;
    msg.blockFn = fn;
    msg.targetStack = m_activeDepth + 1;
    CPY_ASSERT(*m_auxLoopActive);
    m_auxQueue->push(msg);
    ++m_activeDepth;
    run(); //trap and start a new job in the stack until the aux thread is finished.
    --m_activeDepth;

    m_queue->recoverInactiveMessages();
}

void ThreadWorker::signalStop()
{
    if (!m_thread)
        return;

    ThreadWorkerMessage exitMessage;
    exitMessage.type = ThreadMessageType::Exit;
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

    ThreadWorkerMessage runMessage;
    runMessage.type = ThreadMessageType::RunJob;
    runMessage.fn = fn;
    runMessage.ctx = context;
    m_queue->push(runMessage);
}

ThreadWorker* ThreadWorker::getLocalThreadWorker()
{
    return t_localWorker;
}

}
