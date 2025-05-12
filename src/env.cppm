module;
#ifdef _MSC_VER
#include <windows.h>
#endif
#include <filesystem>
#include <string_view>
#include <string>
export module env;

export std::string get_env_value(std::string_view var) {
#ifdef _MSC_VER
	char path[_MAX_PATH];
	std::size_t s = 0;
	getenv_s<_MAX_PATH>(&s, path, var.data());
	path[s] = 0;
	return path;
#else
	return std::getenv(var.data());
#endif
}

export std::filesystem::path get_home_dir() {
	std::filesystem::path homedir = get_env_value("HOME"); // linux
	if (homedir.empty())
		homedir = get_env_value("USERPROFILE"); // windows
	return homedir;
}

export bool is_file_up_to_date(std::filesystem::path const& in, std::filesystem::path const& out) {
	return std::filesystem::exists(out) && (std::filesystem::last_write_time(out) > std::filesystem::last_write_time(in));
}
