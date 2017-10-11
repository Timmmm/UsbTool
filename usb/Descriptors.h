#pragma once

#include <string>
#include <vector>

#include "util/Result.h"

#include "UsbSpecification.h"

struct EndpointDescriptor
{
	uint8_t bEndpointAddress;
	uint8_t bmAttributes;
	uint16_t wMaxPacketSize;
	uint8_t bInterval;
};

struct InterfaceDescriptor
{
	uint8_t bInterfaceNumber;
	uint8_t bAlternateSetting;
	uint8_t bNumEndpoints; // Equal to endpoints.size().
	uint8_t bInterfaceClass;
	uint8_t bInterfaceSubClass;
	uint8_t bInterfaceProtocol;
	
	uint8_t iInterface;
//	std::u16string sInterface;
	
	std::vector<EndpointDescriptor> endpoints;
};

struct ConfigurationDescriptor
{
	uint8_t bNumInterfaces; // Not necessarily equal to interface.size() because of alternate interfaces.
	
	uint8_t bConfigurationValue;
	
	uint8_t iConfiguration;
	
//	std::u16string sConfiguration;
	
	uint8_t bmAttributes;
	uint8_t bMaxPower;
	
	// List of interfaces and their alternates.
	std::vector<InterfaceDescriptor> interfaces;
};

struct DeviceDescriptor
{
	uint16_t bcdUSB;
	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bMaxPacketSize0; // For control transfers.
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice; // Device release
	
	uint8_t iManufacturer; // Indices of the strings.
	uint8_t iProduct;
	uint8_t iSerialNumber;
	
//	std::u16string sManufacturer;
//	std::u16string sProduct;
//	std::u16string sSerialNumber;
	
	uint8_t bNumConfigurations; // Equal to configurations.size().
	
	// Note, WinUsb only supports the first configuration.
	// Multi-configuration devices are not common.
	std::vector<ConfigurationDescriptor> configurations;
};


std::string to_string(const DeviceDescriptor& val);
std::string to_string(const ConfigurationDescriptor& val);
std::string to_string(const InterfaceDescriptor& val);
std::string to_string(const EndpointDescriptor& val);

// Parse a standalone device descriptor.
SResult<DeviceDescriptor> ParseDeviceDescriptor(const std::vector<uint8_t>& data);

// Parse a configuration descriptor. The configuration descriptor is followed by
// interface, endpoint and class and vendor-defined descriptors.
SResult<ConfigurationDescriptor> ParseConfigurationDescriptor(const std::vector<uint8_t>& data);

enum class DescriptorType : uint8_t
{
	Device = USB_DEVICE_DESCRIPTOR_TYPE,
	Configuration = USB_CONFIGURATION_DESCRIPTOR_TYPE,
	String = USB_STRING_DESCRIPTOR_TYPE,
	Interface = USB_INTERFACE_DESCRIPTOR_TYPE,
	Endpoint = USB_ENDPOINT_DESCRIPTOR_TYPE,
	DeviceQualifier = USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE,
	OtherSpeedConfiguration = USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_TYPE,
	InterfacePower = USB_INTERFACE_POWER_DESCRIPTOR_TYPE,
};
