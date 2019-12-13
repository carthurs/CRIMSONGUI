#include "ShowMeshInformationAction.h"

#include <HierarchyManager.h>
#include <VesselMeshingNodeTypes.h>

std::unique_ptr<MeshInformationDialog> ShowMeshInformationAction::_meshInformationDialog;

ShowMeshInformationAction::ShowMeshInformationAction()
{
    if (!_meshInformationDialog) {
        _meshInformationDialog = std::make_unique<MeshInformationDialog>();
    }
}

ShowMeshInformationAction::~ShowMeshInformationAction()
{
}


void ShowMeshInformationAction::Run(const QList<mitk::DataNode::Pointer> &selectedNodes)
{
    for (const mitk::DataNode::Pointer& node : selectedNodes) {
        auto hm = crimson::HierarchyManager::getInstance();

        if (!hm->getPredicate(crimson::VesselMeshingNodeTypes::Mesh())->CheckNode(node)) {
            continue;
        }

        _meshInformationDialog->setMeshNode(node);
        _meshInformationDialog->exec();
        return;
    }
}