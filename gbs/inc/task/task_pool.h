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
	task_pool(size_t n = std::thread::hardware_concurrency());
	~task_pool();

	void enqueue(std::function<void()> job);

private:
	std::vector<std::thread> workers;
	std::queue<std::function<void()>> jobs;
	std::mutex mtx;
	std::condition_variable cv;
	bool stop = false;
};
