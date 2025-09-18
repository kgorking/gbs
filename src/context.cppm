export module context;
import std;
import compiler;


export class context {
	// Configuration of compile ('debug,analyze', etc...)
	std::string_view config;

	// Folder to store gbs related files
	const std::filesystem::path gbs_internal{ ".gbs" };
	const std::filesystem::path gbs_out{ "gbs.out" };

	// Available compilers
	using compiler_collection = std::unordered_map<std::string_view, std::vector<compiler>>;
	compiler_collection all_compilers;
	compiler selected_cl;

	friend void init_response_files(context&);
	using compiler_response_map = std::unordered_map<std::string_view, std::string_view>;
	std::unordered_map<std::string_view, compiler_response_map> response_map;
	std::string resp_args;

public:
	std::filesystem::path const& get_gbs_internal() const {
		return gbs_internal;
	}
	std::filesystem::path const& get_gbs_out() const {
		return gbs_out;
	}

	// Set the compile configuration
	void set_config(std::string_view const cfg) {
		auto const old = config;
		config = cfg;

		std::error_code error_code{};
		std::filesystem::create_directories(output_dir(), error_code);
		if (error_code) {
			config = old;
			std::println(std::cerr, "<gbs> Error: could not create output build directory: '{}'", error_code.message());
		}
	}

	std::string_view get_config() const {
		return config;
	}

	void set_response_args(std::string&& resp) {
		resp_args = std::forward<std::string>(resp);
	}

	std::string_view get_response_args() const {
		return resp_args;
	}

	// Determine output dir, eg. 'gbs.out/msvc/debug
	auto output_dir() const -> std::filesystem::path {
		return gbs_out / selected_cl.name_and_version / config;
	}

	// Determine response directory
	auto response_dir() const -> std::filesystem::path {
		return gbs_internal / selected_cl.name;
	}

	// Returns true if the compiler has a response map
	bool has_response_map() const {
		return response_map.contains(selected_cl.name);
	}

	// Returns the response map for the selected compiler
	compiler_response_map get_response_map() const {
		if (response_map.contains(selected_cl.name))
			return response_map.at(selected_cl.name);
		else
			return {};
	}

	// Selects the first compiler in the list
	void select_first_compiler() {
		if (!all_compilers.empty())
			selected_cl = all_compilers.begin()->second.front();
	}

	bool is_compiler_selected() const {
		return !selected_cl.name.empty();
	}

	compiler const& get_selected_compiler() const {
		return selected_cl;
	}

	compiler_collection const& get_compiler_collection() const {
		return all_compilers;
	}

	// Returns the name of the currently selected compiler
	std::string_view compiler_name() const noexcept {
		return selected_cl.name;
	}

	// Create build command for the currently selected compiler
	std::string build_command_prefix() const {
		auto const compiler = selected_cl.executable.string();
		auto const out = output_dir().string();
		return std::vformat(selected_cl.build_command_prefix, std::make_format_args(compiler, out));
	}

	// Create build args for a single file
	std::string build_command(std::string_view file, std::string_view obj_file) const {
		std::string_view const build_cmd = file.ends_with(".cpp")
			? selected_cl.build_source
			: selected_cl.build_module;

		return std::vformat(build_cmd, std::make_format_args(file, obj_file));
	}

	// Create link command for the currently selected compiler
	std::string link_command(std::string_view exe_name) const {
		auto const linker = selected_cl.linker.string();
		auto const out = output_dir().string();
		return std::vformat(selected_cl.link_command, std::make_format_args(linker, out, exe_name));
	}

	// Create a reference to a module
	auto build_reference(std::string_view module_name) const -> std::string {
		auto const out = (output_dir() / module_name).string();
		return std::vformat(selected_cl.reference, std::make_format_args(module_name, out));
	}

	// Create a reference to modules
	auto build_references(std::set<std::string> const& module_names) const -> std::string {
		std::string refs;
		if (!selected_cl.reference.empty()) {
			for (auto const& s : module_names) {
				auto const out = (output_dir() / s).string();
				refs += std::vformat(selected_cl.reference, std::make_format_args(s, out));
			}
		}
		return refs;
	}

	void fill_compiler_collection() {
		all_compilers.clear();
		enumerate_compilers([&](compiler&& c) {
			all_compilers[c.name].push_back(std::forward<compiler>(c));
			});

		// Sort compilers from the highest version to lowest
		for (std::vector<compiler>& compilers : all_compilers | std::views::values) {
			std::ranges::sort(compilers, [](compiler const& c1, compiler const& c2) {
				if (c1.major == c2.major)
					if (c1.minor == c2.minor)
						return c1.patch > c2.patch;
					else
						return c1.minor > c2.minor;
				else
					return c1.major > c2.major;
				});
		}

		// Patch up clang compilers to use msvc std module on windows
	#ifdef _MSC_VER
		if(all_compilers.contains("clang") && all_compilers.contains("msvc")) {
			// Get the newest msvc compiler
			compiler const& msvc_compiler = all_compilers["msvc"].front();
			auto const std_module = std::filesystem::path(*msvc_compiler.std_module);

			for(compiler& clang : all_compilers["clang"]) {
				clang.std_module = std_module;
			}
		}
	#endif
	}

	bool set_compiler(std::string_view comp) {
		auto split = comp | std::views::split(':'); // cl:version:arch
		std::string_view cl, version, arch;

		switch (std::ranges::distance(split)) {
		case 3:
			// Version and architecture requested
			arch = std::string_view{ *std::next(split.begin(), 2) }; [[fallthrough]];
		case 2:
			// Version requested
			version = std::string_view{ *std::next(split.begin()) }; [[fallthrough]];
		case 1:
			// No version requested, returns newest
			cl = std::string_view{ *split.begin() };
			break;

		default:
			std::println("<gbs>   Error: ill-formed compiler descriptor: {}", comp);
			std::exit(1);
		}


		if (!all_compilers.contains(cl)) {
			return false;
		}

		// Select the compiler
		auto const& named_compilers = all_compilers.at(cl);
		if (version.empty()) {
			selected_cl = named_compilers.front();
			return true;
		}

		// Select the version
		int major = 0, minor = 0, patch = 0;
		auto const dots = std::ranges::count(version, '.');
		extract_compiler_version(version, major, minor, patch);

		auto version_compilers = named_compilers | std::views::filter([&](compiler const& c) {
			bool match = true;
			switch (dots) {
			default: throw std::runtime_error("<gbs>   Error: ill-formed compiler version: " + std::string{ version });
			case 2: match = match && (c.patch == patch); [[fallthrough]];
			case 1: match = match && (c.minor == minor); [[fallthrough]];
			case 0: match = match && (c.major == major);
			}
			return match;
			});

		if (version_compilers.empty()) {
			std::println("<gbs>   Error: requested version not found");
			return {};
		}

		if (arch.empty()) {
			selected_cl = version_compilers.front();
			return true;
		}

		// Select architecture
		auto arch_compilers = version_compilers | std::views::filter([arch](compiler const& c) {
			return arch == c.arch;
			});

		if (arch_compilers.empty()) {
			std::println("<gbs>   Error: A compiler with architecture '{}' was not found.", arch);
			return false;
		}

		selected_cl = arch_compilers.front();
		return true;
	}
};
