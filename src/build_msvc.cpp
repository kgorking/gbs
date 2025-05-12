import std;
import context;
import response;
import env;
import source_enum;

namespace fs = std::filesystem;

struct enum_context {
	std::ofstream modules;
	std::ofstream sources;
	std::ofstream objects;
};

void enumerate_sources_imp(enum_context& ctx, std::filesystem::path dir, std::filesystem::path output_dir) {
	for (fs::directory_entry it : fs::directory_iterator(dir)) {
		if (it.is_directory()) {
			enumerate_sources_imp(ctx, it.path(), output_dir);
		}
	}

	for (fs::directory_entry it : fs::directory_iterator(dir)) {
		if (it.is_directory())
			continue;

		auto ext = it.path().extension();
		if (ext == ".h")
			continue;

		bool const is_module = it.path().extension() == ".cppm";
		if (is_module) {
			auto const ifc = (output_dir / it.path().filename()).replace_extension("ifc");
			ctx.modules << std::format(" /reference {}", ifc.generic_string());
		}

		auto const out = (output_dir / it.path().filename()).replace_extension("obj");
		if (is_file_up_to_date(it.path(), out)) {
			ctx.objects << out << ' ';
		}
		else {
			if (is_module)
				ctx.sources << "/interface /Tp";
			ctx.sources << it.path().generic_string() << ' ';
		}
	}
}

void enumerate_sources(context const& ctx, std::filesystem::path dir, std::filesystem::path output_dir) {
	enum_context e_ctx{
		std::ofstream(ctx.response_dir() / "modules", std::ios::out | std::ios::trunc),
		std::ofstream(ctx.response_dir() / "sources", std::ios::out | std::ios::trunc),
		std::ofstream(ctx.response_dir() / "objects", std::ios::out | std::ios::trunc)
	};

	enumerate_sources_imp(e_ctx, dir, output_dir);
}

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
	//enumerate_sources(ctx, "src", output_dir);
	//
	//std::string const executable = fs::current_path().stem().string() + ".exe";
	//std::string const cmd = std::format("{0} /reference std={3}/std.ifc /Fe:{3}/{4} @{1}/modules @{1}/sources @{1}/objects {5}", cl, ctx.response_dir().generic_string(), view_resp, output_dir.generic_string(), executable, ctx.selected_cl.extra_params);
	//return 0 == std::system(cmd.c_str());

	std::string const executable = fs::current_path().stem().string() + ".exe";

	// Objects to link after compilation
	std::vector<std::string> objects;

	// Find the source files
	auto source_files = enum_sources("src");

	// Set up the compiler helper
	auto compile_cpp = [&](fs::path const& in) {
		auto const obj = (output_dir / in.filename()).replace_extension("obj");

		if (is_file_up_to_date(in, obj))
			return;

		std::string const cmd = std::format("{0} /interface /Tp {1} /reference std={3}/std.ifc {5}"
			, cl
			, in.generic_string()
			, view_resp
			, output_dir.generic_string()
			, executable
			, ctx.selected_cl.extra_params);
		std::puts(cmd.c_str());

		//if (in.extension() == ".cppm") {
		//	auto const pcm = (output_dir / in.filename()).replace_extension("pcm");
		//	cmd += std::format(" -fmodule-output={}", pcm.generic_string());
		//}

		if (0 == std::system(cmd.c_str())) {
			std::puts(in.generic_string().c_str());

			objects.push_back(obj.generic_string());
		}
		};

	// Compile modules
	auto const& modules = source_files[".cppm"];
	std::for_each(std::execution::/*par_un*/seq, modules.begin(), modules.end(), [&](fs::path p) {
		compile_cpp(p);
		});

	// Compile sources
	auto const& regulars = source_files[".cpp"];
	std::for_each(std::execution::/*par_un*/seq, regulars.begin(), regulars.end(), [&](fs::path in) {
		compile_cpp(in);
		});

	return true;
}
