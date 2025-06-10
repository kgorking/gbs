module;
#ifdef _MSC_VER
#include <windows.h>
#endif
export module env;
import std;

export std::string get_env_value(std::string_view var) {
#ifdef _MSC_VER
	std::array<char, _MAX_PATH> path;
	path.fill(0);
	std::size_t s = 0;
	getenv_s(&s, path.data(), path.size(), var.data());
	//path[s] = 0;
	return path.data();
#else
	char* val = std::getenv(var.data());
	return val ? val : "";
#endif
}

export std::filesystem::path get_home_dir() {
	std::filesystem::path homedir = get_env_value("HOME"); // linux
	if (homedir.empty())
		homedir = get_env_value("USERPROFILE"); // windows
	return homedir;
}

export bool is_file_up_to_date(std::filesystem::path const& in, std::filesystem::path const& out) {
	if (!std::filesystem::exists(out))
		return false;

	return (std::filesystem::last_write_time(out) > std::filesystem::last_write_time(in));
}
