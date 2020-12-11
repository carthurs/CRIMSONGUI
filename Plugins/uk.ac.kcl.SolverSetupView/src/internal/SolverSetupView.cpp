#include <algorithm>
#include <memory>

#include <QMessageBox>

#include <vtkNew.h>
#include <vtkCellData.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkDoubleArray.h>
#include <vtkIntArray.h>
#include <vtkUnstructuredGrid.h>
#include <vtkAbstractMapper.h>

// Main include
#include "SolverSetupView.h"

#include <NodePredicateDerivation.h>
#include <NodePredicateNone.h>

#include <utils/TaskStateObserver.h>

// Module includes
#include <VesselPathAbstractData.h>
#include <VesselForestData.h>
#include <HierarchyManager.h>
#include <VascularModelingNodeTypes.h>
#include <VesselMeshingNodeTypes.h>
#include <SolverSetupNodeTypes.h>
#include <ISolverSetupService.h>
#include <ISolverSetupManager.h>
#include <SolverSetupUtils.h>
#include <QtPropertyStorage.h>

#include <SolidData.h>
#include <MeshData.h>

// MITK
#include <mitkInteractionEventObserver.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateOr.h>
#include <mitkNodePredicateProperty.h>
#include <mitkNodePredicateDataType.h>
#include <mitkVtkScalarModeProperty.h>
#include <mitkTransferFunctionProperty.h>

#include <ctkPopupWidget.h>

#include <QmitkIOUtil.h>

#include <usModule.h>
#include <usModuleRegistry.h>
#include <usModuleContext.h>
#include <usServiceInterface.h>

#include <crimsonparticles.h>

using namespace crimson;

Q_DECLARE_METATYPE(ISolverSetupManager*)

const std::string SolverSetupView::VIEW_ID = "org.mitk.views.SolverSetupView";

void SolverSetupView::RemoveNodeDeleter::operator()(mitk::DataNode* node)
{
    crimson::HierarchyManager::getInstance()->getDataStorage()->Remove(node);
}


SolverSetupView::SolverSetupView()
    : _solverStudyBCSetsModel(SolverSetupNodeTypes::BoundaryConditionSet())
    , _solverStudyMaterialsModel(SolverSetupNodeTypes::Material())
	, _particleTrackingGeometriesModel(VesselMeshingNodeTypes::Mesh())
{
    _typeSelectionDialogUi.setupUi(&_typeSelectionDialog);

    auto context = us::ModuleRegistry::GetModule(1)->GetModuleContext();
    context->AddServiceListener(this, &SolverSetupView::_serviceChanged, "(" + us::ServiceConstants::OBJECTCLASS() + "=" +
                                                                             us_service_interface_iid<ISolverSetupService>() +
                                                                             ")");
}

SolverSetupView::~SolverSetupView()
{
    _materialVisNodePtr.reset(); // Force reset the pointer to trigger NodeRemoved() before any members are destroyed
    auto context = us::ModuleRegistry::GetModule(1)->GetModuleContext();
    context->RemoveServiceListener(this, &SolverSetupView::_serviceChanged);
}

void SolverSetupView::SetFocus() {}

void SolverSetupView::_setupAdvancedParticleTrackingOptionsUI()
{
	ctkPopupWidget* popup = new ctkPopupWidget(_UI.particleAnalysisSettingsButton);
	QVBoxLayout* popupLayout = new QVBoxLayout(popup);

	// The popup for advanced option of controlling which particle tracking stages
	// will be run
	auto runReductionStepCheckBox = new QCheckBox(popup);
	runReductionStepCheckBox->setText("Run fluid problem reduction step.");
	runReductionStepCheckBox->setChecked(true);
	popupLayout->addWidget(runReductionStepCheckBox);
	connect(runReductionStepCheckBox, &QCheckBox::toggled, [this](bool checked) { _runReductionStep = checked; });

	auto writeParticleConfigJsonCheckBox = new QCheckBox(popup);
	writeParticleConfigJsonCheckBox->setText("Write a new particle_config.json using the current configuration.");
	writeParticleConfigJsonCheckBox->setChecked(true);
	popupLayout->addWidget(writeParticleConfigJsonCheckBox);
	connect(writeParticleConfigJsonCheckBox, &QCheckBox::toggled, [this](bool checked) { _writeParticleConfigJson = checked; });

	auto performFluidSimToHdf5CreationStepCheckBox = new QCheckBox(popup);
	performFluidSimToHdf5CreationStepCheckBox->setText("Run fluid problem hdf5 file creation step.");
	performFluidSimToHdf5CreationStepCheckBox->setChecked(true);
	popupLayout->addWidget(performFluidSimToHdf5CreationStepCheckBox);
	connect(performFluidSimToHdf5CreationStepCheckBox, &QCheckBox::toggled, [this](bool checked) { _convertFluidSimToHdf5 = checked; });

	auto runActualParticleTrackingStepCheckBox = new QCheckBox(popup);
	runActualParticleTrackingStepCheckBox->setText("Run actual particle tracking simulation step.");
	runActualParticleTrackingStepCheckBox->setChecked(true);
	popupLayout->addWidget(runActualParticleTrackingStepCheckBox);
	connect(runActualParticleTrackingStepCheckBox, &QCheckBox::toggled, [this](bool checked) { _runActualParticleTrackingStep = checked; });

	popup->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);      // at the top left corner
	popup->setHorizontalDirection(Qt::RightToLeft);               // open outside the parent
	popup->setVerticalDirection(ctkBasePopupWidget::TopToBottom); // at the left of the spinbox sharing the top border
	// Control the animation
	popup->setAnimationEffect(ctkBasePopupWidget::FadeEffect); // could also be FadeEffect
	popup->setOrientation(Qt::Horizontal);                     // how to animate, could be Qt::Vertical or Qt::Horizontal|Qt::Vertical
	popup->setEasingCurve(QEasingCurve::OutQuart);             // how to accelerate the animation, QEasingCurve::Type
	popup->setEffectDuration(100);                             // how long in ms.
	// Control the behavior
	popup->setAutoShow(false); // automatically open when the mouse is over the spinbox
	popup->setAutoHide(true);  // automatically hide when the mouse leaves the popup or the spinbox.

	connect(_UI.particleAnalysisSettingsButton, &QAbstractButton::clicked, [popup]() { popup->showPopup(); });
}

void SolverSetupView::CreateQtPartControl(QWidget* parent)
{
    // create GUI widgets from the Qt Designer's .ui file
    _UI.setupUi(parent);

    connect(_UI.setupSolverButton, &QAbstractButton::clicked, this, &SolverSetupView::writeSolverSetup);
	connect(_UI.runSimulationButton, &QAbstractButton::clicked, this, &SolverSetupView::runSimulation);
	connect(_UI.setupLagrangianParticleTrackingAnalysisButton, &QAbstractButton::clicked, this, &SolverSetupView::setupLagrangianParticleTrackingAnalysis);

	_particleSimulationTaskStateObserver = new crimson::TaskStateObserver(_UI.runLagrangianParticleTrackingAnalysisButton, _UI.cancelLagrangianParticleTrackingAnalysisButton, this);
	connect(_particleSimulationTaskStateObserver, &crimson::TaskStateObserver::runTaskRequested, this, &SolverSetupView::runLagrangianParticleTrackingAnalysis);
	connect(_particleSimulationTaskStateObserver, &crimson::TaskStateObserver::taskFinished, this, &SolverSetupView::finishedLagrangianParticleTrackingAnalysis);
	
	_particleSimulationTaskStateObserver->setEnabled(true);
	const std::string task_id("Running Lagrangian particle tracking");
	_particleSimulationTaskStateObserver->setPrimaryObservedUID(task_id);

	_setupAdvancedParticleTrackingOptionsUI();

    connect(_UI.loadSolutionButton, &QAbstractButton::clicked, this, &SolverSetupView::loadSolution);
    connect(_UI.showSolutionButton, &QAbstractButton::toggled, this, &SolverSetupView::showSolution);

    connect(_UI.createSolverRootButton, &QAbstractButton::clicked, this, &SolverSetupView::createSolverRoot);
    connect(_UI.createSolverParametersButton, &QAbstractButton::clicked, this, &SolverSetupView::createSolverParameters);
    connect(_UI.createSolverStudyButton, &QAbstractButton::clicked, this, &SolverSetupView::createSolverStudy);
    connect(_UI.createBoundaryConditionSetButton, &QAbstractButton::clicked, this,
            &SolverSetupView::createBoundaryConditionSet);
    connect(_UI.createBoundaryConditionButton, &QAbstractButton::clicked, this, &SolverSetupView::createBoundaryCondition);
    connect(_UI.createMaterialButton, &QAbstractButton::clicked, this, &SolverSetupView::createMaterial);
    connect(_UI.showMaterialButton, &QAbstractButton::clicked, this, &SolverSetupView::showMaterial);
    connect(_UI.exportMaterialsButton, &QAbstractButton::clicked, this, &SolverSetupView::exportMaterials);

    _UI.solverPropertyBrowser->setRootIsDecorated(false);

    _UI.meshNodeComboBox->SetDataStorage(GetDataStorage());
    _UI.solverParametersNodeComboBox->SetDataStorage(GetDataStorage());
    _UI.boundaryConditionSetsTableView->setModel(&_solverStudyBCSetsModel);
    _UI.materialsTableView->setModel(&_solverStudyMaterialsModel);
	
	_UI.particleBolusMeshComboBox->SetDataStorage(GetDataStorage());
	_UI.particleMeshesTableView->setModel(&_particleTrackingGeometriesModel);
	_UI.particleMeshesTableView->horizontalHeader()->setSortIndicatorShown(true);
	_UI.particleMeshesTableView->horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);
	_UI.particleMeshesTableView->setSortingEnabled(true);
	
    // Initialize the UI
    _updateUI();
}

