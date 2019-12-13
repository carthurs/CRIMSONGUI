#include "MaterialVisualizationWidget.h"

#include <mitkTransferFunction.h>
#include <mitkTransferFunctionProperty.h>
#include <mitkTransferFunctionPropertySerializer.h>
#include <mitkColorProperty.h>

#include <vtkDataSetAttributes.h>
#include <vtkDataArray.h>

#include <gsl.h>
#include <array>

#include <QFileDialog>
#include <QMessageBox>

MaterialVisualizationWidget::MaterialVisualizationWidget(QWidget* parent, Qt::WindowFlags f)
    : QWidget(parent, f)
{
    _UI.setupUi(this);
    resetDataNode();

    _UI.colorTransferFunctionCanvas->SetQLineEdits(_UI.valueLineEdit, nullptr);

    connect(_UI.dataArrayNameComboBox, SIGNAL(currentIndexChanged(const QString&)), this,
            SLOT(_setCurrentArray(const QString&)));
    connect(_UI.componentNameComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(_setCurrentComponent(int)));
    connect(_UI.resetTFButton, &QAbstractButton::clicked, this, &MaterialVisualizationWidget::_resetTF);
    connect(_UI.nanColorButton, &ctkColorPickerButton::colorChanged, this, &MaterialVisualizationWidget::_setNanColor);
    connect(_UI.colorSpaceComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(_setColorSpace(int)));
    connect(_UI.saveTFButton, &QAbstractButton::clicked, this, &MaterialVisualizationWidget::_saveTF);
    connect(_UI.loadTFButton, &QAbstractButton::clicked, this, &MaterialVisualizationWidget::_loadTF);
}

MaterialVisualizationWidget::~MaterialVisualizationWidget() { _removeNodeObserver(); }

