#pragma once

#include "ui_MaterialVisualizationWidget.h"

#include <mitkDataNode.h>
#include <vtkDataObject.h>

#include <array>
#include <gsl.h>

namespace mitk
{
class DataNode;
class TransferFunction;
}

/*! \brief   A widget for editing transfer function. */
class MaterialVisualizationWidget : public QWidget
{
    Q_OBJECT
public:
    MaterialVisualizationWidget(QWidget* parent = nullptr, Qt::WindowFlags f = 0);
    ~MaterialVisualizationWidget();

    /*!
     * \brief   Sets the data node whose transfer function is to be edited.
     *
     * \param   node                The data node.
     * \param   dataSet             The vtkDataSet stored in the data node (the access may differ for mitk::Surface and mitk::UnstructuredGrid).
     * \param   tfPropertyName      Name of the transfer function property used by the renderer.
     * \param   attributeType       Type of the attribute to be shown (vtkDataObject::POINT or vtkDataObject::CELL).
     * \param   propertyStorageNode If non-null, the node where to store various transfer functions to be re-used later. If null, the 'node' will be used.
     */
    void setDataNode(mitk::DataNode* node, vtkDataSet* dataSet, gsl::cstring_span<> tfPropertyName,
                     vtkDataObject::AttributeTypes attributeType = vtkDataObject::POINT,
                     mitk::DataNode* propertyStorageNode = nullptr);
    /*! \brief   Resets the data node to null. */
    void resetDataNode() { setDataNode(nullptr, nullptr, ""); }

    /*! \brief   Gets data node. */
    mitk::DataNode* getDataNode() const { return _node; }

private slots:
    void _setCurrentArray(const QString& arrayName);
    void _setCurrentComponent(int comboBoxIndex);
    void _resetTF();
    void _setNanColor(QColor c);

    void _setColorSpace(int colorSpace);
    void _loadTF();
    void _saveTF();

private:
    void _dataNodeDeleted();
    std::string _getCurrentTFPropName();
    void _setupDataComboBoxes();
    void _setTransferFunction(bool reset = false);
    static void _setupTFProperties(mitk::TransferFunction* tf, int componentIndex);
    static void _generateTF(mitk::TransferFunction* tf, const std::array<double, 2>& range);
    static void _rescaleTF(mitk::TransferFunction* tf, const std::array<double, 2>& range);
    QColor _getNanColor();
    void _addNodeObserver();
    void _removeNodeObserver();

    mitk::DataNode* _node = nullptr;
    vtkDataSet* _dataSet = nullptr;
    std::string _tfPropertyName;
    vtkDataObject::AttributeTypes _attributeType = vtkDataObject::POINT;
    mitk::DataNode* _propertyStorageNode = nullptr;
    Ui::MaterialVisualizationWidget _UI;
    unsigned long _nodeObserverTag = -1;
};
