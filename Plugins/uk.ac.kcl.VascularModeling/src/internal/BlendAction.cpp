#include "BlendAction.h"
#include "VascularModelingUtils.h"

#include <HierarchyManager.h>
#include <VascularModelingNodeTypes.h>

#include <AsyncTaskManager.h>
#include <CreateDataNodeAsyncTask.h>

#include <VesselForestData.h>

#include <ISolidModelKernel.h>

#include <mitkProperties.h>

#include <QMessageBox>

BlendAction::BlendAction() {}

BlendAction::~BlendAction() {}

std::unordered_set<mitk::DataNode*> BlendAction::Run(mitk::DataNode* node)
{
    std::unordered_set<mitk::DataNode*> visibleNodesUsedInBlending;
    std::map<crimson::VesselPathAbstractData::VesselPathUIDType, mitk::BaseData::Pointer> brepDatas;

    auto* hierarchyManager = crimson::HierarchyManager::getInstance();

    auto vesselForest = static_cast<crimson::VesselForestData*>(node->GetData());

    for (const auto& vesselUID : vesselForest->getActiveVessels()) {
        auto vesselPathNode = crimson::VascularModelingUtils::getVesselPathNodeByUID(node, vesselUID);

        if (!vesselPathNode) {
            MITK_ERROR << "Failed to find data node for vessel uid " << vesselUID << ".";
            return std::unordered_set<mitk::DataNode*>{};
        }

        // Get the lofted vessel data object
        mitk::DataNode::Pointer loftDataNode =
            hierarchyManager->getFirstDescendant(vesselPathNode, crimson::VascularModelingNodeTypes::Loft(), true);
        if (loftDataNode.IsNull()) {
            MITK_ERROR << "Vessel '" << vesselPathNode->GetName()
                       << "' does not have a lofted model. Please create a loft or exclude vessel from blending.";
            return std::unordered_set<mitk::DataNode*>{};
        }

        brepDatas[vesselUID] = loftDataNode->GetData();
        if (loftDataNode->IsVisible(nullptr)) {
            visibleNodesUsedInBlending.insert(loftDataNode);
        }
    }

    if (brepDatas.size() < 1) {
        MITK_WARN << "Nothing to blend.";
        return std::unordered_set<mitk::DataNode*>{};
    }

    bool useParallelBlending = false;
    node->GetBoolProperty("lofting.useParallelBlending", useParallelBlending);

    auto blendingTask = crimson::ISolidModelKernel::createBlendTask(
        brepDatas, vesselForest->getActiveBooleanOperations(), vesselForest->getActiveFilletSizeInfos(), useParallelBlending);

    std::map<std::string, mitk::BaseProperty::Pointer> props = {
        {"lofting.blend_result", mitk::BoolProperty::New(true).GetPointer()},
    };

    mitk::DataNode::Pointer prevBlend = hierarchyManager->getFirstDescendant(node, crimson::VascularModelingNodeTypes::Blend());
    if (prevBlend) {
        props["name"] = prevBlend->GetProperty("name");
        props["color"] = prevBlend->GetProperty("color");
        props["opacity"] = prevBlend->GetProperty("opacity");
    } else {
        props["name"] = mitk::StringProperty::New(node->GetName() + " Blended").GetPointer();
    }
    props["vesselForestTopologyMTime"] = mitk::IntProperty::New(vesselForest->getTopologyMTime());

    auto dataNodeTask =
        std::make_shared<crimson::CreateDataNodeAsyncTask>(blendingTask, node, crimson::VascularModelingNodeTypes::VesselTree(),
                                                           crimson::VascularModelingNodeTypes::Blend(), props);
    dataNodeTask->setDescription(std::string("Blend ") + node->GetName());

    crimson::AsyncTaskManager::getInstance()->addTask(dataNodeTask, crimson::VascularModelingUtils::getBlendingTaskUID(node));
    return visibleNodesUsedInBlending;
}

void BlendAction::Run(const QList<mitk::DataNode::Pointer>& selectedNodes)
{
    for (const mitk::DataNode::Pointer& node : selectedNodes) {
        auto hm = crimson::HierarchyManager::getInstance();

        if (!hm->getPredicate(crimson::VascularModelingNodeTypes::VesselTree())->CheckNode(node)) {
            continue;
        }

        Run(node);
    }
}