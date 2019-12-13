#pragma once

#include <set>

#include <mitkDataNode.h>
#include <FaceIdentifier.h>

#include <QAbstractTableModel>

#include "uk_ac_kcl_VascularModeling_Export.h"

/*! \brief   A QAbstractTableModel representing a list of faces of a solid model. */
class VASCULARMODELING_EXPORT FaceListTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    static const int FaceIdRole = Qt::UserRole + 100;

public:
    FaceListTableModel(QObject* parent = nullptr) : QAbstractTableModel(parent) {}
    
    void setVesselTreeNode(const mitk::DataNode::Pointer& vesselTreeNode);
    void setFaces(const std::set<crimson::FaceIdentifier>& faces);
    
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data( const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

protected:
    mitk::DataNode::Pointer _vesselTreeNode;
    mitk::DataNode::Pointer _solidModelNode;
    std::set<crimson::FaceIdentifier> _faces;
};