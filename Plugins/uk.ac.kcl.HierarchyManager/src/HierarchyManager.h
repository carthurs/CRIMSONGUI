#pragma once

#include <gsl.h>

#include <boost/optional.hpp>

#include <mitkDataStorage.h>
#include <mitkNodePredicateBase.h>

#include <mitkOperationActor.h>

#include "uk_ac_kcl_HierarchyManager_Export.h"

#define DECLARE_HIERARCHY_MANAGER_NODE_TYPE(nodeTypeName) static const crimson::HierarchyManager::NodeType& nodeTypeName();

#define DEFINE_HIERARCHY_MANAGER_NODE_TYPE(containerClass, nodeTypeName)                                                       \
    const crimson::HierarchyManager::NodeType& containerClass::nodeTypeName()                                                  \
    {                                                                                                                          \
        static const crimson::HierarchyManager::NodeType type = crimson::HierarchyManager::getInstance()->createNodeType();    \
        return type;                                                                                                           \
    }

namespace crimson
{

/*! \brief   A singleton class for managing the hierarchical relations of CRIMSON data objects. 
 *  
 * The hierarchical relations define the dependencies between various data objects.
 * This mostly affects how the nodes are presented to the user in the MITK's Data Manager view.
 * For example, the vessel paths belonging to the same vessel tree must be grouped under the node
 * representing this vessel tree. Thus, deleting the vessel tree node will also delete the vessel 
 * paths belonging to it. 
 *  
 * Hierarchical grouping also facilitates the discovery of th nodes using the mitk::DataStorage.
 * For example, to find all the contours defining a vessel, it is sufficient to iterate the 
 * child nodes of the vessel path and select the nodes containing contours.
 *  
 * HierarchyManager allows adding new node types and relationships between them. The most 
 * convenient way to do so is to use the DECLARE_HIERARCHY_MANAGER_NODE_TYPE and  
 * DEFINE_HIERARCHY_MANAGER_NODE_TYPE macro pair. This will correctly register the node types in 
 * the hierarchy mananger.
 *     
 * Example usage:
 * 
 *     // MyNodeTypes.h
 *     #pragma once  
 *     #include <HierarchyManager.h>  
 *         
 *     struct MyNodeTypes {   
 *         DECLARE_HIERARCHY_MANAGER_NODE_TYPE(MyParentNode)                  
 *         DECLARE_HIERARCHY_MANAGER_NODE_TYPE(MyChildNode)                       
 *     };            
 *     
 *     // MyNodeTypes.cpp
 *     #include "MyNodeTypes.h"
 *     
 *     DEFINE_HIERARCHY_MANAGER_NODE_TYPE(MyNodeTypes, MyParentNode)
 *     DEFINE_HIERARCHY_MANAGER_NODE_TYPE(MyNodeTypes, MyChildNode)
 *     
 *     // UsingHierarchyManager.cpp
 *     void initialize()
 *     {
 *          auto hm = crimson::HierarchyManager::getInstance();
 *          hm->addNodeType(MyNodeTypes::MyParentNode(), ParentNodePredicate::New());
 *          hm->addNodeType(MyNodeTypes::MyChildNode(),  ChildNodePredicate::New());
 *          hm->addRelation(MyNodeTypes::MyParentNode(), MyNodeTypes::MyChildNode(), crimson::HierarchyManager::rtOneToMany);
 *     }
 *     
 *     void addNode(mitk::DataNode* parent, mitkDataNode* child)
 *     {
 *          auto hm = crimson::HierarchyManager::getInstance();
 *          hm->addNodeToHierarchy(parent, MyNodeTypes::MyParentNode(), child, MyNodeTypes::MyChildNode());
 *          // ...
 *     }
 */

class HIERARCHYMANAGER_EXPORT HierarchyManager : public itk::Object, public mitk::OperationActor
{
public:
    /*! \name Singleton interface */
    ///@{ 
    static bool init();
    static HierarchyManager* getInstance();
    static void term();
    ///@} 

    using NodeType = int;   

    /*!
     * \brief   Creates a unique id for a new node type.
     */
    int createNodeType() { return _lastAssignedNodeType++; }

    /*! \brief   Values that represent relation types between parent and child nodes. */
    enum RelationType { 
        rtOneToOne = 0, ///< Only one child of a particular type is allowed for the parent
        rtOneToMany, ///< Multiple children of a particular type are allowed for the parent
        rtUnknown   ///< No information about the relations between types
    };

    /*! \brief   Values that represent node type flags. */
    enum NodeTypeFlags {
        ntfNone = 0x00, ///< No flags
        ntfRecursiveDeletion = 0x01,    ///< Deleting a parent node also deletes all its children
        ntfUndoableDeletion = 0x02, ///< If the node is deleted, it should be put onto undo stack to allow deletion undo
        ntfPickable = 0x04  ///< The node should be allowed to be picked with mouse clicks in 3D rendering window
    };

    ///@{ 
    /*!
     * \brief   Adds a new node type.
     *
     * \param   type        The new node type identifier.
     * \param   predicate   The predicate defining the node.
     * \param   flags       The flags to be applied for nodes of this type.
     */
    bool addNodeType(const NodeType& type, const mitk::NodePredicateBase::Pointer& predicate, int flags = ntfNone);

