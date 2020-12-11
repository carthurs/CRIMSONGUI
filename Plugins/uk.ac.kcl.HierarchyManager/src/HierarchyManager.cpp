#include <algorithm>

#include "HierarchyManager.h"

#include <vector>
#include <boost/functional/hash.hpp>
#include <boost/range/adaptor/map.hpp>

#include <ctkServiceTracker.h>
#include <mitkIDataStorageService.h>
#include <mitkIDataStorageReference.h>

#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>

#include <mitkOperation.h>
#include <mitkOperationEvent.h>
#include <mitkUndoController.h>
#include <mitkInteractionConst.h>

#include <mitkImage.h>

#include "internal/uk_ac_kcl_HierarchyManager_Activator.h"
#include <queue>

namespace crimson
{

struct HierarchyManager::HierarchyManagerImpl {
    HierarchyManagerImpl()
        : _dataStorageServiceTracker(uk_ac_kcl_HierarchyManager_Activator::GetPluginContext())
        , _undoController(new mitk::UndoController(mitk::UndoController::VERBOSE_LIMITEDLINEARUNDO))
    {
    }

    ctkServiceTracker<mitk::IDataStorageService*> _dataStorageServiceTracker;
    std::unique_ptr<mitk::UndoController> _undoController;

    typedef std::pair<NodeType, NodeType> RelationNodeTypePair;
    std::unordered_map<RelationNodeTypePair, RelationType, boost::hash<RelationNodeTypePair>> _relations;

    struct NodeTypeInfo {
        mitk::NodePredicateBase::Pointer predicate;
        int flags;
    };

    std::map<NodeType, NodeTypeInfo> _nodeTypeInfo;

    bool _startNewUndoGroup = true;
    bool _undoEnabled = true;

    bool _undoOpInProgress = false;
    bool _deletingRecursively = false;
};

class HierarchyOperation : public mitk::Operation
{
public:
    // Relations with weak pointers to parents and strong pointer to children
    using RelationType = std::pair<std::vector<const mitk::DataNode*>, mitk::DataNode::ConstPointer>;
    using RelationsType = std::vector<RelationType>;

    HierarchyOperation(mitk::OperationType ot, const RelationsType& parentChildRelations)
        : mitk::Operation(ot)
        , parentChildRelations(parentChildRelations)
    {
    }

