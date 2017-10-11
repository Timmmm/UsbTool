#if defined(__APPLE__)

#include "../Device.h"
#include "../DeviceInfo.h"
#include "./Device_Mac.h"

#include "util/scope_exit.h"
#include "util/EnumCasts.h"

#include "Util_Mac.h"
#include "TypeWrappers_Mac.h"

#include <string>
#include <thread>
#include <iostream>

using std::string;
using std::cerr;
using std::endl;

#include <IOKit/IOKitLib.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <mach/mach.h>
#include <mach/mach_port.h>
#include <mach/mach_time.h>
#include <CoreFoundation/CFNumber.h>

// Device Mac-specific Implementation.

// Return true if this is associated with a device (instead of default-constructed or closed).
bool Device::isOpen() const
{
	// Double-check everything is non-null.
	return data.device && data.device->device() != nullptr && (*data.device->device()) != nullptr;
}

void Device::close()
{
	data.interfaces.clear();
	data.device.reset();
}

// Get device speed.
SResult<Device::Speed> Device::speed() const
{
	if (!isOpen())
		return Err(string("Device not open"));

	auto dev = data.device->device();

	UInt8 speed = 0xFF;
	kern_return_t kr = (*dev)->GetDeviceSpeed(dev, &speed);
	if (kr != kIOReturnSuccess)
		return Err("Error getting device speed " + KernReturnToString(kr));

	switch (speed)
	{
	case kUSBDeviceSpeedLow:
		return Ok(Speed::Low);
	case kUSBDeviceSpeedFull:
		return Ok(Speed::Full);
	case kUSBDeviceSpeedHigh:
		return Ok(Speed::High);
	case kUSBDeviceSpeedSuper:
		return Ok(Speed::Super);
	case kUSBDeviceSpeedSuperPlus:
		return Ok(Speed::SuperPlus);
	}

	return Err("Unknown speed: " + std::to_string(speed));
}

SResult<std::vector<uint8_t>> Device::controlTransferInSync(Device::Recipient recipient,
                                                               Device::Type type,
                                                               uint8_t bRequest,
                                                               uint16_t wValue,
                                                               uint16_t wIndex,
                                                               uint16_t wLength)
{
	if (!isOpen())
		return Err(string("Device not open"));

	auto dev = data.device->device();

	std::vector<uint8_t> buffer(wLength);

	IOUSBDevRequest request;
	request.bmRequestType = to_integral(recipient) | to_integral(type) | to_integral(Direction::In);
	request.bRequest = bRequest;
	request.wValue = wValue;
	request.wIndex = wIndex;
	request.wLength = wLength;
	request.pData = buffer.data();
	request.wLenDone = 0;

	kern_return_t kr = (*dev)->DeviceRequest(dev, &request);
	if (kr != kIOReturnSuccess)
		return Err("Error sending control transfer: " + KernReturnToString(kr));

	// We may received less data than requested.
	if (buffer.size() > request.wLenDone)
		buffer.resize(request.wLenDone);

	return Ok(buffer);
}

SResult<void> Device::controlTransferOutSync(Device::Recipient recipient, Device::Type type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, std::vector<uint8_t> dat)
{
	if (!isOpen())
		return Err(string("Device not open"));

	if (dat.size() > 0xFFFF)
		return Err(string("Data too long for transfer"));

	auto dev = data.device->device();

	IOUSBDevRequest request;
	request.bmRequestType = to_integral(recipient) | to_integral(type) | to_integral(Direction::Out);
	request.bRequest = bRequest;
	request.wValue = wValue;
	request.wIndex = wIndex;
	request.wLength = dat.size();
	request.pData = dat.data();
	request.wLenDone = 0;

	kern_return_t kr = (*dev)->DeviceRequest(dev, &request);
	if (kr != kIOReturnSuccess)
		return Err("Error sending control transfer: " + KernReturnToString(kr));

	// We should always have sent exactly the amount we tried to.
	if (request.wLenDone != dat.size())
		return Err("Error sending control transfer: Send " + std::to_string(request.wLenDone) + " of " + std::to_string(dat.size()) + " bytes");

	return Ok();
}

