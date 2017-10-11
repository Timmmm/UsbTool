#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QMessageBox>
#include <QDebug>

#include "usb/UsbSpecification.h"

MainWindow::MainWindow(UsbThread& thread, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    usbThread(thread)
{
	ui->setupUi(this);
	
	// All the font hints don't seem to work.
	ui->controlReceiveDataLabelHex->setFont(QFont("courier"));
	ui->controlReceiveDataLabelAscii->setFont(QFont("courier"));
	
	ui->deviceList->setModel(&devicesModel);
	ui->recipientList->setCurrentRow(0);
	ui->typeList->setCurrentRow(2);
	
	ui->interfacesTreeView->setModel(&interfacesModel);
	
	// Start a timer to enumerate devices periodically.
	connect(this, &MainWindow::requestEnumerateDevices, &usbThread, &UsbThread::enumerateDevices);
	connect(this, &MainWindow::requestDeviceDescriptors, &usbThread, &UsbThread::deviceDescriptors);
	connect(this, &MainWindow::requestControlInTransfer, &usbThread, &UsbThread::controlInTransfer);
	connect(this, &MainWindow::requestControlOutTransfer, &usbThread, &UsbThread::controlOutTransfer);
	
	connect(&usbThread, &UsbThread::enumerateDevicesResult, this, &MainWindow::onEnumerateDevicesResult);
	connect(&usbThread, &UsbThread::deviceDescriptorsResult, this, &MainWindow::onDeviceDescriptorsResult);
	connect(&usbThread, &UsbThread::controlInTransferResult, this, &MainWindow::onControlInTransferResult);
	connect(&usbThread, &UsbThread::controlOutTransferResult, this, &MainWindow::onControlOutTransferResult);
			
	connect(ui->deviceList->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onDeviceSelectionChanged);
	
	connect(ui->interfacesTreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::onInterfaceSelectionChanged);
	
	connect(&enumerateTimer, &QTimer::timeout, &usbThread, &UsbThread::enumerateDevices);
#ifdef _WIN32
	enumerateTimer.start(1000);
#endif
	
	emit requestEnumerateDevices();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::onEnumerateDevicesResult(const QVector<DeviceInfo>& devices)
{
	qDebug() << "Updating data with" << devices.size() << "device";
	devicesModel.updateData(devices);
}

void MainWindow::onDeviceDescriptorsResult(DeviceId loc, bool success, DeviceDescriptor desc)
{
	// TODO: We should really verify that it is for the same device
	// in case they click wildly.
	
	if (!success)
	{
		// TODO: Better error.
		return;
	}
	
	QString text = QString(R"#(
<h2>%1</h2>
<p>
<table>
<tr><td>USB version:</td><td>%2.%3</td></tr>
<tr><td>Device class:</td><td>%4</td></tr>
<tr><td>Device sub-class:</td><td>%5</td></tr>
<tr><td>Device protocol:</td><td>%6</td></tr>
<tr><td>Max control endpoint packet size:</td><td>%7 bytes</td></tr>
<tr><td>Vendor ID:</td><td>0x%8</td></tr>
<tr><td>Product ID:</td><td>0x%9</td></tr>
<tr><td>Device release:</td><td>%10.%11</td></tr>
</table>
)#")
	        .arg("Device Descriptor")
	        .arg(desc.bcdUSB >> 8)
	        .arg(desc.bcdUSB & 0xFF)
	        .arg(desc.bDeviceClass)
	        .arg(desc.bDeviceSubClass)
	        .arg(desc.bDeviceProtocol)
	        .arg(desc.bMaxPacketSize0)
	        .arg(desc.idVendor, 4, 16, QChar('0'))
	        .arg(desc.idProduct, 4, 16, QChar('0'))
	        .arg(desc.bcdDevice >> 8)
	        .arg(desc.bcdDevice & 0xFF);
	
	ui->deviceDescriptorLabel->setText(text);
	
	interfacesModel.setDescriptors(desc);
	ui->interfacesTreeView->expandAll();
}

void MainWindow::onControlOutTransferResult(DeviceId loc, bool success)
{
	
}

void MainWindow::onControlInTransferResult(DeviceId loc, bool success, const QByteArray& data)
{
	QString asHex = data.toHex();
	// Add some spaces so word wrap behaves.
	QString asSpacedHex;
	for (int i = 0; i < asHex.size(); i += 2)
	{
		asSpacedHex += asHex.mid(i, 2);
		asSpacedHex += ' ';
	}
	
	QString asLatin1 = QString::fromLatin1(data);
	
	ui->controlReceiveDataLabelHex->setText(asSpacedHex);
	ui->controlReceiveDataLabelAscii->setText(asLatin1);
}

void MainWindow::onDeviceSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	ui->deviceDescriptorLabel->clear();
	selectedLoc = DeviceId();
	interfacesModel.setDescriptors(DeviceDescriptor());
	
	if (selected.indexes().empty())
		return;
	
	// Get the device location.
	QVariant locVar = devicesModel.data(selected.indexes().first(), Qt::UserRole);
	selectedLoc = locVar.value<DeviceId>();

	qDebug() << "Requesting device descriptors";
	
	// Request the device descriptor.
	emit requestDeviceDescriptors(selectedLoc);
}

void MainWindow::onInterfaceSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	ui->pageStack->setCurrentIndex(0);
	if (selected.indexes().empty())
		return;

	QVariant dataVar = interfacesModel.data(selected.indexes().first(), Qt::UserRole);
	auto data = dataVar.value<DeviceInterfacesModel::TreeNodeData>();
	
	switch (data.type)
	{
	case DeviceInterfacesModel::NodeType::Device:
		ui->pageStack->setCurrentIndex(0);
		break;
	case DeviceInterfacesModel::NodeType::EndpointZero:
		ui->pageStack->setCurrentIndex(1);
		break;
	case DeviceInterfacesModel::NodeType::Configuration:
		ui->pageStack->setCurrentIndex(0);
		break;
	case DeviceInterfacesModel::NodeType::Interface:
		ui->pageStack->setCurrentIndex(0);
		break;
	case DeviceInterfacesModel::NodeType::Endpoint:
		// Switch depending on the endpoint type.
		ui->pageStack->setCurrentIndex(2);
		break;
	default:
		break;
	}
}

void MainWindow::on_actionExit_triggered()
{
	close();
}

void MainWindow::on_controlSendButton_clicked()
{
	QMessageBox::information(this, "Not implemented yet", "Not implemented yet");
}

void MainWindow::on_controlReceiveButton_clicked()
{
	if (!selectedLoc)
		return;
	
	Device::Recipient recipient = Device::Recipient::Device;
	switch (ui->recipientList->selectionModel()->selectedIndexes().first().row())
	{
	default:
	case 0:
		recipient = Device::Recipient::Device;
		break;
	case 1:
		recipient = Device::Recipient::Interface;
		break;
	case 2:
		recipient = Device::Recipient::Endpoint;
		break;
	case 3:
		recipient = Device::Recipient::Other;
		break;
	}
	Device::Type type = Device::Type::Vendor;
	switch (ui->typeList->selectionModel()->selectedIndexes().first().row())
	{
	case 0:
		type = Device::Type::Standard;
		break;
	case 1:
		type = Device::Type::Class;
		break;
	default:
	case 2:
		type = Device::Type::Vendor;
		break;
	}
	uint8_t bRequest = ui->bRequestSpinbox->value();
	uint16_t wValue = ui->wValueSpinbox->value();
	uint16_t wIndex = ui->wIndexSpinbox->value();
	int length = ui->controlReceiveLengthSpinbox->value();

	emit requestControlInTransfer(selectedLoc, recipient, type, bRequest, wValue, wIndex, length);
}
