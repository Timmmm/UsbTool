#pragma once

#include <vector>

#include "util/Result.h"
#include "DeviceInfo.h"
#include "DeviceId.h"
#include "Device.h"

// Find all devices that are openable by libusb. This will exclude already-open devices,
// ones without drivers, etc.
SResult<std::vector<DeviceInfo>> EnumerateAvailableDevices();

// Open the specific device regardless of vendorId and protocol (but it must still have
// a DFU interface).
SResult<std::shared_ptr<Device>> OpenUsbDevice(DeviceId id);
