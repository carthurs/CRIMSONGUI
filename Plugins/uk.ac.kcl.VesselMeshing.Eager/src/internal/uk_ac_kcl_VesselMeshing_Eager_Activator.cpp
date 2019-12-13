#include "uk_ac_kcl_VesselMeshing_Eager_Activator.h"

#include <QtPlugin>

#include <QmitkNodeDescriptorManager.h>
#include <mitkNodePredicateDataType.h>

#include <HierarchyManager.h>
#include <VascularModelingNodeTypes.h>
#include <VesselMeshingNodeTypes.h>

#include <MeshData.h>
#include <MeshingParametersData.h>

ctkPluginContext* uk_ac_kcl_VesselMeshing_Eager_Activator::PluginContext;

void uk_ac_kcl_VesselMeshing_Eager_Activator::start(ctkPluginContext* context)
{
    QmitkNodeDescriptorManager* descriptorManager = QmitkNodeDescriptorManager::GetInstance();

    auto hm = crimson::HierarchyManager::getInstance();
    
    hm->addNodeType(crimson::VesselMeshingNodeTypes::Mesh(), mitk::TNodePredicateDataType<crimson::MeshData>::New().GetPointer(), crimson::HierarchyManager::ntfPickable | crimson::HierarchyManager::ntfRecursiveDeletion);
    hm->addNodeType(crimson::VesselMeshingNodeTypes::MeshingParameters(), mitk::TNodePredicateDataType<crimson::MeshingParametersData>::New().GetPointer(), crimson::HierarchyManager::ntfUndoableDeletion);

    hm->addRelation(crimson::VascularModelingNodeTypes::Solid(), crimson::VesselMeshingNodeTypes::Mesh(), crimson::HierarchyManager::rtOneToMany);
    hm->addRelation(crimson::VascularModelingNodeTypes::VesselPath(), crimson::VesselMeshingNodeTypes::MeshingParameters(), crimson::HierarchyManager::rtOneToOne);
    hm->addRelation(crimson::VascularModelingNodeTypes::VesselTree(), crimson::VesselMeshingNodeTypes::MeshingParameters(), crimson::HierarchyManager::rtOneToOne);
    hm->addRelation(crimson::VascularModelingNodeTypes::Solid(), crimson::VesselMeshingNodeTypes::MeshingParameters(), crimson::HierarchyManager::rtOneToOne);

    // Adding loft surface
    mitk::NodePredicateBase::Pointer isMesh = crimson::HierarchyManager::getInstance()->getPredicate(crimson::VesselMeshingNodeTypes::Mesh());
    descriptorManager->AddDescriptor(new QmitkNodeDescriptor(QObject::tr("Mesh"), QString(":/VesselMeshing/icons/DataManagerIcon_Mesh.png"), isMesh, descriptorManager));

    PluginContext = context;
}

void uk_ac_kcl_VesselMeshing_Eager_Activator::stop(ctkPluginContext* context)
{
    Q_UNUSED(context)
}
