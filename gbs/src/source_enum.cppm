export module source_enum;
import std;

namespace fs = std::filesystem;

// Enumerate all files in a directory. They are ordered by their extension
/*export std::vector<fs::path> enum_sources(fs::path start_dir) {
	if (!fs::is_directory(start_dir))
		return {};

	auto q = std::vector<fs::path>{};
	for (fs::directory_entry it : fs::recursive_directory_iterator(start_dir)) {
		if (it.is_directory())
			continue;

		auto const in = it.path();
		auto const ext = in.extension();

		q.push_back(in);
	}

	return q;
}*/
