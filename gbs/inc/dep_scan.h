#pragma once
#include <filesystem>
#include <set>
#include <string>

struct source_dependency {
	std::filesystem::path path;
	std::string export_name{};
	std::set<std::string> import_names{};

	bool is_export() const noexcept {
		return !export_name.empty();
	}
};

source_dependency extract_module_dependencies(std::filesystem::path path);
