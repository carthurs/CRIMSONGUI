// Qt
#include <QMessageBox>
#include <QFileDialog>
#include <QTimer>

#include <ctkDoubleSpinBox.h>

// Main include
#include "VesselMeshingView.h"
#include "MeshAction.h"
#include "MeshingUtils.h"

#include "ShowMeshInformationAction.h"

// Module includes
#include <SolidData.h>
#include <OCCBRepData.h>
#include <SolidDataFacePickingObserver.h>
#include <HierarchyManager.h>
#include <VascularModelingNodeTypes.h>
#include <VesselMeshingNodeTypes.h>

// Meshing kernel
#include <IMeshingKernel.h>
#include <ui/MeshingKernelUI.h>
#include <MeshingParametersData.h>

// MITK
#include <usModule.h>
#include <usModuleRegistry.h>
#include <mitkInteractionEventObserver.h>

#include <boost/range/algorithm/copy.hpp>
#include <boost/range/adaptor/map.hpp>

#include <chrono>
#include <FaceListTableModel.h>

Q_DECLARE_METATYPE(crimson::FaceIdentifier);

/*! \brief   A FaceListTableModel model with an extra column showing whether the local parameters for the face were modified. */
class FaceListTableModelWithModifiedFlag : public FaceListTableModel
{
public:
    FaceListTableModelWithModifiedFlag(QObject* parent = nullptr)
        : FaceListTableModel(parent)
    {
    }

    void setCurrentMeshingParameters(crimson::MeshingParametersData* currentMeshingParameters)
    {
        _currentMeshingParameters = currentMeshingParameters;
    }

    int columnCount(const QModelIndex& /*parent*/ = QModelIndex()) const override { return 3; }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (index.column() < 2) {
            return FaceListTableModel::data(index, role);
        }

        if (role != Qt::DisplayRole) {
            return QVariant();
        }

        if (static_cast<int>(_faces.size()) < index.row() || !_currentMeshingParameters) {
            return QVariant();
        }

        auto faceIterator = _faces.begin();
        std::advance(faceIterator, index.row());

        return QVariant::fromValue(_currentMeshingParameters->localParameters()[*faceIterator].anyOptionSet() ? tr("Yes")
                                                                                                              : tr("No"));
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (orientation == Qt::Vertical || role != Qt::DisplayRole) {
            return QAbstractTableModel::headerData(section, orientation, role);
        }

        if (section < 2) {
            return FaceListTableModel::headerData(section, orientation, role);
        }

        return QVariant::fromValue(tr("Modified"));
    }

    void emitDataChanged(const std::vector<crimson::FaceIdentifier>& faceIds)
    {
        for (const crimson::FaceIdentifier& faceId : faceIds) {
            int row = std::distance(_faces.begin(), _faces.find(faceId));
            emit dataChanged(this->index(row, 2), this->index(row, 2));
        }
    }

private:
    crimson::MeshingParametersData* _currentMeshingParameters = nullptr;
};

const std::string VesselMeshingView::VIEW_ID = "org.mitk.views.VesselMeshingView";

VesselMeshingView::VesselMeshingView()
    : NodeDependentView(crimson::VascularModelingNodeTypes::Solid(), true, "Solid")
    , _pickingEventObserverServiceTracker(us::ModuleRegistry::GetModule(1)->GetModuleContext(),
                                          us::LDAPFilter("(name=DataNodePicker)"))
{
    _pickingEventObserverServiceTracker.Open();
    _facePickingObserverServiceRegistration = us::ModuleRegistry::GetModule(1)->GetModuleContext()->RegisterService(
        (mitk::InteractionEventObserver*)(&_facePickingObserver));
    _facePickingObserver.Disable();
}

VesselMeshingView::~VesselMeshingView()
{
    setFaceSelectionMode(false);
    _facePickingObserverServiceRegistration.Unregister();
}

void VesselMeshingView::SetFocus() {}

void VesselMeshingView::CreateQtPartControl(QWidget* parent)
{
    // create GUI widgets from the Qt Designer's .ui file
    _UI.setupUi(parent);

    _faceListModel = new FaceListTableModelWithModifiedFlag(parent);

    _faceListProxyModel.setSourceModel(_faceListModel);
    _UI.faceSettingsTable->setModel(&_faceListProxyModel);

    connect(_UI.selectFacesButton, &QAbstractButton::toggled, this, &VesselMeshingView::setFaceSelectionMode);

    connect(_UI.editGlobalOptionsButton, &QAbstractButton::clicked, this, &VesselMeshingView::editGlobalMeshingOptions);
    connect(_UI.editLocalOptionsButton, &QAbstractButton::clicked, this, &VesselMeshingView::editLocalMeshingOptions);

    connect(_UI.faceSettingsTable->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &VesselMeshingView::syncFaceSelection);

    connect(_UI.showMeshInformation, &QAbstractButton::clicked, this, [this]() {
        mitk::DataNode::Pointer meshNode = crimson::HierarchyManager::getInstance()->getFirstDescendant(
            currentNode(), crimson::VesselMeshingNodeTypes::Mesh());
        if (!meshNode) {
            return;
        }

        ShowMeshInformationAction action;
        action.Run(QList<mitk::DataNode::Pointer>() << meshNode);
    });

    static_cast<QBoxLayout*>(_UI.solidNameFrame->layout())->insertWidget(0, createSelectedNodeWidget(_UI.solidNameFrame));

    _meshingTaskStateObserver = new crimson::TaskStateObserver(_UI.meshButton, _UI.cancelMeshButton, this);
    connect(_meshingTaskStateObserver, &crimson::TaskStateObserver::runTaskRequested, this, &VesselMeshingView::createMesh);
    connect(_meshingTaskStateObserver, &crimson::TaskStateObserver::taskFinished, this, &VesselMeshingView::meshingFinished);

    // Initialize the UI
    _updateUI();
}

