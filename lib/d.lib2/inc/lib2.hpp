#pragma once

#ifdef _MSC_VER
#ifdef LIB2_EXPORTS
#define LIB2_API __declspec(dllexport)
#else
#define LIB2_API //__declspec(dllimport) // ? huh
#endif
#else
#define LIB2_API
#endif

extern "C" LIB2_API bool does_it_work_dynamic();
