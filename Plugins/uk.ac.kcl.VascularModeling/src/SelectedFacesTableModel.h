#pragma once

#include <FaceListTableModel.h>

#include "uk_ac_kcl_VascularModeling_Export.h"

namespace crimson
{
class SolidDataFacePickingObserver;
}

/*! \brief   A FaceListTableModel model with an extra column showing whether the face is selected. */
class VASCULARMODELING_EXPORT SelectedFacesTableModel : public FaceListTableModel
{
    Q_OBJECT
public:
    SelectedFacesTableModel(QObject* parent = nullptr);

	void setFacePickingObserver(crimson::SolidDataFacePickingObserver* facePickingObserver);
	crimson::SolidDataFacePickingObserver* getFacePickingObserver() const { return _facePickingObserver; }

    int columnCount(const QModelIndex& /*parent*/ = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    //    void emitDataChanged(const std::vector<crimson::FaceIdentifier>& faceIds);

private:
	crimson::SolidDataFacePickingObserver* _facePickingObserver = nullptr;
};