#pragma once
#include "Common.h"

template<typename T>
class ThreadSafeQueue
{
private:
    std::mutex queueMutex;              // ť�� ���� ���ؽ�
    std::queue<T> queue;                // ��Ҹ� �����ϴ� ť
    std::condition_variable cvBlocking;
    std::mutex muxBlocking;

public:
    // �⺻ ������
    ThreadSafeQueue() = default;

    // ���� �����ڿ� ���� �����ڸ� �����Ͽ� ���縦 ����
    ThreadSafeQueue(const ThreadSafeQueue<T>&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue<T>&) = delete;

    // �Ҹ���: ť�� ��� �޸� ������ ����
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

        // �׸��� ť�� �߰��ϸ� ���� ������ �����Ͱ� ���ٴ� ���� �˷���
        // ������忡�� �Ͼ�� �Ѵ�.
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
            // ť�� �۾��� ���� ������ �������� �����Ͽ� CPU �ڿ��� �Ƴ��� �ִ�.
            std::unique_lock<std::mutex> ul(muxBlocking);
            cvBlocking.wait(ul);
        }
    }



};