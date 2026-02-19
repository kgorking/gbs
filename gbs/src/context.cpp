#include "../inc/context.h"
#include "../inc/enumerate_compilers.h"

context::context(char const** envp) : env(envp) {
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
		{"_shared", "/nologo /EHsc /std:c++23preview /sdl /D_MSVC_STL_HARDENING=1 /D_MSVC_STL_DESTRUCTOR_TOMBSTONES=1"},
		{"debug",   "/Od /MDd"},
		{"release", "/DNDEBUG /O2 /MD"},
		{"analyze", "/external:W0 /external:Ilib /external:anglebrackets /analyze:external- /analyze:WX- /analyze:plugin EspXEngine.dll"}
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
			//"-Wconversion "           // warn on type conversions that may lose data
			//"-Wsign-conversion "      // warn on sign conversions
			"-Wnull-dereference "     // warn if a null dereference is detected
			//"-Wdouble-promotion "     // warn if float is implicit promoted to double
			"-Wformat=2 "             // warn on security issues around functions that format output (ie printf)
			"-Wimplicit-fallthrough " // warn on statements that fallthrough without an explicit annotation
		},
		{"_shared",
			"-std=c++23 "
#ifdef _MSC_VER
		// Needed to use std module
		"-Wno-include-angled-in-module-purview -Wno-reserved-module-identifier "
#else
				"-stdlib=libc++ "
#endif
		},
		{"debug", "-O0"},
		{"release", "-O3"},
		{"analyze", "--analyze -Wno-unused-command-line-argument"} // ignore -c
	};
	response_map["clang.wsl"] = response_map["clang"];

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
			//"-Wconversion "             // warn on type conversions that may lose data
			//"-Wsign-conversion "        // warn on sign conversions
			"-Wnull-dereference "       // warn if a null dereference is detected
			//"-Wdouble-promotion "       // warn if float is implicit promoted to double
			"-Wformat=2 "               // warn on security issues around functions that format output (ie printf)
			"-Wimplicit-fallthrough "   // warn on statements that fallthrough without an explicit annotation
			"-Wmisleading-indentation " // warn if indentation implies blocks where blocks do not exist
			"-Wduplicated-cond "        // warn on 'if/else' chain with duplicated conditions
			"-Wduplicated-branches "    // warn on 'if/else' branches with duplicated code
			"-Wlogical-op "             // warn about logical operations being used where bitwise were probably wanted
			"-Wuseless-cast "           // warn if you perform a cast to the same type
			"-Wsuggest-override "       // warn if an overridden member function is not marked 'override' or 'final'
		},
		{"_shared", "-std=c++23 -fmodules"},
		{"debug", "-O0 -g3"},
		{"release", "-O3"},
		{"analyze", "--analyze"}
	};
}

void context::add_unittest(std::filesystem::path const& test_executable) {
	unittests.push_back(test_executable);
}

std::vector<std::filesystem::path> const& context::get_unittests() const noexcept {
	return unittests;
}

void context::clear_unittests() noexcept {
	unittests.clear();
}

void context::set_target_os(operating_system const os) noexcept {
	target_os = os;
}

operating_system context::get_target_os() const noexcept {
	return target_os;
}

// Get an environment variable
std::optional<std::string_view> context::get_env_value(const std::string_view var) const {
	return env.get(var);
}

// Get the home directory of the user
std::filesystem::path context::get_home_dir() const {
	if (auto home = env.get("HOME"); home)
		return *home;
	else
		throw std::runtime_error("Could not locate user home directory in environment variables");
}

// Get the internal gbs path
std::filesystem::path const& context::get_gbs_internal() const noexcept {
	return gbs_internal;
}

// Get the output gbs path
std::filesystem::path const& context::get_gbs_out() const noexcept {
	return gbs_out;
}

// Set the compile configuration
void context::set_config(std::string_view const cfg) {
	auto const old = config;
	config = cfg;

	std::error_code error_code{};
	std::filesystem::create_directories(output_dir(), error_code);
	if (error_code) {
		config = old;
		std::println(std::cerr, "<gbs> Error: could not create output build directory: '{}'", error_code.message());
	}
	else {
		config_dir = config;
		std::replace(config_dir.begin(), config_dir.end(), ',', '_');
		//config_dir = as_monad(config).replace(',', '_').to<std::string>();
	}
}

std::string_view context::get_config() const noexcept {
	return config;
}

void context::set_response_args(std::string&& resp) noexcept {
	resp_args = std::forward<std::string>(resp);
}

std::string_view context::get_response_args() const noexcept {
	return resp_args;
}

// Determine output dir, eg. 'gbs.out/msvc/debug
auto context::output_dir() const -> std::filesystem::path {
	return gbs_out / selected_cl.name_and_version / config_dir;
}

// Determine response directory
auto context::response_dir() const -> std::filesystem::path {
	return gbs_internal / selected_cl.name;
}

// Returns true if the compiler has a response map
bool context::has_response_map() const {
	return response_map.contains(selected_cl.name);
}

