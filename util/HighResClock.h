#pragma once

#include <chrono>

// Works around 1 ms resolution of std::chrono::high_resolution_clock on Windows.

#ifdef _WIN32

// std::chrono::high_resolution_clock has a resolution of only 1 ms in
// this version of mingw.
struct HighResClock
{
	typedef long long rep;
	typedef std::nano period;
	typedef std::chrono::duration<rep, period> duration;
	typedef std::chrono::time_point<HighResClock> time_point;
	static const bool is_steady = true;
	
	static time_point now();
};

#else

typedef std::chrono::high_resolution_clock HighResClock;

#endif
