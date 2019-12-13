#include "SelectedFacesTableModel.h"
#include <SolidDataFacePickingObserver.h>

SelectedFacesTableModel::SelectedFacesTableModel(QObject* parent)
    : FaceListTableModel(parent)
{
}

void SelectedFacesTableModel::setFacePickingObserver(crimson::SolidDataFacePickingObserver* facePickingObserver)
{
    beginResetModel();
    _facePickingObserver = facePickingObserver;
    setFaces(_facePickingObserver ? _facePickingObserver->getSelectableFaces() : std::set<crimson::FaceIdentifier>{});
    endResetModel();
}

int SelectedFacesTableModel::columnCount(const QModelIndex&) const { return 3; }

QVariant SelectedFacesTableModel::data(const QModelIndex& index, int role) const
{
    if (index.column() < 2) {
        return FaceListTableModel::data(index, role);
    }

    if (static_cast<int>(_faces.size()) < index.row() || !_facePickingObserver) {
        return QVariant();
    }

    
    auto faceIterator = _faces.begin();
    std::advance(faceIterator, index.row());

    auto faceSelected = _facePickingObserver->isFaceSelected(*faceIterator);

    switch (role) {
    case Qt::CheckStateRole:
        return faceSelected ? Qt::Checked : Qt::Unchecked;
    case Qt::DisplayRole:
        return faceSelected;
    default:
        return QVariant{};
    }
}

bool SelectedFacesTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (index.column() < 2 || role != Qt::CheckStateRole) {
        return FaceListTableModel::setData(index, value, role);
    }

    auto faceIterator = _faces.begin();
    std::advance(faceIterator, index.row());

    _facePickingObserver->selectFace(*faceIterator, value == Qt::Checked);
    emit dataChanged(index, index, {role});
    return true;
}

QVariant SelectedFacesTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical || role != Qt::DisplayRole) {
        return FaceListTableModel::headerData(section, orientation, role);
    }

    if (section < 2) {
        return FaceListTableModel::headerData(section, orientation, role);
    }

    return QVariant::fromValue(tr("Selected"));
}

Qt::ItemFlags SelectedFacesTableModel::flags(const QModelIndex& index) const
{
    if (index.column() < 2) {
        return FaceListTableModel::flags(index);
    }

    return FaceListTableModel::flags(index) | Qt::ItemIsUserCheckable;
}

//
// void SelectedFacesTableModel::emitDataChanged(const std::vector<crimson::FaceIdentifier>& faceIds)
//{
//    for (const crimson::FaceIdentifier& faceId : faceIds) {
//        int row = std::distance(_faces.begin(), _faces.find(faceId));
//        emit dataChanged(this->index(row, 2), this->index(row, 2));
//    }
//}
