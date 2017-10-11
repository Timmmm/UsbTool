#pragma once

#include <QAbstractListModel>

#include "UsbThread.h"

class DeviceListModel : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit DeviceListModel(QObject *parent = nullptr);
	
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	
	void updateData(const QVector<DeviceInfo>& newDevices);
	
signals:
	
public slots:
	
private:
	QVector<DeviceInfo> devices;
};
