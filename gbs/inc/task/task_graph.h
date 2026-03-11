#pragma once
#include "task.h"
#include "thread_pool.h"
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <thread>
#include <unordered_set>
#include <vector>

struct task_ptr_hash {
	size_t operator()(const task_ptr& ptr) const {
		return std::hash<task*>()(ptr.get());
	}
};

class task_graph {
public:
	explicit task_graph(size_t threads = std::thread::hardware_concurrency());

	task_ptr create_task(std::filesystem::path const& name, std::function<bool()> work);

	// Add edge: parent -> child (parent runs to completion before child)
	void add_dependency(const task_ptr& parent, const task_ptr& child);

	task_ptr find_task(std::filesystem::path const& name) const;

	void run();

	bool was_aborted() const { return abort; }

private:
	void schedule_ready_tasks();
	std::optional<std::vector<task_ptr>> find_circular_path(const task_ptr& start, const task_ptr& target, std::unordered_set<task_ptr, task_ptr_hash>& visited, std::vector<task_ptr>& path) const;

private:
	std::queue<task_ptr> ready;
	std::vector<task_ptr> tasks;
	std::unordered_map<std::filesystem::path, task_ptr> task_names;
	std::unordered_map<task_ptr, std::filesystem::path, task_ptr_hash> task_name_map;
	std::atomic<int> remaining{ 0 };
	std::mutex ready_mtx;
	std::mutex done_mtx;
	std::condition_variable done_cv;
	thread_pool pool;
	bool abort = false;
};
