#pragma once

#include <cstdint>

#pragma pack(push, 1)

// USB 1.1: 9.4 Standard Device Requests, Table 9-5. Descriptor Types
// USB 2.0:	9.4 Standard Device Requests, Table 9-5. Descriptor Types
#define USB_DEVICE_DESCRIPTOR_TYPE                          0x01
#define USB_CONFIGURATION_DESCRIPTOR_TYPE                   0x02
#define USB_STRING_DESCRIPTOR_TYPE                          0x03
#define USB_INTERFACE_DESCRIPTOR_TYPE                       0x04
#define USB_ENDPOINT_DESCRIPTOR_TYPE                        0x05
#define USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE                0x06
#define USB_OTHER_SPEED_CONFIGURATION_DESCRIPTOR_TYPE       0x07
#define USB_INTERFACE_POWER_DESCRIPTOR_TYPE                 0x08

// USB 2.0: 9.6.1 Device, Table 9-8. Standard Device Descriptor
struct UsbDeviceDescriptor {
	uint8_t   bLength;
	uint8_t   bDescriptorType;
	uint16_t  bcdUSB;
	uint8_t   bDeviceClass;
	uint8_t   bDeviceSubClass;
	uint8_t   bDeviceProtocol;
	uint8_t   bMaxPacketSize0;
	uint16_t  idVendor;
	uint16_t  idProduct;
	uint16_t  bcdDevice;
	uint8_t   iManufacturer;
	uint8_t   iProduct;
	uint8_t   iSerialNumber;
	uint8_t   bNumConfigurations;
};

// USB 1.1: 9.6.2 Configuration, Table 9-8. Standard Configuration Descriptor
// USB 2.0: 9.6.3 Configuration, Table 9-10. Standard Configuration Descriptor
// USB 3.0: 9.6.3 Configuration, Table 9-15. Standard Configuration Descriptor
struct UsbConfigurationDescriptor {
	uint8_t   bLength;
	uint8_t   bDescriptorType;
	uint16_t  wTotalLength;
	uint8_t   bNumInterfaces;
	uint8_t   bConfigurationValue;
	uint8_t   iConfiguration;
	uint8_t   bmAttributes;
	uint8_t   MaxPower;
};

// USB 1.1: 9.6.3 Interface, Table 9-9. Standard Interface Descriptor
// USB 2.0: 9.6.5 Interface, Table 9-12. Standard Interface Descriptor
// USB 3.0: 9.6.5 Interface, Table 9-17. Standard Interface Descriptor
struct UsbInterfaceDescriptor {
	uint8_t   bLength;
	uint8_t   bDescriptorType;
	uint8_t   bInterfaceNumber;
	uint8_t   bAlternateSetting;
	uint8_t   bNumEndpoints;
	uint8_t   bInterfaceClass;
	uint8_t   bInterfaceSubClass;
	uint8_t   bInterfaceProtocol;
	uint8_t   iInterface;
};

// USB 1.1: 9.6.4 Endpoint, Table 9-10. Standard Endpoint Descriptor
// USB 2.0: 9.6.6 Endpoint, Table 9-13. Standard Endpoint Descriptor
// USB 3.0: 9.6.6 Endpoint, Table 9-18. Standard Endpoint Descriptor
struct UsbEndpointDescriptor {
	uint8_t   bLength;
	uint8_t   bDescriptorType;
	uint8_t   bEndpointAddress;
	uint8_t   bmAttributes;
	uint16_t  wMaxPacketSize;
	uint8_t   bInterval;
};

// USB 2.0:	9.4 Standard Device Requests, Table 9-4. Standard Request Codes
#define USB_GET_STATUS_REQUEST                 0x00
#define USB_CLEAR_FEATURE_REQUEST              0x01
#define USB_SET_FEATURE_REQUEST                0x03
#define USB_SET_ADDRESS_REQUEST                0x05
#define USB_GET_DESCRIPTOR_REQUEST             0x06
#define USB_SET_DESCRIPTOR_REQUEST             0x07
#define USB_GET_CONFIGURATION_REQUEST          0x08
#define USB_SET_CONFIGURATION_REQUEST          0x09
#define USB_GET_INTERFACE_REQUEST              0x0A
#define USB_SET_INTERFACE_REQUEST              0x0B
#define USB_SYNCH_FRAME_REQUEST                0x0C

#pragma pack(pop)
