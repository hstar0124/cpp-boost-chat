#pragma once
#include "Common.h"

template<typename T>
class ThreadSafeQueue
{
private:
    std::mutex queueMutex;              // ť�� ���� ���ؽ�
    std::queue<T> queue;                // �����͸� �����ϴ� ť
    std::condition_variable cvBlocking; // ���ŷ�� ���� ���� ����
    std::mutex muxBlocking;             // ���ŷ�� ���� ���ؽ�

public:
    // �⺻ ������
    ThreadSafeQueue() = default;

    // ���� �����ڿ� ���� �����ڸ� �����Ͽ� ���縦 ����
    ThreadSafeQueue(const ThreadSafeQueue<T>&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue<T>&) = delete;

    // �Ҹ��ڿ��� ��� �����͸� ������
    ~ThreadSafeQueue() { Clear(); }

    // ť�� �� �� ���Ҹ� ��ȯ
    const T& Front()
    {
        std::scoped_lock lock(queueMutex);
        return queue.front();
    }

    // ť�� �� �� ���Ҹ� ��ȯ
    const T& Back()
    {
        std::scoped_lock lock(queueMutex);
        return queue.back();
    }

    // ť���� ���Ҹ� �����ϰ� ��ȯ
    T Pop()
    {
        std::scoped_lock lock(queueMutex);
        auto t = std::move(queue.front());
        queue.pop();
        return t;
    }

    // ť�� ���Ҹ� �߰�
    void PushBack(const T& item)
    {
        std::scoped_lock lock(queueMutex);
        queue.push(item);

        // ���Ұ� �߰��Ǹ� ���ŷ�� �����ϱ� ���� ���� ������ ����
        std::unique_lock<std::mutex> ul(muxBlocking);
        cvBlocking.notify_one();
    }

    // ť�� ����ִ��� ���θ� ��ȯ
    bool Empty()
    {
        std::scoped_lock lock(queueMutex);
        return queue.empty();
    }

    // ť�� �ִ� ������ ������ ��ȯ
    size_t Count()
    {
        std::scoped_lock lock(queueMutex);
        return queue.size();
    }

    // ť�� ��� ���Ҹ� ����
    void Clear()
    {
        std::scoped_lock lock(queueMutex);
        while (!queue.empty())
            queue.pop();
    }

    // ť�� ������� ������ ��ٸ�
    void Wait()
    {
        while (Empty())
        {
            // ť�� ������� ��� ���ŷ ���·� ����ϸ� CPU �ڿ��� ������
            std::unique_lock<std::mutex> ul(muxBlocking);
            cvBlocking.wait(ul);
        }
    }
};