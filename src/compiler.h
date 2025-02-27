#pragma once
#include <string>
#include <string_view>

struct compiler {
	int major, minor;
	std::string_view name;
	std::string_view arch;
	std::string path;
};
