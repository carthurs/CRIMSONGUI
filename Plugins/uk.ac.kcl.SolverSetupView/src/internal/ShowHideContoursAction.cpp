#include "ShowHideContoursAction.h"
#include "PCMRIUtils.h"

#include <HierarchyManager.h>
#include <SolverSetupNodeTypes.h>

#include <mitkRenderingManager.h>
#include <mitkBaseRenderer.h>

ShowHideContoursAction::ShowHideContoursAction(bool show)
    : _show(show)
{
}

ShowHideContoursAction::~ShowHideContoursAction()
{
}

void ShowHideContoursAction::setContourVisibility(mitk::DataNode* contourNode, bool show, mitk::RenderingManager* manager)
{
    for (vtkRenderWindow* renderWindow : mitk::RenderingManager::GetInstance()->GetAllRegisteredRenderWindows()) {
        mitk::BaseRenderer* renderer = mitk::BaseRenderer::GetInstance(renderWindow);
        if (renderer && renderer->GetMapperID() == mitk::BaseRenderer::Standard3D) {
            contourNode->SetVisibility(show, renderer);
        }
    }

	for (vtkRenderWindow* renderWindow : manager->GetInstance()->GetAllRegisteredRenderWindows()) {
		mitk::BaseRenderer* renderer = mitk::BaseRenderer::GetInstance(renderWindow);
		if (renderer && renderer->GetMapperID() == mitk::BaseRenderer::Standard3D) {
			contourNode->SetVisibility(show, renderer);
		}
	}
}

void ShowHideContoursAction::setContourVisibility(mitk::DataNode* contourNode, bool show)
{
	for (vtkRenderWindow* renderWindow : mitk::RenderingManager::GetInstance()->GetAllRegisteredRenderWindows()) {
		mitk::BaseRenderer* renderer = mitk::BaseRenderer::GetInstance(renderWindow);
		if (renderer && renderer->GetMapperID() == mitk::BaseRenderer::Standard3D) {
			contourNode->SetVisibility(show, renderer);
		}
	}
}

void ShowHideContoursAction::setAllVesselContoursVisibility(mitk::DataNode* vesselPathNode, bool show, mitk::RenderingManager* manager)
{
    // Find all the vessel paths in the vessel tree
    vesselPathNode->SetBoolProperty("lofting.contoursVisible", show);

	mitk::DataStorage::SetOfObjects::ConstPointer contours = crimson::PCMRIUtils::getContourNodes(vesselPathNode);
    for (mitk::DataNode* contourNode : *contours) {
        setContourVisibility(contourNode, show, manager);
    }

    manager->GetInstance()->RequestUpdateAll();
}

void ShowHideContoursAction::setAllVesselContoursVisibility(mitk::DataNode* vesselPathNode, bool show)
{
	// Find all the vessel paths in the vessel tree
	vesselPathNode->SetBoolProperty("lofting.contoursVisible", show);

	mitk::DataStorage::SetOfObjects::ConstPointer contours = crimson::PCMRIUtils::getContourNodes(vesselPathNode);
	for (mitk::DataNode* contourNode : *contours) {
		setContourVisibility(contourNode, show);
	}

	mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void ShowHideContoursAction::Run(const QList<mitk::DataNode::Pointer> &selectedNodes)
{
    for (const mitk::DataNode::Pointer& node : selectedNodes) {
        auto hm = crimson::HierarchyManager::getInstance();

        if (!hm->getPredicate(crimson::SolverSetupNodeTypes::BoundaryCondition())->CheckNode(node)) {
            continue;
        }

        setAllVesselContoursVisibility(node, _show);
    }
}