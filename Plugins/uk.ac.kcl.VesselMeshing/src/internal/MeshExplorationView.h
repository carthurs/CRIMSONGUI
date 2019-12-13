#pragma once

#include <NodeDependentView.h>

#include "ui_MeshExplorationView.h"

#include <mitkClippingProperty.h>

/*!
 * \brief MeshExplorationView is used to let the user look 'inside' the simulation mesh by using the cutting planes.
 */
class MeshExplorationView : public NodeDependentView
{
    Q_OBJECT

public:
    static const std::string VIEW_ID;

    MeshExplorationView();
    ~MeshExplorationView();

    void currentNodeChanged(mitk::DataNode* prevNode) override;
    void NodeAdded(const mitk::DataNode* node) override;
    void NodeChanged(const mitk::DataNode* node) override;
    void NodeRemoved(const mitk::DataNode* node) override;

private:
    using ClippingPlaneInfoTuple = std::tuple<mitk::ClippingProperty::Pointer, bool, bool, unsigned long>;
    using PlaneWidgetToClippingPropertyMap = std::map<const mitk::DataNode*, ClippingPlaneInfoTuple>;

    void _fillClippingPlanesTable();
    void _addClippinigPlaneTableRow(const std::string& clippingPlaneName, const PlaneWidgetToClippingPropertyMap::value_type &planeInfoPair, int rowIndex);

    void _addClippingProperty(const mitk::DataNode* clippingPlaneNode);
    void _clippingPlaneModified(itk::Object *o, const itk::EventObject &e) { _clippingPlaneModifiedC(o, e); }
    void _clippingPlaneModifiedC(const itk::Object *, const itk::EventObject &);
    void _findClippingPlaneNodes();
    void _setupClippingPlanePropertiesForCurrentNode();
    void _removeClippingPlanePropertiesFromNode(mitk::DataNode* dataNode);

    void CreateQtPartControl(QWidget *parent) override;
    void SetFocus() override;

private slots:
    void _setSliceType(int type);
    void _applyPlaneProperties();

private:
    // Ui and main widget of this view
    Ui::MeshExplorationWidget _UI;

    /*! \brief Indices into ClippingPlaneInfoTuple */
    enum {
        ClippingPropertyIndex = 0,
        ClippingEnabledIndex,
        ClippingInvertedIndex,
        ClippingObserverTag
    };

    PlaneWidgetToClippingPropertyMap _planeWidgetToClippingPropertyMap;
};
