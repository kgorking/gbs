#pragma once
#include <string>

struct module_desc {
	std::string name;
	std::string partition;

	bool operator<(module_desc const& other) const noexcept {
		return std::tie(name, partition) < std::tie(other.name, other.partition);
	}

	bool operator==(module_desc const& other) const noexcept {
		return name == other.name && partition == other.partition;
	}
};

template<> struct std::hash<module_desc> {
	size_t operator()(module_desc const& mod) const noexcept {
		return hash<string>{}(mod.name) ^ (hash<string>{}(mod.partition) << 1);
	}
};

// Formatter specialization for std::format support
template <> struct std::formatter<module_desc, char> {
	constexpr auto parse(std::format_parse_context& ctx) {
		return ctx.begin();
	}

	auto format(module_desc const& mod, std::format_context& ctx) const {
		auto out = ctx.out();
		for (char c : mod.name) {
			*out++ = c;
		}
		if (!mod.partition.empty()) {
			*out++ = ':';
			for (char c : mod.partition) {
				*out++ = c;
			}
		}
		return out;
	}
};