void SolverSetupView::_updateUI()
{
    _UI.vesselTreeDependentControls->setEnabled(_currentRootNode != nullptr);
    _UI.editSolverStudyFrame->setEnabled(_currentSolverStudyNode != nullptr);
    _UI.editSolverParametersFrame->setEnabled(_currentSolverParametersNode != nullptr);
    _UI.editBoundaryConditionSetFrame->setEnabled(_currentBoundaryConditionSetNode != nullptr);
    _UI.boundaryConditionEditor->setEnabled(_currentBoundaryConditionNode != nullptr);
    _UI.materialEditor->setEnabled(_currentMaterialNode != nullptr);

    _UI.createSolverRootButton->setEnabled(_currentRootNode != nullptr);
    _UI.createSolverParametersButton->setEnabled(_currentSolverSetupManager != nullptr);
    _UI.createSolverStudyButton->setEnabled(_currentSolverSetupManager != nullptr);
    _UI.createBoundaryConditionSetButton->setEnabled(_currentSolverSetupManager != nullptr);
    _UI.createBoundaryConditionButton->setEnabled(_currentBoundaryConditionSetNode != nullptr &&
                                                  _currentSolverSetupManager != nullptr);
    _UI.createMaterialButton->setEnabled(_currentSolverSetupManager != nullptr);

    _UI.solverRootNodeName->setText(_currentSolverRootNode ? QString::fromStdString(_currentSolverRootNode->GetName())
                                                           : QString());
    _UI.boundaryConditionNodeName->setText(
        _currentBoundaryConditionNode ? QString::fromStdString(_currentBoundaryConditionNode->GetName()) : QString());
    _UI.materialNodeName->setText(_currentMaterialNode ? QString::fromStdString(_currentMaterialNode->GetName()) : QString());
    _UI.boundaryConditionSetNodeName->setText(
        _currentBoundaryConditionSetNode ? QString::fromStdString(_currentBoundaryConditionSetNode->GetName()) : QString());
    _UI.solverParametersNodeName->setText(_currentSolverParametersNode ? QString::fromStdString(_currentSolverParametersNode->GetName())
                                                             : QString());
    _UI.solverStudyNodeName->setText(_currentSolverStudyNode ? QString::fromStdString(_currentSolverStudyNode->GetName())
                                                             : QString());   

    _UI.setupSolverButton->setEnabled(_currentSolverStudyNode && _UI.meshNodeComboBox->GetSelectedNode().IsNotNull() &&
                                      _UI.solverParametersNodeComboBox->GetSelectedNode().IsNotNull());
    _UI.showMaterialButton->setEnabled(_currentSolverStudyNode && _UI.meshNodeComboBox->GetSelectedNode().IsNotNull());
    _UI.solutionManagementFrame->setEnabled(_UI.setupSolverButton->isEnabled());
    _UI.exportMaterialsButton->setEnabled(_materialVisNodePtr.get() != nullptr);

    _UI.showSolutionButton->blockSignals(true);
    _UI.showSolutionButton->setChecked(_UI.solutionVisualizationWidget->getDataNode() != nullptr);
    _UI.showSolutionButton->blockSignals(false);

	_UI.particleMeshesTableView->sortByColumn(0);
}

void SolverSetupView::OnSelectionChanged(berry::IWorkbenchPart::Pointer /*source*/, const QList<mitk::DataNode::Pointer>& nodes)
{
    this->FireNodesSelected(nodes); // Sync selection

    mitk::DataNode* solverRootNode = nullptr;
    mitk::DataNode* solidNode = nullptr;
    mitk::DataNode* vesselTreeNode = nullptr;
    mitk::DataNode* boundaryConditionSetNode = nullptr;
    mitk::DataNode* boundaryConditionNode = nullptr;
    mitk::DataNode* materialNode = nullptr;
    mitk::DataNode* solverParametersNode = nullptr;
    mitk::DataNode* solverStudyNode = nullptr;

    if (nodes.size() > 0) {
        auto hm = HierarchyManager::getInstance();
        auto isNodeOfType = [hm](const mitk::DataNode::Pointer& node, HierarchyManager::NodeType type) {
            return hm->getPredicate(type)->CheckNode(node);
        };

        // Fill up the relevant nodes based on selection
        for (const mitk::DataNode::Pointer& node : nodes) {
            if (isNodeOfType(node, VascularModelingNodeTypes::VesselTree()) && !vesselTreeNode) {
                vesselTreeNode = node;
            } else if (isNodeOfType(node, SolverSetupNodeTypes::SolverRoot()) && !solverRootNode) {
                solverRootNode = node;
                vesselTreeNode = hm->getAncestor(node, VascularModelingNodeTypes::VesselTree());
                solidNode = hm->getAncestor(node, VascularModelingNodeTypes::Solid());
            } else if (isNodeOfType(node, VascularModelingNodeTypes::Solid()) && !solidNode) {
                solidNode = node;
                vesselTreeNode = hm->getAncestor(node, VascularModelingNodeTypes::VesselTree());
                solverRootNode = hm->getAncestor(node, SolverSetupNodeTypes::SolverRoot());
            } else if (isNodeOfType(node, SolverSetupNodeTypes::BoundaryConditionSet()) && !boundaryConditionSetNode) {
                boundaryConditionSetNode = node;
                vesselTreeNode = hm->getAncestor(node, VascularModelingNodeTypes::VesselTree());
                solidNode = hm->getAncestor(node, VascularModelingNodeTypes::Solid());
                solverRootNode = hm->getAncestor(node, SolverSetupNodeTypes::SolverRoot());
            } else if (isNodeOfType(node, SolverSetupNodeTypes::BoundaryCondition()) && !boundaryConditionNode) {
                boundaryConditionNode = node;
                boundaryConditionSetNode = hm->getAncestor(node, SolverSetupNodeTypes::BoundaryConditionSet());
                vesselTreeNode = hm->getAncestor(node, VascularModelingNodeTypes::VesselTree());
                solidNode = hm->getAncestor(node, VascularModelingNodeTypes::Solid());
                solverRootNode = hm->getAncestor(node, SolverSetupNodeTypes::SolverRoot());
            } else if (isNodeOfType(node, SolverSetupNodeTypes::Material()) && !materialNode) {
                materialNode = node;
                vesselTreeNode = hm->getAncestor(node, VascularModelingNodeTypes::VesselTree());
                solidNode = hm->getAncestor(node, VascularModelingNodeTypes::Solid());
                solverRootNode = hm->getAncestor(node, SolverSetupNodeTypes::SolverRoot());
            } else if (isNodeOfType(node, SolverSetupNodeTypes::SolverParameters()) && !solverParametersNode) {
                solverParametersNode = node;
                vesselTreeNode = hm->getAncestor(node, VascularModelingNodeTypes::VesselTree());
                solidNode = hm->getAncestor(node, VascularModelingNodeTypes::Solid());
                solverRootNode = hm->getAncestor(node, SolverSetupNodeTypes::SolverRoot());
            } else if (isNodeOfType(node, SolverSetupNodeTypes::SolverStudy()) && !solverStudyNode) {
                solverStudyNode = node;
                vesselTreeNode = hm->getAncestor(node, VascularModelingNodeTypes::VesselTree());
                solidNode = hm->getAncestor(node, VascularModelingNodeTypes::Solid());
                solverRootNode = hm->getAncestor(node, SolverSetupNodeTypes::SolverRoot());
            }
        }
    }

    // Choose a tab to switch to for the current selection
    std::map<QWidget*, mitk::DataNode*> widgetToNodeMap = {
        {_UI.boundaryConditionSetupTab, boundaryConditionNode},
        {_UI.boundaryConditionSetSetupTab, (boundaryConditionNode != nullptr ? nullptr : boundaryConditionSetNode)},
        {_UI.solverParametersTab, solverParametersNode},
        {_UI.solverStudyTab, solverStudyNode},
        {_UI.materialSetupTab, materialNode},
    };

    bool needTabChange = widgetToNodeMap[_UI.tabWidget->currentWidget()] == nullptr;

    if (needTabChange) {
        auto iter = std::find_if(widgetToNodeMap.begin(), widgetToNodeMap.end(),
                                 [](const std::pair<QWidget*, mitk::DataNode*>& v) { return v.second != nullptr; });
        if (iter != widgetToNodeMap.end()) {
            _UI.tabWidget->setCurrentWidget(iter->first);
        }
    }

    // Set the relevant nodes
    _setCurrentRootNode(vesselTreeNode ? vesselTreeNode : solidNode);
    _setCurrentSolverRootNode(solverRootNode);
    _setCurrentBoundaryConditionSetNode(boundaryConditionSetNode);
    _setCurrentBoundaryConditionNode(boundaryConditionNode);
    _setCurrentMaterialNode(materialNode);
    _setCurrentSolverParametersNode(solverParametersNode);
    _setCurrentSolverStudyNode(solverStudyNode);

    _updateUI();
}

