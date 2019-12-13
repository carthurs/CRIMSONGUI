#include "PCMRIUtils.h"

#include <HierarchyManager.h>
#include <VascularModelingNodeTypes.h>
#include <SolverSetupNodeTypes.h>
#include <VesselPathAbstractData.h>

#include <mitkPlanarFigure.h>
#include <mitkNodePredicateData.h>
#include <mitkPlaneGeometry.h>
#include <mitkImage.h>

#include <boost/format.hpp>

namespace crimson {


crimson::AsyncTaskManager::TaskUID PCMRIUtils::getMappingTaskUID(mitk::DataNode* BCNode)
{
	if (!BCNode) {
        return "";
    }

    return (boost::format("Mapping %1%") % BCNode).str();
}

mitk::Point3D PCMRIUtils::getPlanarFigureGeometryCenter(const mitk::DataNode* node)
{
    auto figure = static_cast<const mitk::PlanarFigure*>(node->GetData());
    return figure->GetPlaneGeometry()->GetCenter();
}

mitk::PlaneGeometry::Pointer PCMRIUtils::getMRAPlane(mitk::DataNode* BCNode, mitk::DataNode* mraNode, mitk::DataNode* solidNode, FaceIdentifier selectedFace)
{
	float resliceWindowSize = 50;
	BCNode->GetFloatProperty("reslice.windowSize", resliceWindowSize);

	mitk::ScalarType paramDelta;
	mitk::Vector3D referenceImageSpacing;
	unsigned int timeSteps;

	std::tie(paramDelta, referenceImageSpacing, timeSteps) = crimson::PCMRIUtils::getResliceGeometryParameters(mraNode);

	auto plane = dynamic_cast<SolidData*>(solidNode->GetData())->getFaceGeometry(selectedFace, referenceImageSpacing, resliceWindowSize);

	return plane;
}

mitk::ScalarType PCMRIUtils::assignPlanarFigureParameter(mitk::DataNode* BCNode, mitk::DataNode* figureNode, mitk::BaseRenderer*  manager)
{
	//auto closestPtRequest = static_cast<VesselPathAbstractData*>(BCNode->GetData())->getClosestPoint(getPlanarFigureGeometryCenter(figureNode));

	//figureNode->SetFloatProperty("mapping.parameterValue", closestPtRequest.t);
	//return closestPtRequest.t;

	//TODO can a PlanarFigure node contain time data?

	auto param = manager->GetSliceNavigationController()->GetTime()->GetPos();

	figureNode->SetFloatProperty("mapping.parameterValue", param);
	return param;
}

void PCMRIUtils::setDefaultContourNodeProperties(mitk::DataNode* contourNode, bool selected)
{
    contourNode->SetBoolProperty("planarfigure.3drendering", true);
    contourNode->SetBoolProperty("planarfigure.drawname", false);
    contourNode->SetFloatProperty("planarfigure.angletolerance", 3.f);
    contourNode->SetColor(0.3, 0.2, 0.94, nullptr, "planarfigure.default.marker.color");
    contourNode->SetColor(0.61, 0.58, 0.95, nullptr, "planarfigure.default.markerline.color");
    contourNode->SetBoolProperty("selected", selected);
    contourNode->SetBoolProperty("lofting.lofted", true);
    contourNode->SetName("contour");
    contourNode->SetBoolProperty("hidden object", true);
}

void PCMRIUtils::setDefaultPointNodeProperties(mitk::DataNode* pointNode, bool selected, float* color, mitk::BaseRenderer* renderer)
{
	pointNode->SetBoolProperty("point.3drendering", true, renderer);
	pointNode->SetBoolProperty("point.drawname", false, renderer);
	pointNode->SetColor(color, renderer);
	pointNode->SetColor(color, renderer, "selectedcolor");
	pointNode->SetColor(color);
	pointNode->SetColor(color, nullptr, "selectedcolor");
	//pointNode->SetBoolProperty("selected", selected);
	pointNode->SetBoolProperty("visible", true, renderer);
	pointNode->SetBoolProperty("Vertex Rendering", true, renderer);
	pointNode->SetBoolProperty("updateDataOnRender", true, renderer);
	pointNode->SetProperty("line width", mitk::IntProperty::New(50.0f), renderer);
	pointNode->SetProperty("point line width", mitk::IntProperty::New(5.0f), renderer);
	pointNode->SetProperty("point 2D size", mitk::FloatProperty::New(15.0f), renderer);
	pointNode->SetProperty("pointsize", mitk::FloatProperty::New(10.0f));
	//pointNode->AddProperty("selectedcolor", mitk::ColorProperty::New(1.0f, 1.0f, 0.0f), renderer, true);  //yellow for selected
	//pointNode->AddProperty("unselectedcolor", mitk::ColorProperty::New(0.5f, 1.0f, 0.5f), renderer, true);  // middle green for unselected
	//pointNode->AddProperty("color", mitk::ColorProperty::New(1.0f, 0.0f, 0.0f), renderer, true);  // red as standard
	pointNode->SetProperty("show contour", mitk::BoolProperty::New(false), renderer);
	pointNode->SetProperty("show points", mitk::BoolProperty::New(true), renderer);
	pointNode->SetProperty("opacity", mitk::FloatProperty::New(1.0), renderer);
	pointNode->SetBoolProperty("hidden object", true);
}

std::map<float, mitk::DataNode*> PCMRIUtils::updatePlanarFigureParameters(mitk::DataNode* bcNode, const mitk::DataNode* nodeToIgnore, mitk::BaseRenderer*  manager, bool forceUpdate)
{
    std::map<float, mitk::DataNode*> parameterMap;
	mitk::DataStorage::SetOfObjects::ConstPointer nodes = getContourNodes(bcNode);
    for (const mitk::DataNode::Pointer& node : *nodes) {
        if (node.GetPointer() == nodeToIgnore) {
            continue;
        }

        float param;
        if (forceUpdate || !node->GetFloatProperty("mapping.parameterValue", param) || param < 0) {
			param = assignPlanarFigureParameter(bcNode, node, manager);
			//TODO can a PlanarFigure node contain time data?
			//param = mitk::RenderingManager::GetInstance()->GetTimeNavigationController()->GetTime()->GetPos();
        }
        parameterMap[param] = node;
    }
    return parameterMap;
}

mitk::DataStorage::SetOfObjects::ConstPointer PCMRIUtils::getContourNodes(mitk::DataNode* bcNode)
{
	return crimson::HierarchyManager::getInstance()->getDescendants(bcNode, crimson::VascularModelingNodeTypes::Contour(), true);
}

mitk::DataNode::Pointer PCMRIUtils::getMraPointNode(mitk::DataNode* bcNode)
{
	return crimson::HierarchyManager::getInstance()->getFirstDescendant(bcNode, crimson::SolverSetupNodeTypes::MRAPoint(), true);
}

mitk::DataNode::Pointer PCMRIUtils::getPcmriPointNode(mitk::DataNode* bcNode)
{
	return crimson::HierarchyManager::getInstance()->getFirstDescendant(bcNode, crimson::SolverSetupNodeTypes::PCMRIPoint(), true);
}

std::vector<mitk::DataNode*> PCMRIUtils::getContourNodesSortedByParameter(mitk::DataNode* bcNode)
{
	mitk::DataStorage::SetOfObjects::ConstPointer nodes_ = getContourNodes(bcNode);
	//updatePlanarFigureParameters(bcNode, nullptr);

    std::vector<mitk::DataNode*> nodes;
    nodes.resize(nodes_->size());
    std::transform(nodes_->begin(), nodes_->end(), nodes.begin(), [](mitk::DataNode::Pointer n) { return n.GetPointer();  });

    std::sort(nodes.begin(), nodes.end(),
        [](const mitk::DataNode* l, const mitk::DataNode* r) { float lv, rv; l->GetFloatProperty("mapping.parameterValue", lv); r->GetFloatProperty("mapping.parameterValue", rv); return lv < rv; });

    return nodes;
}

//pass image node here
std::tuple<double, mitk::Vector3D, unsigned int> PCMRIUtils::getResliceGeometryParameters(mitk::DataNode::Pointer imageNode)
{
    if (imageNode.IsNull()) {
        return std::make_tuple(1.0, mitk::Vector3D(1.0), 1U);
    }

    auto imageData = static_cast<mitk::Image*>(imageNode->GetData());
    float spacing[3];
    imageData->GetGeometry()->GetSpacing().ToArray(spacing);
    double paramDelta = (*std::min_element(spacing, spacing + 3)) / 2;

//    timeBounds = imageData->GetTimeGeometry()->GetTimeBounds();
//    timeBounds[1] = timeBounds[0] + (timeBounds[1] - timeBounds[0]) / timeSteps;

    return std::make_tuple(paramDelta, imageData->GetGeometry()->GetSpacing(), imageData->GetTimeSteps());
}

}