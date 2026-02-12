module;
#include <filesystem>
#include <optional>
#include <string_view>
export module compiler;
import env;

export struct compiler {
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


export void extract_compiler_version(std::string_view sv, int& major, int& minor, int& patch) {
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