void SolverSetupView::NodeRemoved(const mitk::DataNode* node)
{
    if (node == _currentRootNode) {
        _setCurrentRootNode(nullptr);
    } else if (node == _currentSolverRootNode) {
        _setCurrentSolverRootNode(nullptr);
    } else if (node == _currentBoundaryConditionSetNode) {
        _setCurrentBoundaryConditionSetNode(nullptr);
    } else if (node == _currentBoundaryConditionNode) {
        _setCurrentBoundaryConditionNode(nullptr);
    } else if (node == _currentMaterialNode) {
        _setCurrentMaterialNode(nullptr);
    } else if (node == _currentSolverParametersNode) {
        _setCurrentSolverParametersNode(nullptr);
    } else if (node == _currentSolverStudyNode) {
        _setCurrentSolverStudyNode(nullptr);
    } else if (node == _currentSolidNode) {
        _currentSolidNode = nullptr;
    }

    _updateUI();
}

void SolverSetupView::NodeChanged(const mitk::DataNode* /*node*/) { _updateUI(); }

void SolverSetupView::_setCurrentRootNode(mitk::DataNode* node)
{
    _currentSolidNode = nullptr;

    if (node == nullptr) {
        _currentVesselTreeNode = nullptr;
        _UI.boundaryConditionEditor->setCurrentVesselTreeNode(nullptr);
        _UI.boundaryConditionEditor->setCurrentSolidNode(nullptr);
        _UI.materialEditor->setCurrentVesselTreeNode(nullptr);
        _UI.materialEditor->setCurrentSolidNode(nullptr);
        _currentRootNode = nullptr;
        return;
    }

    // The parent node of the solver root node depends on whether the vessel tree is present.
    // If it isn't - the user is setting up the simulation for an imported solid model.
    if (HierarchyManager::getInstance()->getPredicate(VascularModelingNodeTypes::VesselTree())->CheckNode(node)) {
        _currentSolidNode =
            HierarchyManager::getInstance()->getFirstDescendant(node, VascularModelingNodeTypes::Blend()).GetPointer();

        _currentVesselTreeNode = node;
        _currentRootNodeType = VascularModelingNodeTypes::VesselTree();
    } else {
        _currentSolidNode = node;
        _currentVesselTreeNode = nullptr;
        _currentRootNodeType = VascularModelingNodeTypes::Solid();
    }

    _UI.boundaryConditionEditor->setCurrentSolidNode(_currentSolidNode);
    _UI.boundaryConditionEditor->setCurrentVesselTreeNode(_currentVesselTreeNode);
    _UI.materialEditor->setCurrentSolidNode(_currentSolidNode);
    _UI.materialEditor->setCurrentVesselTreeNode(_currentVesselTreeNode);

    _currentRootNode = node;
}

void SolverSetupView::_setCurrentSolverRootNode(mitk::DataNode* node)
{
    if (node == _currentSolverRootNode) {
        return;
    }

    _currentSolverRootNode = node;

    _findCurrentSolverSetupManager();
}

void SolverSetupView::_findCurrentSolverSetupManager()
{
    _currentSolverSetupManager = nullptr;

    if (!_currentSolverRootNode) {
        return;
    }

    std::string solverType;
    _currentSolverRootNode->GetStringProperty(SolverSetupNodeTypes::solverRootPropertyName, solverType);

    auto moduleContext = us::ModuleRegistry::GetModule(1)->GetModuleContext();
    std::vector<us::ServiceReference<ISolverSetupService>> refs = moduleContext->GetServiceReferences<ISolverSetupService>();

    for (const auto& ref : refs) {
        auto solverSetupService = moduleContext->GetService<ISolverSetupService>(ref);
        gsl::finally([moduleContext, ref]() { moduleContext->UngetService(ref); });

        auto solverSetupManager = solverSetupService->getSolverSetupManager();
        if (gsl::cstring_span<>{solverType} == solverSetupManager->getName()) {
            _currentSolverSetupManager = solverSetupManager;
            return;
        }
    }
}

void SolverSetupView::_setCurrentBoundaryConditionSetNode(mitk::DataNode* node)
{
    if (node == _currentBoundaryConditionSetNode) {
        return;
    }

    _currentBoundaryConditionSetNode = node;
}

void SolverSetupView::_setCurrentBoundaryConditionNode(mitk::DataNode* node)
{
    _UI.boundaryConditionEditor->setCurrentNode(node);

    _currentBoundaryConditionNode = node;
}

void SolverSetupView::_setCurrentMaterialNode(mitk::DataNode* node)
{
    _UI.materialEditor->setCurrentNode(node);

    _currentMaterialNode = node;
}

void SolverSetupView::_setCurrentSolverParametersNode(mitk::DataNode* node)
{
    if (node == _currentSolverParametersNode) {
        return;
    }

    _currentSolverParametersNode = node;

    _UI.solverPropertyBrowser->clear();
    if (_currentSolverParametersNode) {
        auto solverParametersData = static_cast<ISolverParametersData*>(_currentSolverParametersNode->GetData());
        _UI.solverPropertyBrowser->setResizeMode(QtTreePropertyBrowser::ResizeToContents);
        _UI.solverPropertyBrowser->setFactoryForManager(solverParametersData->getPropertyStorage()->getPropertyManager(),
                                                        new QtVariantEditorFactory(_UI.solverPropertyBrowser));
        _UI.solverPropertyBrowser->addProperty(solverParametersData->getPropertyStorage()->getTopProperty());
        _UI.solverPropertyBrowser->setResizeMode(QtTreePropertyBrowser::Interactive);
        solverParametersData->Modified(); // Assume modification when exposing properties to the user
    }
}

void SolverSetupView::_setCurrentSolverStudyNode(mitk::DataNode* node)
{
    if (node == _currentSolverStudyNode) {
        return;
    }

    if (_currentSolverStudyNode) {
        _currentSolverStudyNode->GetData()->RemoveObserver(_solverStudyObserverTag);
        _solverStudyObserverTag = -1;

        disconnect(_UI.meshNodeComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this,
                   &SolverSetupView::setMeshNodeForStudy);
        disconnect(_UI.solverParametersNodeComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this,
                   &SolverSetupView::setSolverParametersNodeForStudy);

		disconnect(_UI.particleBolusMeshComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this,
			       &SolverSetupView::setParticleBolusNodeForStudy);

        _UI.meshNodeComboBox->SetPredicate(NodePredicateNone::New());
        _UI.solverParametersNodeComboBox->SetPredicate(NodePredicateNone::New());
        _solverStudyBCSetsModel.setSolverStudyNode(nullptr);
        _solverStudyMaterialsModel.setSolverStudyNode(nullptr);
		_particleTrackingGeometriesModel.setSolverStudyNode(nullptr);
		_UI.particleBolusMeshComboBox->SetPredicate(NodePredicateNone::New());

        _materialVisNodePtr.reset();
    }

    _currentSolverStudyNode = node;

    if (_currentSolverStudyNode) {
        _setupSolverStudyComboBoxes();
        _UI.boundaryConditionSetsTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        _UI.boundaryConditionSetsTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
        
		_UI.materialsTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        _UI.materialsTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
		
		_UI.particleMeshesTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
		_UI.particleMeshesTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    }
}

