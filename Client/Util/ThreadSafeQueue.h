#pragma once
#include "Common.h"

template<typename T>
class ThreadSafeQueue
{
private:
    std::mutex queueMutex;              // 큐에 대한 뮤텍스
    std::queue<T> queue;                // 데이터를 보관하는 큐
    std::condition_variable cvBlocking; // 블로킹을 위한 조건 변수
    std::mutex muxBlocking;             // 블로킹을 위한 뮤텍스

public:
    // 기본 생성자
    ThreadSafeQueue() = default;

    // 복사 생성자와 대입 연산자를 삭제하여 복사를 막음
    ThreadSafeQueue(const ThreadSafeQueue<T>&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue<T>&) = delete;

    // 소멸자에서 모든 데이터를 제거함
    ~ThreadSafeQueue() { Clear(); }

    // 큐의 맨 앞 원소를 반환
    const T& Front()
    {
        std::scoped_lock lock(queueMutex);
        return queue.front();
    }

    // 큐의 맨 뒤 원소를 반환
    const T& Back()
    {
        std::scoped_lock lock(queueMutex);
        return queue.back();
    }

    // 큐에서 원소를 제거하고 반환
    T Pop()
    {
        std::scoped_lock lock(queueMutex);
        auto t = std::move(queue.front());
        queue.pop();
        return t;
    }

    // 큐에 원소를 추가
    void PushBack(const T& item)
    {
        std::scoped_lock lock(queueMutex);
        queue.push(item);

        // 원소가 추가되면 블로킹을 해제하기 위해 조건 변수를 통지
        std::unique_lock<std::mutex> ul(muxBlocking);
        cvBlocking.notify_one();
    }

    // 큐가 비어있는지 여부를 반환
    bool Empty()
    {
        std::scoped_lock lock(queueMutex);
        return queue.empty();
    }

    // 큐에 있는 원소의 개수를 반환
    size_t Count()
    {
        std::scoped_lock lock(queueMutex);
        return queue.size();
    }

    // 큐의 모든 원소를 제거
    void Clear()
    {
        std::scoped_lock lock(queueMutex);
        while (!queue.empty())
            queue.pop();
    }

    // 큐가 비어있을 때까지 기다림
    void Wait()
    {
        while (Empty())
        {
            // 큐가 비어있을 경우 블로킹 상태로 대기하며 CPU 자원을 절약함
            std::unique_lock<std::mutex> ul(muxBlocking);
            cvBlocking.wait(ul);
        }
    }
};