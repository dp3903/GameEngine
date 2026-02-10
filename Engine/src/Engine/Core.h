#pragma once

#ifndef ENGINE_PLATFORM_WINDOWS
	#error We only support Windows
#endif

#ifdef ENGINE_ENABLE_ASSERTS
	#include "Logger.h"
	#define ASSERT(x, ...) { if(!(x)) { ENGINE_LOG_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define ASSERT(x, ...) { if(!(x)) { ENGINE_LOG_ERROR("Assertion Failed: {0}", __VA_ARGS__); std::abort(); } }
#endif


#define BIT(x) (1 << x)