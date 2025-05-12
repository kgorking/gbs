# Gorking Build System
# 
#
cmake_minimum_required (VERSION 3.30)

#set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API ON)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "0e5b6991-d74f-4b3d-a41c-cf096e0b2508")

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()


project ("gbs" LANGUAGES CXX)
set(CMAKE_CXX_MODULE_STD 1)
set(CMAKE_CXX_STANDARD 23)

add_executable (gbs)
target_sources(gbs PUBLIC "src/gbs.cpp" "src/build_msvc.cpp" "src/get_cl.cpp" "src/build_clang.cpp")
target_sources(gbs PUBLIC FILE_SET CXX_MODULES FILES "src/source_enum.cppm" "src/response.cppm" "src/context.cppm" "src/compiler.cppm" "src/env.cppm")

target_compile_features(gbs PRIVATE cxx_std_23)