#pragma once

#include <berryISelectionListener.h>
#include <QmitkAbstractView.h>

#include "ui_vesselSegmentationToolbox.h"

#include <memory>

#include <VesselPathAbstractData.h>

#include "VesselDrivenResliceViewListener.h"


class VesselPathItemModel;
class QShortcut;

/*!
* \brief The view responsible for the creating and editing vessel paths.
*/
class VesselPathPlanningView : public QmitkAbstractView
{
    Q_OBJECT

public:
    static const std::string VIEW_ID;

    VesselPathPlanningView();
    ~VesselPathPlanningView();

    void setVesselDrivenResliceView(VesselDrivenResliceView* view);

    protected slots:
    /// \brief Called when the user clicks the GUI button
    void createNewVesselTree(); 
    void createNewVesselPath(mitk::DataNode* = nullptr);
    void duplicateVesselPath();
    void editVesselPath(bool = false);
    void navigateToControlPoint(int);
    void setAddPointType();
    void setSplineTension(int);
    void startTensionChange();
    void finishTensionChange();
    void selectClosestControlPoints();

    void createMaskedImage();

protected:
    void CreateQtPartControl(QWidget *parent) override;
    void SetFocus() override;

    /// \brief called by QmitkFunctionality when DataManager's selection has changed
    void OnSelectionChanged(berry::IWorkbenchPart::Pointer source, const QList<mitk::DataNode::Pointer>& nodes) override; 
    void NodeAdded(const mitk::DataNode* node) override;
    void NodeChanged(const mitk::DataNode* node) override;
    void NodeRemoved(const mitk::DataNode* node) override;

    // Enable/disable the UI elements according to the current state
    void _updateUI();
    void _updateNodeName(QLineEdit* lineEdit, const mitk::DataNode* node);
    void _selectDataNode(mitk::DataNode* newNode);
    void _createCancelInteractionShortcut();
    void _removeCancelInteractionShortcut();

    Ui::VesselSegmentationToolboxWidget _UI;
    QShortcut* _cancelInteractionShortcut = nullptr;
    QWidget* _viewWidget;

    mitk::DataNode* _currentImageNode = nullptr;
    mitk::DataNode* _currentVesselForestNode = nullptr;
    mitk::DataNode* _currentVesselPathNode = nullptr;

    bool _updatingSelection = false;

    mitk::DataInteractor::Pointer _vesselPathInteractor;

    std::unique_ptr<VesselPathItemModel> _vesselPathItemModel;

    std::map<crimson::VesselPathAbstractData*, mitk::DataNode*> _vesselPathNodes;

    // The part listener detects if the reslice view is available
    VesselDrivenResliceView* _vesselDrivenResliceView = nullptr;
    QScopedPointer<crimson::VesselDrivenResliceViewListener> _partListener;

    //////////////////////////////////////////////////////////////////////////
    // Event handling
    //////////////////////////////////////////////////////////////////////////
    unsigned long _observerTag = 0;
    void _onTensionChanged(itk::Object *caller, const itk::EventObject &event);
    bool _tensionSliderDragStarted = false;
    float _startTension = 0;
};