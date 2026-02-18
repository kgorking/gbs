module;
#include <fstream>
#include <string>
#include <vector>
#include <optional>
#include <ranges>
#include <filesystem>
export module wsl;

export std::vector<std::string> get_wsl_distributions() {
	if (0 != std::system("wsl -l -q > wsl_distros.txt"))
		return {};

	std::vector<std::string> distributions;
	if (auto file = std::ifstream("wsl_distros.txt"); file) {
		std::string distro;

		// WSL outputs in UTF-16 LE, for some *goddamn* reason,
		// so I'm forced to manually read it like a caveman. Unga bunga.
		char c = 0;
		while (file >> c) {
			if (c == '\0') {
				if (!distro.empty()) {
					distributions.push_back(distro);
					distro.clear();
				}
			}
			else {
				distro += c;
			}
			file >> c; // skip null value
		}
	}

	std::filesystem::remove("wsl_distros.txt");
	return distributions;
}

export std::string get_wsl_command(std::optional<std::string> const& distro) {
	if (distro.has_value())
		return "wsl -d " + *distro + " ";
	else
		return "call ";
}