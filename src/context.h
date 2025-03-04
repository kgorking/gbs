#pragma once
#include <unordered_map>
#include <string_view>
#include "compiler.h"

struct context {
	// Folder to store gbs related files
	const std::filesystem::path gbs_internal{ ".gbs" };
	const std::filesystem::path gbs_out{ "gbs.out" };

	// Available compilers
	compiler_collection all_compilers;
	compiler selected_cl;

	using compiler_response_map = std::unordered_map<std::string_view, std::string_view>;
	std::unordered_map<std::string_view, compiler_response_map> response_map;

	// Config of last compile (debug, release, etc...)
	std::string_view output_config;

	// Determine output dir, eg. 'gbs.out/msvc/debug
	std::filesystem::path output_dir() const {
		return gbs_out / selected_cl.name / output_config;
	}

	// Determine response directory
	std::filesystem::path response_dir() const {
		return gbs_internal / selected_cl.name;
	}

	// Selects the first compiler in the list
	void select_first_compiler() {
		selected_cl = all_compilers.begin()->second.front();
	}

	// Returns the name of the currently selected compiler
	std::string_view compiler_name() const {
		return selected_cl.name;
	}
};