    RelationsType parentChildRelations;
};

const char* HierarchyManager::nodeUIDPropertyName = "CRIMSONUID";
HierarchyManager* HierarchyManager::_instance = nullptr;

bool HierarchyManager::init()
{
    assert(_instance == nullptr);
    _instance = new HierarchyManager();
    return true;
}

void HierarchyManager::term()
{
    assert(_instance != nullptr);
    delete _instance;
    _instance = nullptr;
}

HierarchyManager* HierarchyManager::getInstance() { return _instance; }

HierarchyManager::HierarchyManager()
    : _impl(new HierarchyManagerImpl())
{
    _impl->_dataStorageServiceTracker.open();

    _connectDataStorageEvents();
}

HierarchyManager::~HierarchyManager()
{
    _disconnectDataStorageEvents();

    _impl->_dataStorageServiceTracker.close();

    this->InvokeEvent(itk::DeleteEvent()); // Should be issued before the undo controller is deleted
}

bool HierarchyManager::addNodeType(const NodeType& type, const mitk::NodePredicateBase::Pointer& predicate, int flags)
{
    if (_impl->_nodeTypeInfo.find(type) != _impl->_nodeTypeInfo.end()) {
        MITK_ERROR << "Node type " << type << " already added.";
        return false;
    }
    _impl->_nodeTypeInfo[type] = HierarchyManagerImpl::NodeTypeInfo{predicate, flags};
    return true;
}

bool HierarchyManager::addRelation(const NodeType& parentType, const NodeType& childType, const RelationType& relation)
{
    if (_impl->_relations.find(std::make_pair(parentType, childType)) != _impl->_relations.end()) {
        MITK_ERROR << "Relation between type " << parentType << " and " << childType << " already added.";
        return false;
    }

    _impl->_relations[std::make_pair(parentType, childType)] = relation;
    return true;
}

void HierarchyManager::_connectDataStorageEvents()
{
    getDataStorage()->AddNodeEvent.AddListener(
        mitk::MessageDelegate1<HierarchyManager, const mitk::DataNode*>(this, &HierarchyManager::nodeAdded));

    getDataStorage()->RemoveNodeEvent.AddListener(
        mitk::MessageDelegate1<HierarchyManager, const mitk::DataNode*>(this, &HierarchyManager::nodeRemoved));
}

void HierarchyManager::_disconnectDataStorageEvents()
{
    getDataStorage()->AddNodeEvent.RemoveListener(
        mitk::MessageDelegate1<HierarchyManager, const mitk::DataNode*>(this, &HierarchyManager::nodeAdded));

    getDataStorage()->RemoveNodeEvent.RemoveListener(
        mitk::MessageDelegate1<HierarchyManager, const mitk::DataNode*>(this, &HierarchyManager::nodeRemoved));
}

mitk::DataStorage::Pointer HierarchyManager::getDataStorage() const
{
    mitk::IDataStorageService* dsService = _impl->_dataStorageServiceTracker.getService();

    if (dsService) {
        return dsService->GetDataStorage()->GetDataStorage();
    }

    return nullptr;
}

void HierarchyManager::nodeAdded(const mitk::DataNode* node)
{
    auto nonConstNode = const_cast<mitk::DataNode*>(node);

    // Assign a UID to every node
    static mitk::UIDGenerator uidGenerator("CRIMSONUID_");
    std::string UID;
    if (!nonConstNode->GetStringProperty(nodeUIDPropertyName, UID)) {
        nonConstNode->SetStringProperty(nodeUIDPropertyName, uidGenerator.GetUID().c_str());
    }

    // Ensure name propagation to data
    nonConstNode->SetBoolProperty("propagate_name_to_data", true);
    if (nonConstNode->GetData()) {
        nonConstNode->GetData()->GetPropertyList()->SetStringProperty("name", nonConstNode->GetName().c_str());
    }

    // Set some properties that should not be persisted through save/load process
    if (_testFlags(nonConstNode, ntfPickable)) {
        nonConstNode->SetBoolProperty("pickable", true);
    }

    // TODO: REMOVE TO OTHER LOCATION!
    if (nonConstNode->GetProperty("scalar visibility") != nullptr) {
        nonConstNode->SetBoolProperty("scalar visibility", false);
    }

    nonConstNode->SetSelected(false);
}

bool HierarchyManager::_testFlags(const mitk::DataNode* node, NodeTypeFlags flags) const
{
    for (const auto& ntInfoPair : _impl->_nodeTypeInfo) {
        if ((ntInfoPair.second.flags & flags) && ntInfoPair.second.predicate->CheckNode(node)) {
            return true;
        }
    }

    return false;
}

HierarchyOperation::RelationsType HierarchyManager::_getUndoableRemoveNodes(const mitk::DataNode* node) const
{
    // Get the relationship information for undo-redo
    HierarchyOperation::RelationsType parentChildRelations;

    // Traverse data storage hierachically
    std::queue<const mitk::DataNode*> nodesToProcess;
    nodesToProcess.push(node);
    while (!nodesToProcess.empty()) {
        const mitk::DataNode* currentNode = nodesToProcess.front();
        nodesToProcess.pop();

        if (_testFlags(currentNode, ntfUndoableDeletion)) {
            mitk::DataStorage::SetOfObjects::ConstPointer srcs = getDataStorage()->GetSources(currentNode);
            std::vector<const mitk::DataNode*> srcNodePointers;
            std::transform(srcs->begin(), srcs->end(), std::back_inserter(srcNodePointers),
                           [](const mitk::DataNode::Pointer& n) { return n.GetPointer(); });

            parentChildRelations.push_back(std::make_pair(srcNodePointers, mitk::DataNode::ConstPointer(currentNode)));

            if (_testFlags(node, ntfRecursiveDeletion)) {
                mitk::DataStorage::SetOfObjects::ConstPointer derivs = getDataStorage()->GetDerivations(currentNode);
                for (const mitk::DataNode::Pointer& childNode : *derivs) {
                    nodesToProcess.push(childNode.GetPointer());
                }
            }
        }
    }

    return parentChildRelations;
}

void HierarchyManager::nodeRemoved(const mitk::DataNode* node)
{
    bool needRecursiveDeletion = false;
    if (!_impl->_deletingRecursively) {
        needRecursiveDeletion = _testFlags(node, ntfRecursiveDeletion);

        // Get the relationship information for undo-redo
        HierarchyOperation::RelationsType parentChildRelations = _getUndoableRemoveNodes(node);

        if (!_impl->_undoOpInProgress && parentChildRelations.size() > 0 && _impl->_undoEnabled) {
            if (_impl->_startNewUndoGroup) {
                mitk::OperationEvent::IncCurrGroupEventId();
                mitk::OperationEvent::IncCurrObjectEventId();
                //mitk::OperationEvent::ExecuteIncrement();
            }

            auto doOp = new HierarchyOperation(mitk::OpREMOVE, parentChildRelations);
            auto undoOp = new HierarchyOperation(mitk::OpINSERT, parentChildRelations);
            auto operationEvent = new mitk::OperationEvent(this, doOp, undoOp, "Remove node " + node->GetName());
            _impl->_undoController->SetOperationEvent(operationEvent);
        }
    }

    if (needRecursiveDeletion) {
        _impl->_deletingRecursively = true;
        getDataStorage()->Remove(getDataStorage()->GetDerivations(node, nullptr, false));
        _impl->_deletingRecursively = false;
    }

    // Check if the last non-helper node is being removed. If it is - assume the closing of a project and clear the undo stack
    // This is clearly a hack because in fact there is no "project closing" action - it is simply "remove all nodes" action
    static mitk::NodePredicateBase::Pointer notHelper =
        mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("helper object")).GetPointer();
    if (!_impl->_deletingRecursively && getDataStorage()->GetSubset(notHelper)->size() == 0) {
        _impl->_undoController->Clear();
    }
}

