# Gorking Build System
#
#
cmake_minimum_required (VERSION 3.30.2)

set(CMAKE_CXX_STANDARD 23)

if (CMAKE_VERSION VERSION_EQUAL "3.31.6")
	set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")
elseif (CMAKE_VERSION VERSION_EQUAL "3.30.2")
	set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")
elseif (CMAKE_VERSION VERSION_EQUAL "4.0.2")
	set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "a9e1cf81-9932-4810-974b-6eccaf14e457")
else()
	message(FATAL_ERROR "Unsupported CMake version: '${CMAKE_VERSION}', please use CMake 3.30.2, 3.31.6, or 4.0.2")
endif()


set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_MODULE_STD ON)


project ("gbs" LANGUAGES CXX)
add_executable (gbs "src/gbs.cpp" "src/msvc.cpp")
target_sources(gbs PUBLIC FILE_SET CXX_MODULES FILES "src/source_enum.cppm" "src/cmd_get_cl.cppm" "src/response.cppm" "src/context.cppm" "src/compiler.cppm" "src/env.cppm" "src/dep_scan.cppm" "src/get_source_groups.cppm" "src/cmd_build.cppm" "src/cmd_enum_cl.cppm" "src/cmd_clean.cppm" "src/cmd_run.cppm" "src/cmd_cl.cppm" "src/monad.cppm" "src/cmd_ide.cppm")

if(MSVC)
	target_compile_definitions(gbs PRIVATE _MSVC_STL_HARDENING=1 _MSVC_STL_DESTRUCTOR_TOMBSTONES=1)
endif()
