#ifndef __THREAD_POOL_HPP__
#define __THREAD_POOL_HPP__
#include <cinttypes>
#include <vector>
#include <atomic>
#include <thread>
#include <functional>
#include <mutex>
#include <list>
#include <semaphore.h>
#include <exception>


class ThreadPoolError : public std::exception
{
	public:
		ThreadPoolError() = default;
		ThreadPoolError(const char* const error_message);
		ThreadPoolError(const std::string& error_message);
		ThreadPoolError(const ThreadPoolError& obj);
		ThreadPoolError(ThreadPoolError&& obj);
		virtual ~ThreadPoolError() = default;
		
		ThreadPoolError& operator=(const ThreadPoolError& obj);
		ThreadPoolError& operator=(ThreadPoolError&& obj);
		
		const char* what() const noexcept override;
		
	private:
		std::string what_;
};


class ThreadPool
{
	using Task = std::function<void()>;

	public:
		ThreadPool() = default;
		ThreadPool(const std::size_t size);
		ThreadPool(const ThreadPool& obj) = delete;
		ThreadPool(ThreadPool&& obj) = delete;
		~ThreadPool() = default;
		
		ThreadPool& operator=(const ThreadPool& obj) = delete;
		ThreadPool& operator=(ThreadPool&& obj) = delete;

		void queueTask(const Task& task);
		void queueTask(Task&& task);
		std::size_t size() const;
		bool resize(const std::size_t size);
		bool run();
		bool stop(const bool force = false);  // Stop the threadpool without executing all tasks in the task queue
		bool reset();
		bool isRunning() const;
		
	
	private:
		struct ThreadInfo
		{
			std::thread thread_;
			std::atomic<bool> is_task_ = false;
		};
	
		void worker(ThreadInfo& thread_info);
		void waitForRun() const;  // Waits untill all threads are spawned an running
		void waitForTasks() const;  // Waits until all tasks in task queue are completed
		
		
	private:
		std::atomic<bool> run_ = false;
		std::atomic<std::size_t> running_threads_ = 0;
		std::vector<ThreadInfo> threads_;
		std::list<Task> task_queue_;
		std::mutex mutex_;
		sem_t semaphore_ = {0};
};


inline std::size_t ThreadPool::size() const
{
	return threads_.size();
}


inline bool ThreadPool::isRunning() const 
{
	return run_;
}


#endif

