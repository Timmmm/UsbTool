#include "Util_Win.h"

#if defined(_WIN32)

#include <windows.h>

std::string GetLastErrorAsString()
{
	return LastErrorAsString(::GetLastError());
}

std::string LastErrorAsString(uint32_t error)
{
	if (error == 0)
		return std::string();
	
	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPSTR>(&messageBuffer),
		0,
		nullptr
	);
	
	std::string message(messageBuffer, size);
	
	LocalFree(messageBuffer);
	
	return message;
}

#endif
