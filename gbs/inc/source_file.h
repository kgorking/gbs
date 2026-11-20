#pragma once

#include "dep_scan.h"
#include <filesystem>
#include <variant>
#include <vector>

using input_file = std::filesystem::path;

struct source_file {
	std::filesystem::path path;
	source_dependency dependencies;
};

struct object_file {
	std::filesystem::path path;
};

struct library_file {
	std::filesystem::path path;
};

struct dynamic_library_file : library_file {
	std::string export_define;
};

struct executable_file {
	std::filesystem::path path;
};

using input_files = std::vector<input_file>;
using source_files = std::vector<source_file>;
using object_files = std::vector<object_file>;

using files = std::variant<input_files, source_files, object_files, library_file, dynamic_library_file, executable_file>;

// c++ -> source_files
source_files parse_cpp_input_file(input_files const& input);

// source_file -> object_files
object_files compile_source_file(source_files const& source);

// object_file -> library_file
library_file archive_object_files(object_files const& objects);

// object_file -> dynamic_library_file
dynamic_library_file link_dynamic_library(object_files const& objects);

// object_file -> executable_file
executable_file link_executable(object_files const& objects);

// file -> file
