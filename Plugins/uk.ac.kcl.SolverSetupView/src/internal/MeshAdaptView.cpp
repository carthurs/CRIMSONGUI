#include <QMessageBox>
#include <QFileDialog>
#include <QTimer>

// Main include
#include "MeshAdaptView.h"
#include "SolverSetupView.h"

#include "internal/uk_ac_kcl_SolverSetup_Activator.h"

#include <SolverSetupNodeTypes.h>
#include <QtPropertyStorage.h>

#include <NodePredicateNone.h>
#include <NodePredicateDerivation.h>

// Module includes
#include <HierarchyManager.h>
#include <IMeshingKernel.h>
#include <AsyncTaskManager.h>
#include <CreateDataNodeAsyncTask.h>
#include <VesselMeshingNodeTypes.h>
#include <VascularModelingNodeTypes.h>
#include <MeshData.h>
#include <ISolverStudyData.h>

#include <mitkNodePredicateProperty.h>
#include <mitkNodePredicateAnd.h>
#include <mitkStringProperty.h>

#include <vtkPointData.h>
#include <vtkNew.h>
#include <vtkDoubleArray.h>
#include <vtkUnstructuredGridBase.h>

#include <berryIWorkbenchPage.h>
#include <utils/TaskStateObserver.h>

#include <boost/format.hpp>

namespace detail
{
/*!
 * \brief   A node predicate that detects SolutionData representing single-component
 *  double-valued solution which can be used as an error indicator for adaptation.
 */
class NodePredicateSingleComponentDoubleSolution : public mitk::NodePredicateBase
{
public:
    mitkClassMacro(NodePredicateSingleComponentDoubleSolution, NodePredicateBase);

    itkFactorylessNewMacro(NodePredicateSingleComponentDoubleSolution);
    bool CheckNode(const mitk::DataNode* node) const override
    {
        auto solutionData = static_cast<crimson::SolutionData*>(node->GetData())->getArrayData();
        return solutionData->GetNumberOfComponents() == 1 && solutionData->GetDataType() == VTK_DOUBLE;
    }

private:
    NodePredicateSingleComponentDoubleSolution() {}
};
}

// Property names for storage of adaptation parameters in a data node
static const char* studyNodeUIDProperty = "MeshAdaptationData.StudyNodeUID";
static const char* meshNodeUIDProperty = "MeshAdaptationData.MeshNodeUID";
static const char* errorIndicatorArrayNameProperty = "MeshAdaptationData.ErrorIndicatorArrayName";
static const char* minEdgeSizeProperty = "MeshAdaptationData.MinEdgeSize";
static const char* maxEdgeSizeProperty = "MeshAdaptationData.MaxEdgeSize";
static const char* errorReductionFactorProperty = "MeshAdaptationData.ErrorReductionFactor";

const std::string MeshAdaptView::VIEW_ID = "org.mitk.views.MeshAdaptView";

MeshAdaptView::MeshAdaptView()
    : NodeDependentView(crimson::SolverSetupNodeTypes::SolverStudy(), true, "Solver study")
{
}

MeshAdaptView::~MeshAdaptView() { disconnect(_errorIndicatorComboBoxConnection); }

void MeshAdaptView::SetFocus() {}

void MeshAdaptView::CreateQtPartControl(QWidget* parent)
{
    // create GUI widgets from the Qt Designer's .ui file
    _UI.setupUi(parent);

    static_cast<QBoxLayout*>(_UI.solverStudyNodeFrame->layout())
        ->insertWidget(0, createSelectedNodeWidget(_UI.solverStudyNodeFrame));

    _meshAdaptTaskStateObserver = new crimson::TaskStateObserver(_UI.adaptMeshButton, _UI.cancelAdaptMeshButton, this);
    connect(_meshAdaptTaskStateObserver, &crimson::TaskStateObserver::runTaskRequested, this, &MeshAdaptView::adaptMesh);
    connect(_meshAdaptTaskStateObserver, &crimson::TaskStateObserver::taskFinished, this, &MeshAdaptView::meshAdaptFinished);

    _UI.errorIndicatorNodeComboBox->SetDataStorage(GetDataStorage());
    _UI.errorIndicatorNodeComboBox->SetPredicate(crimson::NodePredicateNone::New());

    _errorIndicatorComboBoxConnection =
        connect(_UI.errorIndicatorNodeComboBox,
                static_cast<void (QmitkDataStorageComboBox::*)(int)>(&QmitkDataStorageComboBox::currentIndexChanged), [this]() {
                    _setPropertyFromCurrentErrorIndicatorNode();
                    _updateUI();
                });

    connect(_UI.minimumEdgeSizeSpinBox, &QDoubleSpinBox::editingFinished,
            [this]() { _setPropertyValueFromSpinBox(_UI.minimumEdgeSizeSpinBox, minEdgeSizeProperty); });
    connect(_UI.maximumEdgeSizeSpinBox, &QDoubleSpinBox::editingFinished,
            [this]() { _setPropertyValueFromSpinBox(_UI.maximumEdgeSizeSpinBox, maxEdgeSizeProperty); });
    connect(_UI.errorReductionFactorSpinBox, &QDoubleSpinBox::editingFinished,
            [this]() { _setPropertyValueFromSpinBox(_UI.errorReductionFactorSpinBox, errorReductionFactorProperty); });

    // Initialize the UI
    _updateUI();
}

