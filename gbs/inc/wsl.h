#pragma once
#include <optional>
#include <string>
#include <vector>

std::vector<std::string> get_wsl_distributions();
std::string get_wsl_command(std::optional<std::string> const& distro);
