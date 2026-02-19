#pragma once
#include <filesystem>
#include <generator>
#include <map>
#include <set>
#include <string>

using import_set = std::set<std::string>;

// Holds a single source files module dependencies
using source_info = std::pair<std::filesystem::path, import_set>;

// A map of source files to their module dependencies
using file_to_imports_map = std::map<std::filesystem::path, import_set>;

// A single group of source files that can be compiled in parallel
using source_group = std::map<std::filesystem::path, import_set>;

// A map of source files grouped by their dependency depth
using depth_ordered_sources_map = std::map<std::size_t, source_group>;


bool should_include(std::filesystem::path const& path);

std::generator<std::filesystem::path> get_source_files(std::filesystem::path const& dir);

// Find the source files and dependencies
depth_ordered_sources_map get_grouped_source_files(std::filesystem::path const& dir);
