#include "HighResClock.h"

#ifdef _WIN32

#include <windows.h>

HighResClock::time_point HighResClock::now()
{
	static const long long frequency = []()
	{
		LARGE_INTEGER f;
		QueryPerformanceFrequency(&f);
		return f.QuadPart;
	}();
	
	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);
	return time_point(duration(count.QuadPart * static_cast<rep>(period::den) / frequency));
}

#endif
