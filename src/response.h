#pragma once
#include <string_view>

bool ensure_response_file_exists(context const& ctx, std::string_view resp);
bool check_response_files(context const& ctx, std::string_view args);