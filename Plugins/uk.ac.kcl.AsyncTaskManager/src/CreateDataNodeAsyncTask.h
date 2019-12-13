#pragma once

#include <mitkDataNode.h>

#include <AsyncTaskWithResult.h>
#include <QAsyncTaskAdapter.h>
#include <HierarchyManager.h>

#include <memory>

namespace crimson {

/*! \brief  A asynchronous task that creates a data node from a mitk::BaseData object returned by the child task. */
class ASYNCTASKMANAGER_EXPORT CreateDataNodeAsyncTask : public crimson::QAsyncTaskAdapter {
    Q_OBJECT
public:

    /*!
     * \brief   Constructor.
     *
     * \param   childDataCreatorTask    The child task creating the mitk::BaseData.
     * \param   parent                  The parent node of the new node.
     * \param   parentType              Type of the parent.
     * \param   childType               Type of the child.
     * \param   propertiesForNewNode    The properties for new node.
     */
    CreateDataNodeAsyncTask(std::shared_ptr<crimson::async::TaskWithResult<mitk::BaseData::Pointer>> childDataCreatorTask,
                            mitk::DataNode::Pointer parent, crimson::HierarchyManager::NodeType parentType,
                            crimson::HierarchyManager::NodeType childType,
                            std::map<std::string, mitk::BaseProperty::Pointer> propertiesForNewNode);

    /*!
     * \brief   Gets newly created node.
     */
    mitk::DataNode::Pointer getNewNode() const { return newNode; }

protected slots:
    virtual void processTaskStateChange(crimson::async::Task::State state, QString);

protected:
    mitk::DataNode::Pointer parent;
    crimson::HierarchyManager::NodeType parentType;
    crimson::HierarchyManager::NodeType childType;
    std::map<std::string, mitk::BaseProperty::Pointer> propertiesForNewNode;
    std::vector<mitk::DataNode::Pointer> nodesToHide;
    std::shared_ptr<crimson::async::TaskWithResult<mitk::BaseData::Pointer>> task;
    
    mitk::DataNode::Pointer newNode;
};

}