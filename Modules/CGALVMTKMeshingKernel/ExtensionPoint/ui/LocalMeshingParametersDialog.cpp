#include "LocalMeshingParametersDialog.h"
#include "LocalParameterEditor.h"

#include <ctkDoubleSpinBox.h>

template<typename T>
OverrideStatus getInitialVariableStatus(std::vector<crimson::IMeshingKernel::LocalMeshingParameters*> params,
    boost::optional<T> crimson::IMeshingKernel::LocalMeshingParameters::* member)
{
    OverrideStatus status;

    // Detect the status
    boost::optional<T>& value = params[0]->*member;
    status = value.is_initialized() ? StatusOverriden : StatusNotOverriden;
    for (size_t i = 1; i < params.size(); ++i) {
        if (value.is_initialized() != (params[i]->*member).is_initialized()) {
            status = StatusConflicting;
            break;
        }
        if (value.is_initialized() && value.get() != (params[i]->*member).get()) {
            status = StatusConflicting;
            break;
        }
    }
    return status;
}

template<typename T>
void setVariableAccordingToStatus(std::vector<crimson::IMeshingKernel::LocalMeshingParameters*> params,
    boost::optional<T> crimson::IMeshingKernel::LocalMeshingParameters::* member, OverrideStatus status, T value)
{
    if (status == StatusConflicting) {
        return;
    }

    for (size_t i = 0; i < params.size(); ++i) {
        if (status == StatusNotOverriden) {
            (params[i]->*member).reset();
        }
        else {
            params[i]->*member = value;
        }
    }
}

LocalMeshingParametersDialog::LocalMeshingParametersDialog(const std::vector<crimson::IMeshingKernel::LocalMeshingParameters*>& params, bool editingGlobal, crimson::IMeshingKernel::LocalMeshingParameters& defaultLocalParameters, QWidget* parent /*= nullptr*/) 
    : QDialog(parent)
    , _params(params)
    , _defaultLocalParameters(defaultLocalParameters)
    , _editingGlobal(editingGlobal)
{
    _ui.setupUi(this);

    OverrideStatus s = getInitialVariableStatus<double>(params, &crimson::IMeshingKernel::LocalMeshingParameters::size);
    meshSizeStatusHandler = new OverrideStatusButtonHandler(s, _ui.useDefaultMeshSize, editingGlobal, this);
    auto meshSizeDefaultValuedEditor = new SpinBoxDefaultValuedEditor(_ui.meshSizeSpinBox, defaultLocalParameters.size.get(), this);
    new LocalParameterValueEditor(meshSizeStatusHandler, meshSizeDefaultValuedEditor, this);
    if (s == StatusOverriden) {
        _ui.meshSizeSpinBox->setValue(params[0]->size.get());
    }

    s = getInitialVariableStatus<bool>(params, &crimson::IMeshingKernel::LocalMeshingParameters::sizeRelative);
    sizeRelativeStatusHandler = new OverrideStatusButtonHandler(s, _ui.useDefaultMeshSizeType, editingGlobal, this);
    auto sizeRelativeDefaultValuedEditor = new AutoExclusiveButtonDefaultValuedEditor(std::vector<QAbstractButton*>({ _ui.relativeRadioButton, _ui.absoluteRadioButton }),
        defaultLocalParameters.sizeRelative.get() ? _ui.relativeRadioButton : _ui.absoluteRadioButton, this);
    new LocalParameterValueEditor(sizeRelativeStatusHandler, sizeRelativeDefaultValuedEditor, this);
    if (s == StatusOverriden) {
        params[0]->sizeRelative.get() ? _ui.relativeRadioButton->setChecked(true) : _ui.absoluteRadioButton->setChecked(true);
    }

    s = getInitialVariableStatus<double>(params, &crimson::IMeshingKernel::LocalMeshingParameters::thickness);
    boundaryLayerThicknessStatusHandler = new OverrideStatusButtonHandler(s, _ui.useDefaultThickness, editingGlobal, this);
    auto boundaryLayerThicknessDefaultValuedEditor = new SpinBoxDefaultValuedEditor(_ui.totalThicknessSpinBox, defaultLocalParameters.thickness.get(), this);
    new LocalParameterValueEditor(boundaryLayerThicknessStatusHandler, boundaryLayerThicknessDefaultValuedEditor, this);
    if (s == StatusOverriden) {
        _ui.totalThicknessSpinBox->setValue(params[0]->thickness.get());
    }

    connect(_ui.useBoundaryLayersCheckBox, &QCheckBox::stateChanged, [&]() { _ui.boundaryLayersGroupBox->setEnabled(_ui.useBoundaryLayersCheckBox->isChecked()); });
    _ui.useBoundaryLayersCheckBox->setChecked(_defaultLocalParameters.useBoundaryLayers);
    _ui.useBoundaryLayersCheckBox->setEnabled(editingGlobal);

    _ui.numberOfSubLayersSpinBox->setValue(_defaultLocalParameters.numSubLayers);
    _ui.numberOfSubLayersSpinBox->setEnabled(editingGlobal);
    _ui.subLayerRatioSpinBox->setValue(_defaultLocalParameters.subLayerRatio);
    _ui.subLayerRatioSpinBox->setEnabled(editingGlobal);
}

void LocalMeshingParametersDialog::accept()
{
    // Read back the values
    setVariableAccordingToStatus<double>(_params, &crimson::IMeshingKernel::LocalMeshingParameters::size,
        meshSizeStatusHandler->status(), _ui.meshSizeSpinBox->value());


    setVariableAccordingToStatus<bool>(_params, &crimson::IMeshingKernel::LocalMeshingParameters::sizeRelative,
        sizeRelativeStatusHandler->status(), _ui.relativeRadioButton->isChecked());

    if (_editingGlobal) {
        _params[0]->useBoundaryLayers = _ui.useBoundaryLayersCheckBox->isChecked();
        _params[0]->numSubLayers = _ui.numberOfSubLayersSpinBox->value();
        _params[0]->subLayerRatio = _ui.subLayerRatioSpinBox->value();
    }

    setVariableAccordingToStatus<double>(_params, &crimson::IMeshingKernel::LocalMeshingParameters::thickness,
        boundaryLayerThicknessStatusHandler->status(), _ui.totalThicknessSpinBox->value());

    QDialog::accept();
}