SResult<UsbTransferHandle> Device::controlTransferIn(Device::Recipient recipient, Device::Type type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength)
{
	if (!isOpen())
		return Err(string("Device not open"));

	IOUSBDeviceInterface650** dev = data.device->device();

	auto buffer = std::make_shared<std::vector<uint8_t>>(wLength);

	IOUSBDevRequest request;
	request.bmRequestType = to_integral(recipient) | to_integral(type) | to_integral(Direction::In);
	request.bRequest = bRequest;
	request.wValue = wValue;
	request.wIndex = wIndex;
	request.wLength = wLength;
	request.pData = buffer->data();
	request.wLenDone = 0;
	
	UsbTransferHandle transferHandle;
	transferHandle.data->device = data.device;
	transferHandle.data->buffer = buffer;
	
	// Allocate a new copy of the transferHandle data. This keeps the device, and buffers
	// open until the transfer is completed.
	auto* userData = new std::shared_ptr<UsbTransferHandle::Data>(transferHandle.data);
	
	kern_return_t kr = (*dev)->DeviceRequestAsync(dev, &request, &UsbTransferHandle::callback, userData);
	if (kr != kIOReturnSuccess)
	{
		delete userData;
		return Err("Error sending control transfer: " + KernReturnToString(kr));
	}
	
	return Ok(transferHandle);
}

SResult<UsbTransferHandle> Device::controlTransferOut(Device::Recipient recipient, Device::Type type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, std::vector<uint8_t> dat)
{
	return Err(string("Unimplemented"));
}

SResult<IsochReadBuffer> Device::createIsochReadBuffer(int iface, uint8_t endpointAddress, int numFrames, int bytesPerFrame)
{
	if (!isOpen())
		return Err(string("Device not open"));

	if (iface < 0 || iface >= data.interfaces.size())
		return Err("Interface " + std::to_string(iface) + " out of range " + std::to_string(data.interfaces.size()));

	// The interface interface.
	IOUSBInterfaceInterface700** ifacep = data.interfaces[iface]->iface();

	uint8_t* readBuffer = nullptr;
	uint8_t* frameBuffer = nullptr;

	kern_return_t kr = (*ifacep)->LowLatencyCreateBuffer(ifacep, reinterpret_cast<void**>(&readBuffer), numFrames * bytesPerFrame, kUSBLowLatencyReadBuffer);
	if (kr != kIOReturnSuccess)
		return Err("Error creating isochronous read buffer: " + KernReturnToString(kr));

	kr = (*ifacep)->LowLatencyCreateBuffer(ifacep, reinterpret_cast<void**>(&frameBuffer), numFrames * sizeof(IOUSBLowLatencyIsocFrame), kUSBLowLatencyFrameListBuffer);
	if (kr != kIOReturnSuccess)
		return Err("Error creating isochronous frame list buffer: " + KernReturnToString(kr));

	if (readBuffer == nullptr || frameBuffer == nullptr)
		return Err(string("Null pointer creating isochronous buffer."));

	IsochReadBuffer buffer;
	
	buffer.readBuffer = std::make_shared<LowLatencyBuffer>(readBuffer, data.interfaces[iface]);
	buffer.frameBuffer = std::make_shared<LowLatencyBuffer>(frameBuffer, data.interfaces[iface]);
	buffer.interface = data.interfaces[iface];
	buffer.endpointAddress = endpointAddress;
	buffer.numFrames = numFrames;
	buffer.bytesPerFrame = bytesPerFrame;
	
	return Ok(buffer);
}

SResult<IsochWriteBuffer> Device::createIsochWriteBuffer(int iface, uint8_t endpointAddress, int numFrames, int bytesPerFrame)
{
	if (!isOpen())
		return Err(string("Device not open"));

	if (iface < 0 || iface >= data.interfaces.size())
		return Err("Interface " + std::to_string(iface) + " out of range " + std::to_string(data.interfaces.size()));

	// The interface interface.
	IOUSBInterfaceInterface700** ifacep = data.interfaces[iface]->iface();

	uint8_t* writeBuffer = nullptr;
	uint8_t* frameBuffer = nullptr;

	kern_return_t kr = (*ifacep)->LowLatencyCreateBuffer(ifacep, reinterpret_cast<void**>(&writeBuffer), numFrames * bytesPerFrame, kUSBLowLatencyWriteBuffer);
	if (kr != kIOReturnSuccess)
		return Err("Error creating isochronous write buffer: " + KernReturnToString(kr));
	
	std::cout << "Created low latency write buffer: " << static_cast<void*>(writeBuffer) << std::endl;

	kr = (*ifacep)->LowLatencyCreateBuffer(ifacep, reinterpret_cast<void**>(&frameBuffer), numFrames * sizeof(IOUSBLowLatencyIsocFrame), kUSBLowLatencyFrameListBuffer);
	if (kr != kIOReturnSuccess)
		return Err("Error creating isochronous frame list buffer: " + KernReturnToString(kr));

	std::cout << "Created low latency frame buffer: " << static_cast<void*>(frameBuffer) << std::endl;

	if (writeBuffer == nullptr || frameBuffer == nullptr)
		return Err(string("Null pointer creating isochronous buffer."));

	IsochWriteBuffer buffer;
	
	buffer.writeBuffer = std::make_shared<LowLatencyBuffer>(writeBuffer, data.interfaces[iface]);
	buffer.frameBuffer = std::make_shared<LowLatencyBuffer>(frameBuffer, data.interfaces[iface]);
	buffer.interface = data.interfaces[iface];
	buffer.endpointAddress = endpointAddress;
	buffer.numFrames = numFrames;
	buffer.bytesPerFrame = bytesPerFrame;
	
	return Ok(buffer);
}

