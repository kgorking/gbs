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
elseif (CMAKE_VERSION VERSION_EQUAL "4.1.1")
	set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
else()
	message(FATAL_ERROR "Unsupported CMake version: '${CMAKE_VERSION}', please use CMake 3.30.2, 3.31.6, or 4.0.2")
endif()


set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_MODULE_STD ON)


project ("gbs" LANGUAGES CXX)
add_executable (gbs "src/gbs.cpp" "src/msvc.cpp")
target_sources(gbs PUBLIC FILE_SET CXX_MODULES FILES
	"src/source_enum.cppm"
	"src/cmd_get_cl.cppm"
	
	"src/context.cppm"
	"src/compiler.cppm"
	"src/env.cppm"
	"src/dep_scan.cppm"
	"src/get_source_groups.cppm"
	"src/cmd_build.cppm"
	"src/cmd_enum_cl.cppm"
	"src/cmd_clean.cppm"
	"src/cmd_run.cppm"
	"src/cmd_cl.cppm"
	"src/cmd_ide.cppm"
	"src/monad.cppm"
	"src/cmd_version.cppm"
	"src/cmd_config.cppm")

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
    set(CMAKE_CXX_STANDARD_LIBRARY "libc++")
    target_compile_options(gbs PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wpedantic
    )
endif()