void MaterialVisualizationWidget::setDataNode(mitk::DataNode* node, vtkDataSet* dataSet, gsl::cstring_span<> tfPropertyName,
                                              vtkDataObject::AttributeTypes attributeType, mitk::DataNode* propertyStorageNode)
{
    if (_node) {
        _removeNodeObserver();
        _node->SetBoolProperty("scalar visibility", false);
        _node->SetFloatProperty("material.specularCoefficient", 1);
    }

    if (dataSet && dataSet->GetAttributes(attributeType)->GetNumberOfArrays() == 0) {
        MITK_ERROR << "No data arrays found to visualize";
        node = propertyStorageNode = nullptr;
        dataSet = nullptr;
    }

    _UI.dataArrayNameComboBox->blockSignals(true);
    _UI.dataArrayNameComboBox->clear();
    _UI.dataArrayNameComboBox->blockSignals(false);

    _UI.componentNameComboBox->blockSignals(true);
    _UI.componentNameComboBox->clear();
    _UI.componentNameComboBox->blockSignals(false);

    _node = node;
    _propertyStorageNode = propertyStorageNode ? propertyStorageNode : node;
    _attributeType = attributeType;
    _tfPropertyName = gsl::to_string(tfPropertyName);
    _dataSet = dataSet;

    if (_node != nullptr && _dataSet != nullptr) {
        _addNodeObserver();

        _node->SetBoolProperty("scalar visibility", true);
        _node->SetFloatProperty("material.specularCoefficient", 0);
        _setupDataComboBoxes();
    }

    setEnabled(_node != nullptr);
    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void MaterialVisualizationWidget::_setupDataComboBoxes()
{
    Expects(_node != nullptr);
    auto attributesData = _dataSet->GetAttributes(_attributeType);

    // Add all arrays available in the data set to the array selection combo box
    auto arrayNames = QStringList{};
    for (int i = 0; i < attributesData->GetNumberOfArrays(); ++i) {
        arrayNames.push_back(attributesData->GetArrayName(i));
    }
    _UI.dataArrayNameComboBox->addItems(arrayNames);
}

void MaterialVisualizationWidget::_setCurrentArray(const QString& arrayName)
{
    if (_node == nullptr) {
        return;
    }

    auto attributesData = _dataSet->GetAttributes(_attributeType);
    auto arrayNameStr = arrayName.toStdString();
    auto dataArray = attributesData->GetArray(arrayNameStr.c_str());

    if (dataArray->GetNumberOfComponents() > 1) {
        // If there are more than one components, allow visualizing magintude and individual components
        _UI.componentNameComboBox->setEnabled(true);
        _UI.componentNameComboBox->addItem("Magnitude", QVariant::fromValue(-1));
        for (int i = 0; i < dataArray->GetNumberOfComponents(); ++i) {
            auto componentName = QString::fromLatin1(dataArray->GetComponentName(i));
            if (componentName.isEmpty()) {
                componentName = QString::number(i);
            }
            _UI.componentNameComboBox->addItem(componentName, QVariant::fromValue(i));
        }
    } else {
        _UI.componentNameComboBox->setEnabled(false);
        _UI.componentNameComboBox->clear();
    }

    attributesData->SetActiveScalars(arrayNameStr.c_str());
    _setTransferFunction();
}

void MaterialVisualizationWidget::_setCurrentComponent(int comboBoxIndex)
{
    if (_node == nullptr) {
        return;
    }

    _setTransferFunction();
}

void MaterialVisualizationWidget::_resetTF() { _setTransferFunction(true); }

void MaterialVisualizationWidget::_setNanColor(QColor c)
{
    Expects(_node != nullptr && _propertyStorageNode != nullptr);

    auto tfProp = _node->GetProperty(_tfPropertyName.c_str());

    static_cast<mitk::TransferFunctionProperty*>(tfProp)->GetValue()->GetColorTransferFunction()->SetNanColor(
        c.redF(), c.greenF(), c.blueF());

    _propertyStorageNode->SetProperty("NanColor", mitk::ColorProperty::New(c.redF(), c.greenF(), c.blueF()));
}

QColor MaterialVisualizationWidget::_getNanColor()
{
    Expects(_propertyStorageNode != nullptr);

    auto nanColor = mitk::Color{};
    auto nanColorProp = _propertyStorageNode->GetProperty("NanColor");
    if (nanColorProp) {
        nanColor = static_cast<mitk::ColorProperty*>(nanColorProp)->GetValue();
    }
    return QColor::fromRgbF(nanColor.GetRed(), nanColor.GetGreen(), nanColor.GetBlue());
}

void MaterialVisualizationWidget::_dataNodeDeleted() { resetDataNode(); }

std::string MaterialVisualizationWidget::_getCurrentTFPropName()
{
    auto componentIndex = _UI.componentNameComboBox->isEnabled() ? _UI.componentNameComboBox->currentData().toInt() : -1;
    auto scalars = _dataSet->GetAttributes(_attributeType)->GetScalars();

    return std::string{"tf_"} + scalars->GetName() + std::to_string(componentIndex);
}

void MaterialVisualizationWidget::_setTransferFunction(bool reset)
{
    Expects(_node != nullptr);

    auto componentIndex = _UI.componentNameComboBox->isEnabled() ? _UI.componentNameComboBox->currentData().toInt() : -1;

    auto scalars = _dataSet->GetAttributes(_attributeType)->GetScalars();

    auto tfPropName = _getCurrentTFPropName();
    auto tfProp = _propertyStorageNode->GetProperty(tfPropName.c_str());

    auto range = std::array<double, 2>();
    scalars->GetRange(range.data(), componentIndex);

    mitk::TransferFunction::Pointer tf{};

    if (tfProp && !reset) {
        // Restore tf and rescale if necessary
        tf = static_cast<mitk::TransferFunctionProperty*>(tfProp)->GetValue();
        _rescaleTF(tf.GetPointer(), range);
    } else {
        // Create new tf
        tf = mitk::TransferFunction::New();
        _generateTF(tf.GetPointer(), range);

        _propertyStorageNode->SetProperty(tfPropName.c_str(), mitk::TransferFunctionProperty::New(tf));
    }
    _setupTFProperties(tf, componentIndex);

    _UI.colorSpaceComboBox->blockSignals(true);
    _UI.colorSpaceComboBox->setCurrentIndex(tf->GetColorTransferFunction()->GetColorSpace());
    _UI.colorSpaceComboBox->blockSignals(false);

    _UI.colorTransferFunctionCanvas->SetColorTransferFunction(tf->GetColorTransferFunction());
    _node->ReplaceProperty(_tfPropertyName.c_str(), mitk::TransferFunctionProperty::New(tf)); // Force replace
    _node->SetDoubleProperty("ScalarsRangeMinimum", range[0]);
    _node->SetDoubleProperty("ScalarsRangeMaximum", range[1]);

    _setNanColor(_getNanColor());

    _dataSet->Modified(); // This is only done to propagate the active attribute to the slice filters of unstructured grid mapper...
    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void MaterialVisualizationWidget::_rescaleTF(mitk::TransferFunction* tf, const std::array<double, 2>& range)
{
    std::array<double, 2> oldRange{};
    tf->GetColorTransferFunction()->GetRange(oldRange.data());

    // Check if the old tf only has one value
    if (tf->GetRGBPoints().size() <= 1 || (oldRange[1] - oldRange[0]) < 1e-9) {
        // If yes, simply create a default TF
        _generateTF(tf, range);
        return;
    }

    auto rgbPoints = tf->GetRGBPoints();
    for (auto& rgbPoint : rgbPoints) {
        rgbPoint.first = range[0] + (rgbPoint.first - oldRange[0]) / (oldRange[1] - oldRange[0]) * (range[1] - range[0]);
    }
    tf->SetRGBPoints(rgbPoints);
}

void MaterialVisualizationWidget::_generateTF(mitk::TransferFunction* tf, const std::array<double, 2>& range)
{
    // Generate a cold-to-hot diverging-scale transfer function
    tf->GetColorTransferFunction()->RemoveAllPoints();
    tf->AddRGBPoint(range[0], 59 / 255.0, 76 / 255.0, 192 / 255.0);
    tf->AddRGBPoint(range[1], 180 / 255.0, 4 / 255.0, 38 / 255.0);
    tf->GetColorTransferFunction()->SetColorSpaceToDiverging();
}

void MaterialVisualizationWidget::_setupTFProperties(mitk::TransferFunction* tf, int componentIndex)
{
    auto colorTF = tf->GetColorTransferFunction();
    colorTF->SetNanColor(0, 0, 0);
    if (componentIndex == -1) {
        colorTF->SetVectorModeToMagnitude();
    } else {
        colorTF->SetVectorModeToComponent();
        colorTF->SetVectorComponent(componentIndex);
    }
}

void MaterialVisualizationWidget::_setColorSpace(int colorSpace)
{
    _UI.colorTransferFunctionCanvas->GetColorTransferFunction()->SetColorSpace(colorSpace);
}

void MaterialVisualizationWidget::_loadTF()
{
    auto fileName = QFileDialog::getOpenFileName(this, "Load transfer function", QString{},
                                                 tr("Transfer functions (*.tf);; All files (*.*)"));
    if (fileName.isEmpty()) {
        return;
    }

    auto tf = mitk::TransferFunctionPropertySerializer::DeserializeTransferFunction(fileName.toStdString().c_str());

    if (tf.IsNull()) {
        QMessageBox::critical(this, "Error loading transfer function",
                              "Failed to load the transfer function. Please see log for details.");
        return;
    }

    _propertyStorageNode->SetProperty(_getCurrentTFPropName().c_str(), mitk::TransferFunctionProperty::New(tf));
    _setTransferFunction();
}

void MaterialVisualizationWidget::_saveTF()
{
    auto tfProp =
        dynamic_cast<mitk::TransferFunctionProperty*>(_propertyStorageNode->GetProperty(_getCurrentTFPropName().c_str()));

    if (!tfProp) {
        return;
    }

    auto fileName = QFileDialog::getSaveFileName(this, "Save transfer function", QString{},
                                                 tr("Transfer functions (*.tf);; All files (*.*)"));
    if (fileName.isEmpty()) {
        return;
    }

    mitk::TransferFunctionPropertySerializer::SerializeTransferFunction(fileName.toStdString().c_str(), tfProp->GetValue());
}

void MaterialVisualizationWidget::_addNodeObserver()
{
    auto deleteCommand = itk::SimpleMemberCommand<MaterialVisualizationWidget>::New();
    deleteCommand->SetCallbackFunction(this, &MaterialVisualizationWidget::_dataNodeDeleted);
    _nodeObserverTag = _node->AddObserver(itk::DeleteEvent(), deleteCommand);
}

void MaterialVisualizationWidget::_removeNodeObserver()
{
    if (_node) {
        _node->RemoveObserver(_nodeObserverTag);
        _nodeObserverTag = -1;
    }
}
