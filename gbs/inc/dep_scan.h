#pragma once
#include <filesystem>
#include <set>
#include <string>
#include "module_desc.h"

class context;

struct source_dependency {
	std::filesystem::path path;
	module_desc export_name{};
	std::set<module_desc> import_names{};

	bool is_export() const noexcept {
		return !export_name.name.empty();
	}
};

source_dependency extract_module_dependencies(context const& ctx, std::filesystem::path path);
