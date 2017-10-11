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

// These are slightly annoying duplicates that are needed for getting the product & vendor name during enumeration.
SResult<std::vector<uint8_t>> ControlTransferInSync(IOUSBDeviceInterface650** dev,
                                                    Device::Recipient recipient,
                                                    Device::Type type,
                                                    uint8_t bRequest,
                                                    uint16_t wValue,
                                                    uint16_t wIndex,
                                                    uint16_t wLength)
{
	std::vector<uint8_t> buffer(wLength);
	
	IOUSBDevRequest request;
	request.bmRequestType = to_integral(recipient) | to_integral(type) | to_integral(Device::Direction::In);
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

SResult<std::vector<uint8_t>> GetDescriptor(IOUSBDeviceInterface650** dev, DescriptorType type, uint8_t index, uint8_t languageId)
{
	// First get the (length, type) header.
	std::vector<uint8_t> header = TRY(ControlTransferInSync(dev,
	                                                        Device::Recipient::Device,
	                                                        Device::Type::Standard,
	                                                        USB_GET_DESCRIPTOR_REQUEST,
	                                                        (to_integral(type) << 8) | index,
	                                                        languageId,
	                                                        2));
	if (header.size() != 2)
		return Err(string("Descriptor header retrieval failed: recieved " + std::to_string(header.size()) + " bytes"));

	uint8_t length = header[0];

	std::vector<uint8_t> descriptor = TRY(ControlTransferInSync(dev,
	                                                            Device::Recipient::Device,
	                                                            Device::Type::Standard,
	                                                            USB_GET_DESCRIPTOR_REQUEST,
	                                                            (to_integral(type) << 8) | index,
	                                                            languageId,
	                                                            length));
	if (descriptor.size() != length)
		return Err(string("Descriptor retrieval failed: recieved "
		                  + std::to_string(descriptor.size()) + " bytes, expected " + std::to_string(length)));
	
	return Ok(descriptor);
}


SResult<std::u16string> GetStringDescriptor(IOUSBDeviceInterface650** dev, uint8_t index, uint16_t languageId)
{
	if (index == 0)
		return Err(string("Invalid string descriptor index (0). Use languageIds() to get the language IDs."));

	std::vector<uint8_t> buffer = TRY(GetDescriptor(dev, DescriptorType::String, index, languageId));

	if (buffer.size() <= 2)
		return Ok(std::u16string());
	
	return Ok(std::u16string(reinterpret_cast<char16_t*>(buffer.data() + 2), (buffer.size()/2) - 1));
}

SResult<std::string> GetStringDescriptorAscii(IOUSBDeviceInterface650** dev, uint8_t index, uint16_t languageId)
{
	std::u16string ws = TRY(GetStringDescriptor(dev, index, languageId));
	std::string s(ws.size(), '?');
	for (unsigned int i = 0; i < ws.size(); ++i)
		if (ws[i] < 128)
			s[i] = static_cast<char>(ws[i]);

	return Ok(s);
}

SResult<std::vector<uint16_t>> GetLanguageIds(IOUSBDeviceInterface650** dev)
{
	// See the USB 2 spec section 9.6.7.
	
	std::vector<uint8_t> buffer = TRY(GetDescriptor(dev, DescriptorType::String, 0, 0));

	if (buffer.size() < 2)
		return Err(string("No language IDs found!"));

	int N = (buffer.size()/2) - 1;

	std::vector<uint16_t> ids(N);
	for (int i = 0; i < N; ++i)
		ids[i] = buffer[2 + i*2] + (buffer[2 + i*2 + 1] << 8);

	return Ok(ids);
}

SResult<UsbDeviceDescriptor> GetDeviceDescriptor(IOUSBDeviceInterface650** dev)
{
	std::vector<uint8_t> desc = TRY(GetDescriptor(dev, DescriptorType::Device, 0, 0));
	
	if (desc.size() != sizeof(UsbDeviceDescriptor))
		return Err("Invalid device descriptor size: " + std::to_string(desc.size()));
	
	UsbDeviceDescriptor d = *reinterpret_cast<const UsbDeviceDescriptor*>(desc.data());
	
	return Ok(d);
}

SResult<std::vector<DeviceInfo>> EnumerateAvailableDevices()
{
	// Create a master port for communication with the I/O Kit
	mach_port_t masterPort;
	kern_return_t kr = IOMasterPort(MACH_PORT_NULL, &masterPort);
	if (kr != kIOReturnSuccess || masterPort == 0)
		return Err("Couldn’t create a master I/O Kit port: " + KernReturnToString(kr));

	// Free the port at the end of this function.
	auto se = make_scope_exit([&] { mach_port_deallocate(mach_task_self(), masterPort); });

	// Set up matching dictionary for class IODevice and its subclasses. This means
	// we only get USB devices from the device tree.
	//
	// This is a reference counted object. When you pass it to IOServiceGetMatchingServices
	// that function will consume one reference and therefore take care of deallocating it.
	CFMutableDictionaryRef matchingDict = IOServiceMatching(kIOUSBDeviceClassName);
	if (matchingDict == nullptr)
		return Err(string("Couldn't create USB matching dictionary"));

	// Enumerate the devices.
	io_iterator_t deviceIterator = 0;
	kr = IOServiceGetMatchingServices(masterPort, matchingDict, &deviceIterator);
	if (kr != 0 || masterPort == 0)
		return Err("Couldn’t enumerate USB devices: " + std::to_string(kr));

	// Iterate over them.
	std::vector<DeviceInfo> devInfos;

	while (io_service_t Device = IOIteratorNext(deviceIterator))
	{
		// Get the device path in the IO registry.
		io_string_t pathName = {0};
		kr = IORegistryEntryGetPath(Device, kIOServicePlane, pathName);
		if (kr != kIOReturnSuccess)
		{
			cerr << "IORegistryEntryGetPath failed: " << KernReturnToString(kr) << endl;
			continue;
		}


		IOCFPlugInInterface** plugInInterface = nullptr;
		SInt32 score = 0;

		// Create an intermediate plug-in
		kr = IOCreatePlugInInterfaceForService(Device,
		                                       kIOUSBDeviceUserClientTypeID,
		                                       kIOCFPlugInInterfaceID,
		                                       &plugInInterface,
		                                       &score);

		// We don’t need the device object after the intermediate plug-in is created
		kern_return_t kr2 = IOObjectRelease(Device);
		if (kr2 != kIOReturnSuccess)
		{
			cerr << "Couldn't release USB device" << KernReturnToString(kr2) << endl;
			continue;
		}

		if (kr != kIOReturnSuccess || plugInInterface == nullptr)
		{
			cerr << "Couldn't create USB plugin: " << KernReturnToString(kr) << endl;
			continue;
		}

		// Now create the device interface
		IOUSBDeviceInterface650** dev = nullptr;

		HRESULT result = (*plugInInterface)->QueryInterface(plugInInterface,
		                                                    CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID650),
		                                                    (LPVOID*)&dev);

		// Don’t need the intermediate plug-in after device interface is created
		(*plugInInterface)->Release(plugInInterface);

		if (result != S_OK || dev == nullptr)
		{
			cerr << "Couldn’t create a device interface: " << result << endl;
			continue;
		}

		auto ae2 = make_scope_exit([&] { (*dev)->Release(dev); });

		DeviceInfo info;

		// Check these values for confirmation
		kr = (*dev)->GetDeviceVendor(dev, &info.vendorId);
		if (kr != kIOReturnSuccess)
		{
			cerr << "GetDeviceVendor failed: " << KernReturnToString(kr) << endl;
			continue;
		}

		kr = (*dev)->GetDeviceProduct(dev, &info.productId);
		if (kr != kIOReturnSuccess)
		{
			cerr << "GetDeviceProduct failed: " << KernReturnToString(kr) << endl;
			continue;
		}

		info.id.path = pathName;
		
		// Get the device descriptor. TODO: Don't return if this fails.
		UsbDeviceDescriptor devDesc = TRY(GetDeviceDescriptor(dev));
		
		// So... the documentation is a bit unclear - it says you need to open the
		// device to change its state. I think that is referring to things like setting
		// alternates and configurations. According to a comment in the libusb code the
		// USB Prober doesn't open devices before doing requests. They do try and open
		// the device but don't really care about the result.
		
		// Get the language IDs.
		
		std::vector<uint16_t> languageIds = GetLanguageIds(dev).unwrap_or_default();
		if (languageIds.empty())
		{
			cerr << "Couldn't get language IDs" << endl;
			continue;
		}
		
		// Get the produce name and vendor name.
		info.product = GetStringDescriptorAscii(dev, devDesc.iProduct, languageIds[0]).unwrap_or_default();
		info.manufacturer = GetStringDescriptorAscii(dev, devDesc.iManufacturer, languageIds[0]).unwrap_or_default();

		devInfos.push_back(info);
	}
	return Ok(devInfos);
}

// Set the configuration to the first bConfigurationValue.
kern_return_t ConfigureDevice(IOUSBDeviceInterface650** dev)
{
	UInt8 numConfig = 0;

	// Get the number of configurations.
	kern_return_t kr = (*dev)->GetNumberOfConfigurations(dev, &numConfig);
	if (kr != kIOReturnSuccess)
		return kr;

	if (numConfig == 0)
		return kIOReturnDeviceError;

	IOUSBConfigurationDescriptorPtr configDesc;

	// Get the configuration descriptor for the first configuration.
	// We always use the first configuration. WinUsb only allows you
	// to use the first configuration, and most devices will only have
	// one configuration. Apparently some USB controllers only support
	// one configuration.
	kr = (*dev)->GetConfigurationDescriptorPtr(dev, 0, &configDesc);
	if (kr != kIOReturnSuccess)
		return kr;

	// Set the configuration.
	kr = (*dev)->SetConfiguration(dev, configDesc->bConfigurationValue);
	if (kr != kIOReturnSuccess)
		return kr;

	return kIOReturnSuccess;
}

SResult<DeviceDescriptor> ReadDescriptors(IOUSBDeviceInterface650** dev)
{
	DeviceDescriptor desc;
	
	UsbDeviceDescriptor devDesc = TRY(GetDeviceDescriptor(dev));
	
	desc.bcdUSB = devDesc.bcdUSB;
	desc.bDeviceClass = devDesc.bDeviceClass;
	desc.bDeviceSubClass = devDesc.bDeviceSubClass;
	desc.bDeviceProtocol = devDesc.bDeviceProtocol;
	desc.bMaxPacketSize0 = devDesc.bMaxPacketSize0; 
	desc.idVendor = devDesc.idVendor;
	desc.idProduct = devDesc.idProduct;
	desc.bcdDevice = devDesc.bcdDevice; 
	desc.iManufacturer = devDesc.iManufacturer; 
	desc.iProduct = devDesc.iProduct;
	desc.iSerialNumber = devDesc.iSerialNumber;
	desc.bNumConfigurations = devDesc.bNumConfigurations;
	
	for (int i = 0; i < desc.bNumConfigurations; ++i)
	{
		IOUSBConfigurationDescriptor* configDesc = nullptr;
		
		// Get the configuration descriptor for index i.
		kern_return_t kr = (*dev)->GetConfigurationDescriptorPtr(dev, i, &configDesc);
		if (kr != kIOReturnSuccess)
			return Err("Couldn't get configuration descriptor for configuration " + std::to_string(i) + ": " + KernReturnToString(kr));
		
		// Convert to vector<uint8_t>
		uint8_t* p = reinterpret_cast<uint8_t*>(configDesc);
		std::vector<uint8_t> buffer(p, p + configDesc->wTotalLength);
		
		desc.configurations.push_back(TRY(ParseConfigurationDescriptor(buffer)));
	}
	return Ok(desc);
}

