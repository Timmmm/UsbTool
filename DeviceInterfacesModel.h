#pragma once

#include <QAbstractItemModel>

#include "usb/Descriptors.h"

// This is a read-only tree model which has a list of the device interfaces, alternates, endpoints etc.
// The root element is the device itself - so that the user can click it to show device-wide info
// like the device, config & string descriptors.
class DeviceInterfacesModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	explicit DeviceInterfacesModel(QObject *parent = nullptr);
	
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex &index) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	
	void setDescriptors(DeviceDescriptor d);
	
	enum class NodeType
	{
		Device,
		EndpointZero,
		Configuration,
		Interface,
		Endpoint,
	};
	
	struct TreeNodeData
	{
		NodeType type = NodeType::Device;
		int configuration = -1;
		int interface_ = -1; // MSVC reserves 'interface' annoyingly.
		int endpoint = -1;
	};
	
signals:
	
public slots:
	
private:
	DeviceDescriptor descriptor;
	
	// This is an index into our tree structure.
	// It allows reference to any node, by setting a value to -1 it
	// means not to follow the tree further. For example the Device node is:
	//
	//  { configuration = -1; ...; parent = 0; }
	//
	// The configuration nodes are:
	//
	//  { configuration = N; interface = -1; ...}
	//
	// And so on.
	//
	// There is one special virtual node - the control endpoint that is:
	//
	//  { configuration = -1; interface = -1; endpoint = 0; parent = 1; }

	
	struct Node
	{
		TreeNodeData data;
		
		quintptr parent = 0;
		QVector<quintptr> children;
	};
	
	QMap<quintptr, Node> nodes;
};

Q_DECLARE_METATYPE(DeviceInterfacesModel::TreeNodeData)
Q_DECLARE_METATYPE(DeviceInterfacesModel::NodeType)
