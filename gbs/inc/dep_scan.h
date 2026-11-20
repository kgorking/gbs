#pragma once
#include <filesystem>
#include <set>
#include <string>
#include "module_desc.h"

struct source_dependency {
	std::filesystem::path path;
	module_desc export_name{};
	std::set<module_desc> import_names{};

	bool is_export() const noexcept {
		return !export_name.name.empty();
	}
};

auto extract_module_dependencies(std::filesystem::path path) -> source_dependency;
