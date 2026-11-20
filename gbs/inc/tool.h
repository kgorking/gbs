#pragma once
#include "source_file.h"
#include <concepts>

// Describes a tool interface
template <typename T, typename Input, typename Output>
concept Tool = requires(T tool, Input i) {
	{ tool.process(i) } -> std::same_as<Output>;
};

// Example of the clang compiler/linker, implemented as a Tool
struct clang_compile {
	object_file process(source_file const& in) const {
		return { .path = in.path.replace_extension("obj") };
	}
};

struct clang_link {
	executable_file process(object_files const& in) const {
		return { .path = "a.out" };
	}
};

static_assert(Tool<clang_compile, source_file , object_file    >);
static_assert(Tool<clang_link   , object_files, executable_file>);