SResult<UsbIsochTransferHandle> Device::submitIsoOutTransferAsap(const IsochWriteBuffer& buffer, bool continueStream)
{
	if (!isOpen())
		return Err(string("Device not open"));
	
	// Check we have the pipe info.
	if (buffer.interface->pipes.count(buffer.endpointAddress) != 1)
		return Err("Internal error: pipe for endpoint address " + std::to_string(buffer.endpointAddress) + " not found");
	
	auto& pipeData = buffer.interface->pipes.at(buffer.endpointAddress);

	IOUSBInterfaceInterface700** iface = buffer.interface->iface();
	
	// Get the current frame.
	UInt64 currentFrame = 0;
	AbsoluteTime atTime;
	kern_return_t kr = (*iface)->GetBusFrameNumber(iface, &currentFrame, &atTime);
	if (kr != kIOReturnSuccess)
		return Err("Error getting bus frame number: " + KernReturnToString(kr));
	
	UInt64 frame = continueStream ? pipeData.nextFrame : currentFrame + 1;
	
	// Fill in the frame list.
	IOUSBLowLatencyIsocFrame* frames = reinterpret_cast<IOUSBLowLatencyIsocFrame*>(buffer.frameBuffer->buffer());
	for (int i = 0; i < buffer.numFrames; ++i)
	{
		frames[i].frReqCount = buffer.bytesPerFrame;
		frames[i].frActCount = 0;
		frames[i].frStatus = kIOReturnError;
	}

	// Create a new transfer handle.
	UsbIsochTransferHandle transferHandle;
	transferHandle.data->device = data.device;
	transferHandle.data->iface = buffer.interface;
	transferHandle.data->readOrWriteBuffer = buffer.writeBuffer;
	transferHandle.data->frameBuffer = buffer.frameBuffer;
	
	// Allocate a new copy of the transferHandle data. This keeps the device, and buffers
	// open until the transfer is completed.
	auto* userData = new std::shared_ptr<UsbIsochTransferHandle::Data>(transferHandle.data);
	
	for (int i = 0;; ++i)
	{
		if (i > 32 || (continueStream && i > 0))
		{
			delete userData;
			return Err(string("Couldn't schedule isochronous transfer."));
		}
		
		kr = (*iface)->LowLatencyWriteIsochPipeAsync(iface,
		                                             pipeData.pipeRef,
		                                             buffer.writeBuffer->buffer(),
		                                             frame,
		                                             buffer.numFrames,
		                                             0, // Frame list frequency in milliseconds. 0 means at the end of the transfer. We don't really need the frame list updated - we can just use the frame number.
		                                             frames,
		                                             &UsbIsochTransferHandle::callback,
		                                             userData);
		if (kr == kIOReturnIsoTooOld)
		{
			++frame;
		}
		else if (kr == kIOReturnSuccess)
		{
			break;
		}
		else
		{
			delete userData;
			return Err("Error submitting isoch transfer: " + KernReturnToString(kr));
		}
	}
	
	pipeData.nextFrame = frame + buffer.numFrames;

	return Ok(transferHandle);
}

