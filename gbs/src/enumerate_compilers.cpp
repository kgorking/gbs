#include "../inc/enumerate_compilers.h"
#include "../inc/env.h"
#include "../inc/os.h"

void enumerate_compilers(environment const& env, std::function<void(compiler&&)> callback) {
	enumerate_compilers_msvc(env, callback);
	enumerate_compilers_clang(env, callback);
	enumerate_compilers_gcc(env, callback);
}