// Returns the response map for the selected compiler
compiler_response_map context::get_response_map() const {
	if (response_map.contains(selected_cl.name))
		return response_map.at(selected_cl.name);
	else
		return {};
}

// Selects the first compiler in the list
void context::select_first_compiler() noexcept {
	if (!all_compilers.empty())
		selected_cl = all_compilers.begin()->second.front();
}

bool context::is_compiler_selected() const noexcept {
	return !selected_cl.name.empty();
}

compiler const& context::get_selected_compiler() const noexcept {
	return selected_cl;
}

compiler_collection const& context::get_compiler_collection() const noexcept {
	return all_compilers;
}

// Returns the name of the currently selected compiler
std::string_view context::compiler_name() const noexcept {
	return selected_cl.name;
}

std::string context::make_include_path(std::string_view const path) const {
	return std::vformat(selected_cl.include, std::make_format_args(path));
}

// Create build command for the currently selected compiler
std::string context::build_command_prefix() const {
	auto const compiler = selected_cl.executable.generic_string();
	auto const out = output_dir().generic_string();
	return std::vformat(selected_cl.build_command_prefix, std::make_format_args(compiler, out));
}

// Create build args for a single file
std::string context::build_command(std::string_view file, std::filesystem::path const& obj_file) const {
	std::string_view const build_cmd = (file.ends_with(".cppm") || file.ends_with(".ixx") || file.ends_with(".cc"))
		? selected_cl.build_module
		: selected_cl.build_source;

	auto const str = obj_file.generic_string();
	return std::vformat(build_cmd, std::make_format_args(file, str));
}

// Create link command for the currently selected compiler
std::string context::link_command(std::string_view exe_name, std::string_view const out_dir) const {
	auto const linker = selected_cl.linker.generic_string();
	return std::vformat(selected_cl.link_command, std::make_format_args(linker, out_dir, exe_name));
}

// Create library command for the currently selected compiler
std::string context::static_library_command(std::string_view const out_name, std::string_view const out_dir) const {
	auto const lib = selected_cl.slib.generic_string();
	return std::vformat(selected_cl.slib_command, std::make_format_args(lib, out_dir, out_name));
}

// Create dynamic library command for the currently selected compiler
std::string context::dynamic_library_command(std::string_view const dll_name, std::string_view const lib_name, std::string_view const out_dir) const {
	auto const lib = selected_cl.dlib.generic_string();
	return std::vformat(selected_cl.dlib_command, std::make_format_args(lib, out_dir, dll_name, lib_name));
}

// Create a reference to a module
auto context::get_module_directory() const -> std::string {
	if (!selected_cl.module_path.empty()) {
		auto const out = output_dir().generic_string();
		return std::vformat(selected_cl.module_path, std::make_format_args(out));
	}
	return std::string{};
}

std::string context::build_define(std::string_view const def) const {
	return std::format(" {}{}", selected_cl.define, def);
}

void context::fill_compiler_collection() {
	all_compilers.clear();
	enumerate_compilers(env, [&](compiler&& c) {
		all_compilers[c.name].push_back(std::forward<compiler>(c));
		});

	// Sort compilers from the highest version to lowest
	for (auto& [name, compilers] : all_compilers) {
		std::sort(compilers.begin(), compilers.end(), [](compiler const& c1, compiler const& c2) {
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
#if 0//def _MSC_VER
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

bool context::set_compiler(std::string_view comp) {
	auto split = comp | std::views::split(':'); // cl:version
	std::string_view cl, version;

	switch (std::ranges::distance(split)) {
	case 2:
		// Version requested
		version = std::string_view{ *std::next(split.begin()) }; [[fallthrough]];
	case 1:
		// No version requested, returns newest
		cl = std::string_view{ *split.begin() };
		break;

	default:
		std::println("<gbs>   Error: ill-formed compiler descriptor: '{}'", comp);
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
	auto const dots = std::count(version.begin(), version.end(), '.');
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

	selected_cl = version_compilers.front();
	return true;
}

bool context::ensure_response_file_exists(std::string_view resp) const {
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

bool context::check_response_files(std::string_view args) {
	if (!is_compiler_selected()) {
		std::println("<gbs> Error: select a compiler");
		std::exit(1);
	}

	if (!has_response_map()) {
		std::println("<gbs> Error: selected compiler does not have any default response files");
		std::exit(1);
	}

	if (!std::filesystem::exists(response_dir())) {
		std::error_code ec{};
		if (!std::filesystem::create_directories(response_dir(), ec)) {
			std::println("<gbs> Error: could not create response file directory {}/{}: '{}'", std::filesystem::current_path().generic_string(), response_dir().generic_string(), ec.message());
			return false;
		}
	}

	if (!ensure_response_file_exists("_shared"))
		return false;

	for (auto subrange : args | std::views::split(',')) {
		if (!ensure_response_file_exists(std::string_view{ subrange }))
			return false;
	}

	return true;
}
