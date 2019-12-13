#include <array>

#include <QMessageBox>
#include <QtCore>
#include <QComboBox>

#include <VesselPathAbstractData.h>
#include <VesselForestData.h>

#include "VesselBlendingView.h"
#include "HierarchyManager.h"
#include "UseVesselsInBlendingDialog.h"
#include "BlendAction.h"
#include "VascularModelingUtils.h"
#include "DetectIntersectionsTask.h"
#include "BooleanOperationItemDelegate.h"

#include <VascularModelingNodeTypes.h>

#include <CreateDataNodeAsyncTask.h>
#include <ISolidModelKernel.h>

#include <mitkNodePredicateProperty.h>

#include <mitkOperationEvent.h>
#include <mitkUndoController.h>
#include <utils/TaskStateObserver.h>

#include <boost/format.hpp>
#include <boost/range/size.hpp>
#include <boost/range/adaptors.hpp>

class QComboBox;
Q_DECLARE_METATYPE(mitk::DataNode*)

const std::string VesselBlendingView::VIEW_ID = "org.mitk.views.VesselBlendingView";

VesselBlendingView::VesselBlendingView()
    : NodeDependentView(crimson::VascularModelingNodeTypes::VesselTree(), true, QString("Vessel tree"))
{
}

VesselBlendingView::~VesselBlendingView()
{
    if (currentNode()) {
        crimson::AsyncTaskManager::getInstance()->cancelTask(getBlendPreviewTaskUID());
        _hideBlendPreview();
    }
}

void VesselBlendingView::SetFocus()
{
    //     m_Controls.buttonPerformImageProcessing->setFocus();
    //     m_Controls.buttonPerformImageProcessing->setEnabled(true);
}

void VesselBlendingView::CreateQtPartControl(QWidget* parent)
{
    _viewWidget = parent;
    // create GUI widgets from the Qt Designer's .ui file
    _UI.setupUi(parent);

    connect(_UI.chooseVesselsButton, &QAbstractButton::clicked, this, &VesselBlendingView::chooseVesselsForBlending);

    _blendingTaskStateObserver = new crimson::TaskStateObserver(_UI.blendButton, _UI.cancelAsyncOpButton, this);
    connect(_blendingTaskStateObserver, &crimson::TaskStateObserver::runTaskRequested, this, &VesselBlendingView::createBlend);
    connect(_blendingTaskStateObserver, &crimson::TaskStateObserver::taskFinished, this, &VesselBlendingView::blendingFinished);

    _detectIntersectionsTaskStateObserver = new crimson::TaskStateObserver(_UI.autodetectButton, _UI.cancelAsyncOpButton, this);
    connect(_detectIntersectionsTaskStateObserver, &crimson::TaskStateObserver::runTaskRequested, this,
            &VesselBlendingView::startDetectIntersections);
    connect(_detectIntersectionsTaskStateObserver, &crimson::TaskStateObserver::taskFinished, this,
            &VesselBlendingView::detectIntersectionsFinished);

    auto delegate = new BooleanOperationItemDelegate(_UI.filletSizesTableWidget);
    _UI.filletSizesTableWidget->setItemDelegate(delegate);

    connect(_UI.filletSizesTableWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &VesselBlendingView::syncVesselSelection);
    connect(_UI.filletSizesTableWidget_2->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &VesselBlendingView::syncVesselSelection);

    connect(_UI.filletSizesTableWidget, &QTableWidget::itemChanged, this, &VesselBlendingView::onTableItemChanged);
    connect(_UI.filletSizesTableWidget_2, &QTableWidget::itemChanged, this, &VesselBlendingView::onTableItemChanged);

    _UI.filletSizesTableWidget->addAction(_UI.actionPreview);
    _UI.filletSizesTableWidget_2->addAction(_UI.actionPreview);
    _blendPreviewTaskStateObserver = new crimson::TaskStateObserver(_UI.actionPreview, _UI.cancelAsyncOpButton, this);
    connect(_blendPreviewTaskStateObserver, &crimson::TaskStateObserver::runTaskRequested, this,
            &VesselBlendingView::previewBlend);
    connect(_blendPreviewTaskStateObserver, &crimson::TaskStateObserver::taskFinished, this,
            &VesselBlendingView::previewFinished);

    _UI.filletSizesTableWidget->addAction(_UI.actionRedo_preview);
    _UI.filletSizesTableWidget_2->addAction(_UI.actionRedo_preview);
    connect(_UI.actionRedo_preview, &QAction::triggered, this, &VesselBlendingView::redoPreviewBlend);
    connect(_UI.actionPreview, &QAction::changed, [this]() {
        _UI.actionRedo_preview->setEnabled(_UI.autodetectButton->isEnabled() && _previousPreviewSolidModelNodes.size() > 0);
    });

    auto separator = new QAction(_UI.filletSizesTableWidget);
    separator->setSeparator(true);
    _UI.filletSizesTableWidget->addAction(separator);
    _UI.filletSizesTableWidget_2->addAction(separator);

    _UI.filletSizesTableWidget->addAction(_UI.actionShow);
    _UI.filletSizesTableWidget_2->addAction(_UI.actionShow);
    connect(_UI.actionShow, &QAction::triggered, this, &VesselBlendingView::showSelectedShapeNodes);

    _UI.filletSizesTableWidget->addAction(_UI.actionHide);
    _UI.filletSizesTableWidget_2->addAction(_UI.actionHide);
    connect(_UI.actionHide, &QAction::triggered, this, &VesselBlendingView::hideSelectedShapeNodes);

    _UI.filletSizesTableWidget->addAction(_UI.actionShow_only);
    _UI.filletSizesTableWidget_2->addAction(_UI.actionShow_only);
    connect(_UI.actionShow_only, &QAction::triggered, this, &VesselBlendingView::showOnlySelectedShapeNodes);

    separator = new QAction(_UI.filletSizesTableWidget);
    separator->setSeparator(true);
    _UI.filletSizesTableWidget->addAction(separator);
    _UI.filletSizesTableWidget_2->addAction(separator);

    _UI.filletSizesTableWidget->addAction(_UI.actionShow_all);
    _UI.filletSizesTableWidget_2->addAction(_UI.actionShow_all);
    connect(_UI.actionShow_all, &QAction::triggered, this, &VesselBlendingView::showAllShapeNodes);

    _UI.filletSizesTableWidget->addAction(_UI.actionHide_all);
    _UI.filletSizesTableWidget_2->addAction(_UI.actionHide_all);
    connect(_UI.actionHide_all, &QAction::triggered, this, &VesselBlendingView::hideAllShapeNodes);

    separator = new QAction(_UI.filletSizesTableWidget);
    separator->setSeparator(true);
    _UI.filletSizesTableWidget->addAction(separator);
    _UI.filletSizesTableWidget_2->addAction(separator);

    _UI.filletSizesTableWidget->addAction(_UI.actionShow_blend);
    _UI.filletSizesTableWidget_2->addAction(_UI.actionShow_blend);
    connect(_UI.actionShow_blend, &QAction::toggled, this, &VesselBlendingView::showBlendResult);

    connect(_UI.useParallelBlendingCheckBox, &QAbstractButton::toggled, [this](bool checked) {
        if (currentNode()) {
            currentNode()->SetBoolProperty("lofting.useParallelBlending", checked);
        }
    });

    // Display of current vessel tree
    ((QBoxLayout*)_UI.vesselTreeNameFrame->layout())->insertWidget(0, createSelectedNodeWidget(_UI.vesselTreeNameFrame));

    connect(_UI.moveUpButton, &QAbstractButton::clicked, this, &VesselBlendingView::moveBooleanOperationsUp);
    connect(_UI.moveDownButton, &QAbstractButton::clicked, this, &VesselBlendingView::moveBooleanOperationsDown);

    connect(_UI.swapArgumentsButton, &QAbstractButton::clicked, this, &VesselBlendingView::swapBooleanOperationArguments);

    connect(_UI.customBooleansEnabledCheckBox, &QAbstractButton::toggled, this, &VesselBlendingView::setCustomBooleansEnabled);
    setCustomBooleansEnabled(false);

    OnSelectionChanged(GetSite()->GetPart(), GetDataManagerSelection());

    _updateUI();

    _initializingUI = false;
}

