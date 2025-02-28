#pragma once
#include <filesystem>
#include <string_view>
#include <unordered_map>
#include <vector>

struct compiler {
	int major, minor;
	std::string_view name;
	std::string_view arch;
	std::filesystem::path dir;
	std::filesystem::path inc;
	std::filesystem::path lib;
	std::filesystem::path exe;
};

using compiler_collection = std::unordered_map<std::string_view, std::vector<compiler>>;
using fn_callback = void(*)(compiler&&);

//void enumerate_compilers(fn_callback);
void fill_compiler_collection();
compiler get_compiler(std::string_view comp);
bool is_file_up_to_date(std::filesystem::path const& in, std::filesystem::path const& out);