void MeshAdaptView::_updateUI()
{
    std::string meshNodeName;
    if (_currentMeshNode) {
        meshNodeName = _currentMeshNode->GetName();
    }

    _UI.adaptationParametersFrame->setEnabled(currentNode() != nullptr && _currentMeshNode &&
                                              _UI.errorIndicatorNodeComboBox->GetSelectedNode().IsNotNull());
    _meshAdaptTaskStateObserver->setEnabled(currentNode() != nullptr && _currentMeshNode);
    _UI.meshNodeName->setText(QString::fromStdString(meshNodeName));
}

void MeshAdaptView::currentNodeModified()
{
    _setupDependentNodes();
    _meshAdaptTaskStateObserver->setPrimaryObservedUID(_getMeshAdaptTaskUID());
    _fillAdaptationPropertyValues();
    _updateUI();
}

void MeshAdaptView::currentNodeChanged(mitk::DataNode* prevNode)
{
    _setupDependentNodes();

    if (currentNode()) {
        _fillAdaptationPropertyValues();
    } else {
        _UI.errorIndicatorNodeComboBox->SetPredicate(crimson::NodePredicateNone::New());
    }

    _meshAdaptTaskStateObserver->setPrimaryObservedUID(_getMeshAdaptTaskUID());

    _updateUI();
}

void MeshAdaptView::OnSelectionChanged(berry::IWorkbenchPart::Pointer part, const QList<mitk::DataNode::Pointer>& nodes)
{
    this->FireNodesSelected(nodes);
    NodeDependentView::OnSelectionChanged(part, nodes);
}

void MeshAdaptView::adaptMesh()
{
    Expects(_currentMeshNode != nullptr && _currentAdaptationParametersNode != nullptr);
    auto solverStudyData = static_cast<crimson::ISolverStudyData*>(currentNode()->GetData());
    auto meshData = static_cast<crimson::MeshData*>(_currentMeshNode->GetData());

    auto errorIndicatorData =
        static_cast<crimson::SolutionData*>(_UI.errorIndicatorNodeComboBox->GetSelectedNode()->GetData())->getArrayData();

    if (errorIndicatorData->GetNumberOfTuples() != meshData->getNNodes()) {
        QMessageBox::critical(
            nullptr, "Adaptation error",
            QString{"Error indicator data array %1 has different number of values (%2) than nodes in the mesh (%3). Aborting"}
                .arg(errorIndicatorData->GetName())
                .arg(errorIndicatorData->GetNumberOfTuples())
                .arg(meshData->getNNodes()));
        return;
    }

    auto hm = crimson::HierarchyManager::getInstance();

    // Get all solution arrays from study
    auto solutionNodes = hm->getDescendants(currentNode(), crimson::SolverSetupNodeTypes::Solution());

    for (const auto& solutionNode : *solutionNodes) {
        auto arrayData = static_cast<crimson::SolutionData*>(solutionNode->GetData())->getArrayData();

        if (arrayData->GetDataType() != VTK_DOUBLE) {
            MITK_WARN << "Non-double-valued data array " << arrayData->GetName()
                      << " cannot be transferred during adaptation. Skipping";
            continue;
        }

        if (arrayData->GetNumberOfTuples() != meshData->getNNodes()) {
            MITK_WARN << "Data array " << arrayData->GetName() << " has different number of elements than the mesh. Skipping";
            continue;
        }

        meshData->getPointData()->AddArray(arrayData);
    }

    // Create an async mesh adaptation task
    auto meshAdaptTask = crimson::IMeshingKernel::createAdaptMeshTask(
        meshData, _UI.errorReductionFactorSpinBox->value(), _UI.minimumEdgeSizeSpinBox->value(),
        _UI.maximumEdgeSizeSpinBox->value(), std::string{errorIndicatorData->GetName()});

    // Set up the properties for the new node
    std::map<std::string, mitk::BaseProperty::Pointer> props = {
        {"material.edgeVisibility", mitk::BoolProperty::New(true).GetPointer()},
        {"name", mitk::StringProperty::New(_currentMeshNode->GetName() + " Adapted").GetPointer()}};

    // Launch adaptation task
    auto dataNodeTask = std::make_shared<detail::CreateNewStudyDataAsyncTask>(
        meshAdaptTask, props, currentNode(), _currentMeshNode, solutionNodes->CastToSTLConstContainer());
    dataNodeTask->setDescription(std::string("Adapting mesh ") + _currentMeshNode->GetName());
    dataNodeTask->setSequentialExecutionTag(0);

    crimson::AsyncTaskManager::getInstance()->addTask(dataNodeTask, _getMeshAdaptTaskUID());
}

