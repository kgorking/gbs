#include <string_view>
#include <print>

bool get_cl(std::string_view args) {
	if (args.empty()) {
		std::println("Error: get_cl : specify a compiler to download");
		return false;
	}

	if (!args.starts_with("clang")) {
		std::println("Error: get_cl : only clang downloads supported so far");
		return false;
	}

	if (!args.contains(':')) {
		std::println("Error: get_cl : specify a version to download");
		return false;
	}

	auto url = std::format("curl -L --output {} https://github.com/llvm/llvm-project/releases/download/llvmorg-{}/clang+llvm-{}-x86_64-pc-windows-msvc.tar.xz");
	std::println("Error: get_cl : not implemented yet");
	return false;
}