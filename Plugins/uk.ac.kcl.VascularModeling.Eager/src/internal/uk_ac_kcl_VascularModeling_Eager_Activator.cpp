#include "uk_ac_kcl_VascularModeling_Eager_Activator.h"

#include <QmitkNodeDescriptorManager.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateProperty.h>

#include <mitkImage.h>
#include <mitkPlanarFigure.h>

#include <SolidData.h>
#include <VesselForestData.h>

#include <HierarchyManager.h>
#include <VascularModelingNodeTypes.h>

#include <VesselTreeHierarchyController.h>


ctkPluginContext* uk_ac_kcl_VascularModeling_Eager_Activator::PluginContext;

void uk_ac_kcl_VascularModeling_Eager_Activator::start(ctkPluginContext* context)
{
    PluginContext = context;

    QmitkNodeDescriptorManager* descriptorManager = QmitkNodeDescriptorManager::GetInstance();
    auto hm = crimson::HierarchyManager::getInstance();

    hm->addNodeType(crimson::VascularModelingNodeTypes::Image(), mitk::TNodePredicateDataType<mitk::Image>::New().GetPointer());
    hm->addNodeType(crimson::VascularModelingNodeTypes::VesselTree(), mitk::TNodePredicateDataType<crimson::VesselForestData>::New().GetPointer(), crimson::HierarchyManager::ntfUndoableDeletion | crimson::HierarchyManager::ntfRecursiveDeletion);
    hm->addNodeType(crimson::VascularModelingNodeTypes::VesselPath(), mitk::TNodePredicateDataType<crimson::VesselPathAbstractData>::New().GetPointer(), crimson::HierarchyManager::ntfUndoableDeletion | crimson::HierarchyManager::ntfRecursiveDeletion | crimson::HierarchyManager::ntfPickable);
	hm->addNodeType(crimson::VascularModelingNodeTypes::Solid(), mitk::TNodePredicateDataType<crimson::SolidData>::New().GetPointer(), crimson::HierarchyManager::ntfRecursiveDeletion | crimson::HierarchyManager::ntfPickable);
    hm->addNodeType(crimson::VascularModelingNodeTypes::Loft(), mitk::NodePredicateProperty::New("lofting.loft_result").GetPointer(), crimson::HierarchyManager::ntfRecursiveDeletion | crimson::HierarchyManager::ntfPickable);
    hm->addNodeType(crimson::VascularModelingNodeTypes::LoftPreview(), mitk::NodePredicateProperty::New("lofting.loft_preview").GetPointer(), crimson::HierarchyManager::ntfRecursiveDeletion | crimson::HierarchyManager::ntfPickable);
    hm->addNodeType(crimson::VascularModelingNodeTypes::Blend(), mitk::NodePredicateProperty::New("lofting.blend_result").GetPointer(), crimson::HierarchyManager::ntfRecursiveDeletion | crimson::HierarchyManager::ntfPickable);
    hm->addNodeType(crimson::VascularModelingNodeTypes::BlendPreview(), mitk::NodePredicateProperty::New("lofting.blend_preview").GetPointer(), crimson::HierarchyManager::ntfRecursiveDeletion | crimson::HierarchyManager::ntfPickable);
    hm->addNodeType(crimson::VascularModelingNodeTypes::Contour(), mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::PlanarFigure>::New(), mitk::NodePredicateProperty::New("lofting.lofted")).GetPointer(), crimson::HierarchyManager::ntfUndoableDeletion | crimson::HierarchyManager::ntfRecursiveDeletion);
    hm->addNodeType(crimson::VascularModelingNodeTypes::ContourSegmentationImage(), mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::Image>::New(), mitk::NodePredicateProperty::New("lofting.contour_segmentation_image")).GetPointer(), crimson::HierarchyManager::ntfUndoableDeletion | crimson::HierarchyManager::ntfRecursiveDeletion);
    hm->addNodeType(crimson::VascularModelingNodeTypes::ContourReferenceImage(), mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::Image>::New(), mitk::NodePredicateProperty::New("lofting.contour_reference_image")).GetPointer(), crimson::HierarchyManager::ntfUndoableDeletion | crimson::HierarchyManager::ntfRecursiveDeletion);

    hm->addRelation(crimson::VascularModelingNodeTypes::Image(),         crimson::VascularModelingNodeTypes::VesselTree(),               crimson::HierarchyManager::rtOneToMany);
    hm->addRelation(crimson::VascularModelingNodeTypes::VesselTree(),    crimson::VascularModelingNodeTypes::VesselPath(),               crimson::HierarchyManager::rtOneToMany);
    hm->addRelation(crimson::VascularModelingNodeTypes::VesselTree(),    crimson::VascularModelingNodeTypes::Blend(),                    crimson::HierarchyManager::rtOneToOne);
    hm->addRelation(crimson::VascularModelingNodeTypes::VesselTree(),    crimson::VascularModelingNodeTypes::BlendPreview(),             crimson::HierarchyManager::rtOneToOne);
    hm->addRelation(crimson::VascularModelingNodeTypes::VesselPath(),    crimson::VascularModelingNodeTypes::Loft(),                     crimson::HierarchyManager::rtOneToOne);
    hm->addRelation(crimson::VascularModelingNodeTypes::VesselPath(),    crimson::VascularModelingNodeTypes::LoftPreview(),              crimson::HierarchyManager::rtOneToOne);
    hm->addRelation(crimson::VascularModelingNodeTypes::VesselPath(),    crimson::VascularModelingNodeTypes::Contour(),                  crimson::HierarchyManager::rtOneToMany);
    hm->addRelation(crimson::VascularModelingNodeTypes::Contour(),       crimson::VascularModelingNodeTypes::ContourSegmentationImage(), crimson::HierarchyManager::rtOneToOne);
    hm->addRelation(crimson::VascularModelingNodeTypes::Contour(),       crimson::VascularModelingNodeTypes::ContourReferenceImage(),    crimson::HierarchyManager::rtOneToOne);

    // Adding solid model
    mitk::NodePredicateBase::Pointer isSolid = crimson::HierarchyManager::getInstance()->getPredicate(crimson::VascularModelingNodeTypes::Solid());
    descriptorManager->AddDescriptor(new QmitkNodeDescriptor(QObject::tr("Solid"), QString(":/VascularModeling/icons/DataManagerIcon_Loft.png"), isSolid, descriptorManager));

    // Adding loft surface
    mitk::NodePredicateBase::Pointer isLoft = crimson::HierarchyManager::getInstance()->getPredicate(crimson::VascularModelingNodeTypes::Loft());
    descriptorManager->AddDescriptor(new QmitkNodeDescriptor(QObject::tr("LoftedSurface"), QString(":/VascularModeling/icons/DataManagerIcon_Loft.png"), isLoft, descriptorManager));

    // Adding blend surface
    mitk::NodePredicateBase::Pointer isBlend = crimson::HierarchyManager::getInstance()->getPredicate(crimson::VascularModelingNodeTypes::Blend());
    descriptorManager->AddDescriptor(new QmitkNodeDescriptor(QObject::tr("BlendedSurface"), QString(":/VascularModeling/icons/DataManagerIcon_Blend.png"), isBlend, descriptorManager));

    // Adding "VesselForestData"
    mitk::NodePredicateBase::Pointer isVesselForest = crimson::HierarchyManager::getInstance()->getPredicate(crimson::VascularModelingNodeTypes::VesselTree());
    descriptorManager->AddDescriptor(new QmitkNodeDescriptor(QObject::tr("VesselForestData"), QString(":/VascularModeling/icons/DataManagerIcon_VesselTree.png"), isVesselForest, descriptorManager));

    // Adding "VesselPathAbstractData"
    mitk::NodePredicateBase::Pointer isVesselPath = crimson::HierarchyManager::getInstance()->getPredicate(crimson::VascularModelingNodeTypes::VesselPath());
    descriptorManager->AddDescriptor(new QmitkNodeDescriptor(QObject::tr("VesselPathData"), QString(":/VascularModeling/icons/DataManagerIcon_VesselPath.png"), isVesselPath, descriptorManager));

    crimson::VesselTreeHierarchyController::init();
}

void uk_ac_kcl_VascularModeling_Eager_Activator::stop(ctkPluginContext* context)
{
    Q_UNUSED(context)
    crimson::VesselTreeHierarchyController::term();
}
