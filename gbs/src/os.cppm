module;
#include <format>
#include <stdexcept>
#include <string_view>
export module os;

export enum class operating_system {
	windows,
	linux,
	macos
};

export bool is_target_triple_windows(std::string_view triple) {
	return triple.contains("-windows-") || triple.contains("-w64-");
}
export bool is_target_triple_linux(std::string_view triple) {
	return triple.contains("-linux-");
}
export bool is_target_triple_macos(std::string_view triple) {
	return triple.contains("-apple-");
}

export operating_system os_from_target_triple(std::string_view triple) {
	if (is_target_triple_windows(triple))
		return operating_system::windows;
	else if (is_target_triple_linux(triple))
		return operating_system::linux;
	else if (is_target_triple_macos(triple))
		return operating_system::macos;
	else
		throw std::runtime_error("Unknown target OS in triple: " + std::string(triple));
}

// Get properly named executable for the target platform
export [[nodiscard]] std::string os_get_executable_name(operating_system const target_os, std::string_view base_name) {
	switch (target_os) {
	case operating_system::windows: return std::vformat("{}.exe", std::make_format_args(base_name));
	case operating_system::linux:
	case operating_system::macos:  return std::string{ base_name };
	default: throw std::runtime_error("Unsupported OS");
	}
}

// Get properly named dynamic library for the target platform
export [[nodiscard]] std::string os_get_dynamic_library_name(operating_system const target_os, std::string_view base_name) {
	switch (target_os) {
	case operating_system::windows: return std::vformat("{}.dll", std::make_format_args(base_name));
	case operating_system::linux: return std::vformat("lib{}.so", std::make_format_args(base_name));
	case operating_system::macos: return std::vformat("lib{}.dylib", std::make_format_args(base_name));
	default: throw std::runtime_error("Unsupported OS");
	}
}

// Get properly named static library for the target platform
export [[nodiscard]] std::string os_get_static_library_name(operating_system const target_os, std::string_view base_name) {
	switch (target_os) {
	case operating_system::windows: return std::vformat("{}.lib", std::make_format_args(base_name));
	case operating_system::linux:
	case operating_system::macos: return std::vformat("lib{}.a", std::make_format_args(base_name));
	default: throw std::runtime_error("Unsupported OS");
	}
}

export consteval bool is_host_windows() {
#ifdef _WIN32
	return true;
#else
	return false;
#endif
}

export consteval bool is_host_linux() {
#ifdef __linux__
	return true;
#else
	return false;
#endif
}
export consteval bool is_host_macos() {
#ifdef __APPLE__
	return true;
#else
	return false;
#endif
}