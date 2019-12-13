#include "LoftAction.h"
#include "VascularModelingUtils.h"

#include <HierarchyManager.h>
#include <VascularModelingNodeTypes.h>

#include <AsyncTaskManager.h>
#include <CreateDataNodeAsyncTask.h>

#include <ISolidModelKernel.h>

#include <mitkProperties.h>

LoftAction::LoftAction(bool preview, double interContourDistance)
	: _preview(preview)
	, _interContourDistance(interContourDistance)
{
}

LoftAction::~LoftAction() {}

std::shared_ptr<crimson::CreateDataNodeAsyncTask> LoftAction::Run(const mitk::DataNode::Pointer& node)
{
	auto hm = crimson::HierarchyManager::getInstance();

	if (!hm->getPredicate(crimson::VascularModelingNodeTypes::VesselPath())->CheckNode(node)) {
		return std::shared_ptr<crimson::CreateDataNodeAsyncTask>();
	}

	int loftingAlgorithm = crimson::ISolidModelKernel::laAppSurf;
	node->GetIntProperty("lofting.loftingAlgorithm", loftingAlgorithm);

    int seamEdgeRotation = 0;
    node->GetIntProperty("lofting.seamEdgeRotation", seamEdgeRotation);

    std::vector<mitk::DataNode*> contourNodes = crimson::VascularModelingUtils::getVesselContourNodesSortedByParameter(node);
	crimson::ISolidModelKernel::ContourSet contours(contourNodes.size());

    // Extract contours from the data nodes
	std::transform(contourNodes.begin(), contourNodes.end(), contours.begin(), [](const mitk::DataNode* n) {
		auto pf = static_cast<mitk::PlanarFigure*>(n->GetData());
		float p;
		n->GetFloatProperty("lofting.parameterValue", p);
		pf->GetPropertyList()->SetFloatProperty("lofting.parameterValue", p);
		return pf;
	});

    // Ignore unfinished contours
	contours.erase(std::remove_if(contours.begin(), contours.end(), [](const mitk::PlanarFigure* pf) {
		return !pf->IsFinalized();
	}), contours.end());

	bool useInflowAsWall = false, useOutflowAsWall = false;
	node->GetBoolProperty("lofting.useInflowAsWall", useInflowAsWall);
	node->GetBoolProperty("lofting.useOutflowAsWall", useOutflowAsWall);

    // Create the lofting task
	auto loftingTask = crimson::ISolidModelKernel::createLoftTask(
		static_cast<crimson::VesselPathAbstractData*>(node->GetData()), contours, useInflowAsWall, useOutflowAsWall,
		crimson::ISolidModelKernel::LoftingAlgorithm(loftingAlgorithm), seamEdgeRotation, _preview, _interContourDistance);

    // Setup the node properties for the lofted model
	std::map<std::string, mitk::BaseProperty::Pointer> props;
	if (_preview) {
		props["helper object"] = mitk::BoolProperty::New(true).GetPointer();
		props["lofting.loft_preview"] = mitk::BoolProperty::New(true).GetPointer();
		props["name"] = mitk::StringProperty::New(node->GetName() + " Loft preview").GetPointer();
	}
	else {
		mitk::DataNode::Pointer prevLoft = hm->getFirstDescendant(node, crimson::VascularModelingNodeTypes::Loft());

		props["lofting.loft_result"] = mitk::BoolProperty::New(true).GetPointer();

		if (prevLoft) {
			props["name"] = prevLoft->GetProperty("name");
			props["color"] = prevLoft->GetProperty("color");
			props["opacity"] = prevLoft->GetProperty("opacity");
		}
		else {
			props["name"] = mitk::StringProperty::New(node->GetName() + " Lofted").GetPointer();
		}
	}

    // Launch the lofting task
	auto dataNodeTask = std::make_shared<crimson::CreateDataNodeAsyncTask>(
		loftingTask, node, crimson::VascularModelingNodeTypes::VesselPath(),
		_preview ? crimson::VascularModelingNodeTypes::LoftPreview() : crimson::VascularModelingNodeTypes::Loft(), props);
	dataNodeTask->setDescription(std::string("Loft ") + node->GetName());
	dataNodeTask->setSilentFail(_preview);

	crimson::AsyncTaskManager::getInstance()->addTask(dataNodeTask,
		_preview ? crimson::VascularModelingUtils::getPreviewLoftingTaskUID(node)
		: crimson::VascularModelingUtils::getLoftingTaskUID(node));

	return dataNodeTask;
}

void LoftAction::Run(const QList<mitk::DataNode::Pointer>& selectedNodes)
{
	for (const mitk::DataNode::Pointer& node : selectedNodes) {
		Run(node);
	}
}