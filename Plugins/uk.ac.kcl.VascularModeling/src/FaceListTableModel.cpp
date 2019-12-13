#include "FaceListTableModel.h"

#include <HierarchyManager.h>
#include <VascularModelingNodeTypes.h>
#include <VesselPathAbstractData.h>

#include <QStringList>

Q_DECLARE_METATYPE(crimson::FaceIdentifier);

void FaceListTableModel::setVesselTreeNode(const mitk::DataNode::Pointer& vesselTreeNode)
{
    if (vesselTreeNode != _vesselTreeNode) {
        _vesselTreeNode = vesselTreeNode;
        setFaces(std::set<crimson::FaceIdentifier>()); // will call reset()
    }
}

void FaceListTableModel::setFaces(const std::set<crimson::FaceIdentifier>& faces)
{
    beginResetModel();
    _faces = faces;
    endResetModel();
}

int FaceListTableModel::rowCount(const QModelIndex& /*parent = QModelIndex()*/) const
{
    return _faces.size();
}

int FaceListTableModel::columnCount(const QModelIndex& /*parent = QModelIndex()*/) const
{
    return 2;
}

QVariant FaceListTableModel::data(const QModelIndex& index, int role /*= Qt::DisplayRole*/) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    static std::map<crimson::FaceIdentifier::FaceType, QString> faceTypeNames = {
            { crimson::FaceIdentifier::ftCapInflow, QString("inflow") },
            { crimson::FaceIdentifier::ftCapOutflow, QString("outflow") },
            { crimson::FaceIdentifier::ftWall, QString("wall") }
    };

    auto faceIterator = _faces.begin();
    std::advance(faceIterator, index.row());

    if (role == FaceIdRole) {
        return QVariant::fromValue(*faceIterator);
    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }


    if (index.column() == 0) {
        return QVariant::fromValue(faceTypeNames[faceIterator->faceType]);
    }
    else {
        assert(index.column() == 1);

        std::map<std::string, std::string> vesselNamesMap;
        if (_vesselTreeNode) {
            mitk::DataStorage::SetOfObjects::ConstPointer vesselPathNodes = 
                crimson::HierarchyManager::getInstance()->getDescendants(_vesselTreeNode, crimson::VascularModelingNodeTypes::VesselPath());
            for (const mitk::DataNode::Pointer& vesselNode : *vesselPathNodes) {
                vesselNamesMap[static_cast<crimson::VesselPathAbstractData*>(vesselNode->GetData())->getVesselUID()] = vesselNode->GetName();
            }
        }

        QStringList allVesselNames;
        for (const crimson::VesselPathAbstractData::VesselPathUIDType& vesselUID : faceIterator->parentSolidIndices) {
            if (vesselNamesMap.find(vesselUID) != vesselNamesMap.end()) {
                allVesselNames += QString::fromStdString(vesselNamesMap[vesselUID]);
            }
            else {
                allVesselNames += QString::fromStdString(vesselUID);
            }
        }

        return QVariant::fromValue(allVesselNames.join(", "));
    }
}

QVariant FaceListTableModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole) {
        return QAbstractTableModel::headerData(section, orientation, role);
    }

    switch (section) {
    case 0:
        return QVariant::fromValue(tr("Type"));
    case 1:
        return QVariant::fromValue(tr("Vessels"));
    }

    return QVariant();
}
