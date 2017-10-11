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

// These are slightly annoying duplicates that are needed for getting the product & vendor name during enumeration.
SResult<std::vector<uint8_t>> ControlTransferInSync(WINUSB_INTERFACE_HANDLE handle,
                                                    Device::Recipient recipient,
                                                    Device::Type type,
                                                    uint8_t bRequest,
                                                    uint16_t wValue,
                                                    uint16_t wIndex,
                                                    uint16_t wLength)
{
	std::vector<uint8_t> buffer(wLength);
	
	WINUSB_SETUP_PACKET setup;
	setup.RequestType = to_integral(recipient) | to_integral(type) | to_integral(Device::Direction::In);
	setup.Request = bRequest;
	setup.Value = wValue;
	setup.Index = wIndex;
	setup.Length = 0;
	
	ULONG transferred = 0;
	BOOL result = WinUsb_ControlTransfer(handle, setup, buffer.data(), buffer.size(), &transferred, nullptr);
	if (result == FALSE)
		return Err("WinUsb_ControlTransfer: " + GetLastErrorAsString());
	
	if (transferred != buffer.size())
		return Err("WinUsb_ControlTransfer: Transferred " + std::to_string(transferred) + " Expected: " + std::to_string(buffer.size()));
	
	return Ok(buffer);
}

