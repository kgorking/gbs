import std;
import env;
import context;
import get_source_groups;

namespace fs = std::filesystem;

bool init_msvc([[maybe_unused]] context const& ctx) {
#ifdef _MSC_VER
	// Executes 'vcvars64.bat' and pulls out the INCLUDE, LIB, LIBPATH environment variables
	constexpr std::string_view include_cmd = R"(echo /I"%INCLUDE:;=" /I"%")";
	constexpr std::string_view libpath_cmd = R"(echo /LIBPATH:"%LIB:;=" /LIBPATH:"%" /LIBPATH:"%LIBPATH:;=" /LIBPATH:"%")";

	auto const dir = (ctx.compiler_name() == "msvc") ? ctx.get_selected_compiler().dir : ctx.get_compiler_collection().at("msvc").front().dir;
	auto const vcvars = dir / "../../../../Common7/Tools/VsDevCmd.bat";

	std::string const vcvars_cmd = std::format(R"("cd {0} && {1:?} -no_logo -arch=amd64 -host_arch=amd64 && call {2} >INCLUDE && call {3} >LIBPATH")",
		ctx.output_dir().generic_string(),
		vcvars.generic_string(),
		include_cmd,
		libpath_cmd);
	//std::puts(vcvars_cmd.c_str());

	if (0 != std::system(vcvars_cmd.c_str())) {
		std::println("<gbs> Error: failed to extract vars from 'vcvars64.bat'");
		return false;
	}
#endif
	return true;
}
