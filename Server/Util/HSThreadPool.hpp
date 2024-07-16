#pragma once
#include "Common.h"

class HSThreadPool
{
private:
    // ������ Ǯ�� ������ ����
    size_t m_Threads;
    // ������ Ǯ�� ��Ŀ �������
    std::vector<std::thread> m_Workers;

    // �۾� ť
    std::queue<std::function<void()>> m_Jobs;
    // �۾��� �����ϱ� ���� ���� ������ ���ؽ�
    std::condition_variable m_CVJob;
    std::mutex m_JobMutex;

    // ������ Ǯ�� ���߱� ���� �÷���
    bool m_StopAll;

public:
    // ������: �־��� ������ ������ŭ�� ��Ŀ �����带 �����մϴ�.
    HSThreadPool(size_t num_threads)
        : m_Threads(num_threads), m_StopAll(false)
    {
        m_Workers.reserve(m_Threads);

        // �� �����忡 ���� Start() �Լ��� �����ϴ� ���� �Լ��� �����Ͽ� �����带 �����մϴ�.
        for (size_t i = 0; i < m_Threads; ++i)
        {
            m_Workers.emplace_back([this]() { this->Start(); });
        }
    }

    // �Ҹ���: ������ Ǯ�� �����մϴ�.
    ~HSThreadPool()
    {
        // ��� ��Ŀ �����忡�� ���� ��ȣ�� �����ϴ�.
        m_StopAll = true;

        // ��� �����忡�� �۾��� ������ �˸��� ����ϴ�.
        m_CVJob.notify_all();

        // ��� ��Ŀ �����尡 ����� ������ ����մϴ�.
        for (auto& t : m_Workers)
        {
            t.join();
        }
    }

    // �۾��� ť�� �߰��ϰ�, �ش� �۾��� ����� ��ȯ�ϴ� �Լ��Դϴ�.
    template <class F, class... Args>
    auto EnqueueJob(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        if (m_StopAll)
        {
            throw std::runtime_error("ThreadPool is Stoped");
        }

        using return_type = typename std::invoke_result<F, Args...>::type;
        auto job = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> job_result_future = job->get_future();
        {
            std::lock_guard<std::mutex> lock(m_JobMutex);
            m_Jobs.push([job]() { (*job)(); });
        }

        m_CVJob.notify_one();
        return job_result_future;
    }

private:
    // ��Ŀ �����尡 ������ �����ϴ� �Լ��Դϴ�.
    void Start()
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock(m_JobMutex);

            // �۾��� ���� ������ Ǯ�� ����Ǿ����� ����մϴ�.
            m_CVJob.wait(lock, [this]() { return !this->m_Jobs.empty() || m_StopAll; });

            // ������ Ǯ�� ����ǰ� �۾��� ������ �����մϴ�.
            if (m_StopAll && this->m_Jobs.empty())
            {
                return;
            }

            // �۾��� �����ɴϴ�.
            std::function<void()> job = std::move(m_Jobs.front());
            m_Jobs.pop();
            lock.unlock();

            // �۾��� �����մϴ�.
            std::cout << "Worker pop() : threadID -> " << std::this_thread::get_id() << std::endl;
            job();
        }
    }
};