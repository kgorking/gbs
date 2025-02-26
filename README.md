# Gorking Build System
I wanted to try and create my own build system, just for fun.

## Todo
- [x] Create a simple build system
- [x] Automatic compilation without build scripts
	- [x] Find source files automatically
	- [x] Don't compile things that don't need it
    - [ ] Deduce executable name from current directory
	- [ ] Compile GBS with itself
- [x] Automated support for `import std;`
- [x] Enumerate supported compilers
	- [ ] Choose compiler from command `gbs cl=clang-19 build`
- [ ] Use response files for compilation variance
    - [x] Create variants for debug/release/analyze/sanitize/library/etc.
	- [x] Compile by selecting one-or-more response files, eg. `gbs build=release,analyze,avx2,hardened_stdlib`
	- [ ] Halt compilation on conflicting response files
	- [ ] Allow matrix builds, eg. `gbs build=debug/release,opt1/opt2/opt3` results in 2x3 builds
- [ ] Support multiple operations, eg. `gbs build=debug unittest build=release unittest`
- [ ] Support for compiling/linking libraries
	- [ ] Recursively call `gbs` in lib directories
- [ ] Support for compiling/running unit tests
- [ ] Support for fetching dependencies from external sites like GitHub.
- [ ] Support multiple compilers
	- [x] msvc
	- [x] clang-cl
	- [ ] clang
	- [ ] gcc
- [ ] Support for compiling externally fetched dependencies
- [ ] Support for running custom build steps before/after compilation

## Note
Yes, I'm aware of the irony of using CMake to make a build system.