#pragma once

#if defined(__APPLE__)

#include <string>

#include <mach/mach.h>

// Convert a kern_return_t error to a string. Only some errors are recognised
// - generic ones, and some IOKit errors.
std::string KernReturnToString(kern_return_t kr);

#endif
