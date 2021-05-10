#pragma once

#include <queue>
#include <condition_variable>
#include <mutex>

namespace coalpy
{

template<typename MessageType>
class ThreadQueue
{
public:
    ~ThreadQueue();

    int size() const;
    void push(const MessageType& msg);
    void waitPop(MessageType& msg);

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
        m_queue.push(msg);
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
    msg = m_queue.front();
    m_queue.pop();
}

}
