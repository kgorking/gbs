#include "../inc/context.h"
#include "../inc/enumerate_compilers.h"
#include <algorithm>
#include <print>

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
		{"_shared", "/nologo /EHsc /sdl /std:c++23 /FC /utf-8 /D_MSVC_STL_HARDENING=1 /D_MSVC_STL_DESTRUCTOR_TOMBSTONES=1"},
		{"debug",   "/Zi /FS /Od /MT"},
		{"release", "/DNDEBUG /O2 /MT /GL"},
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
			//"-Wdouble-promotion "       // warn if float is implicit promoted to double
			"-Wformat=2 "               // warn on security issues around functions that format output (ie printf)
			"-Wimplicit-fallthrough "   // warn on statements that fallthrough without an explicit annotation
			"-Wmisleading-indentation " // warn if indentation implies blocks where blocks do not exist
			"-Wduplicated-cond "        // warn on 'if/else' chain with duplicated conditions
			"-Wduplicated-branches "    // warn on 'if/else' branches with duplicated code
			"-Wlogical-op "             // warn about logical operations being used where bitwise were probably wanted
			"-Wuseless-cast "           // warn if you perform a cast to the same type
			"-Wsuggest-override "       // warn if an overridden member function is not marked 'override' or 'final'
			// GCC 16.2 reports false positives from inlined libstdc++ headers (std::generator, std::format).
			"-Wno-null-dereference "
		},
		{"_shared", "-std=c++23 -fmodules"},
		{"debug", "-O0 -g3"},
		{"release", "-O3"},
		{"analyze", "--analyze"}
	};
}

void context::add_unittest(std::filesystem::path const& test_executable, std::optional<std::string> wsl) {
	unittests.push_back({ test_executable, std::move(wsl) });
}

std::vector<unittest_info> const& context::get_unittests() const noexcept {
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
void context::set_config(compiler const& cl, std::string_view const cfg) {
	std::string const old = config_dir;
	config = cfg;
	config_dir = config;
	std::replace(config_dir.begin(), config_dir.end(), ',', '_');

	std::error_code error_code{};
	std::filesystem::create_directories(output_dir(cl), error_code);
	if (error_code) {
		config = old;
		config_dir = config;
		std::replace(config_dir.begin(), config_dir.end(), ',', '_');
		std::println(std::cerr, "<gbs> Error: could not create output build directory: '{}'", error_code.message());
	}
	else {
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
auto context::output_dir(compiler const& cl) const -> std::filesystem::path {
	return gbs_out / cl.name_and_version / config_dir;
}

// Determine response directory
auto context::response_dir(compiler const& cl) const -> std::filesystem::path {
	return gbs_internal / cl.compiler_name();
}

// Returns true if the compiler has a response map
bool context::has_response_map(compiler const& cl) const {
	return response_map.contains(cl.compiler_name());
}

// Returns the response map for the compiler
compiler_response_map context::get_response_map(compiler const& cl) const {
	if (response_map.contains(cl.compiler_name()))
		return response_map.at(cl.compiler_name());
	else
		return {};
}

// Selects the first compiler in the list
void context::select_first_compiler() noexcept {
	selected_compilers.clear();
	if (!all_compilers.empty())
		selected_compilers.push_back(all_compilers.begin()->second.front());
}

std::vector<compiler> const& context::get_selected_compilers() const noexcept {
	return selected_compilers;
}

compiler_collection const& context::get_compiler_collection() const noexcept {
	return all_compilers;
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
	if constexpr (is_host_windows()) {
		if(all_compilers.contains("clang") && all_compilers.contains("msvc")) {
			// Get the newest msvc compiler
			compiler const& msvc_compiler = all_compilers["msvc"].front();
			auto const std_module = std::filesystem::path(*msvc_compiler.std_module);

			for(compiler& clang : all_compilers["clang"]) {
				if (!clang.std_module && !clang.wsl)
					clang.std_module = std_module;
			}
		}
	}
}

namespace {
compiler const* find_compiler(compiler_collection const& all_compilers, std::string const& name, std::string_view version) {
	if (name.empty() || !all_compilers.contains(name))
		return nullptr;

	auto const& named_compilers = all_compilers.at(name);
	if (named_compilers.empty())
		return nullptr;

	if (version.empty())
		return &named_compilers.front();

	int major = 0, minor = 0, patch = 0;
	auto const dots = std::count(version.begin(), version.end(), '.');
	if (dots > 2)
		return nullptr;

	extract_compiler_version(version, major, minor, patch);

	auto const it = std::ranges::find_if(named_compilers, [&](compiler const& c) {
		switch (dots) {
		default: return false;
		case 2: return c.major == major && c.minor == minor && c.patch == patch;
		case 1: return c.major == major && c.minor == minor;
		case 0: return c.major == major;
		}
		});

	return it == named_compilers.end() ? nullptr : &*it;
}
} // namespace

bool context::set_compiler(std::string_view comp) {
	std::vector<compiler> found;

	for (auto descriptor_range : comp | std::views::split(',')) {
		std::string_view const descriptor{ descriptor_range };
		if (descriptor.empty()) {
			std::println("<gbs>   Error: ill-formed compiler descriptor: '{}'", comp);
			return false;
		}

		auto const colon = descriptor.find(':');
		std::string const name{ descriptor.substr(0, colon) };
		std::string_view const version = colon == std::string_view::npos
			? std::string_view{}
			: descriptor.substr(colon + 1);

		compiler const* match = find_compiler(all_compilers, name, version);
		if (!match) {
			std::println("<gbs>   Error: could not find compiler '{}'", descriptor);
			return false;
		}

		if (std::ranges::none_of(found, [&](compiler const& c) { return c.name_and_version == match->name_and_version; }))
			found.push_back(*match);
	}

	if (found.empty())
		return false;

	selected_compilers = std::move(found);
	return true;
}

bool context::ensure_response_file_exists(compiler const& cl, std::string_view resp) const {
	if (resp.empty()) {
		std::println("<gbs> Error: bad build-arguments. Trailing comma?");
		return false;
	}

	auto const& map = get_response_map(cl);

	// Check that it is a valid response file
	if (!std::filesystem::exists(response_dir(cl) / resp)) {
		if (!map.contains(resp)) {
			std::println("<gbs> Error: unknown response file {}", resp);
			return false;
		}
		else {
			std::ofstream file(response_dir(cl) / resp);
			file << map.at(resp);
		}
	}

	return true;
}

bool context::check_response_files(compiler const& cl, std::string_view args) {
	if (cl.name.empty()) {
		std::println("<gbs> Error: select a compiler");
		std::exit(1);
	}

	if (!has_response_map(cl)) {
		std::println("<gbs> Error: compiler '{}' does not have any default response files", cl.name);
		std::exit(1);
	}

	if (!std::filesystem::exists(response_dir(cl))) {
		std::error_code ec{};
		if (!std::filesystem::create_directories(response_dir(cl), ec)) {
			std::println("<gbs> Error: could not create response file directory {}/{}: '{}'", std::filesystem::current_path().generic_string(), response_dir(cl).generic_string(), ec.message());
			return false;
		}
	}

	if (!ensure_response_file_exists(cl, "_shared"))
		return false;

	for (auto subrange : args | std::views::split(',')) {
		if (!ensure_response_file_exists(cl, std::string_view{ subrange }))
			return false;
	}

	return true;
}