void VesselMeshingView::_updateUI()
{
    _meshingTaskStateObserver->setEnabled(currentNode() != nullptr);
    _UI.localOptionsGroup->setEnabled(currentNode() != nullptr);
    _UI.globalOptionsGroup->setEnabled(currentNode() != nullptr);
    _UI.showMeshInformation->setEnabled(currentNode() != nullptr &&
                                        crimson::HierarchyManager::getInstance()
                                            ->getFirstDescendant(currentNode(), crimson::VesselMeshingNodeTypes::Mesh())
                                            .IsNotNull());
}

void VesselMeshingView::createMesh()
{
    MeshAction action;
    action.Run(QList<mitk::DataNode::Pointer>() << currentNode());
}

void VesselMeshingView::meshingFinished(crimson::async::Task::State state)
{
    if (state == crimson::async::Task::State_Finished) {
        currentNode()->SetVisibility(false);
    }
    _updateUI();
}

void VesselMeshingView::currentNodeChanged(mitk::DataNode* prevNode)
{
    if (prevNode) {
        _UI.selectFacesButton->setChecked(false);
        _faceListModel->setCurrentMeshingParameters(nullptr);
        _faceListModel->setFaces(std::set<crimson::FaceIdentifier>());
        _faceListModel->setVesselTreeNode(nullptr);
    }

    _currentMeshingParameters = nullptr;

    if (currentNode()) {
        _currentMeshingParameters = crimson::MeshingUtils::getMeshingParametersForSolid(currentNode());
        _faceListModel->setCurrentMeshingParameters(_currentMeshingParameters);
        _faceListModel->setVesselTreeNode(crimson::HierarchyManager::getInstance()->getAncestor(
            currentNode(), crimson::VascularModelingNodeTypes::VesselTree()));

        // Replace the widget
        if (auto item = static_cast<QWidgetItem*>(_UI.globalOptionsGroup->layout()->takeAt(1))) {
            delete item->widget();
            delete item;
        }

        static_cast<QBoxLayout*>(_UI.globalOptionsGroup->layout())->insertWidget(1, 
            crimson::MeshingKernelUI::createGlobalMeshingParameterWidget(_currentMeshingParameters->globalParameters())
        );
    }

    _meshingTaskStateObserver->setPrimaryObservedUID(crimson::MeshingUtils::getMeshingTaskUID(currentNode()));

    fillFaceSettingsTableWidget();

    _updateUI();
}

