#pragma once

#include "../inc/compiler.h"
#include "../inc/env.h"
#include "../inc/os.h"
#include "../inc/task/task.h"
#include "../inc/task/task_graph.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <ranges>
#include <set>
#include <string_view>
#include <unordered_map>
#include <vector>

using compiler_collection = std::unordered_map<std::string_view, std::vector<compiler>>;
using compiler_response_map = std::unordered_map<std::string_view, std::string_view>;

struct unittest_info {
	std::filesystem::path executable;
	std::optional<std::string> wsl;
};

class context {
	// Configuration of compile ('debug,analyze', etc...)
	std::string_view config{};
	std::string config_dir{};

	// Folder to store gbs related files
	const std::filesystem::path gbs_internal{ ".gbs" };

	// Folder for output generated during compilation
	const std::filesystem::path gbs_out{ "gbs.out" };

	// All available compilers
	compiler_collection all_compilers{};

	// All compilers selected by 'cl=...'
	std::vector<compiler> selected_compilers{};

	// Maps a compiler name to its response files
	std::unordered_map<std::string_view, compiler_response_map> response_map{};

	// The response args to use during build
	std::string resp_args{};

	// The current compilers target OS
	operating_system target_os = operating_system::linux;

	// All the unittests created during last build
	std::vector<unittest_info> unittests;

	// Environment variables
	environment env;

public:
	explicit context(char const** envp);

	void add_unittest(std::filesystem::path const& test_executable, std::optional<std::string> wsl = {});
	[[nodiscard]] std::vector<unittest_info> const& get_unittests() const noexcept;
	void clear_unittests() noexcept;
	void set_target_os(operating_system const os) noexcept;
	[[nodiscard]] operating_system get_target_os() const noexcept;
	[[nodiscard]] std::optional<std::string_view> get_env_value(const std::string_view var) const;
	[[nodiscard]] std::filesystem::path get_home_dir() const;
	[[nodiscard]] std::filesystem::path const& get_gbs_internal() const noexcept;
	[[nodiscard]] std::filesystem::path const& get_gbs_out() const noexcept;
	void set_config(compiler const& cl, std::string_view const cfg);
	[[nodiscard]] std::string_view get_config() const noexcept;
	void set_response_args(std::string&& resp) noexcept;
	[[nodiscard]] std::string_view get_response_args() const noexcept;
	[[nodiscard]] auto output_dir(compiler const& cl) const -> std::filesystem::path;
	[[nodiscard]] auto response_dir(compiler const& cl) const -> std::filesystem::path;
	[[nodiscard]] bool has_response_map(compiler const& cl) const;
	[[nodiscard]] compiler_response_map get_response_map(compiler const& cl) const;
	void select_first_compiler() noexcept;
	[[nodiscard]] std::vector<compiler> const& get_selected_compilers() const noexcept;
	[[nodiscard]] compiler_collection const& get_compiler_collection() const noexcept;

	void fill_compiler_collection();
	bool set_compiler(std::string_view comp);
	bool ensure_response_file_exists(compiler const& cl, std::string_view resp) const;
	bool check_response_files(compiler const& cl, std::string_view args);
};
