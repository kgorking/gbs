//#pragma once

#ifdef _WIN32
	#ifdef LIB2_EXPORTS
		#define LIB2_API __declspec(dllexport)
	#else
		#define LIB2_API __declspec(dllimport)
	#endif
#else
	#define LIB2_API
#endif

extern "C" __declspec(dllimport) bool lib2_does_it_work();
