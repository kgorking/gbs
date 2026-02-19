#pragma once
#include "compiler.h"
#include <functional>
class environment;

void enumerate_compilers(environment const& env, std::function<void(compiler&&)> callback);
void enumerate_compilers_msvc(environment const& env, std::function<void(compiler&&)> callback);
void enumerate_compilers_clang(environment const& env, std::function<void(compiler&&)> callback);
void enumerate_compilers_gcc(environment const& env, std::function<void(compiler&&)> callback);
