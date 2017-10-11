#pragma once

#include <QMainWindow>
#include <QTimer>
#include <QItemSelection>

#include "UsbThread.h"
#include "DeviceListModel.h"
#include "Metatypes.h"
#include "DeviceInterfacesModel.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
	
public:
	// `thread` must outlive this object.
	MainWindow(UsbThread& thread, QWidget *parent = 0);
	~MainWindow();
	
signals:
	// Ask the USB thread to enumerate available deviecs and return the
	// results in onEnumerationResults.
	void requestEnumerateDevices();
	// Ask the USB thread to get the device descriptors of a device and return
	// the result (as a string for now) in onDeviceDescriptors.
	void requestDeviceDescriptors(DeviceId loc);
	
	void requestControlOutTransfer(DeviceId loc,
	                               Device::Recipient recipient,
	                               Device::Type type,
                                   quint8 bRequest,
                                   quint16 wValue,
                                   quint16 wIndex,
                                   const QByteArray& data);
	void requestControlInTransfer(DeviceId loc,
	                              Device::Recipient recipient,
	                              Device::Type type,
                                  quint8 bRequest,
                                  quint16 wValue,
                                  quint16 wIndex,
                                  int length);
	
private slots:
	// Results from the USB thread.
	void onEnumerateDevicesResult(const QVector<DeviceInfo>& devices);
	void onDeviceDescriptorsResult(DeviceId loc, bool success, DeviceDescriptor deviceDescriptor);
	void onControlOutTransferResult(DeviceId loc, bool success);
	void onControlInTransferResult(DeviceId loc, bool success, const QByteArray& data);
	
	
	
	
	void onDeviceSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
	void onInterfaceSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
	
	void on_actionExit_triggered();
	
	void on_controlSendButton_clicked();
	
	void on_controlReceiveButton_clicked();
	
private:
	Ui::MainWindow *ui;
	
	UsbThread& usbThread;
	
	DeviceListModel devicesModel;
	DeviceInterfacesModel interfacesModel;
	
	QTimer enumerateTimer;
	
	DeviceId selectedLoc;
};
