// Enumerate all sources in project tree
// 
// Output findings to appropriate response files
//   - modules, sources, objects
#include <filesystem>

void enumerate_sources(std::filesystem::path dir, int depth = 0) {
	using namespace std::filesystem;
	for(directory_entry it : directory_iterator(dir)) {
		if(it.is_directory()) {
			enumerate_sources(it.path(), depth + 1);
		}
	}
}
