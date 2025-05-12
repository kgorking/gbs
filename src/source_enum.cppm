export module source_enum;
import std;

namespace fs = std::filesystem;

// Enumerate all files in a directory. They are ordered by their extension
export std::unordered_map<fs::path, std::vector<fs::path>> enum_sources(fs::path start_dir) {
	auto q = std::unordered_map<fs::path, std::vector<fs::path>>{};

	auto enum_impl = [&](this auto& self, fs::path p) -> bool {
		for (fs::directory_entry it : fs::directory_iterator(p)) {
			if (it.is_directory()) {
				self(it.path());
			}
		}

		for (fs::directory_entry it : fs::directory_iterator(p)) {
			if (it.is_directory())
				continue;

			auto const in = it.path();
			auto const ext = in.extension();

			q[ext].push_back(in);
		}

		return true;
		};

	enum_impl(start_dir);
	return q;
}
