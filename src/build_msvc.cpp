import std;
import env;
import context;
import response;
import get_source_groups;

namespace fs = std::filesystem;

bool init_msvc(context& ctx) {
	if (!fs::exists(ctx.output_dir() / "INCLUDE") || !fs::exists(ctx.output_dir() / "LIBPATH")) {
		// Executes 'vcvars64.bat' and pulls out the INCLUDE, LIB, LIBPATH environment variables
		constexpr std::string_view include_cmd = R"(echo /I"%INCLUDE:;=" /I"%")";
		constexpr std::string_view libpath_cmd = R"(echo /LIBPATH:"%LIB:;=" /LIBPATH:"%" /LIBPATH:"%LIBPATH:;=" /LIBPATH:"%")";

		auto const vcvars = ctx.selected_cl.dir / "../../../Auxiliary/Build/vcvars64.bat";
		std::string const vcvars_cmd = std::format(R"("cd "{}" && "{}" >nul && call {} >INCLUDE && call {} >LIBPATH")",
			ctx.output_dir().string(),
			vcvars.string(),
			include_cmd,
			libpath_cmd);

		if (0 != std::system(vcvars_cmd.c_str())) {
			std::println("<gbs> Error: failed to extract vars from 'vcvars64.bat'");
			return false;
		}
	}

	return true;
}

bool build_msvc(context& ctx, std::string_view args) {
	// Get the source files to compile
	auto sources = get_grouped_source_files("src");
	if (sources.empty())
		return true;

	// Insert the 'std' module
	if (!ctx.selected_cl.std_module.empty())
		sources[0].emplace_back(source_info{ ctx.selected_cl.std_module, {} });

	// Create file containing the list of objects to link
	std::ofstream objects(ctx.output_dir() / "OBJLIST");
	std::shared_mutex mut;

	// Create the build command
	std::string const cmd_prefix = ctx.build_command_prefix() + args.data();

	// Set up the compiler helper
	auto compile_cpp = [&](this auto& self, source_info const& in) -> bool {
		auto const& [path, imports] = in;

		fs::path const obj = (ctx.output_dir() / path.filename()).replace_extension("obj");

		{
			std::scoped_lock sl(mut);
			objects << obj << ' ';
		}

		if (is_file_up_to_date(path, obj)) {
			return true;
		}

		std::string cmd = cmd_prefix + std::format(" {0:?} ", path.string());
		for(auto const& s : imports)
			cmd += ctx.build_reference(s);

		return (0 == std::system(cmd.c_str()));
		};

	// Compile sources
	for (auto const& paths : sources) {
		std::for_each(std::execution::par_unseq, paths.begin(), paths.end(), compile_cpp);
	}

	// Close the objects file
	objects.close();

	// Link sources
	std::println("<gbs> Linking...");
	std::string const executable = fs::current_path().stem().string();
	return 0 == std::system(ctx.link_command(executable).c_str());
}