void VesselBlendingView::_updateUI()
{
    _viewWidget->setEnabled(currentNode() != nullptr);

    bool anyVesselsInBlending = false;
    if (currentNode()) {
        auto vesselForest = static_cast<crimson::VesselForestData*>(currentNode()->GetData());
        anyVesselsInBlending = boost::size(vesselForest->getActiveVessels()) != 0;
    }

    _blendingTaskStateObserver->setEnabled(anyVesselsInBlending);
    _detectIntersectionsTaskStateObserver->setEnabled(anyVesselsInBlending);

    bool selectionNotEmpty =
        _UI.filletSizesTableWidget->selectedItems().size() > 0 || _UI.filletSizesTableWidget_2->selectedItems().size();
    _blendPreviewTaskStateObserver->setEnabled(selectionNotEmpty);

    _UI.actionShow->setEnabled(selectionNotEmpty);
    _UI.actionHide->setEnabled(selectionNotEmpty);
    _UI.actionShow_only->setEnabled(selectionNotEmpty);

    bool customBooleansEnabled = false;
    if (currentNode()) {
        mitk::DataNode::Pointer blendNode = crimson::HierarchyManager::getInstance()->getFirstDescendant(
            currentNode(), crimson::VascularModelingNodeTypes::Blend());
        _UI.actionShow_blend->setEnabled(blendNode.IsNotNull());
        bool blendResultVisible = blendNode.IsNotNull() && blendNode->IsVisible(nullptr);
        _UI.actionShow_blend->setChecked(blendResultVisible);
        _UI.actionShow_blend->setText(blendResultVisible ? tr("Hide blend result") : tr("Show blend result"));

        bool useParallelBlending = false;
        if (!currentNode()->GetBoolProperty("lofting.useParallelBlending", useParallelBlending)) {
            currentNode()->SetBoolProperty("lofting.useParallelBlending", useParallelBlending);
        }

        _UI.useParallelBlendingCheckBox->setChecked(useParallelBlending);

        currentNode()->GetBoolProperty("lofting.customBooleansEnabled", customBooleansEnabled);
    }
    _UI.customBooleansEnabledCheckBox->setChecked(customBooleansEnabled);
    _UI.swapArgumentsButton->setEnabled(selectionNotEmpty);

    if (_UI.filletSizesTableWidget->rowCount() > 0) {
        _UI.moveUpButton->setEnabled(selectionNotEmpty &&
                                     !_UI.filletSizesTableWidget->selectionModel()->rowIntersectsSelection(0, QModelIndex()));
        _UI.moveDownButton->setEnabled(selectionNotEmpty &&
                                       !_UI.filletSizesTableWidget->selectionModel()->rowIntersectsSelection(
                                           _UI.filletSizesTableWidget->rowCount() - 1, QModelIndex()));
    } else {
        _UI.moveUpButton->setEnabled(false);
        _UI.moveDownButton->setEnabled(false);
    }
}

void VesselBlendingView::currentNodeChanged(mitk::DataNode* prevNode)
{
    if (prevNode) {
        _vesselForestDataConnections.clear();

        crimson::AsyncTaskManager::getInstance()->cancelTask(getBlendPreviewTaskUID());

        _UI.filletSizesTableWidget->setRowCount(0);
        _UI.filletSizesTableWidget_2->setRowCount(0);
        _hideBlendPreview();
        _previousPreviewSolidModelNodes.clear();
    }

    if (currentNode()) {
        _fillFilletSizeTable();

        auto vesselForest = static_cast<crimson::VesselForestData*>(currentNode()->GetData());

        _vesselForestDataConnections.push_back(vesselForest->vesselInsertedSignal.connect(
            [this](const crimson::VesselForestData::VesselPathUIDType&) { _fillFilletSizeTable(); }));
        _vesselForestDataConnections.push_back(vesselForest->vesselRemovedSignal.connect(
            [this](const crimson::VesselForestData::VesselPathUIDType) { _fillFilletSizeTable(); }));

        _vesselForestDataConnections.push_back(vesselForest->filletChangedSignal.connect([this](
            const crimson::VesselForestData::VesselPathUIDPair& vessels, double size) { _updateFilletSize(vessels, size); }));
        _vesselForestDataConnections.push_back(vesselForest->filletRemovedSignal.connect(
            [this](const crimson::VesselForestData::VesselPathUIDPair& vessels) { _removeFilletInfo(vessels); }));

        _vesselForestDataConnections.push_back(vesselForest->booleanOperationChangedSignal.connect(
            [this](const crimson::VesselForestData::BooleanOperationInfo& info) { _updateBooleanOperation(info); }));
        _vesselForestDataConnections.push_back(vesselForest->booleanOperationsSwappedSignal.connect(
            [this](const crimson::VesselForestData::BooleanOperationInfo& from,
                   const crimson::VesselForestData::BooleanOperationInfo& to) { _swapBooleanOperationTableRows(from, to); }));
    }

    crimson::AsyncTaskManager::TaskUID blendingTaskUID = crimson::VascularModelingUtils::getBlendingTaskUID(currentNode());
    _blendingTaskStateObserver->setPrimaryObservedUID(blendingTaskUID);
    _blendingTaskStateObserver->setSecondaryObservedUIDs({getBlendPreviewTaskUID(), getAutodetectTaskUID()});

    _blendPreviewTaskStateObserver->setPrimaryObservedUID(getBlendPreviewTaskUID());
    _blendPreviewTaskStateObserver->setSecondaryObservedUIDs({blendingTaskUID, getAutodetectTaskUID()});

    _detectIntersectionsTaskStateObserver->setPrimaryObservedUID(getAutodetectTaskUID());
    _detectIntersectionsTaskStateObserver->setSecondaryObservedUIDs({blendingTaskUID, getBlendPreviewTaskUID()});

    _updateUI();
}

