#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include "dep_scan.h"

// A single source file, used to track the path and module dependencies of the file
struct source_file {
	std::filesystem::path path;
	std::filesystem::path object_path;
	std::vector<source_dependency> dependencies;
};
