#include "VascularModelingUtils.h"

#include <HierarchyManager.h>
#include <VascularModelingNodeTypes.h>
#include <VesselPathAbstractData.h>

#include <mitkPlanarFigure.h>
#include <mitkNodePredicateData.h>
#include <mitkPlaneGeometry.h>
#include <mitkImage.h>

#include <boost/format.hpp>

namespace crimson {

crimson::AsyncTaskManager::TaskUID VascularModelingUtils::getLoftingTaskUID(mitk::DataNode* vesselPathNode)
{
    if (!vesselPathNode) {
        return "";
    }

    return (boost::format("Lofting %1%") % vesselPathNode).str();
}

crimson::AsyncTaskManager::TaskUID VascularModelingUtils::getPreviewLoftingTaskUID(mitk::DataNode* vesselPathNode)
{
    if (!vesselPathNode) {
        return "";
    }

    return (boost::format("Previewing loft %1%") % vesselPathNode).str();
}

crimson::AsyncTaskManager::TaskUID VascularModelingUtils::getBlendingTaskUID(mitk::DataNode* vesselForestNode)
{
    if (!vesselForestNode) {
        return "";
    }

    return (boost::format("Blending %1%") % vesselForestNode).str();
}

mitk::Point3D VascularModelingUtils::getPlanarFigureGeometryCenter(const mitk::DataNode* node)
{
    auto figure = static_cast<const mitk::PlanarFigure*>(node->GetData());
    return figure->GetPlaneGeometry()->GetCenter();
}

mitk::ScalarType VascularModelingUtils::assignPlanarFigureParameter(mitk::DataNode* vesselPathNode, mitk::DataNode* figureNode)
{
    auto closestPtRequest = static_cast<VesselPathAbstractData*>(vesselPathNode->GetData())->getClosestPoint(getPlanarFigureGeometryCenter(figureNode));

    figureNode->SetFloatProperty("lofting.parameterValue", closestPtRequest.t);
    return closestPtRequest.t;
}

void VascularModelingUtils::setDefaultContourNodeProperties(mitk::DataNode* contourNode, bool selected)
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

std::map<float, mitk::DataNode*> VascularModelingUtils::updatePlanarFigureParameters(mitk::DataNode* vesselPathNode, const mitk::DataNode* nodeToIgnore, bool forceUpdate)
{
    std::map<float, mitk::DataNode*> parameterMap;
    mitk::DataStorage::SetOfObjects::ConstPointer nodes = getVesselContourNodes(vesselPathNode);
    for (const mitk::DataNode::Pointer& node : *nodes) {
        if (node.GetPointer() == nodeToIgnore) {
            continue;
        }

        float param;
        if (forceUpdate || !node->GetFloatProperty("lofting.parameterValue", param) || param < 0) {
            param = assignPlanarFigureParameter(vesselPathNode, node);
        }
        parameterMap[param] = node;
    }
    return parameterMap;
}

mitk::DataStorage::SetOfObjects::ConstPointer VascularModelingUtils::getVesselContourNodes(mitk::DataNode* vesselPathNode)
{
    return crimson::HierarchyManager::getInstance()->getDescendants(vesselPathNode, crimson::VascularModelingNodeTypes::Contour());
}

std::vector<mitk::DataNode*> VascularModelingUtils::getVesselContourNodesSortedByParameter(mitk::DataNode* vesselPathNode)
{
    mitk::DataStorage::SetOfObjects::ConstPointer nodes_ = getVesselContourNodes(vesselPathNode);
    updatePlanarFigureParameters(vesselPathNode, nullptr);

    std::vector<mitk::DataNode*> nodes;
    nodes.resize(nodes_->size());
    std::transform(nodes_->begin(), nodes_->end(), nodes.begin(), [](mitk::DataNode::Pointer n) { return n.GetPointer();  });

    std::sort(nodes.begin(), nodes.end(),
        [](const mitk::DataNode* l, const mitk::DataNode* r) { float lv, rv; l->GetFloatProperty("lofting.parameterValue", lv); r->GetFloatProperty("lofting.parameterValue", rv); return lv < rv; });

    return nodes;
}


mitk::DataNode* VascularModelingUtils::getVesselPathNodeByData(VesselPathAbstractData* vessel)
{
    return crimson::HierarchyManager::getInstance()->getDataStorage()->GetNode(mitk::NodePredicateData::New(vessel));
}

mitk::DataNode* VascularModelingUtils::getVesselSolidModelNode(mitk::DataNode* vesselNode)
{
    return crimson::HierarchyManager::getInstance()->getFirstDescendant(vesselNode, crimson::VascularModelingNodeTypes::Loft());
}

mitk::DataNode* VascularModelingUtils::getVesselPathNodeByUID(mitk::DataNode* vesselForestNode, const VesselPathAbstractData::VesselPathUIDType& vesselUID)
{
    mitk::DataStorage::SetOfObjects::ConstPointer vesselPathNodes = 
        crimson::HierarchyManager::getInstance()->getDescendants(vesselForestNode, crimson::VascularModelingNodeTypes::VesselPath());

    for (const mitk::DataNode::Pointer& node : *vesselPathNodes) {
        if (static_cast<VesselPathAbstractData*>(node->GetData())->getVesselUID() == vesselUID) {
            return node.GetPointer();
        }
    }

    return nullptr;
}

std::tuple<double, mitk::Vector3D, unsigned int> VascularModelingUtils::getResliceGeometryParameters(mitk::DataNode* vesselNode)
{
    mitk::DataNode::Pointer imageNode = crimson::HierarchyManager::getInstance()->getAncestor(vesselNode, crimson::VascularModelingNodeTypes::Image());

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