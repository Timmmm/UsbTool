#pragma once

#include "usb/DeviceId.h"
#include "usb/DeviceInfo.h"
#include "usb/Device.h"

#include <QObject>

Q_DECLARE_METATYPE(DeviceDescriptor)
Q_DECLARE_METATYPE(DeviceInfo)
Q_DECLARE_METATYPE(DeviceId)
Q_DECLARE_METATYPE(Device::Recipient)
Q_DECLARE_METATYPE(Device::Type)
