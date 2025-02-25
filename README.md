# Gorking Build System
I wanted to try and create my own build system, just for fun.

## Todo
- [x] Create a simple build system
- [x] Automatic compilation without build scripts
	- [x] Find source files automatically
	- [x] Don't compile things that don't need it
    - [ ] Deduce executable name from current directory
- [x] Automated support for `import std;`
- [ ] Use response files for compilation variance
    - [ ] Create variants for debug/release/analyze/sanitize/library/etc.
	- [ ] Compile by selecting one-or-more response files, eg. `gbs build=release,analyze,avx2,hardened_stdlib`
	- [ ] Halt compilation on conflicting response files
- [ ] Support for compiling/linking libraries
	- [ ] Recursively call `gbs` in lib directories
- [ ] Support for compiling/running unit tests
- [ ] Support for fetching dependencies from external sites like GitHub.
- [ ] Support clang/gcc/whatever. Only msvc support for now.
- [ ] Support for compiling externally fetched dependencies
- [ ] Support for running custom build steps before/after compilation

## Note
Yes, I'm aware of the irony of using CMake to make a build system.