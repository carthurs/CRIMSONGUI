#pragma once

#include <QStandardItemModel>

#include "VesselPathAbstractData.h"

/*! \brief   A QStandardItemModel representing a list control points of a vessel path. */
class VesselPathItemModel : public QStandardItemModel  {
    Q_OBJECT
public:
    VesselPathItemModel(QObject* parent = nullptr);

    void setVesselPath(crimson::VesselPathAbstractData::Pointer data);
    crimson::VesselPathAbstractData::Pointer getVesselPath() const { return _vesselPath; }

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

private:
    crimson::VesselPathAbstractData::Pointer _vesselPath;
    void _setupHeader();

private:
    //////////////////////////////////////////////////////////////////////////
    // Event processing
    //////////////////////////////////////////////////////////////////////////
    unsigned long _observerTag;
    bool _ignoreVesselPathEvents;

    void _onVesselPathChanged(itk::Object *caller, const itk::EventObject &event);
    void _addControlPointRow(crimson::VesselPathAbstractData::IdType id);
    void _removeControlPointRow(crimson::VesselPathAbstractData::IdType id);
    void _updateControlPointRow(crimson::VesselPathAbstractData::IdType id);
    void _resetAllControlPoints();
};

