#include "MeshAction.h"
#include "MeshingUtils.h"

#include <HierarchyManager.h>
#include <VascularModelingNodeTypes.h>
#include <VesselMeshingNodeTypes.h>

#include <AsyncTaskManager.h>
#include <CreateDataNodeAsyncTask.h>

#include <VesselForestData.h>
#include <MeshingParametersData.h>

#include <mitkProperties.h>
#include <mitkGridRepresentationProperty.h>

MeshAction::MeshAction() {}

MeshAction::~MeshAction() {}

void MeshAction::Run(const QList<mitk::DataNode::Pointer>& selectedNodes)
{
    for (const mitk::DataNode::Pointer& node : selectedNodes) {
        auto hm = crimson::HierarchyManager::getInstance();

        if (!hm->getPredicate(crimson::VascularModelingNodeTypes::Solid())->CheckNode(node)) {
            continue;
        }

        // Collect the vessel names (used for error output by the meshing kernel)
        std::map<crimson::VesselPathAbstractData::VesselPathUIDType, std::string> vesselUIDtoNameMap;
        mitk::DataNode::Pointer vesselForestNode = hm->getAncestor(node, crimson::VascularModelingNodeTypes::VesselTree());
        if (vesselForestNode.IsNotNull()) {
            mitk::DataStorage::SetOfObjects::ConstPointer vesselPaths =
                hm->getDescendants(vesselForestNode, crimson::VascularModelingNodeTypes::VesselPath());

            for (const mitk::DataNode::Pointer& vesselPathNode : *vesselPaths) {
                vesselUIDtoNameMap[static_cast<crimson::VesselPathAbstractData*>(vesselPathNode->GetData())->getVesselUID()] =
                    vesselPathNode->GetName();
            }
        }

        // Find meshing parameters node and create if needed
        crimson::MeshingParametersData* meshingParameters = crimson::MeshingUtils::getMeshingParametersForSolid(node);

        // Create the async meshing task
        auto meshingTask = crimson::IMeshingKernel::createMeshSolidTask(
            node->GetData(), meshingParameters->globalParameters(), meshingParameters->localParameters(), vesselUIDtoNameMap);

        // Set up the properties for the mesh node that will be created
        std::map<std::string, mitk::BaseProperty::Pointer> props = {
            {"material.edgeVisibility", mitk::BoolProperty::New(true).GetPointer()},
            {"grid representation",
             mitk::GridRepresentationProperty::New(mitk::GridRepresentationProperty::SURFACE).GetPointer()}};

        mitk::DataNode::Pointer prevMesh = hm->getFirstDescendant(node, crimson::VesselMeshingNodeTypes::Mesh());
        if (prevMesh) {
            props["name"] = prevMesh->GetProperty("name");
            props["color"] = prevMesh->GetProperty("color");
            props["opacity"] = prevMesh->GetProperty("opacity");
        } else {
            props["name"] = mitk::StringProperty::New(node->GetName() + " Meshed").GetPointer();
        }

        // Launch the meshing task
        auto dataNodeTask = std::make_shared<crimson::CreateDataNodeAsyncTask>(
            meshingTask, node, crimson::VascularModelingNodeTypes::Solid(), crimson::VesselMeshingNodeTypes::Mesh(), props);
        dataNodeTask->setDescription(std::string("Mesh ") + node->GetName());
        dataNodeTask->setSequentialExecutionTag(0);

        crimson::AsyncTaskManager::getInstance()->addTask(dataNodeTask, crimson::MeshingUtils::getMeshingTaskUID(node));
    }
}