#pragma once
#include <filesystem>
#include <generator>
#include <map>
#include <set>
#include <string>
//#include "module_desc.h"

class context; // Forward declaration
struct module_desc; // Forward declaration

using import_set = std::set<module_desc>;

// Holds a single source files module dependencies
using source_info = std::pair<std::filesystem::path, import_set>;

// A map of source files to their module dependencies
using file_to_imports_map = std::map<std::filesystem::path, import_set>;


bool should_include(context const& ctx, std::filesystem::path const& path);

std::generator<std::filesystem::path> get_source_files(context const& ctx, std::filesystem::path const& dir);
