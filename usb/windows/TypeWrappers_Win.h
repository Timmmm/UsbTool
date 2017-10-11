#pragma once

#if defined(_WIN32)

#include <windows.h>
#include <winusb.h>

// Wrappers for some Windows types that close their resources when they go out of scope.
class WinUsbInterfaceHandle
{
public:
	explicit WinUsbInterfaceHandle(WINUSB_INTERFACE_HANDLE h) : handle(h)
	{
	}
	~WinUsbInterfaceHandle()
	{
		if (handle != nullptr)
			WinUsb_Free(handle);
	}
	
	WinUsbInterfaceHandle(WinUsbInterfaceHandle&& other)
	{
		handle = other.handle;
		other.handle = nullptr;
	}

	WinUsbInterfaceHandle& operator=(WinUsbInterfaceHandle&& other)
	{
		handle = other.handle;
		other.handle = nullptr;
		return *this;
	}
	
	WINUSB_INTERFACE_HANDLE handle = nullptr;
private:
	WinUsbInterfaceHandle(const WinUsbInterfaceHandle&) = delete;
	WinUsbInterfaceHandle& operator=(const WinUsbInterfaceHandle&) = delete;
};

class WinUsbIsochBufferHandle
{
public:
	explicit WinUsbIsochBufferHandle(WINUSB_ISOCH_BUFFER_HANDLE h) : handle(h)
	{
	}
	~WinUsbIsochBufferHandle()
	{
		if (handle != nullptr)
			WinUsb_UnregisterIsochBuffer(handle);
	}
	
	WinUsbIsochBufferHandle(WinUsbIsochBufferHandle&& other)
	{
		handle = other.handle;
		other.handle = nullptr;
	}

	WinUsbIsochBufferHandle& operator=(WinUsbIsochBufferHandle&& other)
	{
		handle = other.handle;
		other.handle = nullptr;
		return *this;
	}
	
	WINUSB_ISOCH_BUFFER_HANDLE handle = nullptr;
private:
	WinUsbIsochBufferHandle(const WinUsbIsochBufferHandle&) = delete;
	WinUsbIsochBufferHandle& operator=(const WinUsbIsochBufferHandle&) = delete;
};

class WindowsHandle
{
public:
	explicit WindowsHandle(HANDLE h) : handle(h)
	{
	}
	~WindowsHandle()
	{
		if (handle != nullptr)
			CloseHandle(handle);
	}
	
	WindowsHandle(WindowsHandle&& other)
	{
		handle = other.handle;
		other.handle = nullptr;
	}

	WindowsHandle& operator=(WindowsHandle&& other)
	{
		handle = other.handle;
		other.handle = nullptr;
		return *this;
	}
	
	HANDLE handle;
private:
	WindowsHandle(const WindowsHandle&) = delete;
	WindowsHandle& operator=(const WindowsHandle&) = delete;
};

class Overlapped
{
public:
	Overlapped()
	{
		memset(&overlapped, 0, sizeof(overlapped));
	}

	~Overlapped()
	{
		if (overlapped.hEvent != nullptr)
			CloseHandle(overlapped.hEvent);
	}
	
	// If the memory location of the OVERLAPPED moves, it seems like you
	// get semi-random crashes and stack corruption because the kernel
	// keeps a reference to it. That is why all copying
	// and moving is disabled for this class.
	//
	// This is not described in any of the reference documentation, but it is mentioned here:
	//
	// https://msdn.microsoft.com/en-us/library/windows/desktop/aa365683(v=vs.85).aspx
	OVERLAPPED overlapped;
private:
	Overlapped(const Overlapped&) = delete;
	Overlapped& operator=(const Overlapped&) = delete;
	Overlapped(const Overlapped&&) = delete;
	Overlapped& operator=(const Overlapped&&) = delete;
};

#endif