SResult<std::vector<uint8_t>> GetDescriptor(WINUSB_INTERFACE_HANDLE handle, DescriptorType type, uint8_t index, uint8_t languageId)
{
	// First get the (length, type) header.
	std::vector<uint8_t> header = TRY(ControlTransferInSync(handle,
	                                                        Device::Recipient::Device,
	                                                        Device::Type::Standard,
	                                                        USB_GET_DESCRIPTOR_REQUEST,
	                                                        (to_integral(type) << 8) | index,
	                                                        languageId,
	                                                        2));
	if (header.size() != 2)
		return Err(string("Descriptor header retrieval failed: recieved " + std::to_string(header.size()) + " bytes"));

	uint8_t length = header[0];

	std::vector<uint8_t> descriptor = TRY(ControlTransferInSync(handle,
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


SResult<std::u16string> GetStringDescriptor(WINUSB_INTERFACE_HANDLE handle, uint8_t index, uint16_t languageId)
{
	if (index == 0)
		return Err(string("Invalid string descriptor index (0). Use languageIds() to get the language IDs."));

	std::vector<uint8_t> buffer = TRY(GetDescriptor(handle, DescriptorType::String, index, languageId));

	if (buffer.size() <= 2)
		return Ok(std::u16string());
	
	return Ok(std::u16string(reinterpret_cast<char16_t*>(buffer.data() + 2), (buffer.size()/2) - 1));
}

SResult<std::string> GetStringDescriptorAscii(WINUSB_INTERFACE_HANDLE handle, uint8_t index, uint16_t languageId)
{
	std::u16string ws = TRY(GetStringDescriptor(handle, index, languageId));
	std::string s(ws.size(), '?');
	for (unsigned int i = 0; i < ws.size(); ++i)
		if (ws[i] < 128)
			s[i] = static_cast<char>(ws[i]);

	return Ok(s);
}

SResult<std::vector<uint16_t>> GetLanguageIds(WINUSB_INTERFACE_HANDLE handle)
{
	// See the USB 2 spec section 9.6.7.
	
	std::vector<uint8_t> buffer = TRY(GetDescriptor(handle, DescriptorType::String, 0, 0));

	if (buffer.size() < 2)
		return Err(string("No language IDs found!"));

	int N = (buffer.size()/2) - 1;

	std::vector<uint16_t> ids(N);
	for (int i = 0; i < N; ++i)
		ids[i] = buffer[2 + i*2] + (buffer[2 + i*2 + 1] << 8);

	return Ok(ids);
}

SResult<UsbDeviceDescriptor> GetDeviceDescriptor(WINUSB_INTERFACE_HANDLE handle)
{
	std::vector<uint8_t> desc = TRY(GetDescriptor(handle, DescriptorType::Device, 0, 0));
	
	if (desc.size() != sizeof(UsbDeviceDescriptor))
		return Err("Invalid device descriptor size: " + std::to_string(desc.size()));
	
	UsbDeviceDescriptor d = *reinterpret_cast<const UsbDeviceDescriptor*>(desc.data());
	
	return Ok(d);
}

SResult<std::vector<DeviceInfo>> EnumerateAvailableDevices()
{
	std::vector<DeviceInfo> devInfos;

	// Enumerate all USB devices. If we have WCID set up properly you can use your own GUID here.
	HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	if (deviceInfoSet == INVALID_HANDLE_VALUE)
		return Err("SetupDiGetClassDevs: " + GetLastErrorAsString());
	
	// Run this code when exiting the scope.
	auto se = make_scope_exit([&] { SetupDiDestroyDeviceInfoList(deviceInfoSet); });
	
	// Loop through the results.
	for (int idx = 0;; ++idx)
	{
		SP_DEVICE_INTERFACE_DATA interfaceData;
		interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

		BOOL bResult = SetupDiEnumDeviceInterfaces(deviceInfoSet, nullptr, &GUID_DEVINTERFACE_USB_DEVICE, idx, &interfaceData);
		if (bResult == FALSE)
		{
			DWORD lastError = GetLastError();
			if (lastError == ERROR_NO_MORE_ITEMS)
				break;
			return Err("SetupDiEnumDeviceInterfaces: " + LastErrorAsString(lastError));
		}
		
		// Get the size of the path string
		// We expect to get a failure with insufficient buffer
		ULONG requiredLength = 0;
		bResult = SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &interfaceData, nullptr, 0, &requiredLength, nullptr);

		if (bResult == FALSE && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			return Err("SetupDiGetDeviceInterfaceDetail: " + GetLastErrorAsString());

		// Allocate temporary space for SetupDi structure
		PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LMEM_FIXED, requiredLength);

		if (detailData == nullptr)
			return Err(string("LocalAlloc out of memory."));

		detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		ULONG length = requiredLength;

		// Get the interface's path string
		bResult = SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &interfaceData, detailData, length, &requiredLength, nullptr);

		if (bResult == FALSE)
		{
			LocalFree(detailData);
			return Err("SetupDiGetDeviceInterfaceDetail: " + GetLastErrorAsString());
			break;
		}
		
		// TODO: Get the hardware ID - it contains the VID and PID in an officially documented format that we can parse.
//		SP_DEVINFO_DATA devInfo = ...;
//		SetupDiGetDeviceRegistryProperty(deviceInfoSet, &devInfo, SPDRP_HARDWAREID, nullptr, ...);
		
		// Regex actually seems like a good option here.
		// Extraction of several sub-matches
		std::regex usbRegex(R"(\\\\\?\\usb#vid_([0-9a-fA-F]{4})&pid_([0-9a-fA-F]{4}).*)");
		std::smatch matches;
		
		// SetupDiGetDeviceInterfaceDetail ensured DevicePath is NULL-terminated.
		std::wstring wpath = detailData->DevicePath;
		std::string path = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(wpath);
		
		if (!std::regex_match(path, matches, usbRegex))
			return Err("Failed to match path: " + path);

		if (matches.size() != 3)
			return Err("Failed to submatch path: " + path);
		
		uint16_t vid = std::stol(matches[1].str(), nullptr, 16);
		uint16_t pid = std::stol(matches[2].str(), nullptr, 16);

		DeviceInfo info;
		info.id.path = wpath;
		info.vendorId = vid;
		info.productId = pid;
		
		// TODO: Fill in product and manufacturer.
		
		devInfos.push_back(info);

		LocalFree(detailData);
	}
	return Ok(devInfos);
}

SResult<DeviceDescriptor> ReadDescriptors(WINUSB_INTERFACE_HANDLE interfaceHandle)
{
	DeviceDescriptor desc;
	
	ULONG transferred = 0;
	UsbDeviceDescriptor deviceDescriptor;
	
	// index is only used for configuration descriptors.
	// lanuage is only used for string descriptors.
	BOOL bResult = WinUsb_GetDescriptor(interfaceHandle,
	                                    USB_DEVICE_DESCRIPTOR_TYPE,
	                                    0,
	                                    0,
	                                    reinterpret_cast<PUCHAR>(&deviceDescriptor),
	                                    sizeof(deviceDescriptor),
	                                    &transferred);
	
	if (bResult == FALSE || transferred != sizeof(UsbDeviceDescriptor))
		return Err("WinUsb_GetDescriptor: " + GetLastErrorAsString());
	
	desc.bcdUSB = deviceDescriptor.bcdUSB;
	desc.bDeviceClass = deviceDescriptor.bDeviceClass;
	desc.bDeviceSubClass = deviceDescriptor.bDeviceSubClass;
	desc.bDeviceProtocol = deviceDescriptor.bDeviceProtocol;
	desc.bMaxPacketSize0 = deviceDescriptor.bMaxPacketSize0;
	desc.idVendor = deviceDescriptor.idVendor;
	desc.idProduct = deviceDescriptor.idProduct;
	desc.bcdDevice = deviceDescriptor.bcdDevice;
	desc.iManufacturer = deviceDescriptor.iManufacturer;
	desc.iProduct = deviceDescriptor.iProduct;
	desc.iSerialNumber = deviceDescriptor.iSerialNumber;
	desc.bNumConfigurations = deviceDescriptor.bNumConfigurations;
	
	// For each configuration.
	for (int index = 0; index < deviceDescriptor.bNumConfigurations; ++index)
	{
		// Read the configuration descriptor to get its total length.
		UsbConfigurationDescriptor configDescriptor;
		
		bResult = WinUsb_GetDescriptor(interfaceHandle,
		                               USB_CONFIGURATION_DESCRIPTOR_TYPE,
		                               index,
		                               0,
		                               reinterpret_cast<PUCHAR>(&configDescriptor),
		                               sizeof(configDescriptor),
		                               &transferred);
		
		if (bResult == FALSE || transferred != sizeof(UsbConfigurationDescriptor))
			return Err("WinUsb_GetDescriptor;2: " + GetLastErrorAsString());
		
		// Now get the whole thing.
		std::vector<uint8_t> buffer(configDescriptor.wTotalLength);
		
		bResult = WinUsb_GetDescriptor(interfaceHandle,
		                               USB_CONFIGURATION_DESCRIPTOR_TYPE,
		                               index,
		                               0,
		                               buffer.data(),
		                               buffer.size(),
		                               &transferred);
		
		if (bResult == FALSE || transferred != buffer.size())
			return Err("WinUsb_GetDescriptor;3: " + GetLastErrorAsString());
		
		desc.configurations.push_back(TRY(ParseConfigurationDescriptor(buffer)));
	}
	
	return Ok(desc);
}

SResult<std::shared_ptr<Device>> OpenUsbDevice(DeviceId id)
{
	std::shared_ptr<Device> newDev = std::make_shared<Device>();
	
	HANDLE deviceHandle = CreateFileW(id.path.c_str(),
		GENERIC_WRITE | GENERIC_READ,
		FILE_SHARE_WRITE | FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
		nullptr);

	if (deviceHandle == INVALID_HANDLE_VALUE)
		return Err("CreateFileW: Invalid handle: " + GetLastErrorAsString());
	
	newDev->data.deviceHandle.reset(new WindowsHandle(deviceHandle));
	
	WINUSB_INTERFACE_HANDLE interfaceHandle;

	// Get the specified interface of the USB device.
	BOOL bResult = WinUsb_Initialize(newDev->data.deviceHandle->handle, &interfaceHandle);

	if (bResult == FALSE)
		return Err("WinUsb_Initialize: " + GetLastErrorAsString());
	
	newDev->data.winUsbInterfaceHandle.reset(new WinUsbInterfaceHandle(interfaceHandle));
	
	// Get all the other interface handles.
	for (unsigned int idx = 0; idx <= 255; ++idx)
	{
		WINUSB_INTERFACE_HANDLE iface = nullptr;
		BOOL bResult = WinUsb_GetAssociatedInterface(newDev->data.winUsbInterfaceHandle->handle, idx, &iface);
		
		// If it failed...
		if (bResult == FALSE)
		{
			// If it failed because there aren't any more interfaces, just break.
			DWORD lastError = GetLastError();
			if (lastError == ERROR_NO_MORE_ITEMS)
				break;
			
			return Err("WinUsb_GetAssociatedInterface: " + LastErrorAsString(lastError));
		}
		
		newDev->data.winUsbAssocInterfaceHandles.emplace_back(new WinUsbInterfaceHandle(iface));
	}
	
	newDev->data.descriptors = TRY(ReadDescriptors(newDev->data.winUsbInterfaceHandle->handle));

	return Ok(newDev);
}


#endif
