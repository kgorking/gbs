export module enumerate_compilers;
import compiler;
import env;
import enumerate_compilers_msvc;
import enumerate_compilers_clang;
import enumerate_compilers_gcc;

export void enumerate_compilers(environment const& env, auto&& callback) {
	enumerate_compilers_msvc(env, callback);
	enumerate_compilers_clang(env, callback);
	enumerate_compilers_gcc(env, callback);
}