void VesselBlendingView::_updateFilletSize(const crimson::VesselForestData::VesselPathUIDPair& vessels, double size)
{
    auto tableItem = _findFilletSizeRow(vessels);

    if (!tableItem) {
        _addFlowFaceFilletRow(vessels, size);
        return;
    }

    auto tableWidget = tableItem->tableWidget();
    tableWidget->blockSignals(true);
    tableItem->tableWidget()->item(tableItem->row(), FilletSizeColumn)->setText(QString::number(size));
    tableWidget->blockSignals(false);
}

void VesselBlendingView::_removeFilletInfo(const crimson::VesselForestData::VesselPathUIDPair& vessels)
{
    auto tableItem = _findFilletSizeRow(vessels);

    if (!tableItem) {
        return;
    }

    tableItem->tableWidget()->removeRow(tableItem->row());
}

void VesselBlendingView::_updateBooleanOperation(const crimson::VesselForestData::BooleanOperationInfo& op)
{
    auto tableItem = _findFilletSizeRow(op.vessels);
    if (!tableItem) {
        return;
    }

    auto tableWidget = tableItem->tableWidget();
    tableWidget->blockSignals(true);
    _fillBooleanOperationRow(op, tableItem->row());
    tableWidget->blockSignals(false);
}

void VesselBlendingView::_swapBooleanOperationTableRows(const crimson::VesselForestData::BooleanOperationInfo& from,
                                                        const crimson::VesselForestData::BooleanOperationInfo& to)
{
    auto tableWidget = _UI.filletSizesTableWidget;

    auto itemFrom = _findFilletSizeRow(from.vessels);
    if (!itemFrom || itemFrom->tableWidget() != tableWidget) {
        return;
    }
    auto itemTo = _findFilletSizeRow(to.vessels);
    if (!itemTo || itemTo->tableWidget() != tableWidget) {
        return;
    }

    auto rowFrom = itemFrom->row();
    auto rowTo = itemTo->row();

    // takes and returns the whole row
    auto takeRow = [tableWidget](int row) {
        bool rowSelected = tableWidget->isItemSelected(tableWidget->item(row, 0));
        QList<QTableWidgetItem*> rowItems;
        for (int col = 0; col < tableWidget->columnCount(); ++col) {
            rowItems << tableWidget->takeItem(row, col);
        }
        return std::make_tuple(rowItems, rowSelected);
    };

    // sets the whole row
    auto setRow = [tableWidget](int row, const QList<QTableWidgetItem*>& rowItems, bool select) {
        tableWidget->selectionModel()->blockSignals(true);
        for (int col = 0; col < tableWidget->columnCount(); ++col) {
            tableWidget->setItem(row, col, rowItems.at(col));
            tableWidget->setItemSelected(rowItems.at(col), select);
        }
        tableWidget->selectionModel()->blockSignals(false);
    };

    auto fromRowItemsAndSelect = takeRow(rowFrom);
    auto toRowItemsAndSelect = takeRow(rowTo);
    setRow(rowTo, std::get<0>(fromRowItemsAndSelect), std::get<1>(fromRowItemsAndSelect));
    setRow(rowFrom, std::get<0>(toRowItemsAndSelect), std::get<1>(toRowItemsAndSelect));
}

void VesselBlendingView::startDetectIntersections()
{
    int performCleanDetection = QMessageBox::question(
        nullptr, "Perform clean detection?",
        "Would you like to re-detect all intersections?\nThis is useful in case some vessels no longer intersect.",
        QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);

    if (performCleanDetection == QMessageBox::Cancel) {
        return;
    }

    auto vesselForest = static_cast<crimson::VesselForestData*>(currentNode()->GetData());

    std::map<mitk::DataNode::Pointer, mitk::DataNode::Pointer> vesselNodeToLoftModelNodeMap;

    // Find the relevant nodes
    for (const auto& vesselUID : vesselForest->getActiveVessels()) {
        mitk::DataNode* vesselNode = crimson::VascularModelingUtils::getVesselPathNodeByUID(currentNode(), vesselUID);
        if (!vesselNode) {
            continue;
        }

        mitk::DataNode* vesselSolidModelNode = crimson::VascularModelingUtils::getVesselSolidModelNode(vesselNode);
        if (!vesselSolidModelNode) {
            continue;
        }

        vesselNodeToLoftModelNodeMap[vesselNode] = vesselSolidModelNode;
    }

    // Setup and launch detect intersections task asynchronously
    auto detectIntersectionsTask = std::make_shared<DetectIntersectionsTask>(vesselForest, vesselNodeToLoftModelNodeMap,
                                                                             performCleanDetection == QMessageBox::Yes);
    detectIntersectionsTask->setDescription(std::string("Detect intersections in ") + currentNode()->GetName());

    crimson::AsyncTaskManager::getInstance()->addTask(detectIntersectionsTask, getAutodetectTaskUID());

    _updateUI();
}

void VesselBlendingView::detectIntersectionsFinished(crimson::async::Task::State state)
{
    if (state == crimson::async::Task::State_Finished) {
        _fillFilletSizeTable();
    }
}

void VesselBlendingView::_fillFilletSizeTable()
{
    if (!currentNode()) {
        return;
    }

    _UI.filletSizesTableWidget->setRowCount(0);
    _UI.filletSizesTableWidget_2->setRowCount(0);

    auto vesselForest = static_cast<crimson::VesselForestData*>(currentNode()->GetData());

    _UI.filletSizesTableWidget->blockSignals(true);
    bool wasSortingEnabled = _UI.filletSizesTableWidget->isSortingEnabled();
    _UI.filletSizesTableWidget->setSortingEnabled(false);

    _UI.filletSizesTableWidget_2->blockSignals(true);
    _UI.filletSizesTableWidget_2->setSortingEnabled(false);

    for (const auto& bopInfo : vesselForest->getActiveBooleanOperations()) {
        assert(bopInfo.bop != crimson::VesselForestData::bopInvalidLast);

        int row = _UI.filletSizesTableWidget->rowCount();
        _UI.filletSizesTableWidget->insertRow(row);

        if (!_fillBooleanOperationRow(bopInfo, row)) {
            _UI.filletSizesTableWidget->removeRow(row);
            continue;
        }

        auto filletSizeIter = vesselForest->getFilletSizeInfos().find(bopInfo.vessels);
        assert(filletSizeIter != vesselForest->getFilletSizeInfos().end());

        _UI.filletSizesTableWidget->setItem(row, FilletSizeColumn,
                                            new QTableWidgetItem(QString::number(filletSizeIter->second)));
    }

    for (const auto& filletSizeInfoPair : vesselForest->getActiveFilletSizeInfos()) {
        _addFlowFaceFilletRow(filletSizeInfoPair.first, filletSizeInfoPair.second);
    }

    _UI.filletSizesTableWidget_2->setSortingEnabled(true);
    _UI.filletSizesTableWidget_2->blockSignals(false);
    for (auto i = 0; i < _UI.filletSizesTableWidget_2->columnCount() - 1; ++i) {
        _UI.filletSizesTableWidget_2->resizeColumnToContents(i);
    }

    _UI.filletSizesTableWidget->setSortingEnabled(wasSortingEnabled);
    _UI.filletSizesTableWidget->blockSignals(false);
    for (auto i = 0; i < _UI.filletSizesTableWidget->columnCount() - 1; ++i) {
        _UI.filletSizesTableWidget->resizeColumnToContents(i);
    }
}

