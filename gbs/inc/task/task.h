#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

struct task {
	std::function<bool()> work;
	std::atomic_int32_t deps = 0;
	std::vector<std::shared_ptr<task>> children;
};
using task_ptr = std::shared_ptr<task>;

constexpr auto task_true = [] { return true; };
