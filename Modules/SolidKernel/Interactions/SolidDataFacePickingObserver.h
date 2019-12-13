#pragma once

#include <set>
#include <FaceIdentifier.h>

#include "SolidKernelExports.h"

#include <mitkPickingEventObserver.h>
#include <mitkTransferFunctionProperty.h>

namespace mitk {
    class DataNode;
    class BaseRenderer;
    class StateMachineAction;
    class InteractionEvent;
    class InteractionPositionEvent;
}

namespace crimson {

/*! \brief   The picking event observer that allows selecting the faces of the SolidData in a 3D rendering window. */
class SolidKernel_EXPORT SolidDataFacePickingObserver : public mitk::PickingEventObserver
{
public:
    SolidDataFacePickingObserver();
    virtual ~SolidDataFacePickingObserver();

    void SetDataNode(mitk::DataNode* dataNode);

    ///@{ 
    /*!
     * \brief   Sets the types of the faces that are allowed to be selected.
     */
    void setSelectableTypes(const std::set<crimson::FaceIdentifier::FaceType>& types) { _selectableTypes = types; _updateTransferFunction(); }

    /*!
     * \brief   Gets the types of the faces that are allowed to be selected.
     */
    const std::set<crimson::FaceIdentifier::FaceType>& getSelectableTypes() const { return _selectableTypes; }

    /*! \brief   Reset the selectable types to be all the face types. */
    void resetSelectableTypes();
    ///@} 

    /*!
    * \brief   Gets the selectable faces.
    */
    std::set<crimson::FaceIdentifier> getSelectableFaces() const;

    /*!
    * \brief   Sets the selection states of all faces. Expects(states.size() == getFaceSelectedStates().size()).
    */
    void setFaceSelectedStates(const std::vector<bool>& states);

    /*!
     * \brief   Gets the selection states of the faces by index.
     */
    const std::vector<bool>& getFaceSelectedStates() const { return _faceIdSelectedStates; }

    /*! \brief   Clear selection. */
    void clearSelection() { std::fill(_faceIdSelectedStates.begin(), _faceIdSelectedStates.end(), false); _updateTransferFunction(); }

    /*!
     * \brief   Select a face by index.
     */
    void selectFace(int id, bool select) { _faceIdSelectedStates[id] = select; _updateTransferFunction(); }

    /*!
     * \brief   Select a face by face identifier.
     */
    void selectFace(const FaceIdentifier& id, bool select);

    /*!
     * \brief   Check if face is selected by index.
     */
    bool isFaceSelected(int id) const { return _faceIdSelectedStates[id]; }

    /*!
     * \brief   Check if face is selected by face identifier.
     */
    bool isFaceSelected(const FaceIdentifier& id) const;

    /*!
     * \brief   Set the callback for when the selection changes.
     */
    void setSelectionChangedObserver(const std::function<void(void)>& observer) { _selectionChangedObserver = observer; }

protected:
    void HandlePickOneEvent(mitk::InteractionEvent* interactionEvent) override;
    void HandlePickAddEvent(mitk::InteractionEvent* interactionEvent) override;
    void HandlePickToggleEvent(mitk::InteractionEvent* interactionEvent) override;

private:
    int GetPickedFaceIdentifier(const mitk::InteractionEvent* interactionEvent);

    void _updateTransferFunction();

    mitk::DataNode* _dataNode = nullptr;

    std::vector<bool> _faceIdSelectedStates;
    std::set<crimson::FaceIdentifier::FaceType> _selectableTypes;
    std::function<void(void)> _selectionChangedObserver;
};

} // namespace crimson
