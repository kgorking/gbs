﻿# Gorking Build System
# 
#
cmake_minimum_required (VERSION 3.28)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("gbs")
add_executable (gbs)
target_sources(gbs PUBLIC "src/gbs.cpp"   "src/compiler.h" "src/compiler.cpp"  "src/build_msvc.cpp" "src/response.cpp" "src/get_cl.cpp" "src/response.h" "src/home.h" "src/build_clang.cpp")
target_sources(gbs PUBLIC FILE_SET CXX_MODULES FILES "src/source_enum.cppm")
set_property(TARGET gbs PROPERTY CXX_STANDARD 26)