    /*!
     * \brief   Adds a relation between the parent and child node types.
     *
     * \param   parentType  Type of the parent.
     * \param   childType   Type of the child.
     * \param   relation    The type of relation.
     */
    bool addRelation(const NodeType& parentType, const NodeType& childType, const RelationType& relation);

    /*!
     * \brief   Gets a relation between the parent and child node types.
     *
     * \param   parentNodeType  Type of the parent node.
     * \param   childNodeType   Type of the child node.
     */
    RelationType getRelation(const NodeType& parentNodeType, const NodeType& childNodeType) const;
    ///@} 
                     

    /*!
     * \brief   Gets data storage that the HierarchyManager operates upon.
     */
    mitk::DataStorage::Pointer getDataStorage() const;

    /*!
     * \brief   Set the flag whether to start a new undo group when adding nodes to hierarchy.
     *  
     *  By default, addNodeToHierarchy() will start a new undo group meaning that undoing the
     *  addition will remove nodes one by one. However, when multiple nodes are to be added as a
     *  single unit, set this flag to false (and reset it to true after all nodes are added).
     */
    void setStartNewUndoGroup(bool start);

    /*!
     * \brief   Determine if a child node of a particular type can be added to the parent node of a different type.
     *
     * \param   parentNode          The desired parent node.
     * \param   parentNodeType      Type of the parent node.
     * \param   childNodeType       Type of the child node.
     * \param   allowReplacement    true if replacing a node is allowed (useful for rtOneToOne relationship).
     */
    bool canAddNode(mitk::DataNode* parentNode, const NodeType& parentNodeType, const NodeType& childNodeType,
                    bool allowReplacement) const;

    /*!
     * \brief   Adds a node to hierarchy.
     *
     * \param   parentNode      The parent node.
     * \param   parentNodeType  Type of the parent node.
     * \param   newNode         The new node.
     * \param   newNodeType     Type of the new node.
     *
     * \return  false if the relationship between the node types is unknown.
     */
    bool addNodeToHierarchy(const mitk::DataNode* parentNode, const NodeType& parentNodeType, const mitk::DataNode* newNode,
                            const NodeType& newNodeType);

    /*!
     * \brief   Searches for the first node type that the node satisfies.
     */
    boost::optional<NodeType> findFittingNodeType(mitk::DataNode* node) const;

    /*!
     * \brief   Searches for the potential parents of the node with a particular nodeType.
     */
    std::unordered_set<mitk::DataNode*> findPotentialParents(mitk::DataNode* node, NodeType nodeType) const;

    /*!
     * \brief   Change the parent of the node.
     */
    bool reparentNode(const mitk::DataNode::Pointer& node, const mitk::DataNode::Pointer& newParent);

    /*!
     * \brief   Gets a predicate associated with the node type.
     */
    mitk::NodePredicateBase::Pointer getPredicate(const NodeType& type) const;

    ///@{ 
    /*!
     * \brief   Gets an ancestor of the node of a particular type.
     *
     * \param   node        The child node.
     * \param   parentType  Type of the parent.
     * \param   directOnly  false to search parents of parents.
     */
    mitk::DataNode::Pointer getAncestor(const mitk::DataNode* node, const NodeType& parentType, bool directOnly = false) const;

    /*!
     * \brief   Gets the first descendant node of a particular type.
     *
     * \param   node            The parent node.
     * \param   descendantType  Type of the descendant.
     * \param   directOnly      false to search children of children.
     */
    mitk::DataNode::Pointer getFirstDescendant(const mitk::DataNode* node, const NodeType& descendantType,
                                               bool directOnly = false) const;

    /*!
     * \brief   Gets all the descendant nodes of a particular type.
     *
     * \param   node            The parent node.
     * \param   descendantsType Type of the descendants.
     * \param   directOnly      false to search children of children.
     */
    mitk::DataStorage::SetOfObjects::ConstPointer getDescendants(const mitk::DataNode* node, const NodeType& descendantsType,
                                                                 bool directOnly = false) const;

    ///@} 
     
    static const char* nodeUIDPropertyName; ///< The name of the DataNode property where the nodeUID is stored

    /*!
     * \brief   Searches for the node with a particular UID.
     */
    mitk::DataNode::Pointer findNodeByUID(gsl::cstring_span<> nodeUID) const;

    /*!
     * \brief   Enables/disables the undo for adding and deleting nodes.
     */
    void enableUndo(bool enable);

    /*!
     * \brief   Overriden from mitk::Operation for undo/redo support.
     */
    void ExecuteOperation(mitk::Operation* operation) override;

private:
    HierarchyManager();
    ~HierarchyManager();

    HierarchyManager(const HierarchyManager&) = delete;
    HierarchyManager& operator=(const HierarchyManager&) = delete;

    void _connectDataStorageEvents();
    void _disconnectDataStorageEvents();

    void nodeAdded(const mitk::DataNode*);
    void nodeRemoved(const mitk::DataNode*);

private:
    static HierarchyManager* _instance;

    struct HierarchyManagerImpl;

    std::unique_ptr<HierarchyManagerImpl> _impl;

    int _lastAssignedNodeType = 0;

    bool _testFlags(const mitk::DataNode* node, NodeTypeFlags flags) const;
    std::vector<std::pair<std::vector<const mitk::DataNode*>, mitk::DataNode::ConstPointer>>
    _getUndoableRemoveNodes(const mitk::DataNode* node) const;
};

} // namespace crimson