#include "VesselTreeHierarchyController.h"

#include <ctkServiceTracker.h>
#include <mitkIDataStorageService.h>
#include <mitkIDataStorageReference.h>

#include <HierarchyManager.h>
#include <VascularModelingNodeTypes.h>

#include <VesselForestData.h>
#include <VesselPathAbstractData.h>

#include "internal/uk_ac_kcl_VascularModeling_Eager_Activator.h"

namespace crimson {

struct VesselTreeHierarchyController::VesselTreeHierarchyControllerImpl {
    VesselTreeHierarchyControllerImpl()
        : _dataStorageServiceTracker(uk_ac_kcl_VascularModeling_Eager_Activator::GetPluginContext())  {}


    ctkServiceTracker<mitk::IDataStorageService*> _dataStorageServiceTracker;
};

VesselTreeHierarchyController* VesselTreeHierarchyController::_instance = nullptr;

bool VesselTreeHierarchyController::init()
{
    assert(_instance == nullptr);
    _instance = new VesselTreeHierarchyController();
    return true;
}

void VesselTreeHierarchyController::term()
{
    assert(_instance != nullptr);
    delete _instance;
    _instance = nullptr;
}

VesselTreeHierarchyController* VesselTreeHierarchyController::getInstance()
{
    return _instance;
}

VesselTreeHierarchyController::VesselTreeHierarchyController()
    : _impl(new VesselTreeHierarchyControllerImpl())
{
    _impl->_dataStorageServiceTracker.open();

    _connectDataStorageEvents();
}

VesselTreeHierarchyController::~VesselTreeHierarchyController()
{
    _disconnectDataStorageEvents();

    _impl->_dataStorageServiceTracker.close();
}

void VesselTreeHierarchyController::_connectDataStorageEvents()
{
    getDataStorage()->AddNodeEvent.AddListener(
        mitk::MessageDelegate1<VesselTreeHierarchyController, const mitk::DataNode*>(this, &VesselTreeHierarchyController::nodeAdded));

    getDataStorage()->RemoveNodeEvent.AddListener(
        mitk::MessageDelegate1<VesselTreeHierarchyController, const mitk::DataNode*>(this, &VesselTreeHierarchyController::nodeRemoved));
}

void VesselTreeHierarchyController::_disconnectDataStorageEvents()
{
    getDataStorage()->AddNodeEvent.RemoveListener(
        mitk::MessageDelegate1<VesselTreeHierarchyController, const mitk::DataNode*>(this, &VesselTreeHierarchyController::nodeAdded));

    getDataStorage()->RemoveNodeEvent.RemoveListener(
        mitk::MessageDelegate1<VesselTreeHierarchyController, const mitk::DataNode*>(this, &VesselTreeHierarchyController::nodeRemoved));
}

mitk::DataStorage::Pointer VesselTreeHierarchyController::getDataStorage()
{
    mitk::IDataStorageService* dsService = _impl->_dataStorageServiceTracker.getService();

    if (dsService) {
        return dsService->GetDataStorage()->GetDataStorage();
    }

    return nullptr;
}

void VesselTreeHierarchyController::nodeAdded(const mitk::DataNode* node)
{
    auto hm = crimson::HierarchyManager::getInstance();
    if (hm->getPredicate(crimson::VascularModelingNodeTypes::VesselPath())->CheckNode(node)) {
        mitk::DataNode::Pointer parentVesselForestNode = hm->getAncestor(node, crimson::VascularModelingNodeTypes::VesselTree());
        if (parentVesselForestNode) {
            auto vesselPathData = static_cast<VesselPathAbstractData*>(node->GetData());
            auto vesselForestData = static_cast<VesselForestData*>(parentVesselForestNode->GetData());
            vesselForestData->insertVessel(vesselPathData);
        }
    }
}

void VesselTreeHierarchyController::nodeRemoved(const mitk::DataNode* node)
{
    auto hm = crimson::HierarchyManager::getInstance();
    if (hm->getPredicate(crimson::VascularModelingNodeTypes::VesselPath())->CheckNode(node)) {
        mitk::DataNode::Pointer parentVesselForestNode = hm->getAncestor(node, crimson::VascularModelingNodeTypes::VesselTree());
        if (parentVesselForestNode) {
            auto vesselPathData = static_cast<VesselPathAbstractData*>(node->GetData());
            auto vesselForestData = static_cast<VesselForestData*>(parentVesselForestNode->GetData());
            vesselForestData->removeVessel(vesselPathData);
        }
    }
}

} // namespace crimson