void MeshAdaptView::meshAdaptFinished(crimson::async::Task::State state) {}

void MeshAdaptView::_setSpinBoxValueFromProperty(QDoubleSpinBox* spinBox, const char* propertyName)
{
    if (_currentAdaptationParametersNode == nullptr) {
        return;
    }

    spinBox->blockSignals(true);
    auto value = 0.0;
    _currentAdaptationParametersNode->GetDoubleProperty(propertyName, value);
    spinBox->setValue(value);
    spinBox->blockSignals(false);
}

void MeshAdaptView::_setPropertyValueFromSpinBox(QDoubleSpinBox* spinBox, const char* propertyName)
{
    if (_currentAdaptationParametersNode == nullptr) {
        return;
    }

    _currentAdaptationParametersNode->SetDoubleProperty(propertyName, spinBox->value());
}

void MeshAdaptView::_setPropertyFromCurrentErrorIndicatorNode()
{
    if (_currentAdaptationParametersNode == nullptr) {
        return;
    }

    if (_UI.errorIndicatorNodeComboBox->GetSelectedNode().IsNotNull()) {
        auto solutionData = static_cast<crimson::SolutionData*>(_UI.errorIndicatorNodeComboBox->GetSelectedNode()->GetData());
        _currentAdaptationParametersNode->SetStringProperty(errorIndicatorArrayNameProperty,
                                                            solutionData->getArrayData()->GetName());
    }
}

void MeshAdaptView::_setCurrentErrorIndicatorNodeFromProperty()
{
    if (_currentAdaptationParametersNode == nullptr) {
        return;
    }

    auto arrayName = std::string{};
    _currentAdaptationParametersNode->GetStringProperty(errorIndicatorArrayNameProperty, arrayName);

    if (arrayName.empty()) {
        return;
    }

    _UI.errorIndicatorNodeComboBox->blockSignals(true);

    auto nodes = _UI.errorIndicatorNodeComboBox->GetNodes();
    for (const auto& node : *nodes) {
        auto solutionData = static_cast<crimson::SolutionData*>(node->GetData());

        if (solutionData->getArrayData()->GetName() == arrayName) {
            _UI.errorIndicatorNodeComboBox->SetSelectedNode(node);
            break;
        }
    }
    _UI.errorIndicatorNodeComboBox->blockSignals(false);
}

void MeshAdaptView::_fillAdaptationPropertyValues()
{
    if (_currentAdaptationParametersNode == nullptr) {
        return;
    }

    _setSpinBoxValueFromProperty(_UI.minimumEdgeSizeSpinBox, minEdgeSizeProperty);
    _setSpinBoxValueFromProperty(_UI.maximumEdgeSizeSpinBox, maxEdgeSizeProperty);
    _setSpinBoxValueFromProperty(_UI.errorReductionFactorSpinBox, errorReductionFactorProperty);

    _UI.errorIndicatorNodeComboBox->blockSignals(true);
    _UI.errorIndicatorNodeComboBox->SetPredicate(mitk::NodePredicateAnd::New(
        crimson::NodePredicateDerivation::New(currentNode(), true, GetDataStorage()),
        crimson::HierarchyManager::getInstance()->getPredicate(crimson::SolverSetupNodeTypes::Solution()),
        detail::NodePredicateSingleComponentDoubleSolution::New()));
    _UI.errorIndicatorNodeComboBox->blockSignals(false);

    auto arrayName = std::string{};
    _currentAdaptationParametersNode->GetStringProperty(errorIndicatorArrayNameProperty, arrayName);

    if (arrayName.empty()) {
        _setPropertyFromCurrentErrorIndicatorNode();
    } else {
        _setCurrentErrorIndicatorNodeFromProperty();
    }
}

std::string MeshAdaptView::_getMeshAdaptTaskUID()
{
    if (!currentNode() || !_currentMeshNode) {
        return "";
    }

    return (boost::format("Adapting mesh %1%") % _currentMeshNode).str();
}

void MeshAdaptView::_setupDependentNodes()
{
    _currentMeshNode = _getMeshNode();
    _currentAdaptationParametersNode = _getAdaptationParametersNode();
}

mitk::DataNode* MeshAdaptView::_getMeshNode()
{
    if (currentNode() == nullptr) {
        return nullptr;
    }

    auto solverStudyData = static_cast<crimson::ISolverStudyData*>(currentNode()->GetData());
    return crimson::HierarchyManager::getInstance()->findNodeByUID(solverStudyData->getMeshNodeUID());
}

mitk::DataNode* MeshAdaptView::_getAdaptationParametersNode()
{
    if (currentNode() == nullptr || _currentMeshNode == nullptr) {
        return nullptr;
    }

    auto hm = crimson::HierarchyManager::getInstance();

    auto currentStudyNodeUID = std::string{};
    currentNode()->GetStringProperty(crimson::HierarchyManager::nodeUIDPropertyName, currentStudyNodeUID);

    auto currentMeshNodeUID = std::string{};
    _currentMeshNode->GetStringProperty(crimson::HierarchyManager::nodeUIDPropertyName, currentMeshNodeUID);

    auto adaptationDataNodes = hm->getDescendants(currentNode(), crimson::SolverSetupNodeTypes::AdaptationData());
    for (const auto& adaptationDataNode : *adaptationDataNodes) {
        auto studyUID = std::string{};
        adaptationDataNode->GetStringProperty(studyNodeUIDProperty, studyUID);

        auto meshUID = std::string{};
        adaptationDataNode->GetStringProperty(meshNodeUIDProperty, meshUID);
        if (studyUID == currentStudyNodeUID && meshUID == currentMeshNodeUID) {
            return adaptationDataNode.GetPointer();
        }
    }

    auto newAdaptationDataNode = mitk::DataNode::New();
    newAdaptationDataNode->SetName("Adaptation properties");
    newAdaptationDataNode->SetStringProperty(studyNodeUIDProperty, currentStudyNodeUID.c_str());
    newAdaptationDataNode->SetStringProperty(meshNodeUIDProperty, currentMeshNodeUID.c_str());
    newAdaptationDataNode->SetDoubleProperty(minEdgeSizeProperty, 1.0);
    newAdaptationDataNode->SetDoubleProperty(maxEdgeSizeProperty, 1.0);
    newAdaptationDataNode->SetDoubleProperty(errorReductionFactorProperty, 1.0);
    newAdaptationDataNode->SetBoolProperty(crimson::SolverSetupNodeTypes::adaptationDataPropertyName, true);

    hm->enableUndo(false);
    hm->addNodeToHierarchy(currentNode(), crimson::SolverSetupNodeTypes::SolverStudy(), newAdaptationDataNode.GetPointer(),
                           crimson::SolverSetupNodeTypes::AdaptationData());
    hm->enableUndo(true);

    return newAdaptationDataNode.GetPointer();
}

void MeshAdaptView::NodeRemoved(const mitk::DataNode* node)
{
    NodeDependentView::NodeRemoved(node);
    _setupDependentNodes();
    _updateUI();
}

void MeshAdaptView::NodeAdded(const mitk::DataNode* node)
{
    NodeDependentView::NodeAdded(node);
    _setupDependentNodes();
    _updateUI();
}

