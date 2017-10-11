#pragma once

#include <string>

// This identifies a USB device on the system with an opaque platform-dependent handle.
struct DeviceId
{
#if defined(__APPLE__)
	// This is an up to 512 byte path. Null terminated.
	std::string path;
#elif defined(_WIN32)
	// This is something like this:
	//
	//     \\?\usb#vid_2ac7&pid_fffe#abcd123456789#{a5dcbf10-6530-11d2-901f-00c04fb951ed}
	//
	// It includes the Vendor ID, Product ID, serial number if present (abcd123456789)
	// and the interface GUID (which may be the 'USB Device' one).
	std::wstring path;
#endif
	
	bool operator==(const DeviceId& other) const {
		return path == other.path;
	}
	
	operator bool() const {
		return !path.empty();
	}
	
	bool operator!() const {
		return path.empty();
	}
};

std::string DeviceIdToString(const DeviceId& addr);
