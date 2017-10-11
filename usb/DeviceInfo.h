#pragma once

#include <string>
#include <stdint.h>

#include "DeviceId.h"

struct DeviceInfo
{
	// Identifies the USB device on the system.
	DeviceId id;
	
	// Information about the device.
	std::string manufacturer;
	std::string product;
	std::string serial;
	
	uint16_t vendorId;
	uint16_t productId;
	
	bool operator==(const DeviceInfo& other) const {
		// We can probably actually just compare the location.
		return id == other.id &&
		       manufacturer == other.manufacturer &&
		       product == other.product &&
		       serial == other.serial &&
		       vendorId == other.vendorId &&
		       productId == other.productId;
	}
};
