#pragma once
#include <string_view>

class context;
bool cmd_cl(context& ctx, std::string_view args);
bool cmd_clean(context& ctx, std::string_view args);
bool cmd_config(context& ctx, std::string_view args);
bool cmd_build(context& ctx, std::string_view);
bool cmd_enum_cl(context& ctx, std::string_view args);
bool cmd_get_cl(context& ctx, std::string_view args);
bool cmd_ide(context&, std::string_view args);
bool cmd_unittest(context& ctx, std::string_view args);
bool cmd_version(context& ctx, std::string_view args);