void VesselMeshingView::setFaceSelectionMode(bool enabled)
{
    if (_pickingEventObserverServiceTracker.GetService()) {
        if (enabled) {
            _pickingEventObserverServiceTracker.GetService()->Disable();
            _facePickingObserver.Enable();
        } else {
            _pickingEventObserverServiceTracker.GetService()->Enable();
            _facePickingObserver.Disable();
        }
    }

    _facePickingObserver.SetDataNode(enabled ? currentNode() : nullptr);
    _facePickingObserver.setSelectionChangedObserver(enabled ? std::bind(&VesselMeshingView::syncFaceSelectionToObserver, this)
                                                             : std::function<void(void)>());

    if (!currentNode()) {
        return;
    }

    currentNode()->SetVisibility(true);
    mitk::DataNode::Pointer meshNode =
        crimson::HierarchyManager::getInstance()->getFirstDescendant(currentNode(), crimson::VesselMeshingNodeTypes::Mesh());
    if (meshNode.IsNotNull()) {
        meshNode->SetVisibility(false);
    }

    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void VesselMeshingView::editGlobalMeshingOptions()
{
    std::map<crimson::FaceIdentifier, crimson::IMeshingKernel::LocalMeshingParameters*> params = {
        {crimson::FaceIdentifier(), &_currentMeshingParameters->globalParameters().defaultLocalParameters}};
    editMeshingOptions(params, true);
}

void VesselMeshingView::editLocalMeshingOptions()
{
	crimson::SolidData* brep = static_cast<crimson::SolidData*>(currentNode()->GetData());

    std::map<crimson::FaceIdentifier, crimson::IMeshingKernel::LocalMeshingParameters*> paramsToEdit;

    std::vector<bool> selectionStates = _facePickingObserver.getFaceSelectedStates();
    for (size_t i = 0; i < selectionStates.size(); ++i) {
        if (selectionStates[i]) {
            paramsToEdit[brep->getFaceIdentifierMap().getFaceIdentifier(i)] =
                &_currentMeshingParameters->localParameters()[brep->getFaceIdentifierMap().getFaceIdentifier(i)];
        }
    }

    if (paramsToEdit.size() > 0) {
        editMeshingOptions(paramsToEdit, false);
    }
}

void VesselMeshingView::editMeshingOptions(
    const std::map<crimson::FaceIdentifier, crimson::IMeshingKernel::LocalMeshingParameters*>& paramsMap, bool editingGlobal)
{
    std::vector<crimson::IMeshingKernel::LocalMeshingParameters*> params;
    boost::copy(paramsMap | boost::adaptors::map_values, std::back_inserter(params));

    std::unique_ptr<QDialog> dlg(crimson::MeshingKernelUI::createLocalMeshingParametersDialog(
        params, editingGlobal, _currentMeshingParameters->globalParameters().defaultLocalParameters
    ));

    if (dlg->exec() == QDialog::Accepted && !editingGlobal) {
        // Inform view about updated data
        std::vector<crimson::FaceIdentifier> faceIds;
        boost::copy(paramsMap | boost::adaptors::map_keys, std::back_inserter(faceIds));
        _faceListModel->emitDataChanged(faceIds);
    }
}

void VesselMeshingView::fillFaceSettingsTableWidget()
{
    _UI.faceSettingsTable->setSortingEnabled(false);

    std::set<crimson::FaceIdentifier> allFaceIdentifiers;

    if (currentNode()) {
		crimson::SolidData* brepData = static_cast<crimson::SolidData*>(currentNode()->GetData());

        auto& faceIdMap = brepData->getFaceIdentifierMap();
        for (int i = 0; i < faceIdMap.getNumberOfFaceIdentifiers(); ++i) {
            const auto& faceId = faceIdMap.getFaceIdentifier(i);
            if (!faceIdMap.getModelFacesForFaceIdentifier(faceId).empty()) {
                allFaceIdentifiers.insert(faceId);
            }
        }
    }

    _faceListModel->setFaces(allFaceIdentifiers);

    _UI.faceSettingsTable->setSortingEnabled(true);
}

void VesselMeshingView::NodeChanged(const mitk::DataNode* node)
{
    NodeDependentView::NodeChanged(node);

    if (crimson::HierarchyManager::getInstance()
            ->getPredicate(crimson::VascularModelingNodeTypes::VesselPath())
            ->CheckNode(node)) {
        // React on name changes
        fillFaceSettingsTableWidget();
    }
}

void VesselMeshingView::NodeRemoved(const mitk::DataNode* node)
{
    NodeDependentView::NodeRemoved(node);
    QTimer::singleShot(0, this, SLOT(_updateUI())); // Defer UI update until node is actually removed from the storage
}

void VesselMeshingView::syncFaceSelection(const QItemSelection&, const QItemSelection&)
{
    if (!currentNode()) {
        return;
    }
    _facePickingObserver.setSelectionChangedObserver(std::function<void(void)>());

    _facePickingObserver.clearSelection();
    _facePickingObserver.resetSelectableTypes();

    if (!_UI.faceSettingsTable->selectionModel()->selectedIndexes().empty()) {
        _UI.selectFacesButton->setChecked(true);

        for (QModelIndex index : _UI.faceSettingsTable->selectionModel()->selectedIndexes()) {
            if (index.column() != 0) {
                continue;
            }
            _facePickingObserver.selectFace(
                _UI.faceSettingsTable->model()->data(index, FaceListTableModel::FaceIdRole).value<crimson::FaceIdentifier>(),
                true);
        }
    }

    _facePickingObserver.setSelectionChangedObserver(std::bind(&VesselMeshingView::syncFaceSelectionToObserver, this));

    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void VesselMeshingView::syncFaceSelectionToObserver()
{
    if (!currentNode()) {
        return;
    }

	crimson::SolidData* brepData = static_cast<crimson::SolidData*>(currentNode()->GetData());

    std::set<crimson::FaceIdentifier> selectedFaces;

    for (size_t i = 0; i < _facePickingObserver.getFaceSelectedStates().size(); ++i) {
        if (_facePickingObserver.isFaceSelected(i)) {
            selectedFaces.insert(brepData->getFaceIdentifierMap().getFaceIdentifier(i));
        }
    }

    disconnect(_UI.faceSettingsTable->selectionModel(), &QItemSelectionModel::selectionChanged, this,
               &VesselMeshingView::syncFaceSelection);

    _UI.faceSettingsTable->clearSelection();

    for (int row = 0; row < _faceListModel->rowCount(); ++row) {
        crimson::FaceIdentifier faceIdentifierInRow =
            _UI.faceSettingsTable->model()
                ->data(_UI.faceSettingsTable->model()->index(row, 0), FaceListTableModel::FaceIdRole)
                .value<crimson::FaceIdentifier>();
        if (selectedFaces.find(faceIdentifierInRow) != selectedFaces.end()) {
            _UI.faceSettingsTable->selectRow(row);
        }
    }

    connect(_UI.faceSettingsTable->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &VesselMeshingView::syncFaceSelection);
}
