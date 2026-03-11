#include "../../inc/task/task.h"
#include "../../inc/task/task_graph.h"
#include <print>
#include <unordered_set>

task_graph::task_graph(size_t threads) : pool(threads) {}

task_ptr task_graph::create_task(std::filesystem::path const& name, std::function<bool()> work) {
	task_ptr t = std::make_shared<task>();
	if (!name.empty()) {
		task_names[name] = t;
		task_name_map[t] = name;
	}
	t->work = std::move(work);
	tasks.push_back(t);
	return t;
}

std::optional<std::vector<task_ptr>> task_graph::find_circular_path(const task_ptr& start, const task_ptr& target, std::unordered_set<task_ptr>& visited, std::vector<task_ptr>& path) const {
	if (!start)
		return std::nullopt;

	if (start == target) {
		path.push_back(start);
		return path;
	}

	if (visited.contains(start))
		return std::nullopt;

	visited.insert(start);
	path.push_back(start);

	for (const auto& child : start->children) {
		auto result = find_circular_path(child, target, visited, path);
		if (result.has_value())
			return result;
	}

	path.pop_back();
	return std::nullopt;
}

void task_graph::add_dependency(const task_ptr& parent, const task_ptr& child) {
	// Check for circular dependencies
	std::unordered_set<task_ptr> visited;
	std::vector<task_ptr> path;
	auto circular_path = find_circular_path(child, parent, visited, path);

	if (circular_path.has_value()) {
		std::println("<gbs> error: Circular dependency detected! Dependency chain:\n             ");
					               
		// Print the chain from child to parent
		for (size_t i = 0; i < circular_path->size(); ++i) {
			const auto& task = (*circular_path)[i];
			std::string task_name = task_name_map.contains(task) ? task_name_map[task].string() : "<unnamed>";

			if (i < circular_path->size() - 1) {
				std::print("'{}' -> ", task_name);
			} else {
				std::println("'{}'", task_name);
			}
		}

		// Print the edge that would create the cycle
		std::string parent_name = task_name_map.contains(parent) ? task_name_map[parent].string() : "<unnamed>";
		std::println("  '{}' (would create cycle)", parent_name);
		return;
	}

	child->deps.fetch_add(1, std::memory_order_relaxed);
	parent->children.push_back(child);
}

task_ptr task_graph::find_task(std::filesystem::path const& name) const {
	if (task_names.contains(name))
		return task_names.at(name);
	return {};
}

void task_graph::run() {
	// Initialize ready queue with tasks that have no deps
	{
		std::lock_guard<std::mutex> lock(ready_mtx);
		for (auto& t : tasks) {
			if (t->deps.load(std::memory_order_relaxed) == 0) {
				ready.push(t);
			}
		}
	}
		
	// Count remaining tasks
	remaining.store(static_cast<int>(tasks.size()), std::memory_order_relaxed);
		
	// Kick off initial tasks
	schedule_ready_tasks();
		
	// Wait until all tasks are done
	std::unique_lock<std::mutex> lock(done_mtx);
	done_cv.wait(lock, [&] { return remaining.load(std::memory_order_acquire) == 0; });
}

void task_graph::schedule_ready_tasks() {
	while (!abort) {
		task_ptr t;
		{
			std::lock_guard<std::mutex> lock(ready_mtx);
			if (ready.empty())
				break;
				
			t = ready.front();
			ready.pop();
		}

		pool.enqueue([this, t = std::move(t)] {
			if (abort || !t->work()) {
				abort = true;
				remaining = 0;
				std::lock_guard<std::mutex> lock(done_mtx);
				done_cv.notify_all();
				return;
			}
			else {
				for (auto& child : t->children) {
					int old = child->deps.fetch_sub(1, std::memory_order_acq_rel);
					if (old == 1) {
						{
							std::lock_guard<std::mutex> lock(ready_mtx);
							ready.push(child);
						}

						schedule_ready_tasks(); // opportunistically schedule more
					}
				}
			}

			// Decrement global remaining counter
			if (remaining.fetch_sub(1, std::memory_order_acq_rel) <= 1) {
				std::lock_guard<std::mutex> lock(done_mtx);
				done_cv.notify_all();
			}
			});
	}
}
