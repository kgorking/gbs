#pragma once
#include <filesystem>
#include <optional>
#include <string_view>
#include <unordered_map>

class environment {
	std::unordered_map<std::string_view, std::string_view> vars{};

public:
	explicit environment(char const** envp);

	// Get an environment variable
	[[nodiscard]] std::optional<std::string_view> get(std::string_view const var) const;

	// Get the home directory of the user
	[[nodiscard]] std::filesystem::path get_home_dir() const;
};