SResult<UsbIsochTransferHandle> Device::submitIsoOutTransfer(const IsochWriteBuffer& buffer, uint64_t frame)
{
	if (!isOpen())
		return Err(string("Device not open"));
	
	// Check we have the pipe info.
	if (buffer.interface->pipes.count(buffer.endpointAddress) != 1)
		return Err("Internal error: pipe for endpoint address " + std::to_string(buffer.endpointAddress) + " not found");
	
	auto& pipeData = buffer.interface->pipes.at(buffer.endpointAddress);

	IOUSBInterfaceInterface700** iface = buffer.interface->iface();
	
	// Get the current frame.
	UInt64 currentFrame = 0;
	AbsoluteTime atTime;
	kern_return_t kr = (*iface)->GetBusFrameNumber(iface, &currentFrame, &atTime);
	if (kr != kIOReturnSuccess)
		return Err("Error getting bus frame number: " + KernReturnToString(kr));
	
	// Fill in the frame list.
	IOUSBLowLatencyIsocFrame* frames = reinterpret_cast<IOUSBLowLatencyIsocFrame*>(buffer.frameBuffer->buffer());
	for (int i = 0; i < buffer.numFrames; ++i)
	{
		frames[i].frReqCount = buffer.bytesPerFrame;
		frames[i].frActCount = 0;
		frames[i].frStatus = kIOReturnError;
	}

	// Create a new transfer handle.
	UsbIsochTransferHandle transferHandle;
	transferHandle.data->device = data.device;
	transferHandle.data->iface = buffer.interface;
	transferHandle.data->readOrWriteBuffer = buffer.writeBuffer;
	transferHandle.data->frameBuffer = buffer.frameBuffer;
	
	// Allocate a new copy of the transferHandle data. This keeps the device, and buffers
	// open until the transfer is completed.
	auto* userData = new std::shared_ptr<UsbIsochTransferHandle::Data>(transferHandle.data);
	
	std::cout << "Submitting transfer for frame " << frame << " current frame: " << getBusFrameNumber() << " at time (nanos): " << mach_absolute_time() << std::endl;
	
	kr = (*iface)->LowLatencyWriteIsochPipeAsync(iface,
												 pipeData.pipeRef,
												 buffer.writeBuffer->buffer(),
												 frame,
												 buffer.numFrames,
												 0, // Frame list frequency in milliseconds. 0 means at the end of the transfer. We don't really need the frame list updated - we can just use the frame number.
												 frames,
												 &UsbIsochTransferHandle::callback,
												 userData);
	if (kr != kIOReturnSuccess)
	{
		delete userData;
		return Err("Error submitting isoch transfer: " + KernReturnToString(kr));
	}

	pipeData.nextFrame = frame + buffer.numFrames;

	return Ok(transferHandle);
}

uint64_t Device::getBusFrameNumber()
{
	if (!isOpen())
		return 0;
	
	IOUSBDeviceInterface650** dev = data.device->device();
	UInt64 frame;
	uint64_t atTime;
	
	kern_return_t kr = (*dev)->GetBusFrameNumberWithTime(dev, &frame, reinterpret_cast<AbsoluteTime*>(&atTime));
	if (kr != kIOReturnSuccess)
		return 0;
	
	// Adjust our estimate by atTime.
	uint64_t now = mach_absolute_time();
	
	static mach_timebase_info_data_t timebase = {0, 0};
	if (timebase.denom == 0)
		mach_timebase_info(&timebase);
	
	uint32_t elapsedNano = (now - atTime) * timebase.numer / timebase.denom;
	
	// Something like this. 1 frame is 1 ms.
	frame += (elapsedNano / 1000000);
	
	return frame;
}

int Device::numInterfaces()
{
	if (!isOpen())
		return 0;
	return data.interfaces.size();
}

SResult<void> Device::setAlternate(int iface, uint8_t alternate)
{
	if (!isOpen())
		return Err(string("Device not open"));

	if (iface < 0 || iface >= data.interfaces.size())
		return Err("Interface " + std::to_string(iface) + " out of range " + std::to_string(data.interfaces.size()));

	// The interface interface.
	IOUSBInterfaceInterface700** ifacep = data.interfaces[iface]->iface();
	
	kern_return_t kr = (*ifacep)->SetAlternateInterface(ifacep, alternate);
	if (kr != kIOReturnSuccess)
		return Err("Error setting interface alternate: " + KernReturnToString(kr));
	
	TRY(data.interfaces[iface]->refreshPipes());
	
	return Ok();
}

SResult<std::vector<uint8_t>> UsbTransferHandle::result(bool block)
{
	std::unique_lock<std::mutex> lock(data->mutex);

	// Wait until it is done.
	if (block)
		data->condition.wait(lock, [&] { return data->done; });
	
	if (!data->done)
		return Err(string("Transfer not finished."));
	
	if (data->result != kIOReturnSuccess)
		return Err("Transfer error: " + KernReturnToString(data->result));
	
	return Ok(*data->buffer);
}

