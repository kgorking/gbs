#pragma once
#include "task.h"
#include "thread_pool.h"
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <memory>
#include <queue>
#include <thread>
#include <vector>

class task_graph {
public:
	explicit task_graph(size_t threads = std::thread::hardware_concurrency());

	task_ptr create_task(std::filesystem::path const& name, std::function<bool()> work);

	// Add edge: parent -> child (child depends on parent)
	void add_dependency(const task_ptr& parent, const task_ptr& child);

	task_ptr find_task(std::filesystem::path const& name) const;

	void run();

	bool was_aborted() const { return abort; }

private:
	void schedule_ready_tasks();

private:
	std::queue<task_ptr> ready;
	std::vector<task_ptr> tasks;
	std::unordered_map<std::filesystem::path, task_ptr> task_names;
	std::atomic<int> remaining{ 0 };
	std::mutex ready_mtx;
	std::mutex done_mtx;
	std::condition_variable done_cv;
	thread_pool pool;
	bool abort = false;
};
