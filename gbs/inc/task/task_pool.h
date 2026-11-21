#pragma once
#include "task.h"
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class task_pool {
public:
	task_pool() = delete;
	task_pool(task_pool const&) = delete;
	task_pool(task_pool&&) = delete;
	task_pool(size_t n = std::thread::hardware_concurrency()) noexcept;
	~task_pool() noexcept;

	void enqueue(std::function<void()> job);

private:
	std::vector<std::jthread> workers;
	std::queue<std::function<void()>> jobs;
	std::mutex mtx;
	std::condition_variable cv;
	bool stop = false;
};