// Open all the interfaces of a device.
SResult<std::vector<std::shared_ptr<InterfaceWithMetadata>>> OpenInterfaces(IOUSBDeviceInterface650** dev)
{
	std::vector<std::shared_ptr<InterfaceWithMetadata>> interfaces;

	io_iterator_t iterator;

	IOUSBFindInterfaceRequest request;

	// Find all the interfaces, irrespective of their class, etc.
	request.bInterfaceClass = kIOUSBFindInterfaceDontCare;
	request.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
	request.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
	request.bAlternateSetting = kIOUSBFindInterfaceDontCare;

	// Get all the interfaces.
	kern_return_t kr = (*dev)->CreateInterfaceIterator(dev, &request, &iterator);
	if (kr != kIOReturnSuccess)
		return Err("Couldn't create USB interface iterator: " + KernReturnToString(kr));

	// Loop through the interfaces.
	while (io_service_t usbInterface = IOIteratorNext(iterator))
	{
		IOCFPlugInInterface** plugInInterface = nullptr;
		SInt32 score;

		// Create an intermediate plug-in
		kr = IOCreatePlugInInterfaceForService(usbInterface,
		                                       kIOUSBInterfaceUserClientTypeID,
		                                       kIOCFPlugInInterfaceID,
		                                       &plugInInterface,
		                                       &score);
		// Release the usbInterface object after getting the plug-in
		kern_return_t kr2 = IOObjectRelease(usbInterface);

		if (kr2 != kIOReturnSuccess)
			return Err("Couldn't release USB interface: " + KernReturnToString(kr));

		if (kr != kIOReturnSuccess || plugInInterface == nullptr)
			return Err("Error creating USB interface plugin: " + KernReturnToString(kr));

		// This lets us access the interface.
		// 700 is the version that requires OSX 10.9, which seems to be a good USB cut-off point.
		// There is a newer one, but it only adds pipe idle policy functions which we don't need.
		IOUSBInterfaceInterface700** interface = nullptr;

		// Now create the device interface for the interface
		HRESULT result = (*plugInInterface)->QueryInterface(plugInInterface,
		                                                    CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID700),
		                                                    (LPVOID*)&interface);

		// No longer need the intermediate plug-in
		(*plugInInterface)->Release(plugInInterface);

		if (result != 0 || interface == nullptr)
			return Err("Couldn't get USB interface: " + std::to_string(result));

		// Now open the interface. This will cause the pipes associated with
		// the endpoints in the interface descriptor to be instantiated.
		kr = (*interface)->USBInterfaceOpen(interface);
		if (kr != kIOReturnSuccess)
		{
			(*interface)->Release(interface);
			return Err("Couldn't open USB interface: " + KernReturnToString(kr));
		}
		
		// Let's just check the interface number. I think this is guaranteed but just to avoid doubt.
		UInt8 ifaceNum = 0;
		
		// TODO: We have to get the interface number here.
		kr = (*interface)->GetInterfaceNumber(interface, &ifaceNum);
		if (kr != kIOReturnSuccess)
		{
			(*interface)->USBInterfaceClose(interface);
			(*interface)->Release(interface);
			return Err("Error getting interface number: " + KernReturnToString(kr));
		}
		
		if (ifaceNum != interfaces.size())
		{
			(*interface)->USBInterfaceClose(interface);
			(*interface)->Release(interface);
			return Err("Interface " + std::to_string(interfaces.size()) + " has bInterfaceNumber " + std::to_string(ifaceNum));
		}
		
		std::shared_ptr<InterfaceWithMetadata> iface = std::make_shared<InterfaceWithMetadata>(interface);
		
		TRY(iface->refreshPipes());
		
		interfaces.push_back(iface);
	}
	return Ok(interfaces);
}


