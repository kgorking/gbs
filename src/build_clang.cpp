import std;
import context;
import response;
import source_enum;

namespace fs = std::filesystem;

bool build_clang(context& ctx, std::string_view args) {
	std::string const resp_dir = (ctx.response_dir()).generic_string();

	// Convert ',' into response paths
	// eg. 'a,b,c' -> 'a @{path}/b @{path}/c'
	auto view_args = args | std::views::split(',');
	auto view_resp = view_args | std::views::join_with(std::format(" @{}/", resp_dir));

	// Build response file argument
	std::string const resp_args = std::format("@{0}/_shared @{0}/{1:s}", resp_dir, view_resp);

	// Build output
	ctx.output_config = std::string_view{ view_args.front() };
	auto const output_dir = ctx.output_dir();

	// Create the build dirs if needed
	std::filesystem::create_directories(output_dir);

	// Compile std library
	// https://raw.githubusercontent.com/llvm/llvm-project/refs/heads/main/libcxx/modules/std.cppm.in

	std::vector<std::string> objects;

	auto compile_cpp = [&](fs::path const& in) {
		auto const obj = (output_dir / in.filename()).replace_extension("o");
		auto cmd = std::format("call \"{}\" {} {} -fprebuilt-module-path=\"{}\" -c -o {}", ctx.selected_cl.exe.generic_string(), resp_args, in.generic_string(), output_dir.generic_string(), obj.generic_string());

		if (in.extension() == ".cppm") {
			auto const pcm = (output_dir / in.filename()).replace_extension("pcm");
			cmd += std::format(" -fmodule-output={}", pcm.generic_string());
		}

		if (0 == std::system(cmd.c_str())) {
			std::puts(in.generic_string().c_str());
			objects.push_back(obj.generic_string());
		}
	};

	auto source_files = enum_sources("src");

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

	// Link objects
	std::println("Linking...");
	std::string const executable = fs::current_path().stem().string() + ".exe";
	auto const cmd = std::format("call \"{}\" {:s} -o {}/{}", ctx.selected_cl.exe.generic_string(), objects | std::views::join_with(' '), output_dir.generic_string(), executable);
	return (0 == std::system(cmd.c_str()));
}
