#include "../inc/compiler.h"
#include "../inc/env.h"
#include <print>

void extract_compiler_version(std::string_view sv, int& major, int& minor, int& patch) {
	major = 0;
	minor = 0;
	patch = 0;

	auto dot = sv.find('.');
	major = std::atoi(sv.substr(0, dot).data());
	if (dot == std::string_view::npos)
		return;

	sv.remove_prefix(dot + 1);
	dot = sv.find('.');
	minor = std::atoi(sv.data());
	if (dot == std::string_view::npos)
		return;

	sv.remove_prefix(dot + 1);
	patch = std::atoi(sv.data());
}

// Returns the name of the currently selected compiler
std::string_view compiler::compiler_name() const noexcept {
	return name;
}

std::string compiler::make_include_path(std::filesystem::path const& path) const {
	std::string s = path.generic_string();
	return std::vformat(include, std::make_format_args(s));
}

// Create build command for the currently selected compiler
std::string compiler::build_command_prefix(std::string_view out) const {
	auto const exe = executable.generic_string();
	return std::vformat(build_prefix, std::make_format_args(exe, out));
}

// Create build args for a single file
std::string compiler::build_command(std::string_view file, std::filesystem::path const& obj_file) const {
	std::string_view const build_cmd = (file.ends_with(".cppm") || file.ends_with(".ixx") || file.ends_with(".cc"))
		? build_module
		: build_source;

	auto const str = obj_file.generic_string();
	return std::vformat(build_cmd, std::make_format_args(file, str));
}

// Create link command for the currently selected compiler
std::string compiler::create_link_command(std::string_view exe_name, std::string_view const out_dir) const {
	std::string lnk = linker.generic_string();
	return std::vformat(link_command, std::make_format_args(lnk, out_dir, exe_name));
}

std::string compiler::create_link_library_list(std::string_view out_dir) const {
	return std::format(" @{}/LIBLIST", out_dir);
}

std::string compiler::create_link_libraries() const noexcept {
	return std::string{ link_libraries };
}

// Create library command for the currently selected compiler
std::string compiler::static_library_command(std::string_view const out_name, std::string_view const out_dir) const {
	auto const lib = slib.generic_string();
	return std::vformat(slib_command, std::make_format_args(lib, out_dir, out_name));
}

// Create dynamic library command for the currently selected compiler
std::string compiler::dynamic_library_command(std::string_view const dll_name, std::string_view const lib_name, std::string_view const out_dir) const {
	auto const lib = dlib.generic_string();
	return std::vformat(dlib_command, std::make_format_args(lib, out_dir, dll_name, lib_name));
}

// Create a reference to a module
auto compiler::get_module_directory(std::string_view const out_dir) const -> std::string {
	if (!module_path.empty()) {
		return std::vformat(module_path, std::make_format_args(out_dir));
	}
	return std::string{};
}

std::string compiler::build_define(std::string_view const def) const {
	return std::format(" {}{}", define, def);
}
