#include "ThreadPool.hpp"


ThreadPoolError::ThreadPoolError(const char* const error_message) :
	what_(error_message)
{

}

ThreadPoolError::ThreadPoolError(const std::string& error_message) :
	what_(error_message)
{

}

ThreadPoolError::ThreadPoolError(const ThreadPoolError& obj) :
	what_(obj.what_)
{
	
}

ThreadPoolError::ThreadPoolError(ThreadPoolError&& obj) :
	what_(std::move(obj.what_))
{

}

ThreadPoolError& ThreadPoolError::operator=(const ThreadPoolError& obj)
{
	if (this != &obj) { what_ = obj.what_; }
	return *this;
}

ThreadPoolError& ThreadPoolError::operator=(ThreadPoolError&& obj)
{
	if (this != &obj) { what_ = std::move(obj.what_); }
	return *this;
}

const char* ThreadPoolError::what() const noexcept
{
	return what_.c_str();
}



ThreadPool::ThreadPool(const std::size_t size) :
	threads_(size)
{

}


bool ThreadPool::resize(const std::size_t size)
{
	if (!run_) {
		threads_ = std::move(std::vector<ThreadInfo>(size));
		return true;
	}
	
	return false;
}


bool ThreadPool::run()
{
	if (!run_)
	{
		if (sem_init(&semaphore_, 0, 0) == -1) 
		{
			semaphore_ = {0};
			return false;
		}
		
		run_ = true;
		for (auto& ti : threads_) {
			ti.thread_ = std::thread(&ThreadPool::worker, this, std::ref(ti));
		}
		
		waitForRun();
		return true;
	}
	
	return false;
}


void ThreadPool::waitForRun() const
{
	const std::size_t threads_size = threads_.size();
	while (running_threads_ != threads_size);
}


void ThreadPool::waitForTasks() const
{
	while (!task_queue_.empty());
}


bool ThreadPool::stop(const bool force)
{
	if (run_)
	{	
		if (!force) {
			waitForTasks();
		}
		
		run_ = false;
		for (size_t i = 0; i < threads_.size(); ++i) 
		{
			if (sem_post(&semaphore_) == -1) {
				return false;
			}
		}
		
		for (auto& ti : threads_) 
		{
			if (ti.thread_.joinable()) 
			{
				ti.thread_.join();
			}
		}
		
		if (sem_destroy(&semaphore_) == -1) {
			return false;
		}
		return true;
	}
	
	return false;
}


bool ThreadPool::reset()
{
	if (!run_) 
	{
		running_threads_ = 0;
		threads_.clear();
		task_queue_.clear();
		semaphore_ = {0};
		
		return true;
	}
	
	return false;
}


void ThreadPool::queueTask(const Task& task)
{
	mutex_.lock();
	task_queue_.push_back(task);
	if (sem_post(&semaphore_) == -1)
	{
		task_queue_.pop_back();
		throw ThreadPoolError("Failed to add task to the task queue");
	}
	mutex_.unlock();
}


void ThreadPool::queueTask(Task&& task)
{
	mutex_.lock();
	task_queue_.emplace_back(std::move(task));
	if (sem_post(&semaphore_) == -1)
	{
		task_queue_.pop_back();
		throw ThreadPoolError("Failed to add task to the task queue");
	}
	mutex_.unlock();
}


void ThreadPool::worker(ThreadInfo& thread_info)
{
	Task task;
	bool throw_error = false;
	running_threads_ += 1;
	
	while (run_)
	{
		if (sem_wait(&semaphore_) == -1) 
		{
			throw_error = true;
			break;
		}
		
		mutex_.lock();
		if (!task_queue_.empty())
		{
			task = std::move(task_queue_.front());
			task_queue_.pop_front();
			mutex_.unlock();
		
			thread_info.is_task_ = true;
			if (task) {
				task();
			}
			
			thread_info.is_task_ = false;
		}
		
		else {
			mutex_.unlock();
		}
	}
	
	running_threads_ -= 1;
	if (throw_error) {
		throw ThreadPoolError("Thread synchronization failed");
	}
}

