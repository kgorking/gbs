# Gorking Build System v0.13
I wanted to try and create my own build system, just for fun.

# Features
- TODO

# Note
Yes, I am aware of the irony of using CMake to make a build system.

# Upcoming versions (not in a specific order)
- Make `import std;` work for gcc
- Compile and link libraries in 'lib'
- Compile unit tests in 'unittest'
- Fetch dependencies in 'deps'
- Build dependencies in 'deps'
- Custom build steps (via 'run'?)

## Todo
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
	- [x] clang
	- [ ] gcc
- [x] Enumerate installed compilers
- [x] Choose compiler from command `gbs cl=msvc:19.44 build cl=clang:17 build`
- [x] Use response files for compilation variance
    - [x] Create variants for debug/release/analyze/sanitize/library/etc.
	- [x] Compile by selecting one-or-more response files, eg. `gbs build=release,analyze,avx2,hardened_stdlib`
	- [ ] Allow matrix builds, eg. `gbs build=[debug,release] run` results in 2 builds and 2 runs
- [x] Multiple operations, eg. `gbs build=debug unittest build=release unittest`
- [ ] Compiling/linking libraries
	- [ ] Recursively call `gbs` in lib directories
- [ ] Compiling/running unit tests
- [ ] Fetch dependencies from external sites like GitHub.
	- [ ] Use installed package managers
	- [ ] Support for compiling externally fetched dependencies
- [x] Support multiple compilers
	- [x] msvc
	- [x] clang
	- [x] gcc
- [x] Download compilers, eg. `gbs get_cl=clang:19`
	- [x] msvc
	- [x] clang
	- [x] gcc
- [ ] Support for running custom build steps before/after compilation
- [x] Integrate with Visual Studio [Code]
	- [x] Visual Studio
	- [x] Visual Studio Code
- [ ] WSL support ?