void SolverSetupView::_solverStudyNodeModified() { _setupSolverStudyComboBoxes(); }

void SolverSetupView::_selectDataNode(mitk::DataNode* newNode)
{
    this->FireNodeSelected(newNode);
    this->OnSelectionChanged(GetSite()->GetPart(), QList<mitk::DataNode::Pointer>() << newNode);
}

void SolverSetupView::_serviceChanged(const us::ServiceEvent event)
{
    if (event.GetType() == us::ServiceEvent::REGISTERED || event.GetType() == us::ServiceEvent::UNREGISTERING) {
        _findCurrentSolverSetupManager();
        _updateUI();
    }
}

// This traits class is used to unify the creation of different types of nodes for solver setup
template <class T>
struct SolverObjectTraits {
};

template <>
struct SolverObjectTraits<ISolverStudyData> {
    static ISolverStudyData::Pointer create(ISolverSetupManager* manager, gsl::cstring_span<> typeName)
    {
        return manager->createSolverStudy(typeName);
    }
    static ImmutableValueRange<gsl::cstring_span<>> getOptions(ISolverSetupManager* manager)
    {
        return manager->getSolverStudyNameList();
    }
    static gsl::cstring_span<> objectTypeName()
    {
        static const char name[] = "solver study";
        return gsl::ensure_z(name);
    }
    static HierarchyManager::NodeType nodeType() { return SolverSetupNodeTypes::SolverStudy(); }
    static HierarchyManager::NodeType ownerNodeType() { return SolverSetupNodeTypes::SolverRoot(); }
};

template <>
struct SolverObjectTraits<ISolverParametersData> {
    static ISolverParametersData::Pointer create(ISolverSetupManager* manager, gsl::cstring_span<> typeName)
    {
        return manager->createSolverParameters(typeName);
    }
    static ImmutableValueRange<gsl::cstring_span<>> getOptions(ISolverSetupManager* manager)
    {
        return manager->getSolverParametersNameList();
    }
    static gsl::cstring_span<> objectTypeName()
    {
        static const char name[] = "solver parameters";
        return gsl::ensure_z(name);
    }
    static HierarchyManager::NodeType nodeType() { return SolverSetupNodeTypes::SolverParameters(); }
    static HierarchyManager::NodeType ownerNodeType() { return SolverSetupNodeTypes::SolverRoot(); }
};

template <>
struct SolverObjectTraits<IBoundaryConditionSet> {
    static IBoundaryConditionSet::Pointer create(ISolverSetupManager* manager, gsl::cstring_span<> typeName)
    {
        return manager->createBoundaryConditionSet(typeName);
    }
    static ImmutableValueRange<gsl::cstring_span<>> getOptions(ISolverSetupManager* manager)
    {
        return manager->getBoundaryConditionSetNameList();
    }
    static gsl::cstring_span<> objectTypeName()
    {
        static const char name[] = "boundary condition set";
        return gsl::ensure_z(name);
    }
    static HierarchyManager::NodeType nodeType() { return SolverSetupNodeTypes::BoundaryConditionSet(); }
    static HierarchyManager::NodeType ownerNodeType() { return SolverSetupNodeTypes::SolverRoot(); }
};

template <>
struct SolverObjectTraits<IBoundaryCondition> {
    static IBoundaryCondition::Pointer create(ISolverSetupManager* manager, gsl::cstring_span<> typeName,
                                              IBoundaryConditionSet* ownerBCSet)
    {
        return manager->createBoundaryCondition(ownerBCSet, typeName);
    }
    static ImmutableValueRange<gsl::cstring_span<>> getOptions(ISolverSetupManager* manager, IBoundaryConditionSet* ownerBCSet)
    {
        return manager->getBoundaryConditionNameList(ownerBCSet);
    }
    static gsl::cstring_span<> objectTypeName()
    {
        static const char name[] = "boundary condition";
        return gsl::ensure_z(name);
    }
    static HierarchyManager::NodeType nodeType() { return SolverSetupNodeTypes::BoundaryCondition(); }
    static HierarchyManager::NodeType ownerNodeType() { return SolverSetupNodeTypes::BoundaryConditionSet(); }
};

template <>
struct SolverObjectTraits<IMaterialData> {
    static IMaterialData::Pointer create(ISolverSetupManager* manager, gsl::cstring_span<> typeName)
    {
        return manager->createMaterial(typeName);
    }
    static ImmutableValueRange<gsl::cstring_span<>> getOptions(ISolverSetupManager* manager)
    {
        return manager->getMaterialNameList();
    }
    static gsl::cstring_span<> objectTypeName()
    {
        static const char name[] = "material";
        return gsl::ensure_z(name);
    }
    static HierarchyManager::NodeType nodeType() { return SolverSetupNodeTypes::Material(); }
    static HierarchyManager::NodeType ownerNodeType() { return SolverSetupNodeTypes::SolverRoot(); }
};

template <class T, class... Args>
void SolverSetupView::createSolverObject(mitk::DataNode* ownerNode, Args... additionalArgs)
{
    if (!ownerNode || !_currentSolverSetupManager) {
        return;
    }

    // Get the type name for the particular solver object to be created (e.g. the boundary condition type name)
    auto typeName = _getTypeNameToCreate(SolverObjectTraits<T>::getOptions(_currentSolverSetupManager, additionalArgs...));

    if (typeName.size() == 0) {
        return;
    }

    // Create the solver object
    typename T::Pointer newObject = SolverObjectTraits<T>::create(_currentSolverSetupManager, typeName, additionalArgs...);

    if (!newObject) {
        auto objectTypeName = gsl::to_QString(SolverObjectTraits<T>::objectTypeName());
        QMessageBox::information(nullptr, QString{"Creation of %1 failed"}.arg(objectTypeName),
                                 QString{"Failed to create the %1 %2."}.arg(objectTypeName).arg(gsl::to_QString(typeName)));
        return;
    }

    // Create the node for the solver object
    auto node = mitk::DataNode::New();
    node->SetName(gsl::to_string(typeName));
    node->SetData(newObject);

    // Add the node to the hierarchy
    HierarchyManager::getInstance()->addNodeToHierarchy(ownerNode, SolverObjectTraits<T>::ownerNodeType(), node,
                                                        SolverObjectTraits<T>::nodeType());

	//if (gsl::to_string(typeName) == "PCMRI")
	//{
	//	//find the node with PCMRIUID and reparent it to belong to newly created BC node
	//	std::string uid = newObject->getObjectNodeUID();
	//	auto pcmriNode = HierarchyManager::getInstance()->findNodeByUID(uid);
	//	HierarchyManager::getInstance()->reparentNode(pcmriNode, node);
	//}

    _selectDataNode(node);
}

void SolverSetupView::createSolverRoot()
{
    Expects(_currentRootNode != nullptr);

    auto moduleContext = us::ModuleRegistry::GetModule(1)->GetModuleContext();
    std::vector<us::ServiceReference<ISolverSetupService>> refs = moduleContext->GetServiceReferences<ISolverSetupService>();

    auto managerNames = std::vector<gsl::cstring_span<>>{};
    for (const auto& ref : refs) {
        auto solverSetupService = moduleContext->GetService<ISolverSetupService>(ref);

        auto solverSetupManager = solverSetupService->getSolverSetupManager();
        managerNames.push_back(solverSetupManager->getName());

        moduleContext->UngetService(ref);
    }

    // Let the user choose with solver type to create
    auto managerName = _getTypeNameToCreate(managerNames);
    if (managerName.size() == 0) {
        return;
    }

    auto node = mitk::DataNode::New();
    node->SetName(gsl::to_string(managerName));

    node->SetStringProperty(SolverSetupNodeTypes::solverRootPropertyName, gsl::to_string(managerName).c_str());
    node->SetBoolProperty("show empty data", true); // Solver root only contains the name of the solver type and no data

    HierarchyManager::getInstance()->addNodeToHierarchy(_currentRootNode, _currentRootNodeType, node,
                                                        SolverSetupNodeTypes::SolverRoot());
    _selectDataNode(node);
}

