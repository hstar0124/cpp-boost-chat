#pragma once
#include "Common.h"

class HSThreadPool
{
private:
    // 스레드 풀의 스레드 개수
    size_t m_Threads;
    // 스레드 풀의 워커 스레드들
    std::vector<std::thread> m_Workers;

    // 작업 큐
    std::queue<std::function<void()>> m_Jobs;
    // 작업을 조정하기 위한 조건 변수와 뮤텍스
    std::condition_variable m_CVJob;
    std::mutex m_JobMutex;

    // 스레드 풀을 멈추기 위한 플래그
    bool m_StopAll;

public:
    // 생성자: 주어진 스레드 개수만큼의 워커 스레드를 생성합니다.
    HSThreadPool(size_t num_threads)
        : m_Threads(num_threads), m_StopAll(false)
    {
        m_Workers.reserve(m_Threads);

        // 각 스레드에 대해 Start() 함수를 실행하는 람다 함수를 생성하여 스레드를 생성합니다.
        for (size_t i = 0; i < m_Threads; ++i)
        {
            m_Workers.emplace_back([this]() { this->Start(); });
        }
    }

    // 소멸자: 스레드 풀을 정리합니다.
    ~HSThreadPool()
    {
        // 모든 워커 스레드에게 종료 신호를 보냅니다.
        m_StopAll = true;

        // 모든 스레드에게 작업이 없음을 알리고 깨웁니다.
        m_CVJob.notify_all();

        // 모든 워커 스레드가 종료될 때까지 대기합니다.
        for (auto& t : m_Workers)
        {
            t.join();
        }
    }

    // 작업을 큐에 추가하고, 해당 작업의 결과를 반환하는 함수입니다.
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
    // 워커 스레드가 실제로 실행하는 함수입니다.
    void Start()
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock(m_JobMutex);

            // 작업이 없고 스레드 풀이 종료되었으면 대기합니다.
            m_CVJob.wait(lock, [this]() { return !this->m_Jobs.empty() || m_StopAll; });

            // 스레드 풀이 종료되고 작업이 없으면 종료합니다.
            if (m_StopAll && this->m_Jobs.empty())
            {
                return;
            }

            // 작업을 꺼내옵니다.
            std::function<void()> job = std::move(m_Jobs.front());
            m_Jobs.pop();
            lock.unlock();

            // 작업을 실행합니다.
            std::cout << "Worker pop() : threadID -> " << std::this_thread::get_id() << std::endl;
            job();
        }
    }
};