#if defined(_WIN32)

#include <iostream>
#include <regex>
#include <codecvt>
#include <string>

using std::string;

#include "usb/Device.h"
#include "util/scope_exit.h"
#include "Util_Win.h"
#include "util/EnumCasts.h"
#include "TypeWrappers_Win.h"
#include "usb/Discovery.h"

#include <initguid.h>
#include <winusb.h>
#include <usb.h>
#include <windows.h>
#include <SetupAPI.h>
#include <Usbiodef.h>

// Return true if this is associated with a device (instead of default-constructed or closed).
bool Device::isOpen() const
{
	return data.winUsbInterfaceHandle ? true : false;
}

// Close the device if it is open. It's harmless to call this on a closed device.
void Device::close()
{
	data.winUsbInterfaceHandle.reset();
	data.winUsbAssocInterfaceHandles.clear();
}

// Get device speed.
SResult<Device::Speed> Device::speed() const
{
	if (!isOpen())
		return Err(string("Device not open"));
	
	UCHAR deviceSpeed = 0;
	ULONG length = sizeof(UCHAR);
	BOOL bResult = WinUsb_QueryDeviceInformation(data.winUsbInterfaceHandle->handle, DEVICE_SPEED, &length, &deviceSpeed);
	if (bResult == FALSE)
		return Err("WinUsb_QueryDeviceInformation: " + GetLastErrorAsString());

	switch (deviceSpeed)
	{
	case LowSpeed:
		return Ok(Speed::Low);
	case FullSpeed:
		return Ok(Speed::Full);
	case HighSpeed:
		return Ok(Speed::High);
	}

	return Err("Unknown device speed: " + std::to_string(deviceSpeed));
}

SResult<std::vector<uint8_t> > Device::controlTransferInSync(Device::Recipient recipient, Device::Type type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength)
{
	if (!isOpen())
		return Err(string("Device not open"));
	
	std::vector<uint8_t> buffer(wLength);
	
	WINUSB_SETUP_PACKET setup;
	setup.RequestType = to_integral(Recipient::Device) | to_integral(type) | to_integral(Direction::In);
	setup.Request = bRequest;
	setup.Value = wValue;
	setup.Index = wIndex;
	setup.Length = 0;
	
	ULONG transferred = 0;
	BOOL result = WinUsb_ControlTransfer(data.winUsbInterfaceHandle->handle, setup, buffer.data(), buffer.size(), &transferred, nullptr);
	if (result == FALSE)
		return Err("WinUsb_ControlTransfer: " + GetLastErrorAsString());
	
	if (transferred != buffer.size())
		return Err("WinUsb_ControlTransfer: Transferred " + std::to_string(transferred) + " Expected: " + std::to_string(buffer.size()));
	
	return Ok(buffer);
}

SResult<void> Device::controlTransferOutSync(Device::Recipient recipient, Device::Type type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, std::vector<uint8_t> dat)
{
	if (!isOpen())
		return Err(string("Device not open"));
	
	WINUSB_SETUP_PACKET setup;
	setup.RequestType = to_integral(Recipient::Device) | to_integral(type) | to_integral(Direction::Out);
	setup.Request = bRequest;
	setup.Value = wValue;
	setup.Index = wIndex;
	setup.Length = dat.size();
	
	ULONG transferred = 0;
	BOOL result = WinUsb_ControlTransfer(data.winUsbInterfaceHandle->handle, setup, const_cast<uint8_t*>(dat.data()), dat.size(), &transferred, nullptr);
	if (result == FALSE)
		return Err("WinUsb_ControlTransfer: " + GetLastErrorAsString());
	
	if (transferred != dat.size())
		return Err("WinUsb_ControlTransfer: Transferred " + std::to_string(transferred) + " Expected: " + std::to_string(dat.size()));
	
	return Ok();
}

SResult<UsbTransferHandle> Device::controlTransferIn(Device::Recipient recipient, Device::Type type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength)
{
	if (!isOpen())
		return Err(string("Device not open"));
	
	// OVERLAPPED must be at a fixed memory address!
	UsbTransferHandle transfer;
	transfer.interfaceHandle = data.winUsbInterfaceHandle;
	transfer.overlapped.reset(new Overlapped);
	
	transfer.overlapped->overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

	if (transfer.overlapped->overlapped.hEvent == INVALID_HANDLE_VALUE)
		return Err("CreateEvent: " + GetLastErrorAsString());

	transfer.buffer.reset(new std::vector<uint8_t>(wLength));
	
	transfer.transferred.reset(new uint32_t(0));
	
	WINUSB_SETUP_PACKET setup;
	setup.RequestType = to_integral(Recipient::Device) | to_integral(type) | to_integral(Direction::In);
	setup.Request = bRequest;
	setup.Value = wValue;
	setup.Index = wIndex;
	setup.Length = 0;

	BOOL result = WinUsb_ControlTransfer(data.winUsbInterfaceHandle->handle,
	                                     setup,
	                                     transfer.buffer->data(),
	                                     wLength,
	                                     (PULONG)transfer.transferred.get(),
	                                     &transfer.overlapped->overlapped);

	DWORD lastError = GetLastError();
	
	if (result == FALSE && lastError != ERROR_IO_PENDING)
		return Err("WinUsb_ControlTransfer: " + LastErrorAsString(lastError));
	
	return Ok(transfer);
}

SResult<UsbTransferHandle> Device::controlTransferOut(Device::Recipient recipient, Device::Type type, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, std::vector<uint8_t> dat)
{
	return Err(string("Not implemented"));
}

//SResult<UsbIsochBufferHandle> Device::registerIsochBuffer(int iface, uint8_t pipeId, uint8_t* buffer, int len)
//{
//	if (!isOpen())
//		return Err(string("Device not open"));
	
//	if (iface < 0 || iface >= data.winUsbAssocInterfaceHandles.size() + 1)
//		return Err("Interface out of range: " + std::to_string(iface));
	
//	std::shared_ptr<WinUsbInterfaceHandle>& interfaceHandle = iface == 0 ? data.winUsbInterfaceHandle : data.winUsbAssocInterfaceHandles[iface - 1];
	
//	WINUSB_ISOCH_BUFFER_HANDLE bufferHandle = INVALID_HANDLE_VALUE;
	
//	BOOL result = WinUsb_RegisterIsochBuffer(
//		interfaceHandle->handle,
//		pipeId,
//		buffer,
//		len,
//		&bufferHandle
//	);
	
//	if (result == FALSE)
//		return Err("WinUsb_RegisterIsochBuffer: " + GetLastErrorAsString());
	
//	// TODO: This is a bit ugly; we could do this without allocation.
//	IsochBufferHandle h;
//	h.bufferHandle.reset(new WinUsbIsochBufferHandle(bufferHandle));
//	h.interfaceHandle = interfaceHandle;
//	return Ok(h);
//}

SResult<std::vector<uint8_t>> UsbTransferHandle::result(bool block)
{
	if (!interfaceHandle || !overlapped || !transferred || !buffer)
		return Err(string("Transfer not started"));
	
	DWORD numBytes = 0;
	BOOL result = WinUsb_GetOverlappedResult(
	            interfaceHandle->handle,
	            &overlapped->overlapped,
	            &numBytes,
	            block
	            );
	
	if (result == FALSE)
		return Err("WinUsb_GetOverlappedResult: " + GetLastErrorAsString());
	
	if (numBytes != buffer->size())
		return Err("WinUsb_GetOverlappedResult: Received " + std::to_string(numBytes) + " bytes, expected " + std::to_string(buffer->size()));
	
	return Ok(*buffer);
}


SResult<int> UsbIsochTransferHandle::result(bool block)
{
	if (!interfaceHandle || !overlapped)
		return Err(string("Transfer not started"));
	
	DWORD numBytes = 0;
	BOOL result = WinUsb_GetOverlappedResult(
	            interfaceHandle->handle,
	            &overlapped->overlapped,
	            &numBytes,
	            block
	            );
	
	if (result == FALSE)
		return Err("WinUsb_GetOverlappedResult, Isoch: " + GetLastErrorAsString());
	
	return Ok(static_cast<int>(numBytes));
}

//SResult<UsbIsochTransferHandle> Device::submitIsoOutTransferAsap(const UsbIsochBufferHandle& bufferHandle,
//                                                                       int offset,
//                                                                       int size,
//                                                                       bool continueStream)
//{
//	if (!isOpen())
//		return Err(string("Device not open"));
	
//	// OVERLAPPED must be at a fixed memory address!
//	IsochTransferHandle transfer;
//	transfer.interfaceHandle = bufferHandle.interfaceHandle;
//	transfer.overlapped.reset(new Overlapped);
	
//	transfer.overlappedata.overlapped.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	
//	// Note we can use RegisterWaitForSingleObject to call a callback when this event is completed.
//	// It uses a thread pool of up to 500 threads, but we really shouldn't have that many transfers
//	// in-flight.
//	// https://msdn.microsoft.com/en-us/library/windows/desktop/ms685061(v=vs.85).aspx

//	if (transfer.overlappedata.overlapped.hEvent == INVALID_HANDLE_VALUE)
//		return Err("CreateEvent: " + GetLastErrorAsString());
	
//	BOOL result = WinUsb_WriteIsochPipeAsap(
//		bufferHandle.bufferHandle->handle,
//		offset,
//		size,
//		continueStream,
//		&transfer.overlappedata.overlapped);
	
//	DWORD lastError = GetLastError();
	
//	if (result == FALSE && lastError != ERROR_IO_PENDING)
//		return Err("WinUsb_WriteIsochPipeAsap: " + LastErrorAsString(lastError));
	
//	return Ok(transfer);
//}

//uint64_t Device::getBusFrameNumber()
//{
//	if (!isOpen())
//		return 0;
	
//	ULONG frame;
//	LARGE_INTEGER timestamp;
//	WinUsb_GetCurrentFrameNumber(..., &frame, &timestamp);
	
//	// Adjust it to 'now'.
//	WinUsb_GetAdjustedFrameNumber(&frame, timestamp);

//	return frame;
//}

int Device::numInterfaces()
{
	if (!isOpen())
		return 0;
	return 1 + data.winUsbAssocInterfaceHandles.size();
}

SResult<void> Device::setAlternate(int iface, uint8_t alternate)
{
	if (!isOpen())
		return Err(string("Device not open"));
	
	if (iface < 0 || iface >= data.winUsbAssocInterfaceHandles.size() + 1)
		return Err("Interface out of range: " + std::to_string(iface));
	
	std::shared_ptr<WinUsbInterfaceHandle>& interfaceHandle = iface == 0 ? data.winUsbInterfaceHandle : data.winUsbAssocInterfaceHandles[iface - 1];
	
	BOOL result = WinUsb_SetCurrentAlternateSetting(interfaceHandle->handle, alternate);
	
	if (result == FALSE)
		return Err("WinUsb_SetCurrentAlternateSetting: " + GetLastErrorAsString());

	return Ok();
}

uint64_t Device::getBusFrameNumber()
{
	return 0;
}

#endif
