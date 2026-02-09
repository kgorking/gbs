module;
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
export module thread_pool;
import task;

export class thread_pool{
public:
	thread_pool(size_t n = std::thread::hardware_concurrency()) {
		for (size_t i = 0; i < n; ++i) {
			workers.emplace_back([this] {
				for (;!stop;) {
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

	~thread_pool() {
		{
			std::scoped_lock lock(mtx);
			stop = true;
		}
		cv.notify_all();
		for (auto& t : workers)
			t.join();
	}
	
	void enqueue(std::function<void()> job) {
		{
			std::lock_guard lock(mtx);
			jobs.push(std::move(job));
		}
		cv.notify_one();
	}

private:
	std::vector<std::thread> workers;
	std::queue<std::function<void()>> jobs;
	std::mutex mtx;
	std::condition_variable cv;
	bool stop = false;
};
