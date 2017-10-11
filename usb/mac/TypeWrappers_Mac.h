#pragma once

#include <memory>
#include <IOKit/usb/IOUSBLib.h>

// Wrappers for some OSX types that close their resources when they go out of scope.
// These classes cannot be copied so they generally have to be encapsulated in a shared_ptr.

class DeviceInterface
{
public:
	// IOUSBDeviceInterface650 requires OSX 10.9
	explicit DeviceInterface(IOUSBDeviceInterface650** d) : mDevice(d)
	{
	}
	~DeviceInterface()
	{
		if (mDevice != nullptr)
		{
			(*mDevice)->USBDeviceClose(mDevice);
			(*mDevice)->Release(mDevice);
		}
	}
	
	IOUSBDeviceInterface650** device() const
	{
		return mDevice;
	}

private:
	DeviceInterface(const DeviceInterface&) = delete;
	DeviceInterface& operator=(const DeviceInterface&) = delete;
	
	IOUSBDeviceInterface650** mDevice = nullptr;
};

class InterfaceInterface
{
public:
	// IOUSBInterfaceInterface700 requires OSX 10.9
	explicit InterfaceInterface(IOUSBInterfaceInterface700** i) : mIface(i)
	{
	}
	virtual ~InterfaceInterface()
	{
		if (mIface != nullptr)
		{
			(*mIface)->USBInterfaceClose(mIface);
			(*mIface)->Release(mIface);
		}
	}
	
	IOUSBInterfaceInterface700** iface() const
	{
		return mIface;
	}

private:
	InterfaceInterface(const InterfaceInterface&) = delete;
	InterfaceInterface& operator=(const InterfaceInterface&) = delete;
	
	IOUSBInterfaceInterface700** mIface = nullptr;
};

class LowLatencyBuffer
{
public:
	// LowLatencyBuffer needs to retain a reference to the InterfaceInterface that was used
	// to create it. It needs it to destroy the buffer when it is freed.
	explicit LowLatencyBuffer(uint8_t* buf, std::shared_ptr<InterfaceInterface> iface) : mBuffer(buf), mIface(iface)
	{
	}
	~LowLatencyBuffer()
	{
		if (mBuffer != nullptr && mIface)
		{
			std::cout << "Destroying low latency buffer: " << static_cast<void*>(mBuffer) << std::endl;
			(*mIface->iface())->LowLatencyDestroyBuffer(mIface->iface(), mBuffer);
		}
	}
	
	uint8_t* buffer() const
	{
		return mBuffer;
	}

private:
	LowLatencyBuffer(const LowLatencyBuffer&) = delete;
	LowLatencyBuffer& operator=(const LowLatencyBuffer&) = delete;
	
	uint8_t* mBuffer = nullptr;
	std::shared_ptr<InterfaceInterface> mIface;
};


