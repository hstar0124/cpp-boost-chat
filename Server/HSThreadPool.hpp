#pragma once
#include "Common.h"


class HSThreadPool
{
private:
	// 총 Worker 쓰레드의 개수.
	size_t m_Threads;
	// Worker 쓰레드를 보관하는 벡터.
	std::vector<std::thread> m_Workers;

	// 할일들을 보관하는 job 큐.
	std::queue<std::function<void()>> m_Jobs;

	// 위의 job 큐를 위한 cv 와 m.
	std::condition_variable m_CVJob;
	std::mutex m_JobMutex;

	// 모든 쓰레드 종료 Flag
	bool m_StopAll;

public:
	HSThreadPool(size_t num_threads)
		: m_Threads(num_threads), m_StopAll(false) // 멤버 초기화 리스트
	{
		m_Workers.reserve(m_Threads); // 스레드 벡터의 용량을 예약

		for (size_t i = 0; i < m_Threads; ++i)
		{
			m_Workers.emplace_back([this]() { this->Start(); });
		}
	}

	~HSThreadPool()
	{
		// 모든 스레드를 종료하도록 설정
		m_StopAll = true;

		// 잠자고 있는 Thread까지 제대로 종료하기 위해 호출
		m_CVJob.notify_all();

		// 모든 스레드가 종료될 때까지 대기
		for (auto& t : m_Workers)
		{
			t.join();
		}
	}


	template <class F, class... Args>
	auto EnqueueJob(F&& f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type> 
	{
		if (m_StopAll)
		{
			throw std::runtime_error("ThreadPool 사용 중지됨");
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

	// Worker 쓰레드 작업 시작
	void Start()
	{
		while (true)
		{
			std::unique_lock<std::mutex> lock(m_JobMutex);

			// 조건 변수를 기다립니다. 작업이 추가되거나 stop_all이 true가 되면 깨어난다.
			m_CVJob.wait(lock, [this]() { return !this->m_Jobs.empty() || m_StopAll; });

			if (m_StopAll && this->m_Jobs.empty()) // stop_all이 true이고 작업 큐가 비어 있으면
			{
				return; // 스레드를 종료합니다.
			}

			// 작업 큐에서 작업을 꺼내어 실행
			std::function<void()> job = std::move(m_Jobs.front());
			m_Jobs.pop();
			lock.unlock(); // 뮤텍스를 해제합니다.

			// 작업을 수행
			std::cout << "Worker pop() : threadID -> " << std::this_thread::get_id() << std::endl;
			job();
		}
	}
};
