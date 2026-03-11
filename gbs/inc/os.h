#pragma once
#include <string_view>

enum class operating_system {
	windows,
	linux,
	macos
};

bool is_target_triple_windows(std::string_view triple);
bool is_target_triple_linux(std::string_view triple);
bool is_target_triple_macos(std::string_view triple);

operating_system os_from_target_triple(std::string_view triple);

// Get properly named executable for the target platform
[[nodiscard]] std::string os_get_executable_name(operating_system const target_os, std::string_view base_name);

// Get properly named dynamic library for the target platform
[[nodiscard]] std::string os_get_dynamic_library_name(operating_system const target_os, std::string_view base_name);

// Get properly named static library for the target platform
[[nodiscard]] std::string os_get_static_library_name(operating_system const target_os, std::string_view base_name);

consteval bool is_host_windows() {
#ifdef _WIN32
	return true;
#else
	return false;
#endif
}

consteval bool is_host_linux() {
#ifdef __linux__
	return true;
#else
	return false;
#endif
}
consteval bool is_host_macos() {
#ifdef __APPLE__
	return true;
#else
	return false;
#endif
}