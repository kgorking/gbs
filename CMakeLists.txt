# Gorking Build System
#

cmake_minimum_required (VERSION 4.1.0 FATAL_ERROR)

if (CMAKE_VERSION VERSION_GREATER "4.1.0")
	set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
else()
	message(FATAL_ERROR "Unknown uuid for CMake version: '${CMAKE_VERSION}', check the cmake github for new uuid")
endif()


set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project ("gbs" LANGUAGES CXX)

add_executable (gbs "gbs/src/gbs.cpp" "gbs/src/msvc.cpp")
target_sources(gbs PUBLIC FILE_SET CXX_MODULES FILES
	"gbs/src/source_enum.cppm"
	"gbs/src/cmd_get_cl.cppm"
	"gbs/src/context.cppm"
	"gbs/src/compiler.cppm"
	"gbs/src/env.cppm"
	"gbs/src/dep_scan.cppm"
	"gbs/src/get_source_groups.cppm"
	"gbs/src/cmd_build.cppm"
	"gbs/src/cmd_enum_cl.cppm"
	"gbs/src/cmd_clean.cppm"
	"gbs/src/cmd_cl.cppm"
	"gbs/src/cmd_ide.cppm"
	"gbs/src/monad.cppm"
	"gbs/src/cmd_version.cppm"
	"gbs/src/cmd_config.cppm"
    "gbs/src/cmd_unittest.cppm"
    "gbs/src/enumerate_compilers.cppm"
    "gbs/src/enumerate_compilers_clang.cppm"
    "gbs/src/enumerate_compilers_msvc.cppm"
    "gbs/src/enumerate_compilers_gcc.cppm"
	"gbs/src/wsl.cppm"
	"gbs/src/os.cppm"
	"gbs/src/task/thread_pool.cppm"
	"gbs/src/task/task_graph.cppm"
	"gbs/src/task/task.cppm"
)

if(MSVC)
	target_compile_definitions(gbs PRIVATE _MSVC_STL_HARDENING=1 _MSVC_STL_DESTRUCTOR_TOMBSTONES=1)
	target_compile_options(gbs PRIVATE /permissive- /Zc:preprocessor /GL /fastfail
		/W4         # Baseline reasonable warnings
		/WX         # Warnings are errors
		/w14242     # 'identifier': conversion from 'type1' to 'type2', possible loss of data
		/w14254     # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
		/w14263     # 'function': member function does not override any base class virtual member function
		/w14265     # 'classname': class has virtual functions, but destructor is not virtual instances of this class may not be destructed correctly 
		/w14287     # 'operator': unsigned/negative constant mismatch 
		/we4289     # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside the for-loop scope 
		/w14296     # 'operator': expression is always 'boolean_value' 
		/w14311     # 'variable': pointer truncation from 'type1' to 'type2' 
		/w14545     # expression before comma evaluates to a function which is missing an argument list 
		/w14546     # function call before comma missing argument list 
		/w14547     # 'operator': operator before comma has no effect; expected operator with side effect 
		/w14549     # 'operator': operator before comma has no effect; did you intend 'operator'? 
		/w14555     # expression has no effect; expected expression with side effect 
		/w14619     # pragma warning: there is no warning number 'number' 
		/w14640     # Enable warning on thread unsafe static member initialization 
		/w14826     # Conversion from 'type1' to 'type2' is sign-extended. This may cause unexpected runtime behavior. 
		/w14905     # wide string literal cast to 'LPSTR' 
		/w14906     # string literal cast to 'LPWSTR' 
		/w14928     # illegal copy-initialization; more than one user-defined conversion has been implicitly applied 
	)
	if(NOT VS_CONFIGURATION_TYPE STREQUAL "Debug")
		target_link_options(gbs PRIVATE /LTCG /OPT:REF /OPT:ICF)
	endif()
else()
    target_compile_options(gbs PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wpedantic
    )
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang") # GNU/MSVC
        #target_compile_options(gbs PRIVATE -stdlib=libc++)# -fmodule-mapper=../../c++/libc++.modules.json)
        #target_link_options(gbs PRIVATE -stdlib=libc++)
    else()
        target_compile_options(gbs PRIVATE -stdlib=libstdc++ -fmodule-mapper=../../c++/libstdc++.modules.json)
        target_link_options(gbs PRIVATE -stdlib=libstdc++)
    endif()
endif()
