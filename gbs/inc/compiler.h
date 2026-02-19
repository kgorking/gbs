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
	std::string_view build_command_prefix;
	std::string_view link_command;
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
};


void extract_compiler_version(std::string_view sv, int& major, int& minor, int& patch);
