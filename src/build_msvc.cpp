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
