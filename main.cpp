#include <QApplication>

#include "MainWindow.h"
#include "UsbThread.h"
#include "Metatypes.h"

#include "DeviceInterfacesModel.h"

int main(int argc, char *argv[])
{
	qRegisterMetaType<DeviceInfo>();
	qRegisterMetaType<DeviceDescriptor>();
	qRegisterMetaType<DeviceId>();
	qRegisterMetaType<Device::Recipient>();
	qRegisterMetaType<Device::Type>();
	qRegisterMetaType<QVector<DeviceInfo>>();
	qRegisterMetaType<DeviceInterfacesModel::TreeNodeData>();
	qRegisterMetaType<DeviceInterfacesModel::NodeType>();

	QCoreApplication::setOrganizationName("UsbTool");
	QCoreApplication::setOrganizationDomain("example.com");
	QCoreApplication::setApplicationName("UsbTool");

	QApplication a(argc, argv);
	
	// UsbThread uses its own Qt event loop so it requires QApplication
	// to have been initialised already. When this is destroyed it will block
	// until any outstanding operations are complete. Mabe.
	UsbThread usbThread;
	
	MainWindow w(usbThread);
	w.show();
	
	return a.exec();
}
