#include "SolidDataFacePickingObserver.h"
#include "SolidData.h"


#include <mitkInteractionPositionEvent.h>
#include <mitkBaseRenderer.h>
#include <mitkRenderingManager.h>
#include <mitkVtkPropRenderer.h>
#include <mitkVtkScalarModeProperty.h>
#include <vtkAbstractMapper.h>

#include <vtkCellPicker.h>
#include <vtkCellData.h>

#include <gsl.h>

namespace crimson {

SolidDataFacePickingObserver::SolidDataFacePickingObserver()
{
    resetSelectableTypes();
}

void SolidDataFacePickingObserver::resetSelectableTypes()
{
    _selectableTypes.clear();
    _selectableTypes.insert(crimson::FaceIdentifier::ftCapInflow);
    _selectableTypes.insert(crimson::FaceIdentifier::ftCapOutflow);
    _selectableTypes.insert(crimson::FaceIdentifier::ftWall);
    _updateTransferFunction();
}

SolidDataFacePickingObserver::~SolidDataFacePickingObserver()
{
    SetDataNode(nullptr);
}

void SolidDataFacePickingObserver::SetDataNode(mitk::DataNode* node)
{
    _faceIdSelectedStates.clear();

    if (_dataNode) {
        _dataNode->SetProperty("Surface.TransferFunction", mitk::TransferFunctionProperty::New().GetPointer());
        _dataNode->SetProperty("scalar mode", mitk::VtkScalarModeProperty::New(VTK_SCALAR_MODE_DEFAULT));

        _dataNode->SetBoolProperty("scalar visibility", false);
    }

    _dataNode = node;

    if (_dataNode) {
        auto solidData = static_cast<SolidData*>(_dataNode->GetData());
        _faceIdSelectedStates.resize(solidData->getFaceIdentifierMap().getNumberOfFaceIdentifiers(), false);

        _dataNode->SetProperty("Surface.TransferFunction", mitk::TransferFunctionProperty::New(mitk::TransferFunction::New()).GetPointer());
        _dataNode->SetProperty("scalar mode", mitk::VtkScalarModeProperty::New(VTK_SCALAR_MODE_USE_CELL_DATA));

        _dataNode->SetBoolProperty("scalar visibility", true);

        _updateTransferFunction();
    }
    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void SolidDataFacePickingObserver::HandlePickOneEvent(mitk::InteractionEvent* interactionEvent)
{
    if (!_dataNode) {
        return;
    }

    int pickedFaceId = GetPickedFaceIdentifier(interactionEvent);
    std::fill(_faceIdSelectedStates.begin(), _faceIdSelectedStates.end(), false);
    if (pickedFaceId != -1) {
        _faceIdSelectedStates[pickedFaceId] = true;
    }

    _updateTransferFunction();
    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void SolidDataFacePickingObserver::HandlePickToggleEvent(mitk::InteractionEvent* interactionEvent)
{
    if (!_dataNode) {
        return;
    }

    int pickedFaceId = GetPickedFaceIdentifier(interactionEvent);
    if (pickedFaceId != -1) {
        _faceIdSelectedStates[pickedFaceId] = !_faceIdSelectedStates[pickedFaceId];
        _updateTransferFunction();
    }
}

void SolidDataFacePickingObserver::HandlePickAddEvent(mitk::InteractionEvent* interactionEvent)
{
    if (!_dataNode) {
        return;
    }

    int pickedFaceId = GetPickedFaceIdentifier(interactionEvent);
    if (pickedFaceId != -1) {
        _faceIdSelectedStates[pickedFaceId] = true;
        _updateTransferFunction();
    }
}

int SolidDataFacePickingObserver::GetPickedFaceIdentifier(const mitk::InteractionEvent* interactionEvent)
{
    auto positionEvent = dynamic_cast<const mitk::InteractionPositionEvent*>(interactionEvent);
    if (positionEvent == nullptr)
        return -1;

    if (interactionEvent->GetSender()->GetMapperID() != mitk::BaseRenderer::Standard3D) {
        return -1;
    }

    auto propRenderer = static_cast<mitk::VtkPropRenderer*>(interactionEvent->GetSender());
    propRenderer->SetPickingMode(mitk::VtkPropRenderer::CellPicking);
    mitk::Point3D p3d;

    if (propRenderer->PickObject(positionEvent->GetPointerPositionOnScreen(), p3d) != _dataNode) {
        return -1;
    }
    else {
        vtkDataArray* faceIdArray = const_cast<vtkCellPicker*>(propRenderer->GetCellPicker())->GetDataSet()->GetCellData()->GetArray("Face IDs");
        if (!faceIdArray) {
            return -1;
        }

        auto solidData = static_cast<SolidData*>(_dataNode->GetData());

        int faceId = faceIdArray->GetVariantValue(const_cast<vtkCellPicker*>(propRenderer->GetCellPicker())->GetCellId()).ToShort();
        if (_selectableTypes.find(solidData->getFaceIdentifierMap().getFaceIdentifier(faceId).faceType) == _selectableTypes.end()) {
            return -1;
        }

        return faceId;
    }
}

std::set<crimson::FaceIdentifier> SolidDataFacePickingObserver::getSelectableFaces() const
{
    auto&& faceIdMap = static_cast<SolidData*>(_dataNode->GetData())->getFaceIdentifierMap();
    std::set<crimson::FaceIdentifier> faces;

    for (int i = 0; i < faceIdMap.getNumberOfFaceIdentifiers(); ++i) {
        auto faceId = faceIdMap.getFaceIdentifier(i);
        if (_selectableTypes.count(faceId.faceType) > 0) {
            faces.insert(faceId);
        }
    }

    return faces;
}

void SolidDataFacePickingObserver::setFaceSelectedStates(const std::vector<bool>& states)
{
    Expects(states.size() == _faceIdSelectedStates.size());
    _faceIdSelectedStates = states;
    _updateTransferFunction();
}

void SolidDataFacePickingObserver::selectFace(const FaceIdentifier& id, bool select)
{
    auto solidData = static_cast<SolidData*>(_dataNode->GetData());
    auto index = solidData->getFaceIdentifierMap().faceIdentifierIndex(id);
    if (index == -1) {
        return;
    }
    _faceIdSelectedStates[index] = select;
    _updateTransferFunction();
}

bool SolidDataFacePickingObserver::isFaceSelected(const FaceIdentifier& id) const
{
    auto solidData = static_cast<SolidData*>(_dataNode->GetData());
    auto index = solidData->getFaceIdentifierMap().faceIdentifierIndex(id);
    if (index == -1) {
        return false;
    }
    return _faceIdSelectedStates[index];
}

void SolidDataFacePickingObserver::_updateTransferFunction()
{
    if (!_dataNode) {
        return;
    }

    if (_selectionChangedObserver) {
        _selectionChangedObserver();
    }

    auto solidData = static_cast<SolidData*>(_dataNode->GetData());

    // Build selection state lut
    float normalColor[3];
    _dataNode->GetColor(normalColor);

    float selectedColor[3];
    _dataNode->GetColor(selectedColor, nullptr, "color.selected");

    float selectableColor[3];
    _dataNode->GetColor(selectableColor, nullptr, "color.selectable");

    float opacity;
    _dataNode->GetOpacity(opacity, nullptr);

    auto tf = static_cast<mitk::TransferFunctionProperty*>(_dataNode->GetProperty("Surface.TransferFunction"))->GetValue();

    tf->ClearScalarOpacityPoints();
    tf->AddScalarOpacityPoint(0, opacity);

    tf->ClearRGBPoints();
    for (size_t i = 0; i < _faceIdSelectedStates.size(); ++i) {
        float* color;
        if (_faceIdSelectedStates[i]) {
            color = selectedColor;
        }
        else {
            if (_selectableTypes.find(solidData->getFaceIdentifierMap().getFaceIdentifier(i).faceType) == _selectableTypes.end()) {
                color = normalColor;
            }
            else {
                color = selectableColor;
            }
        }
        tf->AddRGBPoint(i, color[0], color[1], color[2]);
    }
    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

} // namespace crimson
