#pragma once

#include <queue>
#include <condition_variable>
#include <mutex>
#include <chrono>

namespace coalpy
{

template<typename MessageType>
class ThreadQueue
{
public:
    ~ThreadQueue() {}

    int size() const;
    void push(const MessageType& msg);
    void unsafePush(const MessageType& msg);
    bool unsafePop(MessageType& msg);
    void waitPop(MessageType& msg);
    bool waitPopUntil(MessageType& msg, int milliseconds);
    void acquireThread() { m_mutex.lock(); }
    void releaseThread() { m_mutex.unlock(); }

private:
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<MessageType> m_queue;
};

template<typename MessageType>
void ThreadQueue<MessageType>::push(const MessageType& msg)
{
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        unsafePush(msg);
    }
    m_cv.notify_one();
}

template<typename MessageType>
int ThreadQueue<MessageType>::size() const
{
    int sz;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        sz = m_queue.size();
    }
    return sz;
}

template<typename MessageType>
void ThreadQueue<MessageType>::waitPop(MessageType& msg)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [this]() { return !m_queue.empty(); });
    unsafePop(msg);
}

template<typename MessageType>
bool ThreadQueue<MessageType>::waitPopUntil(MessageType& msg, int milliseconds)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait_for(lock, std::chrono::milliseconds(milliseconds), [this]() { return !m_queue.empty(); });
    return unsafePop(msg);
}

template<typename MessageType>
void ThreadQueue<MessageType>::unsafePush(const MessageType& msg)
{
    m_queue.push(msg);
}

template<typename MessageType>
bool ThreadQueue<MessageType>::unsafePop(MessageType& msg)
{
    if (m_queue.empty())
        return false;
    msg = m_queue.front();
    m_queue.pop();
    return true;
}

}
