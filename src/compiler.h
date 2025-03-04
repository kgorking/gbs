#pragma once
#include <filesystem>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <optional>

struct compiler {
	int major = 0, minor = 0;
	std::string_view name;
	std::string_view arch;
	std::filesystem::path dir;
	std::filesystem::path exe;
	//std::filesystem::path inc;
	//std::filesystem::path lib;
};

using compiler_collection = std::unordered_map<std::string_view, std::vector<compiler>>;
using fn_callback = void(*)(struct context&, compiler&&);

void fill_compiler_collection(struct context&);
std::optional<compiler> get_compiler(struct context const&, std::string_view);  // cl:version:arch
bool is_file_up_to_date(std::filesystem::path const& in, std::filesystem::path const& out);
void extract_version(std::string_view, int&, int&);