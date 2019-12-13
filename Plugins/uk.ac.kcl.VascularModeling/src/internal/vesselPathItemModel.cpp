#include "vesselPathItemModel.h"
#include "VesselForestData.h"

#include <mitkUndoController.h>
#include <mitkInteractionConst.h>
#include <VesselPathOperation.h>

#include <itkCommand.h>

VesselPathItemModel::VesselPathItemModel( QObject* parent /*= nullptr*/ )
    : QStandardItemModel(parent)
    , _vesselPath(nullptr)
    , _observerTag(0)
    , _ignoreVesselPathEvents(false)
{
    _setupHeader();
}

void VesselPathItemModel::setVesselPath(crimson::VesselPathAbstractData::Pointer data)
{
    if (_vesselPath == data) {
        return;
    }

    if (_vesselPath) {
        // Unsubscribe from old vessel path change events
        _vesselPath->RemoveObserver(_observerTag);
        _observerTag = 0;
    }

    setRowCount(0);

    _vesselPath = data;

    if (data) {
        _resetAllControlPoints();

        // Subscribe to new vessel forest change events
        auto modifiedCommand = itk::MemberCommand<VesselPathItemModel>::New();
        modifiedCommand->SetCallbackFunction(this, &VesselPathItemModel::_onVesselPathChanged);

        _observerTag = _vesselPath->AddObserver(crimson::VesselPathAbstractData::VesselPathEvent(), modifiedCommand);
    }
}

void VesselPathItemModel::_onVesselPathChanged(itk::Object *, const itk::EventObject &event)
{
    if (_ignoreVesselPathEvents) {
        return;
    }

    auto vesselPathEvt = dynamic_cast<const crimson::VesselPathAbstractData::VesselPathEvent*>(&event);
    if (!vesselPathEvt) {
        return;
    }

    if (auto evt = dynamic_cast<const crimson::VesselPathAbstractData::ControlPointInsertEvent*>(&event)) {
        _addControlPointRow(evt->GetControlPointId());
    }
    else if (auto evt = dynamic_cast<const crimson::VesselPathAbstractData::ControlPointRemoveEvent*>(&event)) {
        _removeControlPointRow(evt->GetControlPointId());
    }
    else if (auto evt = dynamic_cast<const crimson::VesselPathAbstractData::ControlPointModifiedEvent*>(&event)) {
        _updateControlPointRow(evt->GetControlPointId());
    }
    else if (dynamic_cast<const crimson::VesselPathAbstractData::AllControlPointsSetEvent*>(&event)) {
        _resetAllControlPoints();
    }
    else {
        return;
    }
}

void VesselPathItemModel::_addControlPointRow(crimson::VesselPathAbstractData::IdType id)
{
    crimson::VesselPathAbstractData::PointType controlPoint = _vesselPath->getControlPoint(id);

    QVector<QStandardItem*> rowItems;
    for (int i = 0; i < 3; ++i) {
        QStandardItem* coordItem = new QStandardItem(QString("%1").arg(controlPoint[i], 0, 'f', 2));
        rowItems.append(coordItem);
    }
    this->insertRow(id, rowItems.toList());
}

void VesselPathItemModel::_removeControlPointRow(crimson::VesselPathAbstractData::IdType id)
{
    QStandardItemModel::removeRows(id, 1);  // Bypass undo/redo coming from our implementation
}

void VesselPathItemModel::_updateControlPointRow(crimson::VesselPathAbstractData::IdType id)
{
    crimson::VesselPathAbstractData::PointType controlPoint = _vesselPath->getControlPoint(id);
    for (int i = 0; i < 3; ++i) {
        item(id, i)->setText(QString("%1").arg(controlPoint[i], 0, 'f', 2));
    }
}

void VesselPathItemModel::_resetAllControlPoints()
{
    setRowCount(0);

    // Fill the item model from the forest data
    for (size_t i = 0; i < _vesselPath->controlPointsCount(); ++i) {
        _addControlPointRow(i);
    }
}

bool VesselPathItemModel::setData(const QModelIndex &index, const QVariant &value, int role /* = Qt::EditRole */)
{
    bool ok;
    double coord = value.toDouble(&ok);
    if (!ok) {
        return false;
    }

    bool rc = QStandardItemModel::setData(index, value, role);

    if (rc) {
        mitk::OperationEvent::IncCurrObjectEventId();
//        mitk::OperationEvent::ExecuteIncrement();

        crimson::VesselPathAbstractData::PointType startPosition = _vesselPath->getControlPoint(index.row());
        crimson::VesselPathAbstractData::PointType endPosition = startPosition;
        endPosition[index.column()] = coord;

        auto doOp = new crimson::VesselPathOperation(mitk::OpMOVE, endPosition, index.row());
        auto undoOp = new crimson::VesselPathOperation(mitk::OpMOVE, startPosition, index.row());
        auto operationEvent = new mitk::OperationEvent(_vesselPath, doOp, undoOp, "Move control point");
        mitk::UndoController::GetCurrentUndoModel()->SetOperationEvent(operationEvent);

        _vesselPath->ExecuteOperation(doOp);
    }

    return rc;
}

bool VesselPathItemModel::removeRows(int row, int count, const QModelIndex &parent)
{
    _ignoreVesselPathEvents = true;
    for (int i = 0; i < count; ++i) {
        mitk::OperationEvent::IncCurrObjectEventId();
//        mitk::OperationEvent::ExecuteIncrement();

        auto doOp = new crimson::VesselPathOperation(mitk::OpREMOVE, _vesselPath->getControlPoint(row), row);
        auto undoOp = new crimson::VesselPathOperation(mitk::OpINSERT, _vesselPath->getControlPoint(row), row);
        auto operationEvent = new mitk::OperationEvent(_vesselPath, doOp, undoOp, count == 1 ? "Remove control point" : "Remove control points");
        mitk::UndoController::GetCurrentUndoModel()->SetOperationEvent(operationEvent);

        _vesselPath->ExecuteOperation(doOp);
    }
    _ignoreVesselPathEvents = false;

    return QStandardItemModel::removeRows(row, count, parent);
}

void VesselPathItemModel::_setupHeader()
{
    setColumnCount(3);
    setHeaderData(0, Qt::Horizontal, tr("X"), Qt::DisplayRole);
    setHeaderData(1, Qt::Horizontal, tr("Y"), Qt::DisplayRole);
    setHeaderData(2, Qt::Horizontal, tr("Z"), Qt::DisplayRole);
}


