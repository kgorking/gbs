export module context;
import std;
import env;
import compiler;


export class context {
	using compiler_collection = std::unordered_map<std::string_view, std::vector<compiler>>;
	using compiler_response_map = std::unordered_map<std::string_view, std::string_view>;

	// Configuration of compile ('debug,analyze', etc...)
	std::string_view config;

	// Folder to store gbs related files
	const std::filesystem::path gbs_internal{ ".gbs" };

	// Folder for output generated during compilation
	const std::filesystem::path gbs_out{ "gbs.out" };

	// All available compilers
	compiler_collection all_compilers;

	// The currently selected compiler
	compiler selected_cl;

	// Maps a compiler name to its response files
	std::unordered_map<std::string_view, compiler_response_map> response_map;

	// The response args to use during build
	std::string resp_args;


	environment env;

public:
	context(char const** envp) : env(envp) {
		// Default response files
		response_map["msvc"] = {
			{"warnings",
				"/W4 "         // Baseline reasonable warnings
				"/WX "         // Warnings are errors
				"/w14242 "     // 'identifier': conversion from 'type1' to 'type2', possible loss of data
				"/w14254 "     // 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
				"/w14263 "     // 'function': member function does not override any base class virtual member function
				"/w14265 "     // 'classname': class has virtual functions, but destructor is not virtual instances of this class may not be destructed correctly 
				"/w14287 "     // 'operator': unsigned/negative constant mismatch 
				"/we4289 "     // nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside the for-loop scope 
				"/w14296 "     // 'operator': expression is always 'boolean_value' 
				"/w14311 "     // 'variable': pointer truncation from 'type1' to 'type2' 
				"/w14545 "     // expression before comma evaluates to a function which is missing an argument list 
				"/w14546 "     // function call before comma missing argument list 
				"/w14547 "     // 'operator': operator before comma has no effect; expected operator with side effect 
				"/w14549 "     // 'operator': operator before comma has no effect; did you intend 'operator'? 
				"/w14555 "     // expression has no effect; expected expression with side effect 
				"/w14619 "     // pragma warning: there is no warning number 'number' 
				"/w14640 "     // Enable warning on thread unsafe static member initialization 
				"/w14826 "     // Conversion from 'type1' to 'type2' is sign-extended. This may cause unexpected runtime behavior. 
				"/w14905 "     // wide string literal cast to 'LPSTR' 
				"/w14906 "     // string literal cast to 'LPWSTR' 
				"/w14928 "     // illegal copy-initialization; more than one user-defined conversion has been implicitly applied 
			},
			{"_shared", "/nologo /EHsc /std:c++23preview /fastfail /D_MSVC_STL_HARDENING=1 /D_MSVC_STL_DESTRUCTOR_TOMBSTONES=1"},
			{"debug",   "/Od /MDd"},
			{"release", "/DNDEBUG /O2 /MD"},
			{"analyze", "/external:W4 /external:anglebrackets /analyze:external- /analyze:WX- /analyze:plugin EspXEngine.dll"}
		};

		response_map["clang"] = {
			{"warnings",
				"-Wall "
				"-Werror "
				"-Wextra "                // reasonable and standard
				"-Wshadow "               // warn the user if a variable declaration shadows one from a parent context
				"-Wnon-virtual-dtor "     // warn the user if a class with virtual functions has a non-virtual destructor. This helps catch hard to track down memory errors
				"-Wold-style-cast "       // warn for c-style casts
				"-Wcast-align "           // warn for potential performance problem casts
				"-Wunused "               // warn on anything being unused
				"-Woverloaded-virtual "   // warn if you overload (not override) a virtual function
				"-Wpedantic "             // warn if non-standard C++ is used
				"-Wconversion "           // warn on type conversions that may lose data
				"-Wsign-conversion "      // warn on sign conversions
				"-Wnull-dereference "     // warn if a null dereference is detected
				"-Wdouble-promotion "     // warn if float is implicit promoted to double
				"-Wformat=2 "             // warn on security issues around functions that format output (ie printf)
				"-Wimplicit-fallthrough " // warn on statements that fallthrough without an explicit annotation
			},
			{"_shared",
				"-std=c++2b "
	#ifdef _MSC_VER
			// Needed to use std module
			"-Wno-include-angled-in-module-purview -Wno-reserved-module-identifier"
#endif
		},
		{"debug", "-O0"},
		{"release", "-O3"},
		{"analyze", "--analyze -Wno-unused-command-line-argument"} // ignore -c
		};

		response_map["gcc"] = {
			{"warnings",
				"-Wall "
				"-Werror "
				"-Wextra "                  // reasonable and standard
				"-Wshadow "                 // warn the user if a variable declaration shadows one from a parent context
				"-Wnon-virtual-dtor "       // warn the user if a class with virtual functions has a non-virtual destructor. This helps catch hard to track down memory errors
				"-Wold-style-cast "         // warn for c-style casts
				"-Wcast-align "             // warn for potential performance problem casts
				"-Wunused "                 // warn on anything being unused
				"-Woverloaded-virtual "     // warn if you overload (not override) a virtual function
				"-Wpedantic "               // warn if non-standard C++ is used
				"-Wconversion "             // warn on type conversions that may lose data
				"-Wsign-conversion "        // warn on sign conversions
				"-Wnull-dereference "       // warn if a null dereference is detected
				"-Wdouble-promotion "       // warn if float is implicit promoted to double
				"-Wformat=2 "               // warn on security issues around functions that format output (ie printf)
				"-Wimplicit-fallthrough "   // warn on statements that fallthrough without an explicit annotation
				"-Wmisleading-indentation " // warn if indentation implies blocks where blocks do not exist
				"-Wduplicated-cond "        // warn on 'if/else' chain with duplicated conditions
				"-Wduplicated-branches "    // warn on 'if/else' branches with duplicated code
				"-Wlogical-op "             // warn about logical operations being used where bitwise were probably wanted
				"-Wuseless-cast "           // warn if you perform a cast to the same type
				"-Wsuggest-override "       // warn if an overridden member function is not marked 'override' or 'final'
			},
			{"_shared", "-std=c++2b -fmodules"},
			{"debug", "-O0 -g3"},
			{"release", "-O3"},
			{"analyze", "--analyze"}
		};
	}

