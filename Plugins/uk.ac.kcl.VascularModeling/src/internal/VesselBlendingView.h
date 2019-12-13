#pragma once

#include <unordered_map>
#include <unordered_set>

#include <NodeDependentView.h>
#include <VesselForestData.h>

#include <AsyncTaskManager.h>
#include <AsyncTask.h>

#include "ui_VesselBlendingView.h"

namespace crimson{
class TaskStateObserver;
}

class VesselPathAbstractData;

/*!
 * \brief The view responsible for the setting up the vessel blending.
 */
class VesselBlendingView : public NodeDependentView
{
    Q_OBJECT

public:
    static const std::string VIEW_ID;

    VesselBlendingView();
    ~VesselBlendingView();

protected slots:
    /// \brief Called when the user clicks the GUI button
    void createBlend();
    void previewBlend();
    void redoPreviewBlend();
    void startDetectIntersections();
    void syncVesselSelection(const QItemSelection&, const QItemSelection&);
    void highlightRowsParticipatingInPreview();
    void onTableItemChanged(QTableWidgetItem*);
    void showAllShapeNodes();
    void hideAllShapeNodes();
    void showBlendResult(bool);
    void showSelectedShapeNodes();
    void hideSelectedShapeNodes();
    void showOnlySelectedShapeNodes();
    void blendingFinished(crimson::async::Task::State state);
    void previewFinished(crimson::async::Task::State state);
    void detectIntersectionsFinished(crimson::async::Task::State state);
    void chooseVesselsForBlending();

    void setCustomBooleansEnabled(bool enabled);
    void moveBooleanOperationsUp();
    void moveBooleanOperationsDown();
    void swapBooleanOperationArguments();

protected:
    void CreateQtPartControl(QWidget *parent) override;
    void SetFocus() override;

    void OnSelectionChanged(berry::IWorkbenchPart::Pointer part, const QList<mitk::DataNode::Pointer> &nodes) override;

    void _updateTableWidgetSelection(const QList<mitk::DataNode::Pointer> &nodes, QTableWidget* tableWidget);

    void currentNodeChanged(mitk::DataNode* prevNode) override;
    void NodeRemoved(const mitk::DataNode* node) override;
    void NodeChanged(const mitk::DataNode* node) override;

private:
    enum FilletSizeTableColumn {
        Vessel1NameColumn = 0,
        Vessel2NameColumn,
        FilletSizeColumn,
        BOPTypeColumn
    };

    enum {
        VesselPathNodeRole = Qt::UserRole + 1
    };

private:
    void _updateUI();
    void _fillFilletSizeTable();
    bool _fillBooleanOperationRow(const crimson::VesselForestData::BooleanOperationInfo& info, int row);
    void _addFlowFaceFilletRow(const crimson::VesselForestData::VesselPathUIDPair& vessels, double filletSize);
    void _swapBooleanOperations(int from, int to);
    void _hideBlendPreview();
    void _setPreviewOriginalShapesNodeVisibility(bool visible);
    void _setAllOriginalShapeNodesVisibility(bool visible);

    mitk::DataNode* _getFilletSizeTableVesselNode(QTableWidget* tableWidget, int row, FilletSizeTableColumn col);
    QTableWidgetItem* _findFilletSizeRow(const crimson::VesselForestData::VesselPathUIDPair& vessels);
    void _addSolidModelToSelection(mitk::DataNode* vesselNode, QList<mitk::DataNode::Pointer>& selection);
    std::unordered_map<mitk::DataNode*, bool> _getVesselNodesSelectionStatus(QTableWidget*);

    crimson::AsyncTaskManager::TaskUID getTaskUID(const char* prefix);
    crimson::AsyncTaskManager::TaskUID getBlendPreviewTaskUID() { return getTaskUID("Blend preview "); }
    crimson::AsyncTaskManager::TaskUID getAutodetectTaskUID() { return getTaskUID("Autodetect "); }

    bool _initializingUI = true;

    Ui::VesselBlendingWidget _UI;
    QWidget* _viewWidget = nullptr;

    std::unordered_set<mitk::DataNode*> _nodesHiddenForPreview;
    std::unordered_set<mitk::DataNode*> _previousPreviewSolidModelNodes;

    QList<mitk::DataNode::Pointer> _mySelection;

    crimson::TaskStateObserver* _blendingTaskStateObserver;
    crimson::TaskStateObserver* _blendPreviewTaskStateObserver;
    crimson::TaskStateObserver* _detectIntersectionsTaskStateObserver;

    std::vector<boost::signals2::scoped_connection> _vesselForestDataConnections;

    void _updateFilletSize(const crimson::VesselForestData::VesselPathUIDPair& vessels, double size);
    void _removeFilletInfo(const crimson::VesselForestData::VesselPathUIDPair& vessels);
    void _updateBooleanOperation(const crimson::VesselForestData::BooleanOperationInfo& op);
    void _swapBooleanOperationTableRows(const crimson::VesselForestData::BooleanOperationInfo& from, const crimson::VesselForestData::BooleanOperationInfo& to);
};