HierarchyManager::RelationType HierarchyManager::getRelation(const NodeType& parentNodeType,
                                                             const NodeType& childNodeType) const
{
    auto relationKey = std::make_pair(parentNodeType, childNodeType);
    auto relationIter = _impl->_relations.find(relationKey);
    return relationIter == _impl->_relations.end() ? rtUnknown : relationIter->second;
}

bool HierarchyManager::canAddNode(mitk::DataNode* parentNode, const NodeType& parentNodeType, const NodeType& childNodeType,
                                  bool allowReplacement) const
{
    auto relation = getRelation(parentNodeType, childNodeType);

    switch (relation) {
    case rtOneToOne:
        return getFirstDescendant(parentNode, childNodeType, true).IsNull();
    case rtOneToMany:
        return true;
    case rtUnknown:
    default:
        return false;
    }
}

bool HierarchyManager::addNodeToHierarchy(const mitk::DataNode* parentNode, const NodeType& parentNodeType,
                                          const mitk::DataNode* newNode, const NodeType& newNodeType)
{
    // Confirm that nodes are of correct type
    if (!getPredicate(parentNodeType)->CheckNode(parentNode)) {
        MITK_ERROR << "The parent node's type is not recognized (" << parentNode->GetName() << ").";
        assert(false);
        return false;
    }

    if (!getPredicate(newNodeType)->CheckNode(newNode)) {
        MITK_ERROR << "The new node's type is not recognized (" << newNode->GetName() << ").";
        assert(false);
        return false;
    }

    // If parent node doesn't exist in data storage - do nothing
    if (!getDataStorage()->Exists(parentNode)) {
        return false;
    }

    // Find the corresponding relationship rule
    auto relation = getRelation(parentNodeType, newNodeType);

    // Add/remove nodes according to relationship
    bool rc;
    switch (relation) {
    case rtOneToOne: {
        // Find the node of corresponding type in children of parentNode
        mitk::DataStorage::SetOfObjects::ConstPointer childrenToBeRemoved =
            getDataStorage()->GetDerivations(parentNode, _impl->_nodeTypeInfo[newNodeType].predicate, true);

        // Remove them if any found
        for (const mitk::DataNode::Pointer& child : *childrenToBeRemoved) {
            getDataStorage()->Remove(child);
        }

        // Add the new node
        getDataStorage()->Add(const_cast<mitk::DataNode*>(newNode), const_cast<mitk::DataNode*>(parentNode));
    }
        rc = true;
        break;
    case rtOneToMany:
        // Simply add the new node
        getDataStorage()->Add(const_cast<mitk::DataNode*>(newNode), const_cast<mitk::DataNode*>(parentNode));
        rc = true;
        break;
    case rtUnknown:
    default:
        MITK_ERROR << "Nodes have unknown relationship to each other (" << parentNode->GetName() << ", " << newNode->GetName()
                   << ").";
        rc = false;
        break;
    }

    if (rc) {
        bool needCreateUndoItem = _impl->_nodeTypeInfo[newNodeType].flags & ntfUndoableDeletion;

        if (!_impl->_undoOpInProgress && needCreateUndoItem && _impl->_undoEnabled) {
            if (_impl->_startNewUndoGroup) {
                mitk::OperationEvent::IncCurrGroupEventId();
                mitk::OperationEvent::IncCurrObjectEventId();
                //mitk::OperationEvent::ExecuteIncrement();
            }

            std::vector<const mitk::DataNode*> srcs{parentNode};

            HierarchyOperation::RelationsType relations{std::make_pair(srcs, mitk::DataNode::ConstPointer(newNode))};

            auto doOp = new HierarchyOperation(mitk::OpINSERT, relations);
            auto undoOp = new HierarchyOperation(mitk::OpREMOVE, relations);
            auto operationEvent = new mitk::OperationEvent(this, doOp, undoOp, "Add node " + newNode->GetName());
            _impl->_undoController->SetOperationEvent(operationEvent);
        }
    }
    return rc;
}

boost::optional<HierarchyManager::NodeType> HierarchyManager::findFittingNodeType(mitk::DataNode* node) const
{
    // Find the predicate fitting the node
    for (const auto& ntInfoPair : _impl->_nodeTypeInfo) {
        if (ntInfoPair.second.predicate->CheckNode(node)) {
            return ntInfoPair.first;
        }
    }

    return boost::none;
}

std::unordered_set<mitk::DataNode*> HierarchyManager::findPotentialParents(mitk::DataNode* node, NodeType nodeType) const
{
    std::unordered_set<mitk::DataNode*> potentialParents;

    mitk::DataStorage::SetOfObjects::ConstPointer ancestors = getDataStorage()->GetSources(node, nullptr, true);
    mitk::DataNode* currentParentNode = ancestors->size() == 0 ? nullptr : (*ancestors)[0];

    // Find node types of potential parents
    std::unordered_set<NodeType> parentNodeTypes;
    for (const HierarchyManagerImpl::RelationNodeTypePair& relation : (_impl->_relations | boost::adaptors::map_keys)) {
        if (relation.second == nodeType) {
            parentNodeTypes.insert(relation.first);
        }
    }

    for (NodeType parentNodeType : parentNodeTypes) {
        mitk::DataStorage::SetOfObjects::ConstPointer parents = getDataStorage()->GetSubset(getPredicate(parentNodeType));
        std::copy_if(parents->begin(), parents->end(), std::inserter(potentialParents, potentialParents.begin()),
                     [currentParentNode](const mitk::DataNode::Pointer& n) {
                         return n != currentParentNode && n->GetProperty("helper object") == nullptr &&
                                n->GetProperty("hidden object") == nullptr;
                     });
    }

    return potentialParents;
}

bool HierarchyManager::reparentNode(const mitk::DataNode::Pointer& node, const mitk::DataNode::Pointer& newParent)
{
    if (!_testFlags(node.GetPointer(), ntfRecursiveDeletion)) {
        MITK_ERROR << "Node must support recursive deletion to be reparented";
        return false;
    }

    // Get the relationship information
    HierarchyOperation::RelationsType parentChildRelations = _getUndoableRemoveNodes(node);

    // Remove the node and all its children - create the undo operation that adds nodes back in the original place
    getDataStorage()->Remove(node);

    // Create the undo operation that removes the nodes from the new place
    parentChildRelations[0].first = {newParent.GetPointer()};

    auto doOp = new HierarchyOperation(mitk::OpINSERT, parentChildRelations);
    auto undoOp = new HierarchyOperation(mitk::OpREMOVE, parentChildRelations);
    auto operationEvent = new mitk::OperationEvent(this, doOp, undoOp, "Add node");
    _impl->_undoController->SetOperationEvent(operationEvent);

    this->ExecuteOperation(doOp);

    return true;
}

mitk::NodePredicateBase::Pointer HierarchyManager::getPredicate(const NodeType& type) const
{
    auto iter = _impl->_nodeTypeInfo.find(type);
    return iter == _impl->_nodeTypeInfo.end() ? mitk::NodePredicateBase::Pointer() : iter->second.predicate;
}

mitk::DataNode::Pointer HierarchyManager::getAncestor(const mitk::DataNode* node, const NodeType& parentType, bool directOnly /*= false*/) const
{
    mitk::DataStorage::SetOfObjects::ConstPointer ancestors =
        getDataStorage()->GetSources(node, getPredicate(parentType), directOnly);
    return ancestors->size() == 0 ? nullptr : (*ancestors)[0];
}

mitk::DataNode::Pointer HierarchyManager::getFirstDescendant(const mitk::DataNode* node, const NodeType& descendantType,
                                                             bool directOnly /*= false*/) const
{
    mitk::DataStorage::SetOfObjects::ConstPointer nodes = getDescendants(node, descendantType, directOnly);
    return nodes->size() > 0 ? (*nodes)[0] : mitk::DataNode::Pointer();
}

mitk::DataStorage::SetOfObjects::ConstPointer HierarchyManager::getDescendants(const mitk::DataNode* node,
                                                                               const NodeType& descendantsType, bool directOnly /*= false*/) const
{
    return getDataStorage()->GetDerivations(node, getPredicate(descendantsType), directOnly);
}

mitk::DataNode::Pointer HierarchyManager::findNodeByUID(gsl::cstring_span<> nodeUID) const
{
    return getDataStorage()->GetNode(
        mitk::NodePredicateProperty::New(nodeUIDPropertyName, mitk::StringProperty::New(gsl::to_string(nodeUID))));
}

void HierarchyManager::ExecuteOperation(mitk::Operation* operation)
{
    auto op = static_cast<HierarchyOperation*>(operation);

    _impl->_undoOpInProgress = true;

    switch (op->GetOperationType()) {
    case mitk::OpINSERT:
        for (const HierarchyOperation::RelationType& relation : op->parentChildRelations) {
            auto parents = mitk::DataStorage::SetOfObjects::New();
            for (const mitk::DataNode* parentNode : relation.first) {
                if (getDataStorage()->Exists(parentNode)) {
                    parents->push_back(const_cast<mitk::DataNode*>(parentNode));
                }
            }

            getDataStorage()->Add(const_cast<mitk::DataNode*>(relation.second.GetPointer()), parents);
        }
        break;
    case mitk::OpREMOVE:
        for (const HierarchyOperation::RelationType& relation : op->parentChildRelations) {
            getDataStorage()->Remove(relation.second.GetPointer());
        }
        break;
    }

    _impl->_undoOpInProgress = false;
}

void HierarchyManager::setStartNewUndoGroup(bool start) { _impl->_startNewUndoGroup = start; }

void HierarchyManager::enableUndo(bool enable) { _impl->_undoEnabled = enable; }

} // namespace crimson
