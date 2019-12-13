#include "MeshingUtils.h"

#include <HierarchyManager.h>
#include <VascularModelingNodeTypes.h>
#include <VesselMeshingNodeTypes.h>

#include <boost/format.hpp>

namespace crimson {

MeshingParametersData* MeshingUtils::getMeshingParametersForSolid(mitk::DataNode* solidNode)
{
    if (!solidNode) {
        return nullptr;
    }

    auto hm = crimson::HierarchyManager::getInstance();

    crimson::HierarchyManager::NodeType ownerNodeType = crimson::VascularModelingNodeTypes::VesselPath();
    mitk::DataNode::Pointer meshingParametersOwnerNode = hm->getAncestor(solidNode, crimson::VascularModelingNodeTypes::VesselPath());

    if (meshingParametersOwnerNode.IsNull()) {
        // Put the meshing parameters under the vessel tree node if it's available
        ownerNodeType = crimson::VascularModelingNodeTypes::VesselTree();
        meshingParametersOwnerNode = hm->getAncestor(solidNode, crimson::VascularModelingNodeTypes::VesselTree());
    }

    if (meshingParametersOwnerNode.IsNull()) {
        // Alternatively, e.g. for imported models, put meshing parameters under the solid model itself
        // The drawback in this case is that the meshing parameters will be deleted along with the solid model
        // itself and cannot be reused
        MITK_WARN << "No suitable owner for meshing parameters data found. Using the solid itself";
        ownerNodeType = crimson::VascularModelingNodeTypes::Solid();
        meshingParametersOwnerNode = solidNode;
    }

    mitk::DataNode::Pointer currentMeshingParametersNode = hm->getFirstDescendant(meshingParametersOwnerNode, crimson::VesselMeshingNodeTypes::MeshingParameters(), true);

    if (!currentMeshingParametersNode) {
        // Create the meshing parameters node if it doesn't exist
        currentMeshingParametersNode = mitk::DataNode::New();
        currentMeshingParametersNode->SetName(meshingParametersOwnerNode->GetName() + " Meshing Parameters");
        currentMeshingParametersNode->SetBoolProperty("hidden object", true);
        currentMeshingParametersNode->SetData(crimson::MeshingParametersData::New());

        hm->addNodeToHierarchy(meshingParametersOwnerNode, ownerNodeType, currentMeshingParametersNode, crimson::VesselMeshingNodeTypes::MeshingParameters());
    }

    return static_cast<crimson::MeshingParametersData*>(currentMeshingParametersNode->GetData());
}

crimson::AsyncTaskManager::TaskUID MeshingUtils::getMeshingTaskUID(mitk::DataNode* solidNode)
{
    if (!solidNode) {
        return "";
    }

    return (boost::format("Meshing %1%") % solidNode).str();
}

}