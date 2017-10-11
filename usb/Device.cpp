#include "Device.h"

using std::string;

// This file contains functions that have the same implementation on all platforms. 

Device::Device()
{

}

Device::~Device()
{

}

SResult<uint16_t> Device::vendorId() const
{
	if (!isOpen())
		return Err(string("Device not open"));
	return Ok(data.descriptors.idVendor);
}

SResult<uint16_t> Device::productId() const
{
	if (!isOpen())
		return Err(string("Device not open"));
	return Ok(data.descriptors.idProduct);
}

SResult<DeviceId> Device::address() const
{
	if (!isOpen())
		return Err(string("Device not open"));

	return Ok(data.address);
}

SResult<std::string> Device::manufacturer(uint16_t languageId)
{
	if (!isOpen())
		return Err(string("Device not open"));
	if (data.descriptors.iManufacturer == 0)
		return Ok(string());

	return stringDescriptorAscii(data.descriptors.iManufacturer, languageId);
}

SResult<std::string> Device::product(uint16_t languageId)
{
	if (!isOpen())
		return Err(string("Device not open"));
	if (data.descriptors.iProduct == 0)
		return Ok(string());

	return stringDescriptorAscii(data.descriptors.iProduct, languageId);
}

SResult<std::string> Device::serial(uint16_t languageId)
{
	if (!isOpen())
		return Err(string("Device not open"));
	if (data.descriptors.iSerialNumber == 0)
		return Ok(string());

	return stringDescriptorAscii(data.descriptors.iSerialNumber, languageId);
}

SResult<std::u16string> Device::stringDescriptor(uint8_t index, uint16_t languageId)
{
	if (index == 0)
		return Err(string("Invalid string descriptor index (0). Use languageIds() to get the language IDs."));

	std::vector<uint8_t> buffer = TRY(getDescriptor(DescriptorType::String, index, languageId));

	if (buffer.size() <= 2)
		return Ok(std::u16string());
	
	return Ok(std::u16string(reinterpret_cast<char16_t*>(buffer.data() + 2), (buffer.size()/2) - 1));
}

SResult<std::string> Device::stringDescriptorAscii(uint8_t index, uint16_t languageId)
{
	std::u16string ws = TRY(stringDescriptor(index, languageId));
	std::string s(ws.size(), '?');
	for (unsigned int i = 0; i < ws.size(); ++i)
		if (ws[i] < 128)
			s[i] = static_cast<char>(ws[i]);

	return Ok(s);
}

SResult<std::vector<uint16_t>> Device::languageIds()
{
	// See the USB 2 spec section 9.6.7.
	
	std::vector<uint8_t> buffer = TRY(getDescriptor(DescriptorType::String, 0, 0));

	if (buffer.size() < 2)
		return Err(string("No language IDs found!"));

	int N = (buffer.size()/2) - 1;

	std::vector<uint16_t> ids(N);
	for (int i = 0; i < N; ++i)
		ids[i] = buffer[2 + i*2] + (buffer[2 + i*2 + 1] << 8);

	return Ok(ids);
}

SResult<std::vector<uint8_t>> Device::getDescriptor(DescriptorType type, uint8_t index, uint8_t languageId)
{
	// First get the (length, type) header.
	std::vector<uint8_t> header = TRY(controlTransferInSync(Recipient::Device,
	                                                        Type::Standard,
	                                                        USB_GET_DESCRIPTOR_REQUEST,
	                                                        (to_integral(type) << 8) | index,
	                                                        languageId,
	                                                        2));
	if (header.size() != 2)
		return Err(string("Descriptor header retrieval failed: recieved " + std::to_string(header.size()) + " bytes"));

	uint8_t length = header[0];

	std::vector<uint8_t> descriptor = TRY(controlTransferInSync(Recipient::Device,
	                                                            Type::Standard,
	                                                            USB_GET_DESCRIPTOR_REQUEST,
	                                                            (to_integral(type) << 8) | index,
	                                                            languageId,
	                                                            length));
	if (descriptor.size() != length)
		return Err(string("Descriptor retrieval failed: recieved "
		                  + std::to_string(descriptor.size()) + " bytes, expected " + std::to_string(length)));
	
	return Ok(descriptor);
}

SResult<DeviceDescriptor> Device::descriptors()
{
	if (!isOpen())
		return Err(string("Device not open"));

	return Ok(data.descriptors);
}

