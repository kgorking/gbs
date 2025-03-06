# Gorking Build System
I wanted to try and create my own build system, just for fun.

## Upcoming versions
#### v0.06: Download msvc
#### v0.07: Compile test project with clang/clang-cl
#### v0.08: Compile test project with gcc
#### v0.09: Make `import std;` work for clang and gcc
#### v0.10: Fix module dependencies, (recurse-retry)
#### v0.11: Compile and link libraries in 'lib'
#### v0.12: Compile unit tests in 'unittest'
#### v0.13: Fetch dependencies in 'deps'
#### v0.14: Build dependencies in 'deps'
#### v0.15: Custom build steps (via 'run'?)

## Todo
- [x] Create a simple build system
- [x] Automatic compilation without build scripts
	- [x] Find source files automatically
	- [x] Don't compile things that don't need it
    - [x] Deduce executable name from current directory
	- [x] Compile module sources
		- [ ] Figure out a way to do it in correct order
	- [x] Compile nested sources
	- [x] Compile GBS with itself
- [x] Automated support for `import std;`
	- [x] msvc
	- [ ] clang-cl
	- [ ] clang
	- [ ] gcc
- [x] Enumerate supported compilers
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
- [ ] Support multiple compilers
	- [x] msvc
	- [ ] clang-cl
	- [ ] clang
	- [ ] gcc
- [ ] Download compilers, eg. `gbs get_cl=clang:19`
	- [ ] msvc
	- [x] clang
	- [x] gcc
- [ ] Support for running custom build steps before/after compilation
- [ ] Integrate with Visual Studio [Code]
- [ ] WSL

## Note
Yes, I'm aware of the irony of using CMake to make a build system.