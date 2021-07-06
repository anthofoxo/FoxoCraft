#pragma once

#include <mutex>
#include <atomic>
#include <condition_variable>
#include <chrono>

class EasyConditionVariable final
{
public:
	EasyConditionVariable() = default;
	~EasyConditionVariable() = default;
	EasyConditionVariable(const EasyConditionVariable&) = delete;
	EasyConditionVariable& operator=(const EasyConditionVariable&) = delete;
	EasyConditionVariable(EasyConditionVariable&&) noexcept = default;
	EasyConditionVariable& operator=(EasyConditionVariable&&) noexcept = default;

	inline void Wait()
	{
		rdy = false;
		std::unique_lock<std::mutex> lck(mtx);
		while (!rdy) cv.wait(lck);
	}

	template<typename _Rep, typename _Period>
	void WaitFor(const std::chrono::duration<_Rep, _Period>& rel_time)
	{
		rdy = false;
		std::unique_lock<std::mutex> lck(mtx);
		while (!rdy) cv.wait_for(lck, rel_time);
	}

	inline void NotifyOne()
	{
		std::unique_lock<std::mutex> lck(mtx);
		rdy = true;
		cv.notify_one();
	}

	inline void NotifyAll()
	{
		std::unique_lock<std::mutex> lck(mtx);
		rdy = true;
		cv.notify_all();
	}
private:
	std::mutex mtx;
	std::condition_variable cv;
	std::atomic_bool rdy = false;
};