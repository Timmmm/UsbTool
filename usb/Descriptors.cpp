#include "Descriptors.h"

#include "UsbSpecification.h"

std::string to_string(const DeviceDescriptor& val)
{
	std::string s = 
	       "-- Device --\n"
	       "bcdUSB: " + std::to_string(val.bcdUSB) + "\n"
	       "bDeviceClass: " + std::to_string(val.bDeviceClass) + "\n"
	       "bDeviceSubClass: " + std::to_string(val.bDeviceSubClass) + "\n"
	       "bDeviceProtocol: " + std::to_string(val.bDeviceProtocol) + "\n"
	       "bMaxPacketSize: " + std::to_string(val.bMaxPacketSize0) + "\n"
	       "idVendor: " + std::to_string(val.idVendor) + "\n"
	       "idProduct: " + std::to_string(val.idProduct) + "\n"
	       "bcdDevice: " + std::to_string(val.bcdDevice) + "\n"
	       "iManufacturer: " + std::to_string(val.iManufacturer) + "\n"
	       "iProduct: " + std::to_string(val.iProduct) + "\n"
	       "iSerialNumber: " + std::to_string(val.iSerialNumber) + "\n"
	       "bNumConfigurations: " + std::to_string(val.bNumConfigurations) + "\n";
	
	for (const ConfigurationDescriptor& cfg : val.configurations)
		s += to_string(cfg);
	
	return s;
}

std::string to_string(const ConfigurationDescriptor& val)
{
	std::string s = 
	       "-- Configuration --\n"
	       "bNumInterfaces: " + std::to_string(val.bNumInterfaces) + "\n"
	       "bConfigurationValue: " + std::to_string(val.bConfigurationValue) + "\n"
	       "iConfiguration: " + std::to_string(val.iConfiguration) + "\n"
	       "bmAttributes: " + std::to_string(val.bmAttributes) + "\n"
	       "bMaxPower: " + std::to_string(val.bMaxPower) + "\n";
	
	for (const InterfaceDescriptor& iface : val.interfaces)
		s += to_string(iface);
	
	return s;
}

std::string to_string(const InterfaceDescriptor& val)
{
	std::string s = 
	       "    -- Interface --\n"
	       "    bInterfaceNumber:   " + std::to_string(val.bInterfaceNumber) + "\n"
	       "    bAlternateSetting:  " + std::to_string(val.bAlternateSetting) + "\n"
	       "    bNumEndpoints:      " + std::to_string(val.bNumEndpoints) + "\n"
	       "    bInterfaceClass:    " + std::to_string(val.bInterfaceClass) + "\n"
	       "    bInterfaceSubClass: " + std::to_string(val.bInterfaceSubClass) + "\n"
	       "    bInterfaceProtocol: " + std::to_string(val.bInterfaceProtocol) + "\n"
	       "    iInterface:         " + std::to_string(val.iInterface) + "\n";
	
	for (const EndpointDescriptor& ep : val.endpoints)
		s += to_string(ep);
	
	return s;
}

std::string to_string(const EndpointDescriptor& val)
{
	return "        -- Endpoint --\n"
	       "        bEndpointAddress: " + std::to_string(val.bEndpointAddress) + "\n"
	       "        bmAttributes:     " + std::to_string(val.bmAttributes) + "\n"
	       "        wMaxPacketSize:   " + std::to_string(val.wMaxPacketSize) + "\n"
	       "        bInterval:        " + std::to_string(val.bInterval) + "\n";
}

SResult<ConfigurationDescriptor> ParseConfigurationDescriptor(const std::vector<uint8_t>& data)
{
	ConfigurationDescriptor desc;
	
	// They are length-type-data triplets.
	unsigned int offset = 0;
	while (offset + 1 < data.size())
	{
		uint8_t len = data[offset];
		uint8_t type = data[offset + 1];
		
		if (offset + len > data.size())
			break;
		
		switch (type)
		{
		case USB_CONFIGURATION_DESCRIPTOR_TYPE:
		{
			UsbConfigurationDescriptor confDesc;
			if (len != sizeof(confDesc))
				return Err("Unexpected configuration descriptor size: " + std::to_string(len));
			memcpy(&confDesc, data.data() + offset, sizeof(confDesc));
			
			desc.bNumInterfaces = confDesc.bNumInterfaces;
			desc.bConfigurationValue = confDesc.bConfigurationValue;
			desc.iConfiguration = confDesc.iConfiguration;
			desc.bmAttributes = confDesc.bmAttributes;
			desc.bMaxPower = confDesc.MaxPower; // Weirdly missing b!
			
			break;
		}
		case USB_INTERFACE_DESCRIPTOR_TYPE:
		{
			UsbInterfaceDescriptor ifDesc;
			if (len != sizeof(ifDesc))
				return Err("Unexpected interface descriptor size: " + std::to_string(len));
			memcpy(&ifDesc, data.data() + offset, sizeof(ifDesc));
			
			InterfaceDescriptor id;
			id.bInterfaceNumber = ifDesc.bInterfaceNumber;
			id.bAlternateSetting = ifDesc.bAlternateSetting;
			id.bNumEndpoints = ifDesc.bNumEndpoints;
			id.bInterfaceClass = ifDesc.bInterfaceClass;
			id.bInterfaceSubClass = ifDesc.bInterfaceSubClass;
			id.bInterfaceProtocol = ifDesc.bInterfaceProtocol;
			id.iInterface = ifDesc.iInterface;

			desc.interfaces.push_back(id);
			break;
		}
		case USB_ENDPOINT_DESCRIPTOR_TYPE:
		{
			UsbEndpointDescriptor epDesc;
			if (len != sizeof(epDesc))
				return Err("Unexpected endpoint descriptor size: " + std::to_string(len));
			memcpy(&epDesc, data.data() + offset, sizeof(epDesc));
			
			EndpointDescriptor ed;
			ed.bEndpointAddress = epDesc.bEndpointAddress;
			ed.bmAttributes = epDesc.bmAttributes;
			ed.wMaxPacketSize = epDesc.wMaxPacketSize;
			ed.bInterval = epDesc.bInterval;
			
			if (desc.interfaces.empty())
				return Err(std::string("Endpoint descriptor before interface."));

			desc.interfaces.back().endpoints.push_back(ed);
			break;
		}
		default:
			// We don't care about others for now.
			break;
		}
		
		offset += len;
	}
	
	// TODO: Verify bNumEndpoints and bNumInterfaces.
	
	return Ok(desc);
}