void SolverSetupView::createSolverParameters()
{
    Expects(_currentSolverSetupManager != nullptr);
    createSolverObject<ISolverParametersData>(_currentSolverRootNode);
}

void SolverSetupView::createSolverStudy()
{
    Expects(_currentSolverSetupManager != nullptr);
    createSolverObject<ISolverStudyData>(_currentSolverRootNode);
}

void SolverSetupView::createBoundaryConditionSet()
{
    Expects(_currentSolverSetupManager != nullptr);
    createSolverObject<IBoundaryConditionSet>(_currentSolverRootNode);
    _UI.tabWidget->setCurrentWidget(_UI.boundaryConditionSetupTab);
}

void SolverSetupView::createBoundaryCondition()
{
    Expects(_currentSolverSetupManager != nullptr && _currentBoundaryConditionSetNode != nullptr);
    auto bcSet = static_cast<IBoundaryConditionSet*>(_currentBoundaryConditionSetNode->GetData());
    createSolverObject<IBoundaryCondition>(_currentBoundaryConditionSetNode, bcSet);
}

void SolverSetupView::createMaterial()
{
    Expects(_currentSolverSetupManager != nullptr);
    createSolverObject<IMaterialData>(_currentSolverRootNode);
}

Q_DECLARE_METATYPE(gsl::cstring_span<>);

gsl::cstring_span<> SolverSetupView::_getTypeNameToCreate(ImmutableValueRange<gsl::cstring_span<>> options)
{
    _typeSelectionDialogUi.typesListWidget->clear();

    if (std::distance(options.begin(), options.end()) == 1) {
        return options.front();
    }

    for (const auto& optionName : options) {
        auto item = new QListWidgetItem(gsl::to_QString(optionName));
        item->setData(Qt::UserRole, QVariant::fromValue(optionName));
        _typeSelectionDialogUi.typesListWidget->addItem(item);
    }

    if (_typeSelectionDialog.exec() != QDialog::Accepted || _typeSelectionDialogUi.typesListWidget->selectedItems().empty()) {
        return {};
    }

    return qvariant_cast<gsl::cstring_span<>>(_typeSelectionDialogUi.typesListWidget->selectedItems()[0]->data(Qt::UserRole));
}

namespace detail
{
    class HierarchyManagerDataProvider : public IDataProvider {
    public:
        mitk::BaseData* findDataByUID(gsl::cstring_span<> uid) const override
        {
            auto node = crimson::HierarchyManager::getInstance()->findNodeByUID(uid);
            if (node) {
                return node->GetData();
            }

            // Support finding vessel paths by their PATHUID
            auto vesselPaths = crimson::HierarchyManager::getInstance()->getDataStorage()->GetSubset(mitk::TNodePredicateDataType<VesselPathAbstractData>::New())->CastToSTLConstContainer();
            for (const auto& vesselPathNode : vesselPaths) {
                if (gsl::as_temp_span(static_cast<VesselPathAbstractData*>(vesselPathNode->GetData())->getVesselUID()) == uid) {
                    return vesselPathNode->GetData();
                }
            }

            return nullptr;
        }
        std::vector<mitk::BaseData*> getChildrenData(gsl::cstring_span<> parentUID) const override
        {
            auto node = crimson::HierarchyManager::getInstance()->findNodeByUID(parentUID);

            std::vector<mitk::BaseData*> childDatas;

            if (!node) {
                return childDatas;
            }


            auto children = crimson::HierarchyManager::getInstance()->getDataStorage()->GetDerivations(node);
            std::transform(children->begin(), children->end(), std::back_inserter(childDatas),
                [](const mitk::DataNode::Pointer& node) { return node->GetData(); });

            return childDatas;
        }
    };
}

void SolverSetupView::_ensureApplyToAllWalls(mitk::DataNode* node) const
{
    // Make sure that applyToAllWalls flag is respected under the changes to the model
    bool applyToAllWalls = false;
    if (node->GetBoolProperty("bc.applyToAllWalls", applyToAllWalls) && applyToAllWalls) {
        FaceDataEditorWidget::applyToAllWalls(_currentSolidNode, node);
    }
}

void SolverSetupView::runSimulation() {
	auto solverStudyData = static_cast<crimson::ISolverStudyData*>(_currentSolverStudyNode->GetData());
	
	if (!solverStudyData->runFlowsolver()) {
		QMessageBox::critical(nullptr, "Error running simulation",
			"An error has occurred whilst attempting to run a simulation.\n"
			"Please see log for details.");
	}
}

void SolverSetupView::setupLagrangianParticleTrackingAnalysis() {
	const bool isParticleProblem = true;
	_setupSimulationProblem(isParticleProblem);
}

std::shared_ptr<crimson::async::TaskWithResult<bool>> SolverSetupView::createParticleTrackingTask(const bool runReductionStep,
	const bool writeParticleConfigJson,
	const bool convertFluidSimToHdf5,
	const bool runActualParticleTrackingStep,
	const std::string config_file)
{

	return std::static_pointer_cast<crimson::async::TaskWithResult<bool>>(
		std::make_shared<::detail::ParticleTrackingTask>(runReductionStep,
		writeParticleConfigJson,
		convertFluidSimToHdf5,
		runActualParticleTrackingStep,
		config_file)
		);

}

void SolverSetupView::runLagrangianParticleTrackingAnalysis() {

	if (!::detail::checkCrimsonParticlesAvailable())
	{
		return;
	}

	auto solverStudyData = static_cast<crimson::ISolverStudyData*>(_currentSolverStudyNode->GetData());

	const std::string particle_tracking_simulation_folder = solverStudyData->setupParticleTrackingPathsAndGetParticleTrackingFolder();
	
	const bool got_config_file = (particle_tracking_simulation_folder.size() != 0);
	if (got_config_file) {

		std::shared_ptr<async::TaskWithResult<bool>> particleTrackingTask = createParticleTrackingTask(_runReductionStep,
			_writeParticleConfigJson,
			_convertFluidSimToHdf5,
			_runActualParticleTrackingStep,
			particle_tracking_simulation_folder + "\\particle_config.json");
		const std::string task_id("Running Lagrangian particle tracking");
		AsyncTaskManager::getInstance()->addTask(std::make_shared<QAsyncTaskAdapter>(particleTrackingTask), task_id);
	}
	else {
		MITK_ERROR << "Failed to get particle simulation directory or particle_config.json file therein.\nDid you run the Setup Particle Analysis step, and then target that folder for particle simulation?";
		QMessageBox::critical(nullptr, "Failed to find necessary files.",
			"Particle simulation files not found.\nSee the log window for details.");
	}
}

void SolverSetupView::finishedLagrangianParticleTrackingAnalysis(crimson::async::Task::State state) {
	MITK_INFO << "Lagrangian particle tracking complete.";
}

void SolverSetupView::writeSolverSetup()
{
	const bool isParticleProblem = false;
	_setupSimulationProblem(isParticleProblem);
}