SResult<std::shared_ptr<Device>> OpenUsbDevice(DeviceId address)
{
	// Ok as far as I can tell the only way to do this is to iterate through all the devices and find
	// the one with a matching address.path ourselves.
	
	CFMutableDictionaryRef matchingDict = IOServiceMatching(kIOUSBDeviceClassName);
	if (matchingDict == nullptr)
		return Err(string("Couldn't create USB matching dictionary"));

	io_iterator_t deviceIterator = 0;
	kern_return_t kr = IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDict, &deviceIterator);
	if (kr != kIOReturnSuccess)
		return Err("Couldn’t enumerate USB devices: " + std::to_string(kr));

	while (io_service_t usbDevice = IOIteratorNext(deviceIterator))
	{
		// Get the device path in the IO registry.
		io_string_t pathName = {0};
		kr = IORegistryEntryGetPath(usbDevice, kIOServicePlane, pathName);
		if (kr != kIOReturnSuccess)
		{
			cerr << "IORegistryEntryGetPath failed: " << KernReturnToString(kr) << endl;
			continue;
		}

		// Check it matches.
		if (std::string(pathName) != address.path)
			continue;

		// Yeay this is the device!!

		IOCFPlugInInterface** plugInInterface = nullptr;
		SInt32 score = 0;

		kr = IOCreatePlugInInterfaceForService(usbDevice,
		                                       kIOUSBDeviceUserClientTypeID,
		                                       kIOCFPlugInInterfaceID,
		                                       &plugInInterface,
		                                       &score);

		if (kr != kIOReturnSuccess || plugInInterface == nullptr)
			return Err("Couldn't create USB plugin: " + KernReturnToString(kr));

		kr = IOObjectRelease(usbDevice);
		if (kr != kIOReturnSuccess)
			return Err("Couldn't release USB plugin " + KernReturnToString(kr));

		IOUSBDeviceInterface650** dev = nullptr;
		HRESULT result = (*plugInInterface)->QueryInterface(plugInInterface,
		                                                    CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID650),
		                                                    (LPVOID*)&dev);
		(*plugInInterface)->Release(plugInInterface);

		if (result != S_OK || dev == nullptr)
		{
			return Err("Couldn’t create a device interface: " + std::to_string(result));
		}

		// Open the device. TODO: Seize vs not-seize?
		kr = (*dev)->USBDeviceOpenSeize(dev);
		if (kr != kIOReturnSuccess || plugInInterface == nullptr)
		{
			(*dev)->Release(dev);
			return Err("Couldn't open device: " + KernReturnToString(kr));
		}

		// Configure the device. This is necessary in almost all cases.
		// See https://developer.apple.com/library/content/documentation/DeviceDrivers/Conceptual/USBBook/DeviceInterfaces/USBDevInterfaces.html#//apple_ref/doc/uid/TP40002645-TPXREF101
		// Just above Listing 2-5.
		kr = ConfigureDevice(dev);
		if (kr != kIOReturnSuccess)
		{
			(*dev)->USBDeviceClose(dev);
			(*dev)->Release(dev);
			return Err("Couldn't configure device: " + KernReturnToString(kr));
		}
		
		std::shared_ptr<Device> newDev = std::make_shared<Device>();
		
		// After this line the device will be closed properly on return.
		newDev->data.device = std::make_shared<DeviceInterface>(dev);
		
		newDev->data.address = address;

		// Open all the interfaces. If this fails now, the device should be closed cleanly.
		newDev->data.interfaces = TRY(OpenInterfaces(dev));
		
		// Read the device descriptors.
		newDev->data.descriptors = TRY(ReadDescriptors(dev));
		
		// Add an async event source for the device. This is used for control transfers
		// and probably also things like device disconnection. Who knows really.
		CFRunLoopSourceRef runLoopSource;
		kr = (*dev)->CreateDeviceAsyncEventSource(dev, &runLoopSource);
		if (kr != kIOReturnSuccess)
			return Err("Couldn't create device async event source: " + KernReturnToString(kr));
		
		CFRunLoopAddSource(newDev->data.runLoop.loop(), runLoopSource, kCFRunLoopCommonModes);
		
		// We have to add each interface's async event source to the run loop.
		for (auto& it : newDev->data.interfaces)
		{
			// Add the interfaces as async sources.
			IOUSBInterfaceInterface700** iface = it->iface();
			
			CFRunLoopSourceRef runLoopSource;
			kr = (*iface)->CreateInterfaceAsyncEventSource(iface, &runLoopSource);
			if (kr != kIOReturnSuccess)
				return Err("Couldn't create interface async event source: " + KernReturnToString(kr));
			
			cerr << "Adding event loop source" << endl;
			CFRunLoopAddSource(newDev->data.runLoop.loop(), runLoopSource, kCFRunLoopCommonModes);
		}
		
		cerr << "Success" << endl;
		return Ok(newDev);
	}

	return Err(string("Device not found"));
}

#endif
