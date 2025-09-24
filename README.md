# Gorking Build System v0.15
Creating my own build system for fun.

# Features
Uses a fixed directory structure to automatically find source files and compile them.

- top-level directory
  - `src` - source files for main executeable
  - `lib` - libraries, named subdirectory per library.
	- `lib/s.libname/` for static library named _libname_
	- `lib/d.libname/` for dynamic library named _libname_
  - `unittest` - unit tests (not implemented yet)
	- One `.cpp` file per test?
  - `deps` - dependencies (not implemented yet)
	- fetch from eg. github and build them
  - `*` (not implemented yet)
	- other directories, not starting with `.`, are compiled to their own executables

# Usage
`gbs` **_[commands...]_**

The following commands are supported:
- **version**
	- Shows the current version of the build system

- **build**_=[directories]_
	- Builds the current directory.
	- If no `config` is specified, `debug,warnings` is used by default.
	- _[directories]_ is not implemented yet.

- **clean**=_[configuration]_
	- Cleans the build output folder (`gbs.out`).
	- TODO: only clean specified configuration.
 
- **config**=_[configuration]_
	- Sets the configurations to use for compilation. Currently `debug`, `release`, `analyze` have built-in support, and will be created if not found.
	- A configuration name corresponds to a response file in the folder `.gbs/`.
		- Response files are simple text files containing command line arguments for the selected compiler.
		- They can be created manually or you can use the auto generated ones. You are free to change them as you see fit.
	- Example: `gbs config=release,analyze build` will perform a release build with additional analysis enabled.

- **run**_=[parameters]_
	- Runs the last built executable with the optionally specified parameters. If no parameters are specified, the executable is run without parameters.
	- If no executable is found, an error is returned.
	- Example: `gbs build run=version` run in gbs' directory will build gbs and run the built executable as `gbs version`.

- **get_cl**=_compiler_:_version_
	- Downloads the compiler with at least the specified version. Supports clang and gcc.
	- This also sets the compiler for subsequent commands, as if `cl=` was used.
	- Example: `gbs get_cl=gcc` will try and download the latest version of gcc.
	- Example: `gbs get_cl=clang:18` will try and download the latest version 18 of clang _(ie. 18.1.8)_.
	- Example: `gbs get_cl=clang:17.2.2` will try and download version 17.2.2 of clang.

- **cl**=_compiler_:_version_
	- Selects the compiler to use for subsequent commands.
		- If the compiler is not found, an error is returned.
	- Example: `gbs cl=msvc:19 build cl=clang:17.3.1 build`

- _TODO_ **new**=_project_name_
	- Creates a new project with the specified name in the current directory.
	- The project will have the following structure:
		- `src` - source files for main executeable
		- `lib` - libraries
		- `unittest` - unit tests
		- `deps` - dependencies
	- _Not implemented yet_

- _TODO_ **unittest**_=[directories]_
	- Builds and runs unit tests in the specified directories. If no directories are specified, the current directory is used.
	- Unit tests are expected to be in the `unittest` subdirectory of the specified directory.
	- _Not implemented yet_

- **enum_cl**
	- Enumerates installed compilers

- **ide**=_[ide]_
	- Generates **tasks.vs.json** for the specified IDE. Supported IDEs are `vscode` and `vs`.
	- Example: `gbs ide=vs` will allow a folder to be opened in Visual Studio and allow the user to right-click the top-most folder and have several build options available, without needing a project or solution.

# Supported compilers
The following compilers can produce a working executeable:

- [x] msvc 19.38+
- [ ] clang 21+ (crashes on 'gbs' compile when using modules)
- [x] gcc 15+

# Upcoming versions (not in a specific order)
- Compile unit tests in 'unittest'
- Fetch dependencies in 'deps'
- Build dependencies in 'deps'
- Custom build steps (via 'run'?)

## Todo
- [ ] Good documentation for all commands
- [ ] Compiling/running unit tests
- [ ] Fetch dependencies from external sites like GitHub.
	- [ ] Use installed package managers
	- [ ] Support for compiling externally fetched dependencies
- [ ] Add a `get_cl=compiler:?` option to list versions available for download
- [ ] Allow matrix builds, eg. `gbs build=[debug,release] run` results in 2 builds and 2 runs
- [ ] Support for running custom build steps before/after compilation
- [ ] WSL support ?
- [x] Create a simple build system
- [x] Automatic compilation without build scripts
	- [x] Find source files automatically
	- [x] Don't compile things that don't need it
    - [x] Deduce executable name from current directory
	- [x] Compile module sources
		- [x] Figure out a way to do it in correct order
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
