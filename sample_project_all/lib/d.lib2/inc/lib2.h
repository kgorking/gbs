#pragma once

#ifdef LIB2_EXPORTS
#define LIB2_API __declspec(dllexport)
#else
#define LIB2_API __declspec(dllimport)
#endif

LIB2_API bool does_it_work_dynamic();
