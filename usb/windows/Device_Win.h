#pragma once

#if defined(_WIN32)

#include "TypeWrappers_Win.h"

class UsbIsochBufferHandle
{
public:
	// TODO: Keep a copy of the device file handle so it isn't closed prematurely?
	std::shared_ptr<WinUsbIsochBufferHandle> bufferHandle;
	std::shared_ptr<WinUsbInterfaceHandle> interfaceHandle;
};

// Handle to an asynchronous normal pipe operation.
class UsbTransferHandle
{
	friend class UsbDevice;
public:
	
	// Returns bytes transferred on success, except it always is 0.
	SResult<std::vector<uint8_t>> result(bool block = true);

//private:
	// These must always stay at fixed addresses.
	// TODO: Rather than store all these in separate shared_ptr's, it
	// could be better to disable copy/move for this class and then store
	// the whole class in one.
	std::shared_ptr<std::vector<uint8_t>> buffer;
	std::shared_ptr<uint32_t> transferred;
	std::shared_ptr<Overlapped> overlapped;
	std::shared_ptr<WinUsbInterfaceHandle> interfaceHandle;
};

class UsbIsochTransferHandle
{
	friend class UsbDevice;
public:
	
	// Returns bytes transferred on success, except it always is 0.
	SResult<int> result(bool block = true);

private:
	// TODO: This should keep a reference to the isoch buffer too.
	// TODO: This should probably block for the transfer to complete (if it hasn't) in the 
	// descructor. Otherwise what happens if you destroy it while the transfer is running?
	std::shared_ptr<Overlapped> overlapped;
	std::shared_ptr<WinUsbInterfaceHandle> interfaceHandle;
};

class IsochReadBuffer
{
};

class IsochWriteBuffer
{
};

struct UsbDeviceData
{
	// The following interface handles are in shared_ptr's because
	// TransferHandles need to keep references to them. They are automatically
	// freed when the last reference to them dies.

	// A handle to the device.
	std::shared_ptr<WindowsHandle> deviceHandle;
	
	// This is the handle for the first interface of the device.
	std::shared_ptr<WinUsbInterfaceHandle> winUsbInterfaceHandle;
	
	// These are handles for the subsequent interfaces of the device.
	std::vector<std::shared_ptr<WinUsbInterfaceHandle>> winUsbAssocInterfaceHandles;
	
	// Cached.
	DeviceDescriptor descriptors;
	
	// The device address.
	DeviceId address;
};

#endif
