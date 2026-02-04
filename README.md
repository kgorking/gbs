# Gorking Build System v0.18

A rule-based build system to automatically find source files and compile them. No need for build scripts.

# Rules
- A root folder holds one-or-more projects.
- Source files are recursively found in the `src` folder of each project or library.
- Projects are placed in subfolders.
  - The name of the project folder is the name of the resulting executable.
  - Names starting with `s.` or `d.` produce static- and dynamic libraries, respectively.
- `lib` folder is for libraries.
  - Libraries are shared across all projects.
  - Static libraries are in folders starting with `s.`.
  - Dynamic libraries are in folders starting with `d.`.
  - Other folders are added to the includes list. They are not searched for source files.
  - Include paths for libraries are automatically added to projects.
    - Also includes `inc` and `include` subfolders, if present.
- `unittest` folder is for unit tests.
  - Each `test.*.cpp` file is compiled into a unittest executable `test.*.exe`.
  - Other sourcefiles are linked to each unittest executable.
- Files and folders starting with `x.` are ignored.
- TODO: Files and folders postfixed with `.win`, `.linux`, `.darwin` are only compiled on matching platforms.

## Example folder structure
```
example/
├── lib/
│   ├── s.libname1/          (shared static library)
│   │   ├── inc/
│   │   └── src/
│   ├── d.libname2/          (shared dynamic library)
│   │   ├── inc/
│   │   └── src/
│   └── libname3/            (shared header-only include)
│   │   ├── inc/
│   │   └── src/             (for building implementions)
├── example/                 (executable - main project)
│   └── src/
├── other_executeable/       (another executable)
│   └── src/
├── unittest/
│   ├── test.example.cpp
│   └── test.other.cpp
└── d.some_dll/              (dynamic library)
    ├── inc/
    └── src/
```

# Usage
`gbs [command, ...]`

Calling `gbs` without any commands will do a debug build of the current directory and run all unittests.

The following commands are supported:

* `version` Shows the current version of the build system.
* `config=<reponse file, ...>` Sets the configurations to use for compilation.
    * Configurations are provided as a comma-separated list of response files. Currently `debug`, `release`, `analyze` have built-in support, and will be created if not found.
	* A configuration name corresponds to a response file in the folder `.gbs/`.
		* Response files are simple text files containing command line arguments for the selected compiler.
		* They can be created manually or you can use the auto generated ones. You are free to change them as you see fit.
	* Example: `gbs config=release,analyze build` will do an analyzed release build.
* `cl=<compiler>:<major.minor.patch>` Selects the compiler to use for subsequent commands.
	* `gbs cl=msvc build cl=clang:17.3.1 build` will first build with latest msvc, then build with clang 17.3.1.
* `build` Builds the current directory.
	* If no configuration is specified (via `config` command), `debug,warnings` is used by default.
* `clean` cleans the build output folder (`gbs.out`).
    * Uses same format as `config`.
	* TODO: only clean specified configuration (`=<configuration>`).
* `unittest=<args>` Runs built unittests.
    * `args` are passed verbatim to the unittest executables.
* `get_cl=<compiler>:<major.minor.patch>` Downloads the compiler with at least the specified version. Supports clang and gcc.
	* This also sets the compiler for subsequent commands, as if `cl=...` was used.
* `enum_cl` Enumerates installed compilers.
* `ide=<ide name>` Generates **tasks.vs.json** for the specified IDE.
    * Supported IDEs are `vscode` and `vs`.
	* Example: `gbs ide=vs` will let a folder to be opened in Visual Studio and allow the user to right-click a folder and have several build options available, without needing a project or solution.
* _TODO_ `new=<project_name>` Creates a new project with the specified name in the current directory.
	* If no name is specified, the current directory name is used.
    * The project will have the following structure:
		* `src` - source files for main executeable
		* `lib` - libraries
		* `unittest` - unit tests
		* `deps` - dependencies ?

# Supported compilers
The following compilers can be used by `gbs` to build `gbs`:

- [x] msvc 19.38+
- [x] clang 21+
- [x] gcc 15+ (linking error when using `std::print`)
  - Requires manual edits to `print` and `ostream` std headers to fix linking errors. In the function `vprint_unicode`, in both files, change the line:
	```cpp
		#if !defined(_WIN32) || defined(__CYGWIN__)
	```
	to
	```cpp
		#if 1//!defined(_WIN32) || defined(__CYGWIN__)
	```

# Upcoming versions (not in a specific order)
- Support package managers (vcpkg, conan, etc.)

## Todo
- [ ] Good documentation for all commands
- [x] Compiling/running unit tests
- [ ] Use package managers
- [ ] Add a `get_cl=compiler:?` option to list versions available for download
- [ ] Allow matrix builds, eg. `gbs build=[debug,release] run` results in 2 builds and 2 runs
- [ ] Support for running custom build steps before/after compilation
- [x] WSL support
- [x] Create a simple build system
- [x] Automatic compilation without build scripts
	- [x] Find source files automatically
	- [x] Don't compile things that don't need it
	- [ ] Don't link things that don't need it
    - [x] Deduce executable name from current directory
	- [x] Compile module sources in correct order
	- [x] Compile nested sources
	- [x] Compile GBS with itself
- [x] Automated support for `import std;`
	- [x] msvc
	- [x] clang 19+
	- [x] gcc 15+
- [x] Enumerate installed compilers
- [x] Choose compiler from command `gbs cl=msvc:19.44 build cl=clang:17 build`
- [x] Use response files for compilation variance
    - [x] Create variants for debug/release/analyze/sanitize/library/etc.
	- [x] Compile by selecting one-or-more response files, eg. `gbs build=release,analyze,avx2,hardened_stdlib`
- [x] Multiple operations, eg. `gbs build=debug unittest build=release unittest`
- [x] Compiling/linking of static libraries
	- [x] msvc
	- [x] clang
	- [x] gcc
- [x] Compiling/linking of dynamic libraries
	- [x] msvc
	- [x] clang
	- [x] gcc
- [x] Support multiple compilers
	- [x] msvc
	- [x] clang
	- [x] gcc
- [x] Download compilers, eg. `gbs get_cl=clang:19`
	- [x] msvc
	- [x] clang
	- [x] gcc
- [x] Integrate with Visual Studio [Code]
	- [x] Visual Studio
	- [x] Visual Studio Code


# Note
Yes, I am aware of the irony of using CMake to make a build system.
