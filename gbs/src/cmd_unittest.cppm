export module cmd_unittest;
import std;
import context;

namespace fs = std::filesystem;

export bool cmd_unittest(context& ctx, std::string_view args) {
	for (auto const it : fs::directory_iterator(ctx.output_dir())) {
		if (!it.is_regular_file())
			continue;

		if(it.path().extension() != ".exe" || !it.path().filename().string().starts_with("test."))
			continue;

		std::println("<gbs> Running unittest {:?}", it.path().filename().string());

		auto const cmd = std::format("call {:?} {}", it.path().generic_string(), args);
		if (0 != std::system(cmd.c_str())) {
			std::println(std::cerr, "<gbs> Unittest '{}' failed.", it.path().filename().string());
			return false;
		}
	}

	return true;
}