#pragma once

#include <thread>
#include <atomic>
#include <QThread>
#include <QVector>
#include <stdint.h>
#include <QTimer>

#include "usb/Discovery.h"
#include "usb/Device.h"

#include "Metatypes.h"

class UsbThread : public QObject
{
	Q_OBJECT
public:
	// This can't have any parents.
	UsbThread();
	~UsbThread();

signals:
	void constructSignal();
	void destructSignal();
	
	void enumerateDevicesResult(const QVector<DeviceInfo>& devices);
	void deviceDescriptorsResult(DeviceId loc, bool success, const DeviceDescriptor& deviceDescriptor);
	void controlOutTransferResult(DeviceId loc, bool success);
	void controlInTransferResult(DeviceId loc, bool success, const QByteArray& data);

public slots:
	void enumerateDevices();
	void deviceDescriptors(DeviceId loc);

	void controlOutTransfer(DeviceId loc,
	                        Device::Recipient recipient,
	                        Device::Type type,
                            quint8 bRequest,
                            quint16 wValue,
                            quint16 wIndex,
                            const QByteArray& data);
	void controlInTransfer(DeviceId loc,
	                       Device::Recipient recipient,
	                       Device::Type type,
                           quint8 bRequest,
                           quint16 wValue,
                           quint16 wIndex,
                           int length);
	
private slots:
	void constructSlot();
	void destructSlot();

private:
	QThread workerThread;
	
	QVector<Device> openDevices;
	
	std::thread usbEventThread;
	std::atomic_bool usbEventThreadRun{false};
	
	QTimer enumerateTimer;
};
