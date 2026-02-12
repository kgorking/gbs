module;
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <memory>
#include <queue>
#include <thread>
#include <vector>
export module task_graph;
import task;
import thread_pool;

export class task_graph {
public:
	explicit task_graph(size_t threads = std::thread::hardware_concurrency()) : pool(threads) {}

	task_ptr create_task(std::filesystem::path const& name, std::function<void()> work) {
		task_ptr t = std::make_shared<task>();
		if (!name.empty())
			task_names[name] = t;
		t->work = std::move(work);
		tasks.push_back(t);
		return t;
	}

	// Add edge: parent -> child (child depends on parent)
	void add_dependency(const task_ptr& parent, const task_ptr& child) {
		child->deps.fetch_add(1, std::memory_order_relaxed);
		parent->children.push_back(child);
	}

	task_ptr find_task(std::filesystem::path const& name) const {
		if (task_names.contains(name))
			return task_names.at(name);
		return {};
	}

	void run() {
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

private:
	void schedule_ready_tasks() {
		for (;;) {
			task_ptr t;
			{
				std::lock_guard<std::mutex> lock(ready_mtx);
				if (ready.empty())
					break;
				
				t = ready.front();
				ready.pop();
			}
			
			pool.enqueue([this, t] {
				t->work();

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
				
				// Decrement global remaining counter
				if (remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) {
					std::lock_guard<std::mutex> lock(done_mtx);
					done_cv.notify_all();
				}
				});
		}
	}

private:
	std::queue<task_ptr> ready;
	std::vector<task_ptr> tasks;
	std::unordered_map<std::filesystem::path, task_ptr> task_names;
	std::atomic<int> remaining{ 0 };
	std::mutex ready_mtx;
	std::mutex done_mtx;
	std::condition_variable done_cv;
	thread_pool pool;
};
