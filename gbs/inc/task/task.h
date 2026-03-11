#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

struct task {
	std::function<bool()> work;
	std::atomic_int32_t deps = 0;
	std::vector<std::shared_ptr<task>> children;

	int headcount() const {
		int count = (int)children.size();
		for (std::shared_ptr<task> const& c : children) {
			count += c->headcount();
		}
		return count;
	}
};
using task_ptr = std::shared_ptr<task>;

constexpr auto task_true = [] { return true; };
