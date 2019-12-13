#include "CreateDataNodeAsyncTask.h"

#include <mitkRenderingManager.h>

namespace crimson {

CreateDataNodeAsyncTask::CreateDataNodeAsyncTask(
    std::shared_ptr<crimson::async::TaskWithResult<mitk::BaseData::Pointer>> childDataCreatorTask,
    mitk::DataNode::Pointer parent,
    crimson::HierarchyManager::NodeType parentType,
    crimson::HierarchyManager::NodeType childType,
    std::map<std::string, mitk::BaseProperty::Pointer> propertiesForNewNode)
    : QAsyncTaskAdapter(std::static_pointer_cast<crimson::async::Task>(childDataCreatorTask))
    , parent(parent)
    , parentType(parentType)
    , childType(childType)
    , propertiesForNewNode(propertiesForNewNode)
    , task(childDataCreatorTask)
{
    connect(this, &CreateDataNodeAsyncTask::taskStateChanged, this, &CreateDataNodeAsyncTask::processTaskStateChange);
}

void CreateDataNodeAsyncTask::processTaskStateChange(crimson::async::Task::State state, QString /*message*/)
{
    if (state != crimson::async::Task::State_Finished || !task->getResult()) {
        return;
    }

    mitk::BaseData::Pointer result = task->getResult().get();
    if (result.IsNotNull()) {
        newNode = mitk::DataNode::New();

        newNode->SetData(result);
        float rgb[3];
        if (parent->GetColor(rgb)) {
            newNode->SetColor(rgb);
        }
        for (const std::pair<std::string, mitk::BaseProperty::Pointer>& propNameValuePair : propertiesForNewNode) {
            newNode->AddProperty(propNameValuePair.first.c_str(), propNameValuePair.second.GetPointer(), nullptr, true);
        }

        crimson::HierarchyManager::getInstance()->addNodeToHierarchy(parent, parentType, newNode, childType);
        newNode->SetVisibility(true);
    }

    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

}
