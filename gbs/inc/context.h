#pragma once

#include "../inc/compiler.h"
#include "../inc/env.h"
#include "../inc/os.h"
#include "../inc/task/task.h"
#include "../inc/task/task_graph.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <print>
#include <ranges>
#include <set>
#include <string_view>
#include <unordered_map>
#include <vector>

using compiler_collection = std::unordered_map<std::string_view, std::vector<compiler>>;
using compiler_response_map = std::unordered_map<std::string_view, std::string_view>;

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

	// The currently selected compiler
	compiler selected_cl;

	// Maps a compiler name to its response files
	std::unordered_map<std::string_view, compiler_response_map> response_map{};

	// The response args to use during build
	std::string resp_args{};

	// The current compilers target OS
	operating_system target_os = operating_system::linux;

	// All the unittests created during last build
	std::vector<std::filesystem::path> unittests;

	// Environment variables
	environment env;

public:
	explicit context(char const** envp);

	void add_unittest(std::filesystem::path const& test_executable);
	[[nodiscard]] std::vector<std::filesystem::path> const& get_unittests() const noexcept;
	void clear_unittests() noexcept;
	void set_target_os(operating_system const os) noexcept;
	[[nodiscard]] operating_system get_target_os() const noexcept;
	[[nodiscard]] std::optional<std::string_view> get_env_value(const std::string_view var) const;
	[[nodiscard]] std::filesystem::path get_home_dir() const;
	[[nodiscard]] std::filesystem::path const& get_gbs_internal() const noexcept;
	[[nodiscard]] std::filesystem::path const& get_gbs_out() const noexcept;
	void set_config(std::string_view const cfg);
	[[nodiscard]] std::string_view get_config() const noexcept;
	void set_response_args(std::string&& resp) noexcept;
	[[nodiscard]] std::string_view get_response_args() const noexcept;
	[[nodiscard]] auto output_dir() const -> std::filesystem::path;
	[[nodiscard]] auto response_dir() const -> std::filesystem::path;
	[[nodiscard]] bool has_response_map() const;
	[[nodiscard]] compiler_response_map get_response_map() const;
	void select_first_compiler() noexcept;
	[[nodiscard]] bool is_compiler_selected() const noexcept;
	[[nodiscard]] compiler const& get_selected_compiler() const noexcept;
	[[nodiscard]] compiler_collection const& get_compiler_collection() const noexcept;
	[[nodiscard]] std::string_view compiler_name() const noexcept;
	[[nodiscard]] std::string make_include_path(std::string_view const path) const;
	[[nodiscard]] std::string build_command_prefix() const;
	[[nodiscard]] std::string build_define(std::string_view const def) const;
	[[nodiscard]] std::string build_command(std::string_view file, std::filesystem::path const& obj_file) const;
	[[nodiscard]] std::string link_command(std::string_view exe_name, std::string_view const out_dir) const;
	[[nodiscard]] std::string static_library_command(std::string_view const out_name, std::string_view const out_dir) const;
	[[nodiscard]] std::string dynamic_library_command(std::string_view const dll_name, std::string_view const lib_name, std::string_view const out_dir) const;
	[[nodiscard]] auto get_module_directory() const -> std::string;
	[[nodiscard]] std::string build_define(std::string_view const def);
	void fill_compiler_collection();
	bool set_compiler(std::string_view comp);
	bool ensure_response_file_exists(std::string_view resp) const;
	bool check_response_files(std::string_view args);
};
