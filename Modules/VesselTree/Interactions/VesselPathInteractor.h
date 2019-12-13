#pragma once

#include "VesselTreeExports.h"

#include <mitkDataInteractor.h>

namespace mitk {
    class DataNode;
    class BaseRenderer;
    class StateMachineAction;
    class InteractionEvent;
    class InteractionPositionEvent;
}

namespace crimson {

/**
 * \brief User interaction with a vessel path (add, remove, insert or move control points).
 */
class VesselTree_EXPORT VesselPathInteractor : public mitk::DataInteractor
{
public:
    mitkClassMacro(VesselPathInteractor, mitk::DataInteractor);
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);

    int getMovingControlPoint() const { return _selectedPoint; }
    // Get the renderer that sent the last event to this interactor
    mitk::BaseRenderer* lastEventSender() { return _lastEventSender; }

    /*! \brief   Type of add point operation. */
    enum AddPointType {
        aptNone,
        aptAuto,    ///< Add point to closest vessel path end
        aptStart,   ///< Add point to beginning of the path
        aptEnd  ///< Add point to end of the path
    };

protected:
    VesselPathInteractor();
    virtual ~VesselPathInteractor();

    void DataNodeChanged() override;

    void ConnectActionsAndFunctions() override;
    bool FilterEvents(mitk::InteractionEvent* interactionEvent, mitk::DataNode* dataNode) override;

    ////////  Conditions ////////
    bool CheckPointValidity(const mitk::InteractionEvent* interactionEvent);
    bool CheckVesselPathIsCurrent(const mitk::InteractionEvent* interactionEvent);
    bool CheckHoverControlPoint(const mitk::InteractionEvent* interactionEvent);
    bool CheckHoverPath(const mitk::InteractionEvent* interactionEvent);

    ////////  Actions ////////
    void AddPoint(mitk::StateMachineAction*, mitk::InteractionEvent* interactionEvent);
	void AddPointOnPath(mitk::StateMachineAction*, mitk::InteractionEvent* interactionEvent);
	void RemovePoint(mitk::StateMachineAction*, mitk::InteractionEvent* interactionEvent);

	void SelectPoint(mitk::StateMachineAction*, mitk::InteractionEvent* interactionEvent);
	void MoveSelectedPoint(mitk::StateMachineAction*, mitk::InteractionEvent* interactionEvent);
	void DeselectPoint(mitk::StateMachineAction*, mitk::InteractionEvent* interactionEvent);
	void SetMouseCursor(mitk::StateMachineAction*, mitk::InteractionEvent* interactionEvent);

    void setCursorByName(const std::string& newCursorName);

    ////////  Utility functions ////////
    void _addPointToPath(const mitk::Point3D& position, int newPointIndex);
    int _getPointAt(const mitk::InteractionPositionEvent* positionEvent);

    double _getAccuracyInMM(const mitk::InteractionPositionEvent* positionEvent);

    /** \brief to store the value of precision to pick a point */
    unsigned int m_Precision;
    mitk::Point3D _moveStartPosition;
    mitk::Point3D _previousMouseEventPosition;
    int _selectedPoint;
    mitk::BaseRenderer* _lastEventSender;
    std::string _currentCursorName;
};

}