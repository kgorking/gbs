module;
#include <vector>
#include <filesystem>
export module source_enum;

export std::vector<std::vector<std::filesystem::path>> enum_sources(std::filesystem::path start_dir, std::filesystem::path extension) {
	std::vector<std::vector<std::filesystem::path>> q;

	using namespace std::filesystem;

	auto enum_impl = [&](this auto& self, std::filesystem::path p, int level) -> bool {
		for (directory_entry it : directory_iterator(p)) {
			if (it.is_directory()) {
				self(it.path(), 1 + level);
			}
		}

		for (directory_entry it : directory_iterator(p)) {
			if (it.is_directory())
				continue;

			auto const& in = it.path();
			if (in.extension() == extension) {
				while (q.size() < 1 + level)
					q.emplace_back();
				q[level].push_back(in);
			}
		}

		return true;
		};

	enum_impl(start_dir, 0);
	return q;
}
