import std;
import context;
import response;
import env;
import source_enum;

namespace fs = std::filesystem;

bool build_msvc(context& ctx, std::string_view args) {
	// Pull extra parameters if needed
	if (ctx.selected_cl.extra_params.empty()) {
		// Executes 'vcvars64.bat' and pulls out the INCLUDE, LIB, LIBPATH environment variables
		constexpr std::string_view extract_cmd = R"(echo /I"%INCLUDE:;=" /I"%" /link /LIBPATH:"%LIB:;=" /LIBPATH:"%" /LIBPATH:"%LIBPATH:;=" /LIBPATH:"%")";
		auto const vcvars = ctx.selected_cl.dir / "../../../Auxiliary/Build/vcvars64.bat";
		std::string const cmd = std::format(R"(""{}" >nul && call {} >extra_params")", vcvars.generic_string(), extract_cmd);
		if (0 == std::system(cmd.c_str())) {
			std::getline(std::ifstream("extra_params"), ctx.selected_cl.extra_params);
			std::remove("extra_params");
		}
	}

	// Arguments to the compiler(s)
	// Converts arguments into response files
	auto view_args = args | std::views::split(',');
	auto view_resp = view_args | std::views::join_with(std::format(" @{}/", (ctx.gbs_internal / ctx.selected_cl.name).generic_string()));

	// Build cl command
	std::string const cl = std::format("cl @{0}/_shared @{0}/{1:s} ", ctx.response_dir().generic_string(), view_resp);

	// Build output
	ctx.output_config = std::string_view{ view_args.front() };
	auto const output_dir = ctx.output_dir();

	// Create the build dirs if needed
	std::filesystem::create_directories(output_dir);

	// Compile stdlib if needed
	if (!fs::exists(output_dir / "std.obj")) {
		fs::path const stdlib_module = ctx.selected_cl.dir / "modules/std.ixx";
		if (!is_file_up_to_date(stdlib_module, output_dir / "std.obj"))
			if (0 != std::system(std::format("{} /c \"{}\" {}", cl, stdlib_module.generic_string(), ctx.selected_cl.extra_params).c_str()))
				return false;
	}

	// Add source files

	// Objects to link after compilation
	std::vector<std::string> objects;

	// Find the source files
	auto source_files = enum_sources("src");

	// Set up the compiler helper
	auto compile_cpp = [&](fs::path const& in) {
		auto const obj = (output_dir / in.filename()).replace_extension("obj");

		if (is_file_up_to_date(in, obj))
			return;

		std::string const cmd = std::format("{0} /interface /Tp {1} /reference std={3}/std.ifc {4}"
			, cl
			, in.generic_string()
			, view_resp
			, output_dir.generic_string()
			, ctx.selected_cl.extra_params);
		std::puts(cmd.c_str());

		if (0 == std::system(cmd.c_str())) {
			std::puts(in.generic_string().c_str());

			objects.push_back(obj.generic_string());
		}
		};

	// Compile modules
	auto const& modules = source_files[".cppm"];
	for(auto p : modules)
		compile_cpp(p);

	// Compile sources
	auto const& regulars = source_files[".cpp"];
	for(auto p : regulars)
		compile_cpp(p);

	std::string const executable = fs::current_path().stem().string() + ".exe";
	return true;
}
