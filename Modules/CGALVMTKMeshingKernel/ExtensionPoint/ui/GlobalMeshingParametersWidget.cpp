#include <ui/GlobalMeshingParametersWidget.h>

#include <IMeshingKernel.h>


namespace crimson
{
GlobalMeshingParametersWidget::GlobalMeshingParametersWidget(IMeshingKernel::GlobalMeshingParameters& params, QWidget* parent)
    : QWidget(parent)
    , _params(params)
{
    _UI.setupUi(this);
    _UI.surfaceOptimizationLevelSlider->setDecimals(0);
    _UI.volumeOptimizationLevelSlider->setDecimals(0);
    syncUIValuesToMeshingParameters();

    connect(_UI.surfaceOptimizationLevelSlider, &ctkSliderWidget::valueChanged, this,
        &GlobalMeshingParametersWidget::syncMeshingParametersToUIValues);
    connect(_UI.surfaceMeshingOnlyCheckBox, &QAbstractButton::toggled, this,
        &GlobalMeshingParametersWidget::syncMeshingParametersToUIValues);
    connect(_UI.maxRadiusEdgeRatioSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this,
        &GlobalMeshingParametersWidget::syncMeshingParametersToUIValues);
    connect(_UI.minDihedralAngleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this,
        &GlobalMeshingParametersWidget::syncMeshingParametersToUIValues);
    connect(_UI.maxDihedralAngleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this,
        &GlobalMeshingParametersWidget::syncMeshingParametersToUIValues);
    connect(_UI.volumeOptimizationLevelSlider, &ctkSliderWidget::valueChanged, this,
        &GlobalMeshingParametersWidget::syncMeshingParametersToUIValues);

}

GlobalMeshingParametersWidget::~GlobalMeshingParametersWidget() {
    }

void GlobalMeshingParametersWidget::syncUIValuesToMeshingParameters() {
    _UI.surfaceMeshingOnlyCheckBox->setChecked(_params.meshSurfaceOnly);
    _UI.surfaceOptimizationLevelSlider->setValue(_params.surfaceOptimizationLevel);
    _UI.maxRadiusEdgeRatioSpinBox->setValue(_params.maxRadiusEdgeRatio);
    _UI.minDihedralAngleSpinBox->setValue(_params.minDihedralAngle);
    _UI.maxDihedralAngleSpinBox->setValue(_params.maxDihedralAngle);
    _UI.volumeOptimizationLevelSlider->setValue(_params.volumeOptimizationLevel);
}

void GlobalMeshingParametersWidget::syncMeshingParametersToUIValues() {
    _params.meshSurfaceOnly = _UI.surfaceMeshingOnlyCheckBox->isChecked();
    _params.surfaceOptimizationLevel = _UI.surfaceOptimizationLevelSlider->value();
    _params.maxRadiusEdgeRatio = _UI.maxRadiusEdgeRatioSpinBox->value();
    _params.minDihedralAngle = _UI.minDihedralAngleSpinBox->value();
    _params.maxDihedralAngle = _UI.maxDihedralAngleSpinBox->value();
    _params.volumeOptimizationLevel = _UI.volumeOptimizationLevelSlider->value();
}

}
