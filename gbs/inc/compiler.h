#pragma once
#include <filesystem>
#include <optional>
#include <string_view>

struct compiler {
	int major = 0, minor = 0, patch = 0;
	std::string name_and_version;
	std::string_view name;
	std::string_view build_source;
	std::string_view build_module;
	std::string_view build_prefix;
	std::string_view link_command;
	std::string_view link_libraries;
	std::string_view slib_command;
	std::string_view dlib_command;
	std::string_view define;
	std::string_view include;
	std::string_view module_path;

	std::filesystem::path dir;
	std::filesystem::path executable;
	std::filesystem::path linker;
	std::filesystem::path slib;
	std::filesystem::path dlib;
	std::optional<std::filesystem::path> std_module;

	std::optional<std::string> wsl;

	[[nodiscard]] std::string_view compiler_name() const noexcept;
	[[nodiscard]] std::string make_include_path(std::filesystem::path const& path) const;
	[[nodiscard]] std::string build_command_prefix(std::string_view out) const;
	[[nodiscard]] std::string build_define(std::string_view const def) const;
	[[nodiscard]] std::string build_command(std::string_view file, std::filesystem::path const& obj_file) const;
	[[nodiscard]] std::string create_link_command(std::string_view exe_name, std::string_view const out_dir) const;
	[[nodiscard]] std::string create_link_library_list(std::string_view out_dir) const;
	[[nodiscard]] std::string create_link_libraries() const noexcept;
	[[nodiscard]] std::string static_library_command(std::string_view const out_name, std::string_view const out_dir) const;
	[[nodiscard]] std::string dynamic_library_command(std::string_view const dll_name, std::string_view const lib_name, std::string_view const out_dir) const;
	[[nodiscard]] auto get_module_directory(std::string_view const out_dir) const -> std::string;
	[[nodiscard]] std::string build_define(std::string_view const def);
};


void extract_compiler_version(std::string_view sv, int& major, int& minor, int& patch);
