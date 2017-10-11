#include "UsbThread.h"

#include <QApplication>
#include <QDebug>
#include <QList>

//int LIBUSB_CALL hotplugCallback(libusb_context* ctx,
//                                libusb_device* device,
//                                libusb_hotplug_event event,
//                                void* user_data)
//{
//	qDebug() << "Hotplug event!";
//	if (user_data == nullptr)
//		return 1; // Deregister this callback since it is useless.
	
//	UsbThread* thread = static_cast<UsbThread*>(user_data);
	
//	QMetaObject::invokeMethod(thread, "enumerateDevices", Qt::QueuedConnection);
	
//	// Don't deregister this callback.
//	return 0;
//}

void UsbThread::constructSlot()
{	
//	auto result = libusb_hotplug_register_callback(nullptr,
//	                                               static_cast<libusb_hotplug_event>(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
//	                                               LIBUSB_HOTPLUG_NO_FLAGS,
//	                                               LIBUSB_HOTPLUG_MATCH_ANY,
//	                                               LIBUSB_HOTPLUG_MATCH_ANY,
//	                                               LIBUSB_HOTPLUG_MATCH_ANY,
//	                                               &hotplugCallback,
//	                                               this,
//	                                               &hotplugCallbackHandle);
	
	// Start a thread to handle events.
	
	usbEventThreadRun = true;
	
	usbEventThread = std::thread([&]{
		while (usbEventThreadRun)
		{
		}
		qDebug() << "usbEventThread exiting";
	});
}

void UsbThread::destructSlot()
{
	qDebug() << "Joining event thread";
	usbEventThreadRun = false;
	usbEventThread.join();
	
	// Move this object back to the main thread.
	moveToThread(QApplication::instance()->thread());
	// Exit the background thread.
	workerThread.quit();
}

UsbThread::UsbThread()
{
	// Move this object to it so slots are evaluated by that thread.
	moveToThread(&workerThread);
	
	// Run some code when this object is created or destroyed.
	connect(this, &UsbThread::constructSignal, this, &UsbThread::constructSlot, Qt::BlockingQueuedConnection);
	connect(this, &UsbThread::destructSignal, this, &UsbThread::destructSlot, Qt::BlockingQueuedConnection);
	
	// Start the worker thread.
	workerThread.start();
	
	// Execute constructSlot() in the workerThread context, but block until it is finished.
	emit constructSignal();
	
	// TODO: Re-implement hotplug support.
	connect(&enumerateTimer, &QTimer::timeout, this, &UsbThread::enumerateDevices);
	
	enumerateTimer.start(1000);
}

UsbThread::~UsbThread()
{
	// Execute destructSlot() in the workerThread context, but block until it is finished.
	emit destructSignal();
}

void UsbThread::enumerateDevices()
{
	qDebug() << "Enumerating devices.";
	
	SResult<std::vector<DeviceInfo>> devices = EnumerateAvailableDevices();
	
	if (!devices)
	{
		qDebug() << "Enumeration error: " << QString::fromStdString(devices.unwrap_err()) << endl;
		return;
	}
	
	qDebug() << "Got" << devices.unwrap().size() << "devices";

	// Qt will automatically convert the reference to a copy, so don't worry about us referencing 
	// a temporary object.
	emit enumerateDevicesResult(QVector<DeviceInfo>::fromStdVector(devices.unwrap()));
}

void UsbThread::deviceDescriptors(DeviceId loc)
{
	SResult<std::shared_ptr<Device>> devRes = OpenUsbDevice(loc);
	if (!devRes)
	{
		qDebug() << "Error opening device:" << QString::fromStdString(devRes.unwrap_err());
		emit deviceDescriptorsResult(loc, false, DeviceDescriptor());
		return;
	}
	
	std::shared_ptr<Device> dev = devRes.unwrap();
	
	SResult<DeviceDescriptor> descRes = dev->descriptors();
	if (!descRes)
	{
		qDebug() << "Error getting device descriptor:" << QString::fromStdString(devRes.unwrap_err());
		emit deviceDescriptorsResult(loc, false, DeviceDescriptor());
		return;
	}
	
	DeviceDescriptor desc = descRes.unwrap();
	
	emit deviceDescriptorsResult(loc, true, desc);
}

void UsbThread::controlOutTransfer(DeviceId loc, Device::Recipient recipient, Device::Type type, quint8 bRequest, quint16 wValue, quint16 wIndex, const QByteArray& data)
{
	// TODO: Remember which devices are open so we don't have to re-open them for each request which sucks.
	
	SResult<std::shared_ptr<Device>> devRes = OpenUsbDevice(loc);
	if (!devRes)
	{
		qDebug() << "Error opening device:" << QString::fromStdString(devRes.unwrap_err());
		emit controlOutTransferResult(loc, false);
		return;
	}
	
	std::shared_ptr<Device> dev = devRes.unwrap();
	
	SResult<void> xferRes = dev->controlTransferOutSync(recipient,
	                                                    type,
	                                                    bRequest,
	                                                    wValue,
	                                                    wIndex,
	                                                    std::vector<uint8_t>(data.data(), data.data() + data.size()));
	if (!xferRes)
	{
		qDebug() << "Transfer error:" << QString::fromStdString(xferRes.unwrap_err());
		emit controlOutTransferResult(loc, false);
		return;
	}
	
	emit controlOutTransferResult(loc, true);
}

void UsbThread::controlInTransfer(DeviceId loc, Device::Recipient recipient, Device::Type type, quint8 bRequest, quint16 wValue, quint16 wIndex, int length)
{
	// TODO: Remember which devices are open so we don't have to re-open them for each request which sucks.
	
	SResult<std::shared_ptr<Device>> devRes = OpenUsbDevice(loc);
	if (!devRes)
	{
		qDebug() << "Error opening device:" << QString::fromStdString(devRes.unwrap_err());
		emit controlInTransferResult(loc, false, QByteArray());
		return;
	}
	
	std::shared_ptr<Device> dev = devRes.unwrap();
	
	SResult<std::vector<uint8_t>> xferRes = dev->controlTransferInSync(recipient, type, bRequest, wValue, wIndex, length);
	if (!xferRes)
	{
		qDebug() << "Transfer error:" << QString::fromStdString(xferRes.unwrap_err());
		emit controlInTransferResult(loc, false, QByteArray());
		return;
	}
	
	auto data = xferRes.unwrap();
	
	emit controlInTransferResult(loc, true, QByteArray(reinterpret_cast<const char*>(data.data()), data.size()));
}