bool VesselBlendingView::_fillBooleanOperationRow(const crimson::VesselForestData::BooleanOperationInfo& info, int row)
{
    mitk::DataNode* vessel1Node = crimson::VascularModelingUtils::getVesselPathNodeByUID(currentNode(), info.vessels.first);
    mitk::DataNode* vessel2Node = crimson::VascularModelingUtils::getVesselPathNodeByUID(currentNode(), info.vessels.second);

    if (!vessel1Node || !vessel2Node) {
        return false;
    }

    auto vessel1NameItem = new QTableWidgetItem(QString::fromStdString(vessel1Node->GetName()));
    vessel1NameItem->setFlags(vessel1NameItem->flags() & ~Qt::ItemIsEditable);
    vessel1NameItem->setData(VesselPathNodeRole, QVariant::fromValue(vessel1Node));
    _UI.filletSizesTableWidget->setItem(row, Vessel1NameColumn, vessel1NameItem);

    auto vessel2NameItem = new QTableWidgetItem(QString::fromStdString(vessel2Node->GetName()));
    vessel2NameItem->setFlags(vessel1NameItem->flags() & ~Qt::ItemIsEditable);
    vessel2NameItem->setData(VesselPathNodeRole, QVariant::fromValue(vessel2Node));
    _UI.filletSizesTableWidget->setItem(row, Vessel2NameColumn, vessel2NameItem);

    auto bopTypeItem = new QTableWidgetItem(BooleanOperationItemDelegate::getTextForBOPType(info.bop));
    bopTypeItem->setData(Qt::UserRole, static_cast<int>(info.bop));
    _UI.filletSizesTableWidget->setItem(row, BOPTypeColumn, bopTypeItem);

    return true;
}

void VesselBlendingView::_addFlowFaceFilletRow(const crimson::VesselForestData::VesselPathUIDPair& vessels, double filletSize)
{
    if (vessels.second != crimson::VesselForestData::InflowUID && vessels.second != crimson::VesselForestData::OutflowUID) {
        return;
    }

    int row = _UI.filletSizesTableWidget_2->rowCount();
    _UI.filletSizesTableWidget_2->insertRow(row);

    mitk::DataNode* vessel1Node = crimson::VascularModelingUtils::getVesselPathNodeByUID(currentNode(), vessels.first);
    if (!vessel1Node) {
        return;
    }

    auto vessel1NameItem = new QTableWidgetItem(QString::fromStdString(vessel1Node->GetName()));
    vessel1NameItem->setFlags(vessel1NameItem->flags() & ~Qt::ItemIsEditable);
    vessel1NameItem->setData(VesselPathNodeRole, QVariant::fromValue(vessel1Node));
    _UI.filletSizesTableWidget_2->setItem(row, Vessel1NameColumn, vessel1NameItem);

    auto vessel2NameItem = new QTableWidgetItem(QString::fromStdString(vessels.second));
    vessel2NameItem->setFlags(vessel1NameItem->flags() & ~Qt::ItemIsEditable);
    _UI.filletSizesTableWidget_2->setItem(row, Vessel2NameColumn, vessel2NameItem);

    _UI.filletSizesTableWidget_2->setItem(row, FilletSizeColumn, new QTableWidgetItem(QString::number(filletSize)));
}

void VesselBlendingView::OnSelectionChanged(berry::IWorkbenchPart::Pointer part, const QList<mitk::DataNode::Pointer>& nodes)
{
    if (!_initializingUI) {
        if (_mySelection == nodes) {
            return; // Selection did not actually change
        }
        // Sync selection
        _mySelection = nodes;
        this->FireNodesSelected(nodes);

        _hideBlendPreview();
    }

    NodeDependentView::OnSelectionChanged(part, nodes);

    if (!currentNode()) {
        return;
    }

    _updateTableWidgetSelection(nodes, _UI.filletSizesTableWidget);
    _updateTableWidgetSelection(nodes, _UI.filletSizesTableWidget_2);

    highlightRowsParticipatingInPreview();
    _updateUI();
}

void VesselBlendingView::NodeRemoved(const mitk::DataNode* node)
{
    NodeDependentView::NodeRemoved(node);

    _nodesHiddenForPreview.erase(const_cast<mitk::DataNode*>(node));
    _previousPreviewSolidModelNodes.erase(const_cast<mitk::DataNode*>(node));
}

void VesselBlendingView::NodeChanged(const mitk::DataNode* node)
{
    if (!currentNode()) {
        return;
    }

    if (crimson::HierarchyManager::getInstance()
            ->getPredicate(crimson::VascularModelingNodeTypes::VesselPath())
            ->CheckNode(node)) {

        // Only refill table if any of the relevant node names changed
        bool needRefillTable = false;
        for (int i = 0; i < _UI.filletSizesTableWidget->rowCount() && !needRefillTable; ++i) {
            for (FilletSizeTableColumn col : {Vessel1NameColumn, Vessel2NameColumn}) {
                if (_getFilletSizeTableVesselNode(_UI.filletSizesTableWidget, i, col) == node &&
                    _UI.filletSizesTableWidget->item(i, col)->text().toStdString() != node->GetName()) {
                    needRefillTable = true;
                }
            }
        }
        if (needRefillTable) {
            _fillFilletSizeTable();
        }
    }
}

