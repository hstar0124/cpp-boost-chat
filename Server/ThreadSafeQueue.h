#pragma once
#include "Common.h"

template<typename T>
class ThreadSafeQueue
{
public:
    // 기본 생성자
    ThreadSafeQueue() = default;

    // 복사 생성자와 대입 연산자를 삭제하여 복사를 금지
    ThreadSafeQueue(const ThreadSafeQueue<T>&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue<T>&) = delete;

    // 소멸자: 큐를 비워 메모리 누수를 방지
    ~ThreadSafeQueue() { Clear(); }

    const T& Front()
    {
        std::scoped_lock lock(queueMutex);
        return queue.front();
    }

    const T& Back()
    {
        std::scoped_lock lock(queueMutex);
        return queue.back();
    }

    T Pop()
    {
        std::scoped_lock lock(queueMutex);
        auto t = std::move(queue.front());
        queue.pop();
        return t;
    }

    void PushBack(const T& item)
    {
        std::scoped_lock lock(queueMutex);
        queue.push(std::move(item));        

        // 항목을 큐에 추가하면 조건 변수에 데이터가 들어갔다는 것을 알려줘
        // 절전모드에서 일어나게 한다.
        std::unique_lock<std::mutex> ul(muxBlocking);
        cvBlocking.notify_one();
    }

    bool Empty()
    {
        std::scoped_lock lock(queueMutex);
        return queue.empty();
    }

    size_t Count()
    {
        std::scoped_lock lock(queueMutex);
        return queue.size();
    }

    void Clear()
    {
        std::scoped_lock lock(queueMutex);
        while (!queue.empty())
            queue.pop();
    }

    void Wait()
    {
        while (Empty())
        {
            // 큐에 작업할 것이 없으면 절전모드로 진행하여 CPU 자원을 아낄수 있다.
            std::unique_lock<std::mutex> ul(muxBlocking);
            cvBlocking.wait(ul);
        }
    }


private:
    std::mutex queueMutex;              // 큐에 대한 뮤텍스
    std::queue<T> queue;                // 요소를 보유하는 큐
    std::condition_variable cvBlocking;
    std::mutex muxBlocking;
};