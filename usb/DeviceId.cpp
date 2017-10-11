#include "DeviceId.h"

#include <sstream>

#if defined(__APPLE__)

std::string DeviceIdToString(const DeviceId& addr)
{
	return addr.path;
}
#elif defined(_WIN32)

std::string DeviceIdToString(const DeviceId& addr)
{
	std::string s(addr.path.size(), '?');
	for (unsigned int i = 0; i < addr.path.size(); ++i)
	{
		if (addr.path[i] <= 0xFF)
			s[i] = static_cast<char>(addr.path[i]);
	}
	return s;
}

#endif