void VesselBlendingView::previewBlend()
{
    _previousPreviewSolidModelNodes.clear();

    QTableWidget* focusedTableWidget = qobject_cast<QTableWidget*>(QApplication::focusWidget());
    if (!focusedTableWidget) {
        return;
    }

    // Fill the _previousPreviewSolidModelNodes and launch preview as a "redoPreview"
    for (QTableWidgetItem* selectedTableWidgetItem : focusedTableWidget->selectedItems()) {
        if (selectedTableWidgetItem->column() == FilletSizeColumn) {
            continue;
        }

        mitk::DataNode* vesselDataNode = _getFilletSizeTableVesselNode(
            focusedTableWidget, selectedTableWidgetItem->row(), static_cast<FilletSizeTableColumn>(selectedTableWidgetItem->column()));
        if (!vesselDataNode) {
            continue;
        }

        mitk::DataNode* solidModelNode = crimson::VascularModelingUtils::getVesselSolidModelNode(vesselDataNode);
        if (solidModelNode) {
            _previousPreviewSolidModelNodes.insert(solidModelNode);
        }
    }

    redoPreviewBlend();
}

void VesselBlendingView::redoPreviewBlend()
{
    std::map<crimson::VesselPathAbstractData::VesselPathUIDType, mitk::BaseData::Pointer> brepDatas;

    mitk::DataNode::Pointer blendNode = crimson::HierarchyManager::getInstance()->getFirstDescendant(
        currentNode(), crimson::VascularModelingNodeTypes::Blend());
    if (blendNode.IsNotNull() && blendNode->IsVisible(nullptr)) {
        _nodesHiddenForPreview.insert(blendNode);
    }

    // Collect all the solid models for preview
    for (mitk::DataNode* solidModelNode : _previousPreviewSolidModelNodes) {
        auto vesselPathNode = crimson::HierarchyManager::getInstance()->getAncestor(
            solidModelNode, crimson::VascularModelingNodeTypes::VesselPath());

        brepDatas[static_cast<crimson::VesselPathAbstractData*>(vesselPathNode->GetData())->getVesselUID()] =
            solidModelNode->GetData();
        if (solidModelNode->IsVisible(nullptr)) {
            _nodesHiddenForPreview.insert(solidModelNode);
        }
    }

    if (brepDatas.size() < 1) {
        _nodesHiddenForPreview.clear();
        return;
    }

    auto vesselForest = static_cast<crimson::VesselForestData*>(currentNode()->GetData());

    // Filter all the info for the selected vessels only
    auto filteredBooleanOperations =
        vesselForest->getActiveBooleanOperations() |
        boost::adaptors::filtered(
            [&brepDatas](const crimson::VesselForestData::BooleanOperationContainerType::value_type& bop) {
                return brepDatas.count(bop.vessels.first) == 1 && brepDatas.count(bop.vessels.second) == 1;
            });

    auto filteredFilletInfo = vesselForest->getActiveFilletSizeInfos() |
                              boost::adaptors::filtered([&brepDatas](
                                  const crimson::VesselForestData::FilletSizeInfoContainerType::value_type& filletInfo) {
                                  return brepDatas.count(filletInfo.first.first) == 1 &&
                                         (filletInfo.first.second == crimson::VesselForestData::InflowUID ||
                                          filletInfo.first.second == crimson::VesselForestData::OutflowUID ||
                                          brepDatas.count(filletInfo.first.second) == 1);
                              });

    // Setup a blend preview task
    bool useParallelBlending = false;
    currentNode()->GetBoolProperty("lofting.useParallelBlending", useParallelBlending);
    auto blendingTask = crimson::ISolidModelKernel::createBlendTask(brepDatas, filteredBooleanOperations, filteredFilletInfo,
                                                                    useParallelBlending);

    // Setup the properties for blend preview result node
    std::map<std::string, mitk::BaseProperty::Pointer> props = {
        {"lofting.blend_preview", mitk::BoolProperty::New(true).GetPointer()},
        {"name", mitk::StringProperty::New(currentNode()->GetName() + " Blended preview").GetPointer()},
        {"helper object", mitk::BoolProperty::New(true).GetPointer()},
        {"color", mitk::ColorProperty::New(0.3, 0.87, 0.21).GetPointer()}

    };

    // Launch the blend preview task
    auto dataNodeTask = std::make_shared<crimson::CreateDataNodeAsyncTask>(
        blendingTask, currentNode(), crimson::VascularModelingNodeTypes::VesselTree(),
        crimson::VascularModelingNodeTypes::BlendPreview(), props);
    dataNodeTask->setDescription(std::string("Preview blend in ") + currentNode()->GetName());

    crimson::AsyncTaskManager::getInstance()->addTask(dataNodeTask, getBlendPreviewTaskUID());

    _updateUI();
}

void VesselBlendingView::previewFinished(crimson::async::Task::State state)
{
    if (state == crimson::async::Task::State_Finished) {
        assert(currentNode());
        mitk::DataNode::Pointer blendNode = crimson::HierarchyManager::getInstance()->getFirstDescendant(
            currentNode(), crimson::VascularModelingNodeTypes::BlendPreview());

        // Hide the source shapes 
        _setPreviewOriginalShapesNodeVisibility(false);
    }
}

crimson::AsyncTaskManager::TaskUID VesselBlendingView::getTaskUID(const char* prefix)
{
    if (!currentNode()) {
        return "";
    }

    return (boost::format("%1% %2%") % prefix % currentNode()).str();
}

void VesselBlendingView::createBlend()
{
    _hideBlendPreview();

    BlendAction blendAction;
    std::shared_ptr<crimson::CreateDataNodeAsyncTask> newTask;
    _nodesHiddenForPreview = blendAction.Run(currentNode());

    _updateUI();
}

void VesselBlendingView::blendingFinished(crimson::async::Task::State state)
{
    if (state == crimson::async::Task::State_Finished) {
        _hideBlendPreview();
        _updateUI();
    }
}

void VesselBlendingView::_addSolidModelToSelection(mitk::DataNode* vesselNode, QList<mitk::DataNode::Pointer>& selection)
{
    mitk::DataNode* vesselSolidModelNode = crimson::VascularModelingUtils::getVesselSolidModelNode(vesselNode);
    if (vesselSolidModelNode) {
        selection.push_back(vesselSolidModelNode);
    }
}

