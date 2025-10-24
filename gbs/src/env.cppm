export module env;
import std;

export class environment {
	std::unordered_map<std::string_view, std::string_view> vars{};

public:
	explicit environment(char const** envp) {
		if(envp == nullptr)
			throw std::runtime_error("Environment pointer is null");

		//[[gsl::suppress("bounds.1")]] // warning C26481: Don't use pointer arithmetic. Use span instead (bounds.1).
		for(char const* const* var = envp; var != nullptr && *var != nullptr; ++var) {
			std::string_view const ev{ *var };
			vars[ev.substr(0, ev.find('='))] = ev.substr(ev.find('=') + 1);
		}

		if (!vars.contains("HOME")) {
			if (vars.contains("USERPROFILE"))
				vars["HOME"] = vars["USERPROFILE"];
			else
				throw std::runtime_error("Could not locate user home directory in environment variables");
		}
	}

	// Get an environment variable
	[[nodiscard]] std::optional<std::string_view> get(std::string_view const var) const {
		if (vars.contains(var))
			return vars.at(var);
		else
			return {};
	}

	// Get the home directory of the user
	[[nodiscard]] std::filesystem::path get_home_dir() const {
		return vars.at("HOME");
	}
};
