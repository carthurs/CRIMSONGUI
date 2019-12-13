#pragma once

#include <QSortFilterProxyModel>

#include <NodeDependentView.h>

#include "ui_VesselMeshingView.h"

#include <SolidDataFacePickingObserver.h>

#include <IMeshingKernel.h>

#include <usServiceTracker.h>

#include <AsyncTaskManager.h>
#include <utils/TaskStateObserver.h>

namespace mitk {
    struct InteractionEventObserver;
}

namespace crimson {
    class MeshingParametersData;
}

class MeshInformationDialog;
class FaceListTableModelWithModifiedFlag;

/**
  \brief VesselMeshingView lets the user set up the meshing parameters and run the meshing process.
  */
class VesselMeshingView : public NodeDependentView
{
    Q_OBJECT

public:
    static const std::string VIEW_ID;

    VesselMeshingView();
    ~VesselMeshingView();

    void currentNodeChanged(mitk::DataNode* prevNode) override;
    void NodeChanged(const mitk::DataNode* node) override;
    void NodeRemoved(const mitk::DataNode* node) override;

private slots:
    void createMesh();
    void meshingFinished(crimson::async::Task::State state);
    void setFaceSelectionMode(bool);
    void editGlobalMeshingOptions();
    void editLocalMeshingOptions();
    void fillFaceSettingsTableWidget();
    void syncFaceSelection(const QItemSelection &selected, const QItemSelection &deselected); // Set face selection from table view to face picker
    void syncFaceSelectionToObserver(); // Set face selection from face picker to table view

    void _updateUI();

private:
    void CreateQtPartControl(QWidget *parent) override;
    void SetFocus() override;

    void editMeshingOptions(const std::map<crimson::FaceIdentifier, crimson::IMeshingKernel::LocalMeshingParameters*>& paramsMap, bool);

private:
    // Ui and main widget of this view
    Ui::VesselMeshingWidget _UI;

    crimson::MeshingParametersData* _currentMeshingParameters = nullptr;

	crimson::SolidDataFacePickingObserver _facePickingObserver;
    us::ServiceRegistration<mitk::InteractionEventObserver> _facePickingObserverServiceRegistration;

    us::ServiceTracker<mitk::InteractionEventObserver> _pickingEventObserverServiceTracker;

    FaceListTableModelWithModifiedFlag* _faceListModel;
    QSortFilterProxyModel _faceListProxyModel;

    crimson::TaskStateObserver* _meshingTaskStateObserver;
};
