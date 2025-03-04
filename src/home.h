#pragma once
#include <cstdlib>

inline char const*const get_home_dir() {
	// Find the users folder
	char const* homedir = std::getenv("HOME"); // linux
	if (!homedir)
		homedir = std::getenv("USERPROFILE"); // windows

	return homedir;
}