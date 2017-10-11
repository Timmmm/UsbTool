#include "DeviceListModel.h"

#include "Metatypes.h"

#include <QDebug>

DeviceListModel::DeviceListModel(QObject *parent) : QAbstractListModel(parent)
{
	
}

int DeviceListModel::rowCount(const QModelIndex& parent) const
{
	return devices.size();
}

QVariant DeviceListModel::data(const QModelIndex& index, int role) const
{
	int i = index.row();
	
	if (i < 0 || i >= devices.size())
		return QVariant();
	
	switch (role)
	{
	case Qt::DisplayRole:
		if (!devices[i].product.empty())
			return QString::fromStdString(devices[i].product);
		return QString::fromStdString(DeviceIdToString(devices[i].id));
	case Qt::UserRole:
		qDebug() << "Returning address:" << QString::fromStdString(DeviceIdToString(devices[i].id));
		return QVariant::fromValue(devices[i].id);
	default:
		break;
	}
	return QVariant();
}

QVariant DeviceListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
		return QVariant();
	
	return "Device";
}

void DeviceListModel::updateData(const QVector<DeviceInfo>& newDevices)
{
	// For now we use a simple method of detecting device removal/addition. If every
	// element of DeviceInfo (which includes the port location and address) is the same
	// then it is probably the same device.
	
	// This is a simple but suboptimal way to do it but we're probably never going to have that many devices
	// anyway so whatever.
	
	// Check for removed devices.
	for (int i = devices.size()-1; i >= 0; --i)
	{
		// See if it is still in newDevices.
		bool stillThere = false;
		for (const DeviceInfo& newDev : newDevices)
		{
			if (newDev == devices[i])
			{
				stillThere = true;
				break;
			}
		}
		if (!stillThere)
		{
			// Remove it.
			beginRemoveRows(QModelIndex(), i, i);
			devices.removeAt(i);
			endRemoveRows();
		}
	}
	
	// Check for new devices.
	for (const DeviceInfo& newDev : newDevices)
	{
		bool isNew = true;
		for (DeviceInfo& dev : devices)
		{
			if (dev == newDev)
			{
				isNew = false;
				break;
			}
		}
		if (isNew)
		{
			// Insert in alphabetical order.
			QString newDevProductLowercase = QString::fromStdString(newDev.product).toLower();
			int i = 0;
			for (; i < devices.size(); ++i)
			{
				QString devProductLowercase = QString::fromStdString(devices[i].product).toLower();
				if (newDevProductLowercase < devProductLowercase)
				{
					break;
				}
			}
			beginInsertRows(QModelIndex(), i, i);
			devices.insert(i, newDev);
			endInsertRows();
		}
	}
}
