module;
#include <filesystem>
#include <string>
#include <string_view>
#include <fstream>
#include <unordered_map>
export module dep_scan;
//import std;

export auto detect_module_dependencies(std::filesystem::path dir) {
    std::unordered_map<std::filesystem::path, std::vector<std::string>> dependencies;

    for (auto const& entry : std::filesystem::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file())
            continue;

        auto path = entry.path();
        if (path.extension() != ".cppm" && path.extension() != ".cpp")
            continue;

        std::ifstream file(path);
        if (!file.is_open())
            continue;

        auto& module_deps = dependencies[path];

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty())
                continue;

            // Simple pattern: look for "\bimport\s+<module-name>;"
            std::string_view sv = line;

            // Skip initial whitespaces
            auto text_start = sv.find_first_not_of(" \t", 0);
            if (text_start == std::string_view::npos)
                continue;

            auto import_pos = sv.substr(text_start);
            if (!import_pos.starts_with("import "))
                continue;

            // Find the start of the module name
            import_pos = import_pos.substr(7);

            // Skip whitespace
            auto const whitespaces = import_pos.find_first_not_of(" \t");
            if (whitespaces != std::string_view::npos)
                import_pos = import_pos.substr(whitespaces);

            // Find end of module name (until ';' or whitespace)
            auto end = import_pos.find_first_of(" ;\t");
            if (end == std::string_view::npos)
                continue;

            import_pos = import_pos.substr(0, end);
            module_deps.emplace_back(import_pos.data(), import_pos.size());
        }
    }

    return dependencies;
}
