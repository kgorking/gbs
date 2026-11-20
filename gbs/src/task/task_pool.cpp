#include "../../inc/task/task_pool.h"

task_pool::task_pool(size_t n) {
	for (size_t i = 0; i < n; ++i) {
		workers.emplace_back([this] {
			for (; !stop;) {
				std::function<void()> job;

				{
					std::unique_lock lock(mtx);
					cv.wait(lock, [&] { return stop || !jobs.empty(); });

					if (stop && jobs.empty())
						return;

					job = std::move(jobs.front());
					jobs.pop();
				}

				job();
			}
			});
	}
}

task_pool::~task_pool() {
	{
		std::scoped_lock lock(mtx);
		stop = true;
	}
	cv.notify_all();
	for (auto& t : workers)
		t.join();
}

void task_pool::enqueue(std::function<void()> job) {
	{
		std::lock_guard lock(mtx);
		jobs.push(std::move(job));
	}
	cv.notify_one();
}
