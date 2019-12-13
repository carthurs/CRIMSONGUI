#include "VesselPathInteractor.h"
#include "VesselPathAbstractData.h"
#include "VesselPathOperation.h"
#include <Utilities/vesselTreeUtils.h>

#include "mitkInteractionPositionEvent.h"

#include "mitkBaseRenderer.h"
#include "mitkRenderingManager.h"

#include <mitkOperationEvent.h>
#include <mitkUndoController.h>

#include <mitkInteractionConst.h>
#include <mitkStateMachineAction.h>

#include <mitkApplicationCursor.h>

#include "usModule.h"
#include "usGetModuleContext.h"
#include "usModuleResource.h"
#include "usModuleResourceStream.h"


namespace crimson {

VesselPathInteractor::VesselPathInteractor()
    : DataInteractor()
    , m_Precision(5)
    , _selectedPoint(-1)
    , _lastEventSender(nullptr)
    , _currentCursorName("cursor_none")
{
}

VesselPathInteractor::~VesselPathInteractor()
{
    if (_currentCursorName != "cursor_none") {
        mitk::ApplicationCursor::GetInstance()->PopCursor();
    }
    _currentCursorName = "cursor_none";
}

void VesselPathInteractor::DataNodeChanged()
{
    if (_currentCursorName != "cursor_none") {
        mitk::ApplicationCursor::GetInstance()->PopCursor();
    }
    _currentCursorName = "cursor_none";
    if (GetDataNode()) {
        setCursorByName("cursor_normal");
    }
}

bool VesselPathInteractor::FilterEvents(mitk::InteractionEvent* interactionEvent, mitk::DataNode* dataNode)
{
    _lastEventSender = interactionEvent->GetSender();
    return DataInteractor::FilterEvents(interactionEvent, dataNode);
}

void VesselPathInteractor::ConnectActionsAndFunctions()
{
    CONNECT_CONDITION("point_is_valid", CheckPointValidity);
    CONNECT_CONDITION("vessel_path_is_current", CheckVesselPathIsCurrent);
    CONNECT_CONDITION("is_over_point", CheckHoverControlPoint);
    CONNECT_CONDITION("is_over_path", CheckHoverPath);

    CONNECT_FUNCTION("add_new_point_end", AddPoint);
    CONNECT_FUNCTION("add_new_point_on_path", AddPointOnPath);
    CONNECT_FUNCTION("remove_point", RemovePoint);

    CONNECT_FUNCTION("select_point", SelectPoint);
    CONNECT_FUNCTION("move_selected_point", MoveSelectedPoint);
    CONNECT_FUNCTION("deselect_point", DeselectPoint);

    CONNECT_FUNCTION("set_cursor_normal", SetMouseCursor);
    CONNECT_FUNCTION("set_cursor_movepoint", SetMouseCursor);
    CONNECT_FUNCTION("set_cursor_removepoint", SetMouseCursor);
    CONNECT_FUNCTION("set_cursor_addpoint", SetMouseCursor);
    CONNECT_FUNCTION("set_cursor_insertpoint", SetMouseCursor);
}

bool VesselPathInteractor::CheckPointValidity(const mitk::InteractionEvent* /*interactionEvent*/)
{
    return true;
}

bool VesselPathInteractor::CheckVesselPathIsCurrent(const mitk::InteractionEvent* /*interactionEvent*/)
{
    bool current = false;
    GetDataNode()->GetBoolProperty("vesselpath.editing", current);
    return current;
}

bool VesselPathInteractor::CheckHoverControlPoint(const mitk::InteractionEvent* interactionEvent)
{
    auto positionEvent = dynamic_cast<const mitk::InteractionPositionEvent*>(interactionEvent);
    if (positionEvent == nullptr)
        return false;

    return _getPointAt(positionEvent) != -1;
}

bool VesselPathInteractor::CheckHoverPath(const mitk::InteractionEvent* interactionEvent)
{
    auto positionEvent = dynamic_cast<const mitk::InteractionPositionEvent*>(interactionEvent);
    if (positionEvent == nullptr)
        return false;

    VesselPathAbstractData *vesselPath = static_cast<VesselPathAbstractData *>(GetDataNode()->GetData());

    VesselPathAbstractData::ClosestPointQueryResult closestPointQuery = vesselPath->getClosestPoint(positionEvent->GetPositionInWorld());

    return closestPointQuery.resultType == VesselPathAbstractData::ClosestPointQueryResult::CLOSEST_POINT_CURVE &&
        closestPointQuery.closestPoint.EuclideanDistanceTo(positionEvent->GetPositionInWorld()) < _getAccuracyInMM(positionEvent);
}

void VesselPathInteractor::AddPoint(mitk::StateMachineAction*, mitk::InteractionEvent* interactionEvent)
{
    mitk::InteractionPositionEvent* positionEvent = dynamic_cast<mitk::InteractionPositionEvent*>(interactionEvent);
	if (positionEvent == nullptr)
		//return false;
		return;

    mitk::Point3D eventPosition = positionEvent->GetPositionInWorld();

    VesselPathAbstractData *vesselPath = static_cast<VesselPathAbstractData *>(GetDataNode()->GetData());

    int addPointType = static_cast<int>(aptNone);
    GetDataNode()->GetIntProperty("vesselpath.addPointType", addPointType);

    int newPointIndex = vesselPath->controlPointsCount();
    switch (static_cast<AddPointType>(addPointType)) {
    case aptStart:
        newPointIndex = 0;
        break;
    case aptEnd:
        newPointIndex = vesselPath->controlPointsCount();
        break;
    case aptNone:
    case aptAuto:
    default:
        // If the click is closer to the beginning of the path - then add to the beginning. Otherwise - to the end.
        if (vesselPath->controlPointsCount() >= 2) {
            if (vesselPath->getControlPoint(0).SquaredEuclideanDistanceTo(eventPosition) <
                vesselPath->getControlPoint(vesselPath->controlPointsCount() - 1).SquaredEuclideanDistanceTo(eventPosition)) {
                newPointIndex = 0;
            }
        }
        break;
    }

    _addPointToPath(eventPosition, newPointIndex);

    // Update rendered scene
    interactionEvent->GetSender()->GetRenderingManager()->RequestUpdateAll();

    //return true;
}

void VesselPathInteractor::AddPointOnPath(mitk::StateMachineAction*, mitk::InteractionEvent* interactionEvent)
{
    mitk::InteractionPositionEvent* positionEvent = dynamic_cast<mitk::InteractionPositionEvent*>(interactionEvent);
    if (positionEvent == nullptr)
        return;

    VesselPathAbstractData *vesselPath = static_cast<VesselPathAbstractData *>(GetDataNode()->GetData());

    VesselPathAbstractData::ClosestPointQueryResult closestPointQuery = vesselPath->getClosestPoint(positionEvent->GetPositionInWorld());

    if (closestPointQuery.resultType == VesselPathAbstractData::ClosestPointQueryResult::CLOSEST_POINT_CURVE) {
        _addPointToPath(positionEvent->GetPositionInWorld(), closestPointQuery.controlPointId + 1);

        // Update rendered scene
        interactionEvent->GetSender()->GetRenderingManager()->RequestUpdateAll();
        //return true;
    }

    return;
}

void VesselPathInteractor::_addPointToPath(const mitk::Point3D& position, int newPointIndex)
{
    mitk::OperationEvent::IncCurrObjectEventId();
    //mitk::OperationEvent::ExecuteIncrement();

    VesselPathAbstractData *vesselPath = static_cast<VesselPathAbstractData *>(GetDataNode()->GetData());

    VesselPathAbstractData::PointType point;
    point.CastFrom(position);

    auto doOp = new VesselPathOperation(mitk::OpINSERT, point, newPointIndex);

    if (m_UndoEnabled) {
        auto undoOp = new VesselPathOperation(mitk::OpREMOVE, point, newPointIndex);
        auto operationEvent = new mitk::OperationEvent(vesselPath, doOp, undoOp, "Add control point");
        m_UndoController->SetOperationEvent(operationEvent);
    }

    //execute the Operation
    vesselPath->ExecuteOperation(doOp);

    if (!m_UndoEnabled)
        delete doOp;
}

void VesselPathInteractor::RemovePoint(mitk::StateMachineAction*, mitk::InteractionEvent* interactionEvent)
{
    mitk::InteractionPositionEvent* positionEvent = dynamic_cast<mitk::InteractionPositionEvent*>(interactionEvent);
    if (positionEvent == nullptr)
        return;

    int toRemoveId = _getPointAt(positionEvent);
    if (toRemoveId == -1) {
        return;
    }

    mitk::OperationEvent::IncCurrObjectEventId();
    //mitk::OperationEvent::ExecuteIncrement();

    VesselPathAbstractData *vesselPath = static_cast<VesselPathAbstractData *>(GetDataNode()->GetData());

    VesselPathAbstractData::PointType point;
    point.CastFrom(positionEvent->GetPositionInWorld());

    auto doOp = new VesselPathOperation(mitk::OpREMOVE, vesselPath->getControlPoint(toRemoveId), toRemoveId);

    if (m_UndoEnabled) {
        auto undoOp = new VesselPathOperation(mitk::OpINSERT, vesselPath->getControlPoint(toRemoveId), toRemoveId);
        auto operationEvent = new mitk::OperationEvent(vesselPath, doOp, undoOp, "Remove control point");
        m_UndoController->SetOperationEvent(operationEvent);
    }

    //execute the Operation
    vesselPath->ExecuteOperation(doOp);

    if (!m_UndoEnabled)
        delete doOp;

    // Update rendered scene
    interactionEvent->GetSender()->GetRenderingManager()->RequestUpdateAll();

    //return true;
}

void VesselPathInteractor::SelectPoint(mitk::StateMachineAction*, mitk::InteractionEvent* interactionEvent)
{
    mitk::InteractionPositionEvent* positionEvent = dynamic_cast<mitk::InteractionPositionEvent*>(interactionEvent);
    if (positionEvent == nullptr)
        return;

    VesselPathAbstractData *vesselPath = static_cast<VesselPathAbstractData *>(GetDataNode()->GetData());

    _selectedPoint = _getPointAt(positionEvent);
    if (_selectedPoint == -1) {
        return;
    }
    _moveStartPosition = vesselPath->getControlPoint(_selectedPoint);
    _previousMouseEventPosition = positionEvent->GetPositionInWorld();

    // Update rendered scene
    interactionEvent->GetSender()->GetRenderingManager()->RequestUpdateAll();
    //return true;
}


void VesselPathInteractor::MoveSelectedPoint(mitk::StateMachineAction*, mitk::InteractionEvent* interactionEvent)
{
    assert(_selectedPoint != -1);

    mitk::InteractionPositionEvent* positionEvent = dynamic_cast<mitk::InteractionPositionEvent*>(interactionEvent);
    if (positionEvent == nullptr)
        return;

    mitk::Vector3D delta = positionEvent->GetPositionInWorld() - _previousMouseEventPosition;
    _previousMouseEventPosition = positionEvent->GetPositionInWorld();

    VesselPathAbstractData *vesselPath = static_cast<VesselPathAbstractData *>(GetDataNode()->GetData());

    mitk::Point3D newControlPointPosition = vesselPath->getControlPoint(_selectedPoint) + delta;
    vesselPath->setControlPoint(_selectedPoint, newControlPointPosition);

    // Update rendered scene
    interactionEvent->GetSender()->GetRenderingManager()->RequestUpdateAll();

    //return true;
}

void VesselPathInteractor::DeselectPoint(mitk::StateMachineAction*, mitk::InteractionEvent* interactionEvent)
{
    assert(_selectedPoint != -1);

    mitk::InteractionPositionEvent* positionEvent = dynamic_cast<mitk::InteractionPositionEvent*>(interactionEvent);
    if (positionEvent == nullptr)
        return;

    mitk::OperationEvent::IncCurrObjectEventId();
    //mitk::OperationEvent::ExecuteIncrement();

    VesselPathAbstractData *vesselPath = static_cast<VesselPathAbstractData *>(GetDataNode()->GetData());

    auto doOp = new VesselPathOperation(mitk::OpMOVE, vesselPath->getControlPoint(_selectedPoint), _selectedPoint);

    if (m_UndoEnabled) {

        auto undoOp = new VesselPathOperation(mitk::OpMOVE, _moveStartPosition, _selectedPoint);
        auto operationEvent = new mitk::OperationEvent(vesselPath, doOp, undoOp, "Move control point");
        m_UndoController->SetOperationEvent(operationEvent);
    }

    _selectedPoint = -1;

    //execute the Operation
    vesselPath->ExecuteOperation(doOp);

    if (!m_UndoEnabled)
        delete doOp;

    // Update rendered scene
    interactionEvent->GetSender()->GetRenderingManager()->RequestUpdateAll();

    //return true;
}

void VesselPathInteractor::SetMouseCursor(mitk::StateMachineAction* action, mitk::InteractionEvent* interactionEvent)
{
    mitk::InteractionPositionEvent* positionEvent = dynamic_cast<mitk::InteractionPositionEvent*>(interactionEvent);
    if (positionEvent == nullptr)
        return;

    std::string newCursorName = action->GetActionName().substr(4);
    setCursorByName(newCursorName);
    return;
}


void VesselPathInteractor::setCursorByName(const std::string& newCursorName)
{
    if (newCursorName == _currentCursorName) {
        return;
    }

    if (_currentCursorName != "cursor_none") {
        mitk::ApplicationCursor::GetInstance()->PopCursor();
    }

    us::Module* module = us::GetModuleContext()->GetModule();
    us::ModuleResource resource = module->GetResource(newCursorName + ".png");

    if (!resource.IsValid()) {
        MITK_ERROR << "Failed to set cursor " << newCursorName << ".png";
        _currentCursorName = "cursor_none";
        return;
    }

    us::ModuleResourceStream cursor(resource, std::ios::binary);
    mitk::ApplicationCursor::GetInstance()->PushCursor(cursor, 0, 0);
    _currentCursorName = newCursorName;
}


int VesselPathInteractor::_getPointAt(const mitk::InteractionPositionEvent* positionEvent)
{
    VesselPathAbstractData *vesselPath = dynamic_cast<VesselPathAbstractData *>(GetDataNode()->GetData());

    int index = -1;
    float minDist = std::numeric_limits<float>::max();
    for (size_t i = 0; i < vesselPath->controlPointsCount(); ++i) {
        float dist = positionEvent->GetPositionInWorld().EuclideanDistanceTo(vesselPath->getControlPoint(i));
        if (dist < minDist) {
            minDist = dist;
            index = i;
        }
    }

    if (minDist < _getAccuracyInMM(positionEvent)) {
        return index;
    }

    return -1;
}

double VesselPathInteractor::_getAccuracyInMM(const mitk::InteractionPositionEvent* positionEvent)
{
    float pixelSizeInMM = VesselTreeUtils::getPixelSizeInMM(positionEvent->GetSender(), positionEvent->GetPositionInWorld());
    int controlPointSizeInPixels = 6;
    GetDataNode()->GetIntProperty("vesselpath.glyph.editing_screen_size", controlPointSizeInPixels);

    return pixelSizeInMM * controlPointSizeInPixels / 2.0f * sqrt(3.0f);
}

}