void UsbTransferHandle::callback(void* refcon, IOReturn result, void* arg0)
{
	if (refcon == nullptr)
	{
		cerr << "Refcon null! Fatal internal error argh!!" << endl;
		abort();
		return;
	}
	
	std::shared_ptr<UsbTransferHandle::Data>* dataPtrPtr = static_cast<std::shared_ptr<UsbTransferHandle::Data>*>(refcon);
	
	std::unique_lock<std::mutex> lock((*dataPtrPtr)->mutex);

	(*dataPtrPtr)->result = result;
//	data->transferred = ??? TODO: Get this from arg0
	(*dataPtrPtr)->done = true;
	
	(*dataPtrPtr)->condition.notify_one();
	
	// We don't need the temporary shared_ptr to the TransferHandle::Data any more - the transfer is done.
	delete dataPtrPtr;
}


SResult<int> UsbIsochTransferHandle::result(bool block)
{
	std::unique_lock<std::mutex> lock(data->mutex);

	// Wait until it is done.
	if (block)
		data->condition.wait(lock, [&] { return data->done; });
	
	if (!data->done)
		return Err(string("Transfer not finished."));
	
	if (data->result != kIOReturnSuccess)
		return Err("Isoch transfer error: " + KernReturnToString(data->result));
	
	return Ok(data->transferred);
}

void UsbIsochTransferHandle::callback(void* refcon, IOReturn result, void* arg0)
{
	// arg0 is a pointer to the framelist and can be used to identify the particular request apparently.
	
	if (refcon == nullptr)
	{
		cerr << "Refcon null! Fatal internal error argh!" << endl;
		abort();
		return;
	}
	
	std::cout << "Transfer finished at time (nanos): " << mach_absolute_time() << std::endl;
	
	
	std::shared_ptr<UsbIsochTransferHandle::Data>* dataPtrPtr = static_cast<std::shared_ptr<UsbIsochTransferHandle::Data>*>(refcon);
	
	std::unique_lock<std::mutex> lock((*dataPtrPtr)->mutex);

	(*dataPtrPtr)->result = result;
//	data->transferred = ??? TODO: Get this from the frame list (arg0).
	(*dataPtrPtr)->done = true;
	
	(*dataPtrPtr)->condition.notify_one();
	
	// We don't need the shared_ptr to the TransferHandle::Data any more - the transfer is done.
	delete dataPtrPtr;
}

const uint8_t* IsochReadBuffer::data() const
{
	return readBuffer ? readBuffer->buffer() : nullptr;
}

int IsochReadBuffer::size() const
{
	return bytesPerFrame * numFrames;
}

uint8_t* IsochWriteBuffer::data()
{
	return writeBuffer ? writeBuffer->buffer() : nullptr;
}

int IsochWriteBuffer::size()
{
	return bytesPerFrame * numFrames;
}

SResult<void> InterfaceWithMetadata::refreshPipes()
{
	// Make a map from endpoint address to the pipe reference.
	UInt8 interfaceNumEndpoints = 0;

	// Get the number of endpoints associated with this interface
	kern_return_t kr = (*iface())->GetNumEndpoints(iface(), &interfaceNumEndpoints);
	if (kr != kIOReturnSuccess)
		return Err("Couldn't get USB interface endpoint count: " + KernReturnToString(kr));

	cerr << "Interface has " << int(interfaceNumEndpoints) << " endpoints" << endl;
	
	pipes.clear();
	
	// The pipe at index 0 is the default control pipe and should be
	// accessed using (*Device)->DeviceRequest() instead
	
	for (uint8_t pipeRef = 1; pipeRef <= interfaceNumEndpoints; ++pipeRef)
	{
		UInt8 direction = 0;
		UInt8 number = 0;
		UInt8 transferType = 0;
		UInt16 maxPacketSize = 0;
		UInt8 interval = 0;

		kr = (*iface())->GetPipeProperties(iface(),
		                                     pipeRef,
		                                     &direction,
		                                     &number,
		                                     &transferType,
		                                     &maxPacketSize,
		                                     &interval);
		if (kr != kIOReturnSuccess)
			return Err("Couldn't get USB pipe properties: " + KernReturnToString(kr));
		
		uint8_t pipeAddress = number;
		
		if (direction == kUSBIn)
			pipeAddress |= 1 << kUSBRqDirnShift;
		
		pipes[pipeAddress].pipeRef = pipeRef;
		cerr << "Interface pipe address " << int(pipeAddress) << " -> pipe ref " << int(pipeRef) << endl;
	}
	
	return Ok();
}

#endif