	// Get an environment variable
	std::optional<std::string_view> get_env_value(std::string_view var) const {
		return env.get(var);
	}

	// Get the home directory of the user
	std::filesystem::path get_home_dir() const {
		if (auto home = env.get("HOME"); home)
			return *home;
		else
			throw std::runtime_error("Could not locate user home directory in environment variables");
	}

	// Get the internal gbs path
	std::filesystem::path const& get_gbs_internal() const noexcept {
		return gbs_internal;
	}

	// Get the output gbs path
	std::filesystem::path const& get_gbs_out() const noexcept {
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

	std::string_view get_config() const noexcept {
		return config;
	}

	void set_response_args(std::string&& resp) noexcept {
		resp_args = std::forward<std::string>(resp);
	}

	std::string_view get_response_args() const noexcept {
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
	void select_first_compiler() noexcept {
		if (!all_compilers.empty())
			selected_cl = all_compilers.begin()->second.front();
	}

	bool is_compiler_selected() const noexcept {
		return !selected_cl.name.empty();
	}

	compiler const& get_selected_compiler() const noexcept {
		return selected_cl;
	}

	compiler_collection const& get_compiler_collection() const noexcept {
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
		std::string_view const build_cmd = file.ends_with(".cppm")
			? selected_cl.build_module
			: selected_cl.build_source;

		return std::vformat(build_cmd, std::make_format_args(file, obj_file));
	}

	// Create link command for the currently selected compiler
	std::string link_command(std::string_view exe_name, std::string_view const out_dir) const {
		auto const linker = selected_cl.linker.string();
		return std::vformat(selected_cl.link_command, std::make_format_args(linker, out_dir, exe_name));
	}

	// Create library command for the currently selected compiler
	std::string static_library_command(std::string_view const out_name, std::string_view const out_dir) const {
		auto const lib = selected_cl.slib.string();
		return std::vformat(selected_cl.slib_command, std::make_format_args(lib, out_dir, out_name));
	}

	// Create library command for the currently selected compiler
	std::string dynamic_library_command(std::string_view const out_name, std::string_view const out_dir) const {
		auto const lib = selected_cl.dlib.string();
		return std::vformat(selected_cl.dlib_command, std::make_format_args(lib, out_dir, out_name));
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
		enumerate_compilers(env, [&](compiler&& c) {
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

	bool ensure_response_file_exists(std::string_view resp) {
		if (resp.empty()) {
			std::println("<gbs> Error: bad build-arguments. Trailing comma?");
			return false;
		}

		auto const& map = get_response_map();

		// Check that it is a valid response file
		if (!std::filesystem::exists(response_dir() / resp)) {
			if (!map.contains(resp)) {
				std::println("<gbs> Error: unknown response file {}", resp);
				return false;
			}
			else {
				std::ofstream file(response_dir() / resp);
				file << map.at(resp);
			}
		}

		return true;
	}

	bool check_response_files(std::string_view args) {
		if (!is_compiler_selected()) {
			std::println("<gbs> Error: select a compiler");
			std::exit(1);
		}

		if (!has_response_map()) {
			std::println("<gbs> Error: selected compiler does not have any default response files");
			std::exit(1);
		}

		if (!std::filesystem::exists(response_dir())) {
			std::filesystem::create_directories(response_dir());
		}

		if (!ensure_response_file_exists("_shared"))
			return false;

		for (auto subrange : args | std::views::split(',')) {
			if (!ensure_response_file_exists(std::string_view{ subrange }))
				return false;
		}

		return true;
	}
};
