export module context;
import std;
import compiler;


export struct context {
	// Folder to store gbs related files
	const std::filesystem::path gbs_internal{ ".gbs" };
	const std::filesystem::path gbs_out{ "gbs.out" };

	// Available compilers
	using compiler_collection = std::unordered_map<std::string_view, std::vector<compiler>>;
	compiler_collection all_compilers;
	compiler selected_cl;

	using compiler_response_map = std::unordered_map<std::string_view, std::string_view>;
	std::unordered_map<std::string_view, compiler_response_map> response_map;

	// Config of last compile (debug, release, etc...)
	std::string_view config;

	// Determine output dir, eg. 'gbs.out/msvc/debug
	auto output_dir() const -> std::filesystem::path {
		return gbs_out / selected_cl.name / config;
	}

	// Determine response directory
	auto response_dir() const -> std::filesystem::path {
		return gbs_internal / selected_cl.name;
	}

	// Selects the first compiler in the list
	void select_first_compiler() {
		selected_cl = all_compilers.begin()->second.front();
	}

	// Returns the name of the currently selected compiler
	std::string_view compiler_name() const noexcept {
		return selected_cl.name;
	}

	// Create build command for the currently selected compiler
	std::string build_command_prefix() const {
		auto const compiler = selected_cl.compiler.string();
		auto const out = output_dir().string();
		return std::vformat(selected_cl.build_command_prefix, std::make_format_args(compiler, out));
	}

	// Create build args for a single file
	std::string build_file(std::string_view file, std::string_view obj_file) const {
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
};

export void fill_compiler_collection(context& ctx) {
	ctx.all_compilers.clear();
	enumerate_compilers([&](compiler&& c) {
		ctx.all_compilers[c.name].push_back(std::forward<compiler>(c));
		});

	// Sort compilers from highest version to lowest
	for (auto& kv : ctx.all_compilers) {
		std::ranges::sort(kv.second, [](compiler const& c1, compiler const& c2) {
			if (c1.major == c2.major)
				return c1.minor > c2.minor;
			else
				return c1.major > c2.major;
			});
	}

	// Patch up clang compilers to use msvc std module on windows
#ifdef _MSC_VER
	
	if(ctx.all_compilers.contains("clang") && ctx.all_compilers.contains("msvc")) {
		auto const link = ctx.output_dir() / "clang/std.cppm";
		if (!std::filesystem::exists(link)) {
			std::filesystem::create_directories(link.parent_path());

			// Get the newest msvc compiler
			compiler const& msvc_compiler = ctx.all_compilers["msvc"].front();

			// Copy the msvc std module
			auto const target = std::filesystem::path(msvc_compiler.std_module);
			std::filesystem::copy_file(target, link);

			// Update the clang compilers to use the msvc std module
			auto& clang_compilers = ctx.all_compilers["clang"];
			for (auto& c : clang_compilers) {
				c.std_module = link;
			}
		}
	}
#endif
}

export std::optional<compiler> get_compiler(context const& ctx, std::string_view comp) {
	auto split = comp | std::views::split(':'); // cl:version:arch
	std::string_view cl, version, arch;

	switch (std::ranges::distance(split)) {
	case 1:
		// No version requested, returns newest
		cl = comp;
		break;

	case 2:
		// Version requested
		cl = std::string_view{ *split.begin() };
		version = std::string_view{ *std::next(split.begin()) };
		break;

	case 3:
		// Version and architecture requested
		cl = std::string_view{ *split.begin() };
		version = std::string_view{ *std::next(split.begin()) };
		arch = std::string_view{ *std::next(split.begin(), 2) };
		break;

	default:
		std::println("<gbs>   Error: ill-formed compiler descriptor: {}", comp);
		exit(1);
	}


	if (!ctx.all_compilers.contains(cl)) {
		return {};
	}

	// Select the compiler
	auto const& named_compilers = ctx.all_compilers.at(cl);
	if (version.empty())
		return named_compilers.front();

	// Select the version
	int major = 0, minor = 0;
	extract_compiler_version(version, major, minor);

	auto version_compilers = named_compilers | std::views::filter([major, minor](compiler const& c) {
		if (c.major == major)
			return c.minor >= minor;
		return false;// c.major >= major;
		});

	if (version_compilers.empty()) {
		std::println("<gbs>   Error: requested version not found");
		return {};
	}

	if (arch.empty())
		return version_compilers.front();

	// Select architecture
	auto arch_compilers = version_compilers | std::views::filter([arch](compiler const& c) {
		return arch == c.arch;
		});

	if (arch_compilers.empty()) {
		std::println("<gbs>   Error: A compiler with architecture '{}' was not found.", arch);
		return {};
	}

	return arch_compilers.front();
}
