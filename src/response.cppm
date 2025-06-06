export module response;
import std;
import context;

namespace fs = std::filesystem;

export void init_response_files(context& ctx) {
	// Default response files
	ctx.response_map["msvc"] = {
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
			"/w14547 "     // 'operator': operator before comma has no effect; expected operator with side-effect 
			"/w14549 "     // 'operator': operator before comma has no effect; did you intend 'operator'? 
			"/w14555 "     // expression has no effect; expected expression with side- effect 
			"/w14619 "     // pragma warning: there is no warning number 'number' 
			"/w14640 "     // Enable warning on thread un-safe static member initialization 
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

	ctx.response_map["clang"] = {
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

	ctx.response_map["gcc"] = {
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
			"-Wduplicated-cond "        // warn if if / else chain has duplicated conditions
			"-Wduplicated-branches "    // warn if if / else branches have duplicated code
			"-Wlogical-op "             // warn about logical operations being used where bitwise were probably wanted
			"-Wuseless-cast "           // warn if you perform a cast to the same type
			"-Wsuggest-override "       // warn if an overridden member function is not marked 'override' or 'final'
		},
		{"_shared", "-std=c++2b -fmodules-ts"},
		{"debug", "-O0 -g3"},
		{"release", "-O3"},
		{"analyze", "--analyze"}
	};
}

export bool ensure_response_file_exists(context const& ctx, std::string_view resp) {
	if (resp.empty()) {
		std::println("<gbs> Error: bad build-arguments. Trailing comma?");
		return false;
	}

	auto& map = ctx.response_map.at(ctx.selected_cl.name);

	// Check that it is a valid response file
	if (!fs::exists(ctx.response_dir() / resp)) {
		if (!map.contains(resp)) {
			std::println("<gbs> Error: unknown response file {}", resp);
			return false;
		}
		else {
			std::ofstream file(ctx.response_dir() / resp);
			file << map.at(resp);
		}
	}

	return true;
}


export bool check_response_files(context const& ctx, std::string_view args) {
	if (ctx.selected_cl.name.empty()) {
		std::println("<gbs> Error: select a compiler");
		exit(1);
	}

	if (!ctx.response_map.contains(ctx.selected_cl.name)) {
		std::println("<gbs> Error: selected compiler does not have any default response files");
		exit(1);
	}

	if (!fs::exists(ctx.response_dir())) {
		std::filesystem::create_directories(ctx.response_dir());
	}

	ensure_response_file_exists(ctx, "_shared");

	for (auto subrange : args | std::views::split(',')) {
		if (!ensure_response_file_exists(ctx, std::string_view{ subrange }))
			return false;
	}

	return true;
}
