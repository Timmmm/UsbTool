#pragma once

#if defined(_WIN32)

#include <string>
#include <cstdint>

// Get the last error and return it as a string.
std::string GetLastErrorAsString();

// Get the string for a given return from GetLastError();
std::string LastErrorAsString(uint32_t error);

#endif
