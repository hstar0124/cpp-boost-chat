#pragma once
#include "Common.h"

template<typename T>
class ThreadSafeVector
{
private:
	std::mutex vectorMutex;
	std::vector<T> vector;
	std::condition_variable cvBlocking;
	std::mutex muxBlocking;

public:
	ThreadSafeVector() = default;

	ThreadSafeVector(const ThreadSafeVector<T>&) = delete;
	ThreadSafeVector& operator=(const ThreadSafeVector<T>&) = delete;

	~ThreadSafeVector() { Clear(); }

public:

	void PushBack(const T& item)
	{
		std::scoped_lock lock(vectorMutex);
		vector.push_back(std::move(item));
		std::unique_lock<std::mutex> ul(muxBlocking);
		cvBlocking.notify_one();
	}

	bool PopBack(T& item)
	{
		std::scoped_lock lock(vectorMutex);
		if (vector.empty()) {
			return false;
		}
		item = vector.back();
		vector.pop_back();
		return true;
	}

	void Clear()
	{
		std::scoped_lock lock(vectorMutex);
		vector.clear();
	}

	size_t Count()
	{
		std::scoped_lock lock(vectorMutex);
		return vector.size();
	}

	bool Empty() const
	{
		std::scoped_lock lock(vectorMutex);
		return vector.empty();
	}

	void Wait()
	{
		while (Empty())
		{
			std::unique_lock<std::mutex> ul(muxBlocking);
			cvBlocking.wait(ul);
		}
	}

	typename std::vector<T>::iterator Begin() 
	{
		std::scoped_lock lock(vectorMutex);
		return vector.begin();
	}

	typename std::vector<T>::const_iterator Begin() const 
	{
		std::scoped_lock lock(vectorMutex);
		return vector.begin();
	}

	typename std::vector<T>::iterator End() 
	{
		std::scoped_lock lock(vectorMutex);
		return vector.end();
	}

	typename std::vector<T>::const_iterator End() const 
	{
		std::scoped_lock lock(vectorMutex);
		return vector.end();
	}


	void Erase(const T& value) 
	{
		std::scoped_lock lock(vectorMutex);
		auto it = std::find(vector.begin(), vector.end(), value);
		if (it != vector.end()) {
			vector.erase(it);
		}
	}



};