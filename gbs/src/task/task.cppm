module;
#include <atomic>
#include <functional>
#include <memory>
#include <vector>
export module task;

export struct task {
	std::function<void()> work;
	std::atomic_int32_t deps = 0;
	std::vector<std::shared_ptr<task>> children;
};

export using task_ptr = std::shared_ptr<task>;