void VesselBlendingView::syncVesselSelection(const QItemSelection&, const QItemSelection&)
{
    _hideBlendPreview();

    // Check if the selection change signal came from the between-vessels fillet
    // or from the vessel end fillet table
    QTableWidget* tableWidget;
    if (_UI.filletSizesTableWidget->selectionModel() == sender()) {
        tableWidget = _UI.filletSizesTableWidget;
    } else {
        tableWidget = _UI.filletSizesTableWidget_2;
    }

    // Select the corresponding solid nodes
    QList<mitk::DataNode::Pointer> newSelection;
    newSelection.push_back(currentNode());

    for (const std::pair<mitk::DataNode*, bool>& nodeAndSelectedFlagPair : _getVesselNodesSelectionStatus(tableWidget)) {
        if (nodeAndSelectedFlagPair.second) {
            _addSolidModelToSelection(nodeAndSelectedFlagPair.first, newSelection);
        }
    }

    _mySelection = newSelection;
    this->FireNodesSelected(newSelection);
    _updateTableWidgetSelection(newSelection, tableWidget == _UI.filletSizesTableWidget ? _UI.filletSizesTableWidget_2
                                                                                        : _UI.filletSizesTableWidget);

    highlightRowsParticipatingInPreview();

    _updateUI();
    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void VesselBlendingView::onTableItemChanged(QTableWidgetItem* item)
{
    if (!currentNode() || (item->column() != FilletSizeColumn && item->column() != BOPTypeColumn)) {
        return;
    }

    mitk::DataNode* vessel1Node = _getFilletSizeTableVesselNode(item->tableWidget(), item->row(), Vessel1NameColumn);
    mitk::DataNode* vessel2Node = _getFilletSizeTableVesselNode(item->tableWidget(), item->row(), Vessel2NameColumn);

    crimson::VesselForestData::VesselPathUIDPair vessels;

    vessels.first = static_cast<crimson::VesselPathAbstractData*>(vessel1Node->GetData())->getVesselUID();

    // For the vessel end fillet table, the second vessel is not a UID, but a special value
    // which should be used directly
    if (vessel2Node) {
        vessels.second = static_cast<crimson::VesselPathAbstractData*>(vessel2Node->GetData())->getVesselUID();
    } else {
        vessels.second = item->tableWidget()->item(item->row(), Vessel2NameColumn)->text().toStdString();
    }

    auto vesselForest = static_cast<crimson::VesselForestData*>(currentNode()->GetData());

    auto filletInfoIter = vesselForest->getFilletSizeInfos().find(vessels);
    assert(filletInfoIter != vesselForest->getFilletSizeInfos().end());

    if (item->column() == FilletSizeColumn) {
        bool ok;
        mitk::ScalarType filletSize = item->text().toDouble(&ok);

        if (!ok || filletSize < 0) {
            item->tableWidget()->blockSignals(true);
            item->setText(QString::number(filletInfoIter->second));
            item->tableWidget()->blockSignals(false);
            return;
        }

        // Undoable fillet change
        if (item->text() != QString::number(filletInfoIter->second)) {
            mitk::OperationEvent::IncCurrObjectEventId();
//            mitk::OperationEvent::ExecuteIncrement();

            using FilletChangeOperation = crimson::VesselForestData::FilletChangeOperation;

            auto doOp = new FilletChangeOperation(FilletChangeOperation::operationChange, vessels, filletSize);
            auto undoOp = new FilletChangeOperation(FilletChangeOperation::operationChange, vessels, filletInfoIter->second);
            auto operationEvent = new mitk::OperationEvent(vesselForest, doOp, undoOp, "Set fillet size");
            mitk::UndoController::GetCurrentUndoModel()->SetOperationEvent(operationEvent);

            vesselForest->ExecuteOperation(doOp);
        }
    } else if (item->column() == BOPTypeColumn) {
        crimson::VesselForestData::BooleanOperationInfo oldBOPInfo = vesselForest->findBooleanOperationInfo(vessels);
        auto bopType = static_cast<crimson::VesselForestData::BooleanOperationType>(item->data(Qt::UserRole).toInt());

        // Undoable boolean operation change
        if (bopType != oldBOPInfo.bop) {
            mitk::OperationEvent::IncCurrObjectEventId();
//            mitk::OperationEvent::ExecuteIncrement();

            using BooleanInfoChangeOperation = crimson::VesselForestData::BooleanInfoChangeOperation;

            auto newBOPInfo = oldBOPInfo;
            newBOPInfo.bop = static_cast<crimson::VesselForestData::BooleanOperationType>(bopType);

            auto doOp = new BooleanInfoChangeOperation(BooleanInfoChangeOperation::operationChange, newBOPInfo);
            auto undoOp = new BooleanInfoChangeOperation(BooleanInfoChangeOperation::operationChange, oldBOPInfo);
            auto operationEvent = new mitk::OperationEvent(vesselForest, doOp, undoOp, "Set boolean operation type");
            mitk::UndoController::GetCurrentUndoModel()->SetOperationEvent(operationEvent);

            vesselForest->ExecuteOperation(doOp);
        }
    }
}

void VesselBlendingView::_hideBlendPreview()
{
    if (!currentNode()) {
        return;
    }

    // Restore the visibility of nodes hidden after preview has completed
    _setPreviewOriginalShapesNodeVisibility(true);
    _nodesHiddenForPreview.clear();

    mitk::DataStorage::SetOfObjects::ConstPointer previewNodes =
        GetDataStorage()->GetDerivations(currentNode(), mitk::NodePredicateProperty::New("lofting.blend_preview"));

    if (previewNodes->size() > 0) {
        assert(previewNodes->size() == 1);
        (*previewNodes)[0]->SetVisibility(false);
    }
}

void VesselBlendingView::_setPreviewOriginalShapesNodeVisibility(bool visible)
{
    for (mitk::DataNode* node : _nodesHiddenForPreview) {
        node->SetVisibility(visible);
    }

    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void VesselBlendingView::showAllShapeNodes()
{
    _nodesHiddenForPreview.clear();
    _setAllOriginalShapeNodesVisibility(true);
}

void VesselBlendingView::hideAllShapeNodes()
{
    _nodesHiddenForPreview.clear();
    _setAllOriginalShapeNodesVisibility(false);
}

void VesselBlendingView::showBlendResult(bool show)
{
    auto blendNode = crimson::HierarchyManager::getInstance()->getFirstDescendant(currentNode(),
                                                                                  crimson::VascularModelingNodeTypes::Blend());
    if (blendNode.IsNotNull()) {
        blendNode->SetVisibility(show);
        _updateUI();
        mitk::RenderingManager::GetInstance()->RequestUpdateAll();
    }
}

std::unordered_map<mitk::DataNode*, bool> VesselBlendingView::_getVesselNodesSelectionStatus(QTableWidget* tableWidget)
{
    // As the vessels can be duplicated in different rows, first collect the data nodes that need to be selected
    std::unordered_map<mitk::DataNode*, bool> selectionStatus;

    for (int row = 0; row < tableWidget->rowCount(); ++row) {
        bool selected = tableWidget->item(row, Vessel1NameColumn)->isSelected(); // row selection

        auto node1 = _getFilletSizeTableVesselNode(tableWidget, row, Vessel1NameColumn);
        auto node2 = _getFilletSizeTableVesselNode(tableWidget, row, Vessel2NameColumn);

        selectionStatus[node1] = selectionStatus[node1] || selected;
        if (node2) {
            selectionStatus[node2] = selectionStatus[node2] || selected;
        }
    }

    return selectionStatus;
}

void VesselBlendingView::showOnlySelectedShapeNodes()
{
    QTableWidget* focusedTableWidget = qobject_cast<QTableWidget*>(QApplication::focusWidget());
    if (!focusedTableWidget) {
        return;
    }

    _nodesHiddenForPreview.clear();
    showBlendResult(false);

    for (const std::pair<mitk::DataNode*, bool>& nodeAndSelectedFlagPair : _getVesselNodesSelectionStatus(focusedTableWidget)) {
        mitk::DataNode* vesselSolidModelNode =
            crimson::VascularModelingUtils::getVesselSolidModelNode(nodeAndSelectedFlagPair.first);
        if (vesselSolidModelNode) {
            vesselSolidModelNode->SetVisibility(nodeAndSelectedFlagPair.second);
        }
    }

    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void VesselBlendingView::showSelectedShapeNodes()
{
    QTableWidget* focusedTableWidget = qobject_cast<QTableWidget*>(QApplication::focusWidget());
    if (!focusedTableWidget) {
        return;
    }

    _nodesHiddenForPreview.clear();

    for (const std::pair<mitk::DataNode*, bool>& nodeAndSelectedFlagPair : _getVesselNodesSelectionStatus(focusedTableWidget)) {
        mitk::DataNode* vesselSolidModelNode =
            crimson::VascularModelingUtils::getVesselSolidModelNode(nodeAndSelectedFlagPair.first);
        if (vesselSolidModelNode && nodeAndSelectedFlagPair.second) {
            vesselSolidModelNode->SetVisibility(true);
        }
    }

    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void VesselBlendingView::hideSelectedShapeNodes()
{
    QTableWidget* focusedTableWidget = qobject_cast<QTableWidget*>(QApplication::focusWidget());
    if (!focusedTableWidget) {
        return;
    }
    _nodesHiddenForPreview.clear();

    for (const std::pair<mitk::DataNode*, bool>& nodeAndSelectedFlagPair : _getVesselNodesSelectionStatus(focusedTableWidget)) {
        mitk::DataNode* vesselSolidModelNode =
            crimson::VascularModelingUtils::getVesselSolidModelNode(nodeAndSelectedFlagPair.first);
        if (vesselSolidModelNode && nodeAndSelectedFlagPair.second) {
            vesselSolidModelNode->SetVisibility(false);
        }
    }

    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void VesselBlendingView::_setAllOriginalShapeNodesVisibility(bool visible)
{
    auto vesselForestData = static_cast<crimson::VesselForestData*>(currentNode()->GetData());

    for (const auto& vesselUID : vesselForestData->getActiveVessels()) {
        mitk::DataNode* vesselNode = crimson::VascularModelingUtils::getVesselPathNodeByUID(currentNode(), vesselUID);
        if (!vesselNode) {
            continue;
        }
        mitk::DataNode* shapeNode = crimson::VascularModelingUtils::getVesselSolidModelNode(vesselNode);
        if (shapeNode) {
            shapeNode->SetVisibility(visible);
        }
    }

    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void VesselBlendingView::highlightRowsParticipatingInPreview()
{
    // Some of the non-selected fillet size rows need to be highlighted 
    // because they participate in the preview generation despite not being 
    // directly selected

    std::set<mitk::DataNode*> allSelectedVessels;
    for (int row = 0; row < _UI.filletSizesTableWidget->rowCount(); ++row) {
        if (_UI.filletSizesTableWidget->item(row, FilletSizeColumn)->isSelected()) {
            allSelectedVessels.insert(_getFilletSizeTableVesselNode(_UI.filletSizesTableWidget, row, Vessel1NameColumn));
            allSelectedVessels.insert(_getFilletSizeTableVesselNode(_UI.filletSizesTableWidget, row, Vessel2NameColumn));
        }
    }

    for (int row = 0; row < _UI.filletSizesTableWidget->rowCount(); ++row) {
        bool highlightRow =
            allSelectedVessels.find(_getFilletSizeTableVesselNode(_UI.filletSizesTableWidget, row, Vessel1NameColumn)) !=
                allSelectedVessels.end() &&
            allSelectedVessels.find(_getFilletSizeTableVesselNode(_UI.filletSizesTableWidget, row, Vessel2NameColumn)) !=
                allSelectedVessels.end();

        // Highlight with green background. Account for white or black text by selecting color value (HSV) opposite to that of
        // normal text
        float highlightBrushColorV = 0.6 * (1.0 - _UI.filletSizesTableWidget->palette().text().color().valueF() + 0.2);
        QBrush highlightBrush = QBrush(QColor::fromHsvF(0.33, 0.5, highlightBrushColorV));

        for (int col = 0; col < _UI.filletSizesTableWidget->columnCount(); ++col) {
            QTableWidgetItem* item = _UI.filletSizesTableWidget->item(row, col);
            if (item) {
                item->setBackground(highlightRow ? highlightBrush : QBrush());
            }
        }
    }
}

mitk::DataNode* VesselBlendingView::_getFilletSizeTableVesselNode(QTableWidget* tableWidget, int row, FilletSizeTableColumn col)
{
    if (col == FilletSizeColumn) {
        return nullptr;
    }

    return qvariant_cast<mitk::DataNode*>(tableWidget->item(row, col)->data(VesselPathNodeRole));
}

QTableWidgetItem* VesselBlendingView::_findFilletSizeRow(const crimson::VesselForestData::VesselPathUIDPair& vessels)
{
    if (vessels.second == crimson::VesselForestData::InflowUID || vessels.second == crimson::VesselForestData::OutflowUID) {
        // Vessel end fillet
        for (int row = 0; row < _UI.filletSizesTableWidget_2->rowCount(); ++row) {
            auto vesselUID = static_cast<crimson::VesselPathAbstractData*>(
                                 _getFilletSizeTableVesselNode(_UI.filletSizesTableWidget_2, row, Vessel1NameColumn)->GetData())
                                 ->getVesselUID();
            if ((vesselUID == vessels.first &&
                 _UI.filletSizesTableWidget_2->item(row, Vessel2NameColumn)->text().toStdString() == vessels.second)) {

                return _UI.filletSizesTableWidget_2->item(row, Vessel1NameColumn);
            }
        }
    } else {
        // Between-vessel end fillets
        for (int row = 0; row < _UI.filletSizesTableWidget->rowCount(); ++row) {
            auto vessel1UID = static_cast<crimson::VesselPathAbstractData*>(
                                  _getFilletSizeTableVesselNode(_UI.filletSizesTableWidget, row, Vessel1NameColumn)->GetData())
                                  ->getVesselUID();
            auto vessel2UID = static_cast<crimson::VesselPathAbstractData*>(
                                  _getFilletSizeTableVesselNode(_UI.filletSizesTableWidget, row, Vessel2NameColumn)->GetData())
                                  ->getVesselUID();
            if ((vessel1UID == vessels.first && vessel2UID == vessels.second) ||
                (vessel2UID == vessels.first && vessel1UID == vessels.second)) {

                return _UI.filletSizesTableWidget->item(row, Vessel1NameColumn);
            }
        }
    }

    return nullptr;
}

void VesselBlendingView::chooseVesselsForBlending()
{
    UseVesselsInBlendingDialog dialog(currentNode());

    if (dialog.exec() == QDialog::Accepted) {
        _fillFilletSizeTable();
        _updateUI();
    }
}

void VesselBlendingView::setCustomBooleansEnabled(bool enabled)
{
    _UI.customBooleansFrame->setVisible(enabled);
    _UI.filletSizesTableWidget->setColumnHidden(BOPTypeColumn, !enabled);
    _UI.filletSizesTableWidget->setSortingEnabled(!enabled);

    if (enabled) {
        _fillFilletSizeTable();
    }

    if (currentNode()) {
        currentNode()->SetBoolProperty("lofting.customBooleansEnabled", enabled);
    }
}

void VesselBlendingView::moveBooleanOperationsUp()
{
    mitk::OperationEvent::IncCurrObjectEventId();
//    mitk::OperationEvent::ExecuteIncrement();

    for (auto row = 1; row < _UI.filletSizesTableWidget->rowCount(); ++row) {
        if (_UI.filletSizesTableWidget->selectionModel()->isRowSelected(row, QModelIndex{})) {
            _swapBooleanOperations(row - 1, row);
        }
    }
}

void VesselBlendingView::moveBooleanOperationsDown()
{
    mitk::OperationEvent::IncCurrObjectEventId();
//    mitk::OperationEvent::ExecuteIncrement();

    for (auto row = _UI.filletSizesTableWidget->rowCount(); row >= 0; --row) {
        if (_UI.filletSizesTableWidget->selectionModel()->isRowSelected(row, QModelIndex{})) {
            _swapBooleanOperations(row + 1, row);
        }
    }
}

void VesselBlendingView::_swapBooleanOperations(int from, int to)
{
    auto tableWidget = _UI.filletSizesTableWidget;
    auto vesselForest = static_cast<crimson::VesselForestData*>(currentNode()->GetData());

    // Swap BOPs in vessel tree
    auto vesselsFrom = std::make_pair(static_cast<crimson::VesselPathAbstractData*>(
                                          _getFilletSizeTableVesselNode(tableWidget, from, Vessel1NameColumn)->GetData())
                                          ->getVesselUID(),
                                      static_cast<crimson::VesselPathAbstractData*>(
                                          _getFilletSizeTableVesselNode(tableWidget, from, Vessel2NameColumn)->GetData())
                                          ->getVesselUID());

    auto vesselsTo = std::make_pair(static_cast<crimson::VesselPathAbstractData*>(
                                        _getFilletSizeTableVesselNode(tableWidget, to, Vessel1NameColumn)->GetData())
                                        ->getVesselUID(),
                                    static_cast<crimson::VesselPathAbstractData*>(
                                        _getFilletSizeTableVesselNode(tableWidget, to, Vessel2NameColumn)->GetData())
                                        ->getVesselUID());

    using BooleanInfoChangeOperation = crimson::VesselForestData::BooleanInfoChangeOperation;

    auto bopFrom = vesselForest->findBooleanOperationInfo(vesselsFrom);
    auto bopTo = vesselForest->findBooleanOperationInfo(vesselsTo);

    // Undoable boolean operation swap
    auto doOp = new BooleanInfoChangeOperation(BooleanInfoChangeOperation::operationSwap, bopFrom, bopTo);
    auto undoOp = new BooleanInfoChangeOperation(BooleanInfoChangeOperation::operationSwap, bopTo, bopFrom);
    auto operationEvent = new mitk::OperationEvent(vesselForest, doOp, undoOp, "Swap boolean operations");
    mitk::UndoController::GetCurrentUndoModel()->SetOperationEvent(operationEvent);

    vesselForest->ExecuteOperation(doOp);
}

void VesselBlendingView::swapBooleanOperationArguments()
{
    for (auto& selectedRange : _UI.filletSizesTableWidget->selectedRanges()) {
        for (auto row = selectedRange.topRow(); row <= selectedRange.bottomRow(); ++row) {
            auto vessel1Node = _getFilletSizeTableVesselNode(_UI.filletSizesTableWidget, row, Vessel1NameColumn);
            auto vessel2Node = _getFilletSizeTableVesselNode(_UI.filletSizesTableWidget, row, Vessel2NameColumn);

            auto vessels =
                std::make_pair(static_cast<crimson::VesselPathAbstractData*>(vessel1Node->GetData())->getVesselUID(),
                               static_cast<crimson::VesselPathAbstractData*>(vessel2Node->GetData())->getVesselUID());

            auto vesselForest = static_cast<crimson::VesselForestData*>(currentNode()->GetData());

            auto booleanOperationInfo = vesselForest->findBooleanOperationInfo(vessels);
            std::swap(booleanOperationInfo.vessels.first, booleanOperationInfo.vessels.second);

            vesselForest->replaceBooleanOperationInfo(booleanOperationInfo);

            auto v1Item = _UI.filletSizesTableWidget->takeItem(row, Vessel1NameColumn);
            auto v2Item = _UI.filletSizesTableWidget->takeItem(row, Vessel2NameColumn);
            _UI.filletSizesTableWidget->setItem(row, Vessel1NameColumn, v2Item);
            _UI.filletSizesTableWidget->setItem(row, Vessel2NameColumn, v1Item);
        }
    }
}

void VesselBlendingView::_updateTableWidgetSelection(const QList<mitk::DataNode::Pointer>& nodes, QTableWidget* tableWidget)
{
    disconnect(tableWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this,
               &VesselBlendingView::syncVesselSelection);
    tableWidget->clearSelection();

    QItemSelection selection;
    // Sync selected vessels
    for (int row = 0; row < tableWidget->rowCount(); ++row) {
        // Check if both vessels in a row are selected (for two-way fillets)
        mitk::DataNode* vessel = _getFilletSizeTableVesselNode(tableWidget, row, Vessel1NameColumn);
        if (!nodes.contains(vessel) &&
            !nodes.contains(crimson::HierarchyManager::getInstance()->getFirstDescendant(
                vessel, crimson::VascularModelingNodeTypes::Loft()))) {
            continue;
        }

        vessel = _getFilletSizeTableVesselNode(tableWidget, row, Vessel2NameColumn);
        if (vessel && !nodes.contains(vessel) &&
            !nodes.contains(crimson::HierarchyManager::getInstance()->getFirstDescendant(
                vessel, crimson::VascularModelingNodeTypes::Loft()))) {
            continue;
        }

        // .. they are
        for (int col = 0; col < tableWidget->columnCount(); ++col) {
            tableWidget->item(row, col)->setSelected(true);
        }
    }
    connect(tableWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &VesselBlendingView::syncVesselSelection);
}