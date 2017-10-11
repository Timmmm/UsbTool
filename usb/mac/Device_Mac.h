#pragma once

#if defined(__APPLE__)

#include <memory>
#include <thread>
#include <map>
#include <mutex>
#include <condition_variable>

#include "../DeviceId.h"
#include "../Descriptors.h"

#include "TypeWrappers_Mac.h"
#include "RunLoop.h"

// This contains a pointer to the interface interface,
// but also we record the pipe reference <-> endpoint address mapping
// and the last frame that isoch transfers occurred on for isoch endpoints.
//
// It can't be copied and must be put in a shared_ptr.
class InterfaceWithMetadata : public InterfaceInterface
{
public:
	using InterfaceInterface::InterfaceInterface;
	
	// Need to do this initially and after setting a new alternate.
	// This requeries OSX to find out the pipe references for the endpoints of this interface
	// (which will change when changing alternates).
	SResult<void> refreshPipes();
	
	struct PipeData
	{
		// The pipe reference for this pipe.
		uint8_t pipeRef = 0;
		// The next frame in which we should send/receive data so there are no gaps.
		// Pipes are unidirectional so we don't need separate in/out variables.
		// This is only used for isochronous pipes.
		uint64_t nextFrame = 0;
	};
	
	// Map from endpoint address (number & direction) to data.
	std::map<uint8_t, PipeData> pipes;
};

// These are buffers for one transfer, since that means we can do it in the same way on OSX and Windows.
// We'll only have 2 or 3 transfers in flight at a time anyway so don't need many buffers.
class IsochReadBuffer
{
public:
	const uint8_t* data() const;
	int size() const;
	
// private:
	std::shared_ptr<LowLatencyBuffer> readBuffer;
	std::shared_ptr<LowLatencyBuffer> frameBuffer;
	std::shared_ptr<InterfaceWithMetadata> interface;
	uint8_t endpointAddress;
	int numFrames;
	int bytesPerFrame;
};

class IsochWriteBuffer
{
public:
	uint8_t* data();
	int size();

// private:
	std::shared_ptr<LowLatencyBuffer> writeBuffer;
	std::shared_ptr<LowLatencyBuffer> frameBuffer;
	std::shared_ptr<InterfaceWithMetadata> interface;
	uint8_t endpointAddress;
	int numFrames;
	int bytesPerFrame;
};

// Handle to an asynchronous normal pipe operation.
class UsbTransferHandle
{
	friend class Device;
public:
	SResult<std::vector<uint8_t>> result(bool block = true);

private:
	static void callback(void* refcon, IOReturn result, void* arg0);

	class Data
	{
	public:
		Data() = default;
		~Data() = default;
		
		// Condition variable and status.
		std::condition_variable condition;
		std::mutex mutex;
		
		bool done = false;
		IOReturn result = kIOReturnInternalError;
		int transferred = 0;
		
		// We need to keep a reference to the interface and device so that it isn't destroyed before
		// the transfer is complete.
		std::shared_ptr<DeviceInterface> device;
		std::shared_ptr<InterfaceWithMetadata> iface;
		
		// And the buffer.
		std::shared_ptr<std::vector<uint8_t>> buffer;
	private:
		Data(const Data&) = delete;
		Data& operator=(const Data&) = delete;
	};
	
	std::shared_ptr<Data> data = std::make_shared<Data>();
};

struct UsbDeviceData;

class UsbIsochTransferHandle
{
	friend class Device;
public:
	// Returns bytes transferred on success, except it always is 0.
	SResult<int> result(bool block = true);

private:
	static void callback(void* refcon, IOReturn result, void* arg0);
	
	class Data
	{
	public:
		Data() = default;
		~Data() = default;
		
		// Condition variable and status.
		std::condition_variable condition;
		std::mutex mutex;
		
		bool done = false;
		IOReturn result = kIOReturnInternalError;
		int transferred = 0;
		
		// We need to keep a reference to the interface and device so that it isn't destroyed before
		// the transfer is complete.
		std::shared_ptr<DeviceInterface> device;
		std::shared_ptr<InterfaceWithMetadata> iface;
		// And either the read/write and frame buffers.
		std::shared_ptr<LowLatencyBuffer> readOrWriteBuffer;
		std::shared_ptr<LowLatencyBuffer> frameBuffer;
	private:
		Data(const Data&) = delete;
		Data& operator=(const Data&) = delete;
	};
	
	std::shared_ptr<Data> data = std::make_shared<Data>();
};


struct UsbDeviceData
{
	// The device address.
	DeviceId address;

	// The following interface handles are in shared_ptr's because
	// TransferHandles need to keep references to them. They are automatically
	// freed when the last reference to them dies.

	// Handle to the device.
	std::shared_ptr<DeviceInterface> device;

	// Handle to all the interfaces (we open all of them) in order.
	std::vector<std::shared_ptr<InterfaceWithMetadata>> interfaces;

	// Cached.
	DeviceDescriptor descriptors;
	
	// The run loop thread. This is the thread that async callbacks are run from.
	RunLoop runLoop;
};


#endif