void SolverSetupView::_setupSimulationProblem(const bool isParticleProblem)
{
	if (isParticleProblem && !::detail::checkCrimsonParticlesAvailable()) {
		return;
	}

    if (!_currentSolverSetupManager) {
        return;
    }

    if (!_currentSolidNode) {
        QMessageBox::warning(nullptr, "Cannot write solver output",
                             "Please create a blended solid for the current vessel tree.");
        return;
    }

    if (!_UI.meshNodeComboBox->GetSelectedNode()) {
        QMessageBox::warning(nullptr, "Cannot write solver output", "Please select a mesh for the study.");
        return;
    }

    auto solverStudyData = static_cast<ISolverStudyData*>(_currentSolverStudyNode->GetData());

    // Ensure the applyToAllWalls flag is respected for all BC's
    for (auto node : _solverStudyBCSetsModel.getCheckedNodes()) {
        auto bcNodes = HierarchyManager::getInstance()->getDescendants(node, SolverSetupNodeTypes::BoundaryCondition());

        for (const auto& bcNode : *bcNodes) {
            _ensureApplyToAllWalls(bcNode);
        }
    }

    // Ensure the applyToAllWalls flag is respected for all materials
    for (auto materialNode : _solverStudyMaterialsModel.getCheckedNodes()) {
        _ensureApplyToAllWalls(materialNode);
    }

    auto hm = HierarchyManager::getInstance();
    mitk::DataNode* vesselForestNode = hm->getAncestor(_currentSolidNode, VascularModelingNodeTypes::VesselTree());

    VesselForestData* vesselForestData = nullptr;
    if (vesselForestNode) {
        vesselForestData = static_cast<VesselForestData*>(vesselForestNode->GetData());

        auto blendNodeTopologyMTime = 0;
        _currentSolidNode->GetIntProperty("vesselForestTopologyMTime", blendNodeTopologyMTime);

        if (blendNodeTopologyMTime != vesselForestData->getTopologyMTime()) {
            if (QMessageBox::warning(nullptr, "Vessel tree topology inconsistent",
                                     "Vessel tree's topology has changed after the blend was created.\n"
                                     "This may lead to incorrect solver setup.\n"
                                     "Continue with writing solver setup?",
                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {

                return;
            }
        }
    }

    auto dataProvider = ::detail::HierarchyManagerDataProvider{};

    auto solutions = std::vector<const SolutionData*>{};

    auto solutionNodes = hm->getDescendants(_currentSolverStudyNode, SolverSetupNodeTypes::Solution());
    for (const auto& solutionNodePtr : *solutionNodes) {
        solutions.push_back(static_cast<const SolutionData*>(solutionNodePtr->GetData()));
    }

    if (!solverStudyData->writeSolverSetup(dataProvider, vesselForestData, _currentSolidNode->GetData(), solutions, isParticleProblem)) {
        QMessageBox::critical(nullptr, "Error writing solver setup",
                              "An error has occurred during the process of writing solver setup.\n"
                              "Please see log for details.");
    }
}

void SolverSetupView::loadSolution()
{
    Expects(_currentSolverStudyNode != nullptr);

    // Load the solutions
    auto solutionDataPtrs = static_cast<ISolverStudyData*>(_currentSolverStudyNode->GetData())->loadSolution();

    // Add the solutions to the hierarchy
    auto hm = crimson::HierarchyManager::getInstance();

    auto existingSolutionNodes = hm->getDescendants(_currentSolverStudyNode, SolverSetupNodeTypes::Solution(), true);

    auto findExistingNodeWithName = [&existingSolutionNodes](gsl::cstring_span<> name) {
        return std::find_if(
            existingSolutionNodes->begin(), existingSolutionNodes->end(), [&name](const mitk::DataNode::Pointer& node) {
                return gsl::ensure_z<const char>(static_cast<SolutionData*>(node->GetData())->getArrayData()->GetName()) ==
                       name;
            });
    };

    // Arrays with the same name under the same study are not allowed. 
    // Let the user decide whether to replace the existing solutions.
    auto replaceAll = QMessageBox::NoButton;
    for (const auto& solutionDataPtr : solutionDataPtrs) {
        auto name = solutionDataPtr->getArrayData()->GetName();

        auto existingNodeIter = findExistingNodeWithName(gsl::ensure_z<const char>(name));
        if (existingNodeIter != existingSolutionNodes->end()) {
            if (replaceAll != QMessageBox::NoToAll && replaceAll != QMessageBox::YesToAll) {
                replaceAll = QMessageBox::question(
                    nullptr, "Replace existing solution?",
                    QString("Solution data with array name '%1' already exists in the study.\nWould you like to replace it?")
                        .arg(name),
                    QMessageBox::Yes | QMessageBox::YesAll | QMessageBox::No | QMessageBox::NoAll | QMessageBox::Cancel);
            }

            switch (replaceAll) {
            case QMessageBox::No:
            case QMessageBox::NoToAll:
                continue;
            case QMessageBox::Yes:
            case QMessageBox::YesToAll:
                GetDataStorage()->Remove(existingNodeIter->GetPointer());
                break;
            case QMessageBox::Cancel:
                return;
            }
        }

        auto node = mitk::DataNode::New();
        node->SetName(name);
        node->SetData(solutionDataPtr.GetPointer());
        hm->addNodeToHierarchy(_currentSolverStudyNode, SolverSetupNodeTypes::SolverStudy(), node.GetPointer(),
                               SolverSetupNodeTypes::Solution());
    }
}

void SolverSetupView::showSolution(bool show)
{
    if (!show) {
        _UI.solutionVisualizationWidget->resetDataNode();
        _updateUI();
        return;
    }

    Expects(_currentSolverStudyNode != nullptr);

    auto hm = crimson::HierarchyManager::getInstance();

    // Todo: remove duplication with MeshAdaptView
    auto solverStudyData = static_cast<ISolverStudyData*>(_currentSolverStudyNode->GetData());
    auto meshNode = HierarchyManager::getInstance()->findNodeByUID(solverStudyData->getMeshNodeUID());
    auto meshData = static_cast<MeshData*>(meshNode->GetData());

    // Get all solution arrays from study
    auto solutionNodes = hm->getDescendants(_currentSolverStudyNode, crimson::SolverSetupNodeTypes::Solution());

    auto unstructuredGridData = meshData->getUnstructuredGridRepresentation()->GetVtkUnstructuredGrid();
    for (const auto& solutionNode : *solutionNodes) {
        auto arrayData = static_cast<crimson::SolutionData*>(solutionNode->GetData())->getArrayData();

        if (arrayData->GetNumberOfTuples() != meshData->getNNodes()) {
            MITK_WARN << "Data array " << arrayData->GetName() << " has different number of elements than the mesh. Skipping";
            continue;
        }

        unstructuredGridData->GetPointData()->AddArray(arrayData);
    }

    _UI.solutionVisualizationWidget->setDataNode(meshNode, unstructuredGridData, "TransferFunction", vtkDataObject::POINT);
    meshNode->SetVisibility(true);
}

void SolverSetupView::showMaterial()
{
    Expects(_currentSolverSetupManager != nullptr && _currentSolidNode != nullptr && _currentSolverStudyNode != nullptr);

    auto dataProvider = ::detail::HierarchyManagerDataProvider{};

    auto hm = HierarchyManager::getInstance();
    mitk::DataNode* vesselForestNode = hm->getAncestor(_currentSolidNode, VascularModelingNodeTypes::VesselTree());

    // TODO: remove duplication with writeSolverSetup
    for (auto materialNode : _solverStudyMaterialsModel.getCheckedNodes()) {
        _ensureApplyToAllWalls(materialNode);
    }

    auto solverStudyData = static_cast<ISolverStudyData*>(_currentSolverStudyNode->GetData());
    auto materials = solverStudyData
                         ->computeMaterials(dataProvider, vesselForestNode ? vesselForestNode->GetData() : nullptr,
                                            _currentSolidNode->GetData());

    auto meshNode = HierarchyManager::getInstance()->findNodeByUID(solverStudyData->getMeshNodeUID());
    meshNode->SetVisibility(false);
    _currentSolidNode->SetVisibility(false);
    auto meshData = static_cast<MeshData*>(meshNode->GetData());

    // Create a temporary mitk::Surface node containing the surface of the mesh.
    auto newNode = mitk::DataNode::New();
    newNode->SetData(meshData->getSurfaceRepresentation());

    auto polyData = meshData->getSurfaceRepresentation()->GetVtkPolyData();
    auto cellData = polyData->GetCellData();
    auto originalFaceIdsArray = static_cast<vtkIntArray*>(cellData->GetArray("originalFaceIds"));
    Expects(cellData->GetArray("originalFaceIds")->GetDataType() == VTK_INT);

    // Get the surface-only values for materials from the data
    std::array<double, 2> range;
    for (const auto &materialData : materials) {
        if (materialData->getArrayData()->GetDataType() != VTK_DOUBLE) {
            MITK_ERROR << "Only double-valued arrays are supported for materials.";
            continue;
        }

        vtkNew<vtkDoubleArray> filteredArray;
        filteredArray->SetNumberOfComponents(materialData->getArrayData()->GetNumberOfComponents());
        filteredArray->SetNumberOfTuples(originalFaceIdsArray->GetNumberOfTuples());
        filteredArray->SetName(materialData->getArrayData()->GetName());

        for (int i = 0; i < filteredArray->GetNumberOfComponents(); ++i) {
            filteredArray->SetComponentName(i, materialData->getArrayData()->GetComponentName(i));
        }

        for (int i = 0; i < originalFaceIdsArray->GetNumberOfTuples(); ++i) {
            auto originalId = originalFaceIdsArray->GetValue(i);
            filteredArray->SetTuple(i, materialData->getArrayData()->GetTuple(originalId));
        }

        cellData->AddArray(filteredArray.GetPointer());
        if (materialData->getArrayData()->GetNumberOfComponents() == 1) {
            filteredArray->GetRange(range.data());
        }
    }
    newNode->SetName("Material vis node");
    newNode->SetVisibility(true);
    newNode->SetBoolProperty("helper object", true);
    GetDataStorage()->Add(newNode, _currentSolverStudyNode);
    _UI.materialVisualizationWidget->setDataNode(newNode, polyData, "Surface.TransferFunction", vtkDataObject::CELL, _currentSolverStudyNode);

    _materialVisNodePtr.reset(newNode);
}

void SolverSetupView::exportMaterials()
{
    QmitkIOUtil::Save(_materialVisNodePtr->GetData(), QString::fromLatin1("materials"));
}

void SolverSetupView::setMeshNodeForStudy(const mitk::DataNode* node)
{
    _updateUI();
    if (!node || !_currentSolverStudyNode) {
        return;
    }
    std::string uid;
    node->GetStringProperty(crimson::HierarchyManager::nodeUIDPropertyName, uid);
    static_cast<ISolverStudyData*>(_currentSolverStudyNode->GetData())->setMeshNodeUID(uid);
}


void SolverSetupView::setParticleBolusNodeForStudy(const mitk::DataNode* node)
{
	_updateUI();
	if (!node || !_currentSolverStudyNode) {
		return;
	}
	std::string uid;
	node->GetStringProperty(crimson::HierarchyManager::nodeUIDPropertyName, uid);
	static_cast<ISolverStudyData*>(_currentSolverStudyNode->GetData())->setParticleBolusMeshNodeUID(uid);
}


void SolverSetupView::setSolverParametersNodeForStudy(const mitk::DataNode* node)
{
    _updateUI();
    if (!node || !_currentSolverStudyNode) {
        return;
    }
    std::string uid;
    node->GetStringProperty(crimson::HierarchyManager::nodeUIDPropertyName, uid);
    static_cast<ISolverStudyData*>(_currentSolverStudyNode->GetData())->setSolverParametersNodeUID(uid);
}

void SolverSetupView::_setupSolverStudyComboBoxes()
{
    _materialVisNodePtr.reset();

    // TODO: remove duplication
    auto solverStudyData = static_cast<ISolverStudyData*>(_currentSolverStudyNode->GetData());

	disconnect(_UI.particleBolusMeshComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this,
			   &SolverSetupView::setParticleBolusNodeForStudy);
    disconnect(_UI.meshNodeComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this,
               &SolverSetupView::setMeshNodeForStudy);
    disconnect(_UI.solverParametersNodeComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this,
               &SolverSetupView::setSolverParametersNodeForStudy);

    if (_solverStudyObserverTag != -1) {
        _currentSolverStudyNode->GetData()->RemoveObserver(_solverStudyObserverTag);
        _solverStudyObserverTag = -1;
    }

	auto particleMeshPredicate = HierarchyManager::getInstance()->getPredicate(VesselMeshingNodeTypes::Mesh());
	_UI.particleBolusMeshComboBox->SetPredicate(particleMeshPredicate);
	auto particleBolusMeshUID = solverStudyData->getParticleBolusMeshNodeUID();

	if (particleBolusMeshUID.size() != 0) {
		mitk::DataNode* nodeToSelect = HierarchyManager::getInstance()->findNodeByUID(particleBolusMeshUID);
		if (nodeToSelect) {
			_UI.particleBolusMeshComboBox->SetSelectedNode(nodeToSelect);
		}
	}
	setParticleBolusNodeForStudy(_UI.particleBolusMeshComboBox->GetSelectedNode());

    // Allowed meshes are the children of the root node
    auto meshPredicate = mitk::NodePredicateAnd::New(
        HierarchyManager::getInstance()->getPredicate(VesselMeshingNodeTypes::Mesh()),
        NodePredicateDerivation::New(_currentRootNode, true, GetDataStorage()));

    _UI.meshNodeComboBox->SetPredicate(meshPredicate);
    auto meshNodeUID = solverStudyData->getMeshNodeUID();
    if (meshNodeUID.size() != 0) {
        mitk::DataNode* nodeToSelect = HierarchyManager::getInstance()->findNodeByUID(meshNodeUID);
        if (nodeToSelect) {
            _UI.meshNodeComboBox->SetSelectedNode(nodeToSelect);
        }
    }
    setMeshNodeForStudy(_UI.meshNodeComboBox->GetSelectedNode());

    _UI.solverParametersNodeComboBox->SetPredicate(
        mitk::NodePredicateAnd::New(HierarchyManager::getInstance()->getPredicate(SolverSetupNodeTypes::SolverParameters()),
                                    NodePredicateDerivation::New(_currentSolverRootNode, true, GetDataStorage())));
    auto solverParametersNodeUID = solverStudyData->getSolverParametersNodeUID();
    if (solverParametersNodeUID.size() != 0) {
        mitk::DataNode* nodeToSelect = HierarchyManager::getInstance()->findNodeByUID(solverParametersNodeUID);
        if (nodeToSelect) {
            _UI.solverParametersNodeComboBox->SetSelectedNode(nodeToSelect);
        }
    }
    setSolverParametersNodeForStudy(_UI.solverParametersNodeComboBox->GetSelectedNode());

    connect(_UI.meshNodeComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this, &SolverSetupView::setMeshNodeForStudy);
	connect(_UI.particleBolusMeshComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this, &SolverSetupView::setParticleBolusNodeForStudy);
    connect(_UI.solverParametersNodeComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this,
            &SolverSetupView::setSolverParametersNodeForStudy);

    _solverStudyBCSetsModel.setSolverStudyNode(_currentSolverStudyNode);
    _solverStudyMaterialsModel.setSolverStudyNode(_currentSolverStudyNode);
	_particleTrackingGeometriesModel.setSolverStudyNode(_currentSolverStudyNode);

    auto modifiedCommand = itk::SimpleMemberCommand<SolverSetupView>::New();
    modifiedCommand->SetCallbackFunction(this, &SolverSetupView::_solverStudyNodeModified);
    _solverStudyObserverTag = _currentSolverStudyNode->GetData()->AddObserver(itk::ModifiedEvent(), modifiedCommand);
}

namespace detail {
	bool checkCrimsonParticlesAvailable()
	{
		if (crimsonparticles::is_available() == crimsonparticles::ReturnCode::NotAvailable)
		{
			MITK_ERROR << "Lagrangian Particle Tracking not found on this computer." << std::endl;
			MITK_ERROR << "Please acquire a suitable executable. Contact CRIMSON Technologies for advice." << std::endl;

			QMessageBox::critical(nullptr, "Particle Tracking Not Available",
				"Lagrangian Particle Tracking is not available on this computer.\n"
				"Please see log window for details.");

			return false;
		}
		else
		{
			MITK_INFO << "Lagrangian Particle Tracking is available." << std::endl;
			return true;
		}
	}

	bool callIfEnabledAndReportFailure(const bool enabled,
		const std::function<crimsonparticles::ReturnCode(const std::string)> task,
		const std::string config_filename,
		const std::string humanReadableTaskInfo)
	{
		if (enabled) {
			MITK_INFO << "Running " << humanReadableTaskInfo << ".";
			crimsonparticles::ReturnCode return_code = task(config_filename);
			const bool failure = crimsonparticles::error_occurred(return_code);
			return failure;
		}
		else {
			MITK_INFO << "Skipping " << humanReadableTaskInfo << ", as per user request.";
		}

		const bool failure = false;
		return failure;
	}

	std::tuple<async::Task::State, std::string> ParticleTrackingTask::runTask()
	{
		if (!checkCrimsonParticlesAvailable())
		{
			return std::make_tuple(State_Failed, std::string("Lagrangian Particle Tracking not available.\nSee log window for details."));
		}

		crimsonparticles::ReturnCode docker_desktop_check_return_code = crimsonparticles::is_docker_server_running(_config_file);
		if (docker_desktop_check_return_code == crimsonparticles::ReturnCode::Ok)
		{
			MITK_INFO << "Docker Desktop is running." << std::endl;
		}
		else if (docker_desktop_check_return_code == crimsonparticles::ReturnCode::CwdWriteFailure)
		{
			MITK_ERROR << "Could not write to the current folder.\nTry running from a folder in which you have write permissions.\nUse the flag --help to see options." << std::endl;
			setResult(false);
			return std::make_tuple(State_Failed, std::string("Failure writing to the selected folder\nSee the log window for details."));
		}
		else if (docker_desktop_check_return_code == crimsonparticles::ReturnCode::StatusRetrievalFailed)
		{
			MITK_ERROR << "No exit status was available. Do you have a suitable Lagrangian particle tracking executable in place on your system?" << std::endl;
			setResult(false);
			return std::make_tuple(State_Failed, std::string("Failure getting exit status of particle tracking executable.\nSee the log window for details."));
		}
		else if (docker_desktop_check_return_code == crimsonparticles::ReturnCode::NotRunning)
		{
			MITK_ERROR << "Docker Desktop does not seem to be running. Please start it from your start menu, and try again. Download and install it if necessary (requires Windows 10 Pro). A Lagrangian particles Docker image is also required." << std::endl;
			setResult(false);
			return std::make_tuple(State_Failed, std::string("Docker Destkop is not running. See log window for details."));
		}
		else
		{
			MITK_ERROR << "An unknown error occurred running Lagrangian particle tracking. Please ensure that you have Docker running, a Lagrangian particles Docker image installed, and a working particle tracking binary installed. Contact the developers for support." << std::endl;
			setResult(false);
			return std::make_tuple(State_Failed, std::string("An unknown error occurred. See log window for details."));
		}

		bool failure;
		failure = crimsonparticles::error_occurred(crimsonparticles::remove_existing_logfile(_config_file));
		if (failure) { setResult(false); return std::make_tuple(State_Failed, std::string("Failure removing existing logfile.")); }

		const crimsonparticles::ReturnCode system_check_return_code = crimsonparticles::check_system(_config_file);
		if (system_check_return_code == crimsonparticles::ReturnCode::Ok)
		{
			MITK_INFO << "System check passed successfully.";
		}
		else if (system_check_return_code == crimsonparticles::ReturnCode::RequestedTooManyCpuCores)
		{
			MITK_ERROR << "Requested more CPU cores for the particle simulation than Docker is configured to provide."
				" Recommend using one fewer core than Docker has available. Please either reduce the number of cores"
				" you requested in Solver Parameters and re-run Setup Particle Analysis, or configure Docker Desktop"
				" to use more cores. Note that if your CPU supports hyperthreading, only half as many cores as you"
				" assign in the Docker Desktop configuration are physical cores, and so you should not request more"
				" than half that number in your CRIMSON Solver Setup.";
			setResult(false);
			return std::make_tuple(State_Failed, std::string("Requested too many CPU cores.\n See the log window for details."));
		}

		if (isCancelling()) {
			const std::string cancellation_message("Lagrangian particle tracking cancelled by user.");
			MITK_INFO << cancellation_message;
			return std::make_tuple(State_Cancelled, cancellation_message);
		}

		failure |= callIfEnabledAndReportFailure(_runReductionStep, crimsonparticles::run_multivispostsolver, _config_file, "fluid problem reduction step");
		if (failure) { setResult(false); return std::make_tuple(State_Failed, std::string("Failure during fluid problem reduction step.")); }

		if (isCancelling()) {
			const std::string cancellation_message("Lagrangian particle tracking cancelled by user, after the fluid problem reduction step.");
			MITK_INFO << cancellation_message;
			return std::make_tuple(State_Cancelled, cancellation_message);
		}

		failure |= crimsonparticles::error_occurred(crimsonparticles::run_mesh_extract(_config_file));
		if (failure) { setResult(false); return std::make_tuple(State_Failed, std::string("Failure during mesh extraction.")); }

	{
		auto writeConfigForFluidProblemFunction = std::bind(crimsonparticles::write_particle_config_json_for_mesh_extraction,
			std::placeholders::_1,
			crimsonparticles::FLUID_PROBLEM);

		failure |= callIfEnabledAndReportFailure(_writeParticleConfigJson, writeConfigForFluidProblemFunction,
			_config_file, "creation of particle_config.json for fluid problem");
		if (failure) { setResult(false); return std::make_tuple(State_Failed, std::string("Failure during writing of particle_config.json")); }

		if (isCancelling()) {
			const std::string cancellation_message("Lagrangian particle tracking cancelled by user, after the particle_config.json was written.");
			MITK_INFO << cancellation_message;
			return std::make_tuple(State_Cancelled, cancellation_message);
		}
	}

	failure |= crimsonparticles::error_occurred(crimsonparticles::extract_fluid_mesh(_config_file));
	if (failure) { setResult(false); return std::make_tuple(State_Failed, std::string("Failure extracting fluid mesh.")); }
	failure |= crimsonparticles::error_occurred(crimsonparticles::write_particle_config_json_for_mesh_extraction(_config_file, crimsonparticles::BOLUS_OR_BIN));
	if (failure) { setResult(false); return std::make_tuple(State_Failed, std::string("Failure writing patricle_config.json for mesh extraction.")); }
	failure |= crimsonparticles::error_occurred(crimsonparticles::extract_particle_mesh(_config_file));
	if (failure) { setResult(false); return std::make_tuple(State_Failed, std::string("Failure extracting particle mesh.")); }

	failure |= callIfEnabledAndReportFailure(_convertFluidSimToHdf5, crimsonparticles::convert_fluid_sim_to_hdf5,
		_config_file, "creation of hdf5 file for fluid problem");
	if (failure) { setResult(false); return std::make_tuple(State_Failed, std::string("Failure during hdf5 fluid simulation file creation.")); }

	if (isCancelling()) {
		const std::string cancellation_message("Lagrangian particle tracking cancelled by user, after the fluid problem hdf5 file creation step.");
		MITK_INFO << cancellation_message;
		return std::make_tuple(State_Cancelled, cancellation_message);
	}

	failure |= crimsonparticles::error_occurred(crimsonparticles::emplace_particle_config_json(_config_file));
	if (failure) { setResult(false); return std::make_tuple(State_Failed, std::string("Failure placing particle_config.json")); }
	failure |= crimsonparticles::error_occurred(crimsonparticles::extract_bin_meshes(_config_file));
	if (failure) { setResult(false); return std::make_tuple(State_Failed, std::string("Failure extracting particle bin meshes.")); }

	if (isCancelling()) {
		const std::string cancellation_message("Lagrangian particle tracking cancelled by user, before the Lagrangian particle tracking step.");
		MITK_INFO << cancellation_message;
		return std::make_tuple(State_Cancelled, cancellation_message);
	}

	failure |= callIfEnabledAndReportFailure(_runActualParticleTrackingStep, crimsonparticles::run_particle_tracking,
		_config_file, "particle tracking simulation");
	if (failure) { setResult(false); return std::make_tuple(State_Failed, std::string("Failure running Lagrangian particle tracking step.")); }

	if (isCancelling()) {
		const std::string cancellation_message("Lagrangian particle tracking cancelled by user, after the Lagrangian particle tracking step.");
		MITK_INFO << cancellation_message;
		return std::make_tuple(State_Cancelled, cancellation_message);
	}

	failure |= crimsonparticles::error_occurred(crimsonparticles::extract_final_data(_config_file));
	if (failure) { setResult(false); return std::make_tuple(State_Failed, std::string("Failure extracting final timestep particle state.")); }
	failure |= crimsonparticles::error_occurred(crimsonparticles::finish(_config_file));
	if (failure) { setResult(false); return std::make_tuple(State_Failed, std::string("Failure during finalisation of particle tracking.")); }


	auto boolToYesNo = [](const bool mybool){return mybool ? std::string("yes") : std::string("no"); };
	MITK_INFO << "===================================================";
	MITK_INFO << "===== User-defined configuration that was used: ===";
	MITK_INFO << "===================================================";
	MITK_INFO << "* Fluid problem reduction step was run - " << boolToYesNo(_runReductionStep);
	MITK_INFO << "* Master particle_config.json was created - " << boolToYesNo(_writeParticleConfigJson);
	MITK_INFO << "* Fluid problem hdf5 file was created - " << boolToYesNo(_convertFluidSimToHdf5);
	MITK_INFO << "* Actual particle simulation was performed - " << boolToYesNo(_runActualParticleTrackingStep);
	MITK_INFO << "===================================================";

	return std::make_tuple(State_Finished, std::string("Particles task finished."));
	}
}