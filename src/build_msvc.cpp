import std;
import context;
import response;
import env;
import source_enum;
import dep_scan;

namespace fs = std::filesystem;
using namespace std::string_view_literals;

bool build_msvc(context& ctx, std::string_view args) {
	// Build output
	ctx.output_config = args.substr(0, args.find(','));
	auto const output_dir = ctx.output_dir();

	// Create the build dirs if needed
	std::filesystem::create_directories(ctx.output_dir());

	// Pull extra parameters if needed
	if (ctx.selected_cl.extra_params.empty()) {
		// Executes 'vcvars64.bat' and pulls out the INCLUDE, LIB, LIBPATH environment variables
		constexpr std::string_view include_cmd = R"(echo /I"%INCLUDE:;=" /I"%")";
		constexpr std::string_view libpath_cmd = R"(echo /LIBPATH:"%LIB:;=" /LIBPATH:"%" /LIBPATH:"%LIBPATH:;=" /LIBPATH:"%")";

		auto const vcvars = ctx.selected_cl.dir / "../../../Auxiliary/Build/vcvars64.bat";
		std::string const cmd = std::format(R"("cd "{}" && "{}" >nul && call {} >INCLUDE && call {} >LIBPATH")",
			ctx.output_dir().generic_string(),
			vcvars.generic_string(),
			include_cmd,
			libpath_cmd);

		if (0 == std::system(cmd.c_str())) {
			ctx.selected_cl.extra_params = " ";
		}
	}

	// Arguments to the compiler(s)
	// Converts arguments into response files
	auto joiner = std::format(" @{}/", ctx.response_dir().generic_string());
	auto view_resp = args
		| std::views::split(',')
		| std::views::join_with(joiner);

	// Build cl command
	std::string const cl = std::format("cl @{0}/_shared @{0}/{1:s} ", ctx.response_dir().generic_string(), view_resp);

	// Compile stdlib if needed
	if (!fs::exists(output_dir / "std.obj")) {
		fs::path const stdlib_module = ctx.selected_cl.dir / "modules/std.ixx";
		if (!is_file_up_to_date(stdlib_module, output_dir / "std.obj")) {
			auto const cmd = std::format("{} /c \"{}\" @{}/INCLUDE",
				cl,
				stdlib_module.generic_string(),
				output_dir.generic_string()
			);

			if (0 != std::system(cmd.c_str()))
				return false;
		}
	}

	// Find the source files
	// Detect dependencies
	auto source_files = std::unordered_map<fs::path, source_dependency>{};
	std::unordered_map<std::string, fs::path> module_map;

	for (fs::directory_entry it : fs::recursive_directory_iterator("src")) {
		if (it.is_directory())
			continue;

		auto const path = it.path();
		auto const deps = detect_module_dependencies(path);
		source_files[path] = deps;

		if (!deps.export_name.empty())
			module_map[deps.export_name] = path;
	}


	//auto source_is_compiled = std::make_unique<bool[]>(source_files.size());

	// Set up the compiler helper
	auto compile_cpp = [&](this auto& self, fs::path const& in, source_dependency const& deps, std::set<std::string_view>& refs_set) -> bool {
		auto const obj = (output_dir / in.filename()).replace_extension("obj");
		if (is_file_up_to_date(in, obj)) {
			refs_set.insert_range(deps.import_names);
			return true;
		}

		for (auto const& dep : deps.import_names) {
			if (auto it = module_map.find(dep); it != module_map.end()) {
				auto const dep_path = it->second;
				self(dep_path, source_files[dep_path], refs_set);
			}
		}
		refs_set.insert_range(deps.import_names);

		std::string refs;
		for(auto ref : refs_set)
			refs += std::format("/reference {0}={1}/{0}.ifc ", ref, output_dir.generic_string());

		std::string const cmd = std::format("{0} @{2}/INCLUDE /c /interface /Tp\"{1}\" /reference std={2}/std.ifc {3}",
			cl,
			in.generic_string(),
			output_dir.generic_string(),
			refs
		);
		//std::puts(cmd.c_str());
		std::println("{} deps on {}", in.generic_string(), refs_set);

		return (0 == std::system(cmd.c_str()));
		};

	// Compile sources
	for (auto [path, deps] : source_files) {
		auto refs = std::set<std::string_view>{};
		if (!compile_cpp(path, deps, refs))
			return false;
	}

	std::string const executable = fs::current_path().stem().string() + ".exe";
	return true;
}
