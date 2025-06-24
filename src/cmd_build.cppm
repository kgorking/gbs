export module cmd_build;
import std;
import env;
import context;
import response;
import get_source_groups;
import monad;

namespace fs = std::filesystem;
using namespace std::string_view_literals;

export bool cmd_build(context& ctx, std::string_view args) {
	// Select default compiler if none is selected
	if (ctx.selected_cl.name.empty()) {
		std::println(std::cerr, "<gbs> No compiler selected/found.");
		return false;
		//if (ctx.all_compilers.empty()) {
		//	fill_compiler_collection(ctx);
		//}
		//ctx.select_first_compiler();
	}

	std::println(std::cerr, "<gbs> Building with '{} {}.{}.{}'", ctx.selected_cl.name, ctx.selected_cl.major, ctx.selected_cl.minor, ctx.selected_cl.patch);

	if (!fs::exists("src/")) {
		std::println("<gbs> Error: no 'src' directory found at '{}'", fs::current_path().generic_string());
		return false;
	}

	// Default build config
	if (args.empty())
		args = "release,warnings";

	// Ensure the needed response files are present
	init_response_files(ctx);
	check_response_files(ctx, args);

	// Build output
	ctx.config = args.substr(0, args.find(','));
	auto const output_dir = ctx.output_dir();

	// Create the build dirs if needed
	fs::create_directories(ctx.output_dir());

	// Arguments to the compiler.
	// Converts arguments into response files
	auto arg_to_str = [&](auto arg) {
		return " @" + (ctx.response_dir() / std::string{ arg.begin(), arg.end() }).string();
		};

	// TODO std::views::concat("_shared", args)
	std::string const resp_args = arg_to_str("_shared"sv) + monad(args)
		.split(',')
		.map(arg_to_str)
		.join()
		.to<std::string>();

#ifdef _MSC_VER
	if (ctx.selected_cl.name == "msvc") {
		extern bool init_msvc(context const&);
		if (!init_msvc(ctx))
			return false;
	}
	else
#endif
		if (ctx.selected_cl.name.starts_with("clang")) {
		}
		else if (ctx.selected_cl.name == "gcc") {
		}
		else {
			std::println("<gbs> INTERNAL : unknown compiler {:?}", ctx.selected_cl.name);
			return false;
		}

	// Get the source files to compile
	std::map<std::size_t, source_group> source_groups = get_grouped_source_files("src");
	if (source_groups.empty()) {
		return true;
	}

	// Insert the 'std' module
	if (!ctx.selected_cl.std_module.empty()) {
		source_groups[0].emplace_back(source_info{ ctx.selected_cl.std_module, {} });
	}

	// Create file containing the list of objects to link
	std::ofstream objects(ctx.output_dir() / "OBJLIST");
	std::shared_mutex mut;

	// Create the build command
	std::string const cmd_prefix = ctx.build_command_prefix();// +resp_args.data();

	// Set up the compiler helper
	bool keep_going = true;

	auto append_obj = [&](source_info const& in) {
		return std::tuple{ in.first, in.second, (ctx.output_dir() / in.first.filename()).replace_extension("obj") };
		};
	auto log_obj_to_file = [&](auto const& tup) {
		fs::path const& obj = std::get<2>(tup);
		std::scoped_lock sl(mut);
		objects << obj << ' ';
		};
	auto obj_up_to_date = [](auto const& tup) {
		auto const& [path, _, obj] = tup;
		return !is_file_up_to_date(path, obj);
		};
	auto make_build_cmd = [&](auto const& tup) {
		auto const& [path, imports, obj] = tup;
		std::string cmd = cmd_prefix;
		cmd += ctx.build_file(path.string(), obj.string());
		cmd += resp_args.data();

		if (!ctx.selected_cl.reference.empty())
			for (auto const& s : imports)
				cmd += ctx.build_reference(s);

		return cmd;
		};
	auto execute_build_cmd = [&](std::string const& cmd) {
		keep_going = keep_going && (0 == std::system(cmd.c_str()));
		};
	auto compile_source_group = [&](source_group const& sg) {
		monad(sg)
			.async()
			.take_while(keep_going)
			.map(append_obj)
			.and_then(log_obj_to_file)
			.filter(obj_up_to_date)
			.map(make_build_cmd)
			.then(execute_build_cmd);
		};

	monad{ source_groups }.values().then(compile_source_group);

	// Close the objects file
	objects.close();

	// Link source_groups
	if (keep_going) {
		std::println("<gbs> Linking...");
		std::string const executable = fs::current_path().stem().string();
		std::string const link = ctx.link_command(executable);
		return 0 == std::system(link.c_str());
	}
	else {
		return false;
	}
}
