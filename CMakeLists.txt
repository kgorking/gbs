# Gorking Build System
# 
#
cmake_minimum_required (VERSION 3.30)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")

project ("gbs" LANGUAGES CXX)

set(CMAKE_CXX_MODULE_STD 1)

add_executable (gbs)
target_compile_features(gbs PRIVATE cxx_std_23)
target_sources(gbs PUBLIC "src/gbs.cpp" "src/build_msvc.cpp" "src/build_clang.cpp")
target_sources(gbs PUBLIC FILE_SET CXX_MODULES FILES "src/source_enum.cppm" "src/cmd_get_cl.cppm" "src/response.cppm" "src/context.cppm" "src/compiler.cppm" "src/env.cppm" "src/dep_scan.cppm" "src/get_source_groups.cppm" "src/cmd_build.cppm" "src/cmd_enum_cl.cppm")
