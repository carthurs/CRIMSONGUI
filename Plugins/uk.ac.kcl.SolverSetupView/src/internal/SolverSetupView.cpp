#include <algorithm>

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

#include <QmitkIOUtil.h>

#include <usModule.h>
#include <usModuleRegistry.h>
#include <usModuleContext.h>
#include <usServiceInterface.h>

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

void SolverSetupView::CreateQtPartControl(QWidget* parent)
{
    // create GUI widgets from the Qt Designer's .ui file
    _UI.setupUi(parent);

    connect(_UI.setupSolverButton, &QAbstractButton::clicked, this, &SolverSetupView::writeSolverSetup);
	connect(_UI.runSimulationButton, &QAbstractButton::clicked, this, &SolverSetupView::runSimulation);
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

        _UI.meshNodeComboBox->SetPredicate(NodePredicateNone::New());
        _UI.solverParametersNodeComboBox->SetPredicate(NodePredicateNone::New());
        _solverStudyBCSetsModel.setSolverStudyNode(nullptr);
        _solverStudyMaterialsModel.setSolverStudyNode(nullptr);

        _materialVisNodePtr.reset();
    }

    _currentSolverStudyNode = node;

    if (_currentSolverStudyNode) {
        _setupSolverStudyComboBoxes();
        _UI.boundaryConditionSetsTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        _UI.boundaryConditionSetsTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
        _UI.materialsTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        _UI.materialsTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
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

void SolverSetupView::writeSolverSetup()
{
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

    if (!solverStudyData->writeSolverSetup(dataProvider, vesselForestData, _currentSolidNode->GetData(), solutions)) {
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

    auto solverStudyData = static_cast<ISolverStudyData*>(_currentSolverStudyNode->GetData());

    disconnect(_UI.meshNodeComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this,
               &SolverSetupView::setMeshNodeForStudy);
    disconnect(_UI.solverParametersNodeComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this,
               &SolverSetupView::setSolverParametersNodeForStudy);

    if (_solverStudyObserverTag != -1) {
        _currentSolverStudyNode->GetData()->RemoveObserver(_solverStudyObserverTag);
        _solverStudyObserverTag = -1;
    }

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
    connect(_UI.solverParametersNodeComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this,
            &SolverSetupView::setSolverParametersNodeForStudy);

    _solverStudyBCSetsModel.setSolverStudyNode(_currentSolverStudyNode);
    _solverStudyMaterialsModel.setSolverStudyNode(_currentSolverStudyNode);

    auto modifiedCommand = itk::SimpleMemberCommand<SolverSetupView>::New();
    modifiedCommand->SetCallbackFunction(this, &SolverSetupView::_solverStudyNodeModified);
    _solverStudyObserverTag = _currentSolverStudyNode->GetData()->AddObserver(itk::ModifiedEvent(), modifiedCommand);
}
