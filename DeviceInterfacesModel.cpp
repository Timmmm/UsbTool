#include "DeviceInterfacesModel.h"

DeviceInterfacesModel::DeviceInterfacesModel(QObject *parent) : QAbstractItemModel(parent)
{
	
}

QVariant DeviceInterfacesModel::data(const QModelIndex& index, int role) const
{
	// The (invisible) root node. This should never happen.
	if (!index.isValid())
		return QVariant();

	switch (role)
	{
	case Qt::DisplayRole:
	{
		quintptr id = index.internalId();
		if (!nodes.contains(id))
			return "";
		
		switch (nodes[id].data.type)
		{
		case NodeType::Device:
			return "Device";
		case NodeType::EndpointZero:
			return "Endpoint 0";
		case NodeType::Configuration:
		{
			int c = nodes[id].data.configuration;
			if (c < 0 || c >= descriptor.configurations.size())
				return "";
			return "bConfigurationValue " + QString::number(descriptor.configurations[c].bConfigurationValue);
		}
		case NodeType::Interface:
		{
			int c = nodes[id].data.configuration;
			if (c < 0 || c >= descriptor.configurations.size())
				return "";
			int i = nodes[id].data.interface_;
			if (i < 0 || i >= descriptor.configurations[c].interfaces.size())
				return "";
			return "bInterfaceNumber " + QString::number(descriptor.configurations[c].interfaces[i].bInterfaceNumber);
		}
		case NodeType::Endpoint:
		{
			int c = nodes[id].data.configuration;
			if (c < 0 || c >= descriptor.configurations.size())
				return "";
			int i = nodes[id].data.interface_;
			if (i < 0 || i >= descriptor.configurations[c].interfaces.size())
				return "";
			int e = nodes[id].data.endpoint;
			if (e < 0 || e >= descriptor.configurations[c].interfaces[i].endpoints.size())
				return "";
			return "bEndpointAddress " + QString::number(descriptor.configurations[c].interfaces[i].endpoints[e].bEndpointAddress);
		}
		default:
			return "";
		}
		break;
	}
	case Qt::UserRole:
	{
		quintptr id = index.internalId();
		if (!nodes.contains(id))
			return QVariant();
		
		return QVariant::fromValue(nodes[id].data);
	}
	default:
		break;
	}
	return QVariant();
}

Qt::ItemFlags DeviceInterfacesModel::flags(const QModelIndex& index) const
{
	return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant DeviceInterfacesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	return QVariant();
}

QModelIndex DeviceInterfacesModel::index(int row, int column, const QModelIndex& parent) const
{
	// This checks the row and column against rowCount/columnCount.
	if (!hasIndex(row, column, parent))
		return QModelIndex();
	
	// Must be the root device node, which always has an index of 1.
	if (!parent.isValid())
		return createIndex(row, column, 1);
	
	quintptr id = parent.internalId();
	if (!nodes.contains(id))
		return QModelIndex();
	
	// We need to return the index of the child item of `parent` at the given row/column.
	if (row < 0 || row >= nodes[id].children.size())
		return QModelIndex();

	return createIndex(row, column, nodes[id].children[row]);
}

QModelIndex DeviceInterfacesModel::parent(const QModelIndex& index) const
{
	if (!index.isValid())
		return QModelIndex();
	
	quintptr id = index.internalId();
	if (!nodes.contains(id))
		return QModelIndex();
	
	// Get the row.
	quintptr parentId = nodes[id].parent;
	if (!nodes.contains(parentId))
		return QModelIndex();
	
	for (int r = 0; r < nodes[parentId].children.size(); ++r)
		if (nodes[parentId].children[r] == id)
			return createIndex(r, 0, parentId);
	
	return QModelIndex();
}

int DeviceInterfacesModel::rowCount(const QModelIndex& parent) const
{
	if (parent.column() > 0)
		return 0;
	
	// The top level item. We have one device.
	if (!parent.isValid())
		return 1;
	
	quintptr id = parent.internalId();
	if (!nodes.contains(id))
		return 0;
	
	return nodes[id].children.size();
}

int DeviceInterfacesModel::columnCount(const QModelIndex& parent) const
{
	return 1;
}

void DeviceInterfacesModel::setDescriptors(DeviceDescriptor d)
{
	beginResetModel();
	descriptor = d;
	
	nodes.clear();
	
	// 0 is like a null pointer. If QModelIndex::internalId() is 0 then QModelIndex::isValid() returns false.
	quintptr idx = 1;
	
	// Populate the indices.
	quintptr deviceId = idx++;
	nodes[deviceId].parent = 0;
	nodes[deviceId].data.type = NodeType::Device;
	
	// Add the virtual endpoint 0 node.
	quintptr ep0Id = idx++;
	nodes[ep0Id].parent = deviceId;
	nodes[ep0Id].data.type = NodeType::EndpointZero;
	nodes[deviceId].children.append(ep0Id);
	
	for (int c = 0; c < descriptor.configurations.size(); ++c)
	{
		quintptr configId = idx++;
		nodes[configId].data.configuration = c;
		nodes[configId].parent = deviceId;
		nodes[configId].data.type = NodeType::Configuration;
		nodes[deviceId].children.append(configId);
		
		for (int i = 0; i < descriptor.configurations[c].interfaces.size(); ++i)
		{
			quintptr interfaceId = idx++;
			nodes[interfaceId].data.configuration = c;
			nodes[interfaceId].data.interface_ = i;
			nodes[interfaceId].parent = configId;
			nodes[interfaceId].data.type = NodeType::Interface;
			nodes[configId].children.append(interfaceId);
			
			for (int e = 0; e < descriptor.configurations[c].interfaces[i].endpoints.size(); ++e)
			{
				quintptr endpointId = idx++;
				nodes[endpointId].data.configuration = c;
				nodes[endpointId].data.interface_ = i;
				nodes[endpointId].data.endpoint = e;
				nodes[endpointId].parent = interfaceId;
				nodes[endpointId].data.type = NodeType::Endpoint;
				nodes[interfaceId].children.append(endpointId);
			}
		}
	}
	
	endResetModel();
}
