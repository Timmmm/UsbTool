#pragma once

#include "util/Result.h"
#include "DeviceId.h"

#include "EndpointInfo.h"
#include "Descriptors.h"

#include <string>
#include <vector>

#include <stdint.h>


#if defined(_WIN32)
#include "windows/Device_Win.h"
#endif

#if defined(__APPLE__)
#include "mac/Device_Mac.h"
#endif

// Notes on USB terminology.
//
// USB splits time into 1 ms frames. In Full/High speed (USB 2) these are
// split into eight 125 us microframes.
//
// Interrupt and isochronous pipes may not be contacted every frame/microframe.
// The ones in which they are are called 'service intervals'.
//
// USB data is sent in 'packets'. The packets have a maximum size which can
// be obtained from the endpoint descriptor. There are data packets, token packets,
// handshake packets, etc.
//
// The next level up from a packet is a 'transaction'. This consists of a token,
// an optional data packet, and a handshake packet. It basically sends one data
// packet wrapped with some admin packets.
//
// Next is a 'transfer', this is an aritrary sized chunk of data that is sent over
// the pipe by splitting it into packets/transactions. NOTE THAT FOR ISOCHRONOUS
// TRANSFERS THE TRANSFER SIZE MUST START AND END ON A FRAME BOUNDARY. No idea why.
//
// (By the way, an 'endpoint' is the destination; a 'pipe' is the actual connection
// to that endpoint. The distinction is mostly unimportant.)
//
// Figure 5-14 in the USB 2 spec gives a good overview of all this.

// Normally there's only one transaction per microframe. But
// if a high-speed isochronous endpoint requires more than 1024 bytes per microframe
// it is a 'high bandwidth endpoint', and can use multiple transactions per microframe.
// It must have a bInterval of 1.

#if defined(_WIN32)
class WinUsbIsochBufferHandle;
class WinUsbInterfaceHandle;
class Overlapped;
#endif

class Device
{
private:
	friend SResult<std::shared_ptr<Device>> OpenUsbDevice(DeviceId address);

public:
	// Creates a non-open device. Get a non-empty device using OpenUsbDevice(...);
	Device();
	// This automatically closes the device if is isn't already and no transfers are
	// outstanding. If there are then it is closed when they are finished.
	~Device();
	
	// Return true if this is associated with a device (instead of default-constructed or closed).
	bool isOpen() const;
	
	// Close the device if it is open. It's harmless to call this on a closed device.
	void close();
	
	// Convenience functions for descriptors()->...;
	SResult<uint16_t> vendorId() const;
	SResult<uint16_t> productId() const;
	
	// Get the device address (USB bus and port path). This only fails if the device isn't open.
	SResult<DeviceId> address() const;
	
	// Get the product name from the USB string descriptors.
	SResult<std::string> manufacturer(uint16_t languageId);
	// Get the product name from the USB string descriptors.
	SResult<std::string> product(uint16_t languageId);
	// Get the serial number from the USB string descriptors.
	SResult<std::string> serial(uint16_t languageId);
	
	// Get string descriptors in the given language.
	SResult<std::u16string> stringDescriptor(uint8_t index, uint16_t languageId);
	// Get the string descriptor (they are UTF-16) with non-ASCII characters replaced with '?'.
	SResult<std::string> stringDescriptorAscii(uint8_t index, uint16_t languageId);
	
	// Get the language IDs supported by the device. It is required to support at least one and
	// will return an error if there are no supported language IDs.
	SResult<std::vector<uint16_t>> languageIds();
	
	enum class Speed
	{
		Low,         // USB 1.1; 1.5 Mb/s
		Full,        // USB 1.1; 12 Mb/s
		High,        // USB 2.0; 480 Mb/s
		Super,       // USB 3.1 Gen 1; 5 Gb/s (AKA USB 3.0)
		SuperPlus,   // USB 3.1 Gen 2; 10 Gb/s
	};
	
	// Get device speed.
	SResult<Speed> speed() const;
	
	// bmRequestType fields.
	enum class Recipient : uint8_t
	{
		Device = 0x00,
		Interface = 0x01,
		Endpoint  = 0x02,
		Other = 0x03,
	};
	
	enum class Type : uint8_t
	{
		Standard = 0x00,
		Class = 0x20,
		Vendor = 0x40,
	};
	