namespace detail
{

CreateNewStudyDataAsyncTask::CreateNewStudyDataAsyncTask(
    std::shared_ptr<crimson::async::TaskWithResult<mitk::BaseData::Pointer>> childDataCreatorTask,
    std::map<std::string, mitk::BaseProperty::Pointer> propertiesForNewNode, mitk::DataNode::Pointer originalStudyNode,
    mitk::DataNode::Pointer originalMeshNode, std::vector<mitk::DataNode::Pointer> solutionNodes)
    : QAsyncTaskAdapter(std::static_pointer_cast<crimson::async::Task>(childDataCreatorTask))
    , task(childDataCreatorTask)
    , propertiesForNewNode(std::move(propertiesForNewNode))
    , originalStudyNode(std::move(originalStudyNode))
    , originalMeshNode(std::move(originalMeshNode))
    , solutionNodes(std::move(solutionNodes))
{
    connect(this, &CreateNewStudyDataAsyncTask::taskStateChanged, this, &CreateNewStudyDataAsyncTask::processTaskStateChange);
}

void CreateNewStudyDataAsyncTask::processTaskStateChange(crimson::async::Task::State state, QString)
{
    if (state != crimson::async::Task::State_Finished || !task->getResult()) {
        return;
    }

    mitk::BaseData::Pointer newMeshData = task->getResult().get();
    if (newMeshData.IsNotNull()) {
        auto hm = crimson::HierarchyManager::getInstance();
        auto studyParentNode = hm->getAncestor(originalStudyNode, crimson::SolverSetupNodeTypes::SolverRoot());

        if (studyParentNode.IsNull()) {
            MITK_ERROR << "Failed to find parent node for the adapted solver study";
            return;
        }

        auto modelNode = hm->getAncestor(originalMeshNode, crimson::VascularModelingNodeTypes::Solid());

        if (modelNode.IsNull()) {
            MITK_WARN << "Trying to adapt the mesh under the solver study is no longer supported."
                         "It is recommended to create the adapted mesh from scratch."
                         "The new mesh and the model may be inconsistent and produce wrong results.";
            // Try the fallback mechanism for old scenes - find the vessel tree and it's blend node
            auto vesselTreeNode = hm->getAncestor(originalMeshNode, crimson::VascularModelingNodeTypes::VesselTree());
            if (vesselTreeNode) {
                modelNode = hm->getFirstDescendant(vesselTreeNode, crimson::VascularModelingNodeTypes::Blend());
            }
        }

        if (modelNode.IsNull()) {
            MITK_ERROR << "Failed to find model node for the adapted mesh";
            return;
        }

        // Create a data node for the adapted mesh
        auto newMeshNode = mitk::DataNode::New();

        newMeshNode->SetData(newMeshData);
        for (const std::pair<std::string, mitk::BaseProperty::Pointer>& propNameValuePair : propertiesForNewNode) {
            newMeshNode->AddProperty(propNameValuePair.first.c_str(), propNameValuePair.second.GetPointer(), nullptr, true);
        }
        newMeshNode->SetVisibility(true);

        // Create new study node by cloning the original solver study
        auto newStudyNode = mitk::DataNode::New();
        newStudyNode->SetData(static_cast<mitk::BaseData*>(originalStudyNode->GetData()->Clone().GetPointer()));
        newStudyNode->SetName(originalStudyNode->GetName() + " Adapted");

        hm->addNodeToHierarchy(studyParentNode, crimson::SolverSetupNodeTypes::SolverRoot(), newStudyNode,
                               crimson::SolverSetupNodeTypes::SolverStudy());

        hm->addNodeToHierarchy(modelNode, crimson::VascularModelingNodeTypes::Solid(), newMeshNode,
                               crimson::VesselMeshingNodeTypes::Mesh());

        // Replace the mesh UID
        auto newStudyData = static_cast<crimson::ISolverStudyData*>(newStudyNode->GetData());
        auto meshUID = std::string{};
        newMeshNode->GetStringProperty(crimson::HierarchyManager::nodeUIDPropertyName, meshUID);
        newStudyData->setMeshNodeUID(meshUID);

        // Copy the solution nodes and fill them with transferred solutions
        auto newMeshPointData = static_cast<crimson::MeshData*>(newMeshData.GetPointer())->getPointData();
        auto originalMeshPointData = static_cast<crimson::MeshData*>(originalMeshNode->GetData())->getPointData();
        for (const auto& solutionNode : solutionNodes) {
            auto arrayName = static_cast<crimson::SolutionData*>(solutionNode->GetData())->getArrayData()->GetName();

            auto array = newMeshPointData->GetArray(arrayName);

            if (array == nullptr) {
                MITK_WARN << "Transferred array " << arrayName << " not found.";
                continue;
            }

            auto newSolutionNode = mitk::DataNode::New();
            newSolutionNode->SetData(crimson::SolutionData::New(array));
            newSolutionNode->SetName("Transferred " + solutionNode->GetName());

            hm->addNodeToHierarchy(newStudyNode, crimson::SolverSetupNodeTypes::SolverStudy(), newSolutionNode,
                                   crimson::SolverSetupNodeTypes::Solution());

            // Remove data arrays from both new and old mesh data
            originalMeshPointData->RemoveArray(arrayName);
            newMeshPointData->RemoveArray(arrayName);
        }
    }

    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}
}