	enum class Direction : uint8_t
	{
		Out = 0x00,
		In = 0x80,
	};
	
	// Synchronous control transfers.
	SResult<std::vector<uint8_t>> controlTransferInSync(Recipient recipient,
	                                                    Type type,
	                                                    uint8_t bRequest,
	                                                    uint16_t wValue,
	                                                    uint16_t wIndex,
	                                                    uint16_t wLength);
	SResult<void> controlTransferOutSync(Recipient recipient,
	                                     Type type,
	                                     uint8_t bRequest,
	                                     uint16_t wValue,
	                                     uint16_t wIndex,
	                                     std::vector<uint8_t> dat = std::vector<uint8_t>()); // This could theoretically be *slightly* more efficient with a reference, but I doubt it will ever matter.

	// Asynchronous control transfers.
	SResult<UsbTransferHandle> controlTransferIn(Recipient recipient,
	                                             Type type,
	                                             uint8_t bRequest,
	                                             uint16_t wValue,
	                                             uint16_t wIndex,
	                                             uint16_t wLength);
	SResult<UsbTransferHandle> controlTransferOut(Recipient recipient,
	                                              Type type,
	                                              uint8_t bRequest,
	                                              uint16_t wValue,
	                                              uint16_t wIndex,
	                                              std::vector<uint8_t> dat = std::vector<uint8_t>());
	
	// Convenience function to synchronously get a descriptor. languageId should be 0 for non-string descriptors.
	SResult<std::vector<uint8_t>> getDescriptor(DescriptorType type, uint8_t index, uint8_t languageId = 0);
	
	// These functions create a buffer for a single transfer. In fact both operating systems allow using one
	// buffer for more than one transfer, but they do it differently so it is simpler to restrict it to one buffer
	// per transfer. Buffers can be reused.
	//
	// On Windows these functions are identical, but they are different on OSX.
	//
	// The buffers store a reference counted pointer to the interface they are for, and the device in general
	// so it won't be closed until they are.
	//
	// The reason you have to specially register a buffer is so that the kernel and userspace can both access it at once.
	// This in theory lets you read/write to it as a transfer is proceeding, so that you are *just* before the kernel
	// and can achieve very low latency. However for now I will just use 1-frame transfers.
	
	// Create an isochronous read buffer for IN transfers.
	// 
	//   `pipe` is the pipeRef on OSX and the endpoint number on Windows. TODO: Is it??
	//   `iface` is the interface index (not the bInterfaceValue or whatever). TODO: ...
	SResult<IsochReadBuffer> createIsochReadBuffer(int iface, uint8_t endpointAddress, int numFrames, int bytesPerFrame);
	// Create an isochronous write buffer for OUT transfers. `pipe` is the pipeRef on OSX and the endpoint number on Windows.
	SResult<IsochWriteBuffer> createIsochWriteBuffer(int iface, uint8_t endpointAddress, int numFrames, int bytesPerFrame);

	// Submit a transfer ASAP. If continueStream is true, this fails if it can't schedule a transfer for the next frame.
	SResult<UsbIsochTransferHandle> submitIsoOutTransferAsap(const IsochWriteBuffer& buffer, bool continueStream);
	
	// Submit a transfer to start at a specific frame.
	SResult<UsbIsochTransferHandle> submitIsoOutTransfer(const IsochWriteBuffer& buffer, uint64_t frame);
	
	// Get the current bus frame number. This loops.
	uint64_t getBusFrameNumber();
	
	// Get all of the USB descriptors. This is cached when the device is opened.
	SResult<DeviceDescriptor> descriptors();
	
	// WinUsb only supports the first configuration so this doesn't work. Fortunately few devices are multi-configuration.
	//   void setConfiguration(int conf);
	
	// Get the number of interface, including the first one. Minimum 1, unless
	// the device isn't open in which case it returns 0.
	int numInterfaces();
	
	// Set an interface to an alternate setting. I'm not sure how safe this is with pending isoch transfers.
	SResult<void> setAlternate(int iface, uint8_t alternate);
	
private:
	// This class cannot be copied. Put it in a shared_ptr if you want to.
	Device(const Device&) = delete;
	Device& operator=(const Device&) = delete;
	
	UsbDeviceData data;
};

