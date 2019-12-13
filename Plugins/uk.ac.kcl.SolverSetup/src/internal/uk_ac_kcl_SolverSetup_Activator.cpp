#include "uk_ac_kcl_SolverSetup_Activator.h"

#include <ISolverParametersData.h>
#include <IBoundaryConditionSet.h>
#include <IBoundaryCondition.h>
#include <ISolverStudyData.h>
#include <IMaterialData.h>
#include <PCMRIData.h>

#include <QmitkNodeDescriptorManager.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateProperty.h>
#include <mitkNodePredicateAnd.h>
#include <mitkPointSet.h>

#include <HierarchyManager.h>
#include <VascularModelingNodeTypes.h>
#include <SolverSetupNodeTypes.h>
#include <VesselMeshingNodeTypes.h>

ctkPluginContext* uk_ac_kcl_SolverSetup_Activator::PluginContext;

void uk_ac_kcl_SolverSetup_Activator::start(ctkPluginContext* context)
{
    auto hm = crimson::HierarchyManager::getInstance();
    hm->addNodeType(crimson::SolverSetupNodeTypes::SolverRoot(),
                    mitk::NodePredicateProperty::New(crimson::SolverSetupNodeTypes::solverRootPropertyName).GetPointer(),
                    crimson::HierarchyManager::ntfUndoableDeletion | crimson::HierarchyManager::ntfRecursiveDeletion);
    hm->addNodeType(crimson::SolverSetupNodeTypes::SolverParameters(),
                    mitk::TNodePredicateDataType<crimson::ISolverParametersData>::New().GetPointer(),
                    crimson::HierarchyManager::ntfUndoableDeletion | crimson::HierarchyManager::ntfRecursiveDeletion);
    hm->addNodeType(crimson::SolverSetupNodeTypes::BoundaryConditionSet(),
                    mitk::TNodePredicateDataType<crimson::IBoundaryConditionSet>::New().GetPointer(),
                    crimson::HierarchyManager::ntfUndoableDeletion | crimson::HierarchyManager::ntfRecursiveDeletion);
    hm->addNodeType(crimson::SolverSetupNodeTypes::BoundaryCondition(),
                    mitk::TNodePredicateDataType<crimson::IBoundaryCondition>::New().GetPointer(),
                    crimson::HierarchyManager::ntfUndoableDeletion | crimson::HierarchyManager::ntfRecursiveDeletion);
    hm->addNodeType(crimson::SolverSetupNodeTypes::Material(), 
                    mitk::TNodePredicateDataType<crimson::IMaterialData>::New().GetPointer(),
                    crimson::HierarchyManager::ntfUndoableDeletion | crimson::HierarchyManager::ntfRecursiveDeletion);
    hm->addNodeType(crimson::SolverSetupNodeTypes::SolverStudy(), mitk::TNodePredicateDataType<crimson::ISolverStudyData>::New().GetPointer(),
                    crimson::HierarchyManager::ntfUndoableDeletion | crimson::HierarchyManager::ntfRecursiveDeletion);
    hm->addNodeType(crimson::SolverSetupNodeTypes::Solution(),
                    mitk::TNodePredicateDataType<crimson::SolutionData>::New().GetPointer(),
                    crimson::HierarchyManager::ntfUndoableDeletion | crimson::HierarchyManager::ntfRecursiveDeletion);
    hm->addNodeType(crimson::SolverSetupNodeTypes::AdaptationData(),
                    mitk::NodePredicateProperty::New(crimson::SolverSetupNodeTypes::adaptationDataPropertyName).GetPointer(),
                    crimson::HierarchyManager::ntfUndoableDeletion | crimson::HierarchyManager::ntfRecursiveDeletion);
	hm->addNodeType(crimson::SolverSetupNodeTypes::PCMRIData(),
					mitk::TNodePredicateDataType<crimson::PCMRIData>::New().GetPointer(),
					crimson::HierarchyManager::ntfUndoableDeletion | crimson::HierarchyManager::ntfRecursiveDeletion);
	hm->addNodeType(crimson::SolverSetupNodeTypes::MRAPoint(), mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::PointSet>::New(),
					mitk::NodePredicateProperty::New("mapping.mraPoint")).GetPointer(), crimson::HierarchyManager::ntfUndoableDeletion | 
					crimson::HierarchyManager::ntfRecursiveDeletion);
	hm->addNodeType(crimson::SolverSetupNodeTypes::PCMRIPoint(), mitk::NodePredicateAnd::New(mitk::TNodePredicateDataType<mitk::PointSet>::New(),
					mitk::NodePredicateProperty::New("mapping.pcmriPoint")).GetPointer(), crimson::HierarchyManager::ntfUndoableDeletion | 
					crimson::HierarchyManager::ntfRecursiveDeletion);

    hm->addRelation(crimson::SolverSetupNodeTypes::SolverRoot(), crimson::SolverSetupNodeTypes::SolverParameters(),
                    crimson::HierarchyManager::rtOneToMany);
    hm->addRelation(crimson::SolverSetupNodeTypes::SolverRoot(), crimson::SolverSetupNodeTypes::BoundaryConditionSet(),
                    crimson::HierarchyManager::rtOneToMany);
    hm->addRelation(crimson::SolverSetupNodeTypes::BoundaryConditionSet(), crimson::SolverSetupNodeTypes::BoundaryCondition(),
                    crimson::HierarchyManager::rtOneToMany);
    hm->addRelation(crimson::SolverSetupNodeTypes::SolverRoot(), crimson::SolverSetupNodeTypes::Material(),
                    crimson::HierarchyManager::rtOneToMany);
    hm->addRelation(crimson::SolverSetupNodeTypes::SolverRoot(), crimson::SolverSetupNodeTypes::SolverStudy(),
                    crimson::HierarchyManager::rtOneToMany);
    hm->addRelation(crimson::SolverSetupNodeTypes::SolverStudy(), crimson::SolverSetupNodeTypes::Solution(),
                    crimson::HierarchyManager::rtOneToMany);
    hm->addRelation(crimson::SolverSetupNodeTypes::SolverStudy(), crimson::SolverSetupNodeTypes::AdaptationData(),
                    crimson::HierarchyManager::rtOneToMany);

    hm->addRelation(crimson::VascularModelingNodeTypes::VesselTree(), crimson::SolverSetupNodeTypes::SolverRoot(),
                    crimson::HierarchyManager::rtOneToMany);
    // For imported data not associated with vessel tree
    hm->addRelation(crimson::VascularModelingNodeTypes::Solid(), crimson::SolverSetupNodeTypes::SolverRoot(),
                    crimson::HierarchyManager::rtOneToMany);

	//For PCMRI mapping
	hm->addRelation(crimson::VascularModelingNodeTypes::Image(), crimson::VascularModelingNodeTypes::Contour(),
					crimson::HierarchyManager::rtOneToMany);
	hm->addRelation(crimson::VascularModelingNodeTypes::Image(), crimson::VascularModelingNodeTypes::ContourReferenceImage(),
					crimson::HierarchyManager::rtOneToMany);
	hm->addRelation(crimson::VascularModelingNodeTypes::Image(), crimson::VascularModelingNodeTypes::ContourSegmentationImage(),
					crimson::HierarchyManager::rtOneToMany);
	hm->addRelation(crimson::SolverSetupNodeTypes::BoundaryCondition(), crimson::SolverSetupNodeTypes::PCMRIData(),
					crimson::HierarchyManager::rtOneToOne);
	hm->addRelation(crimson::SolverSetupNodeTypes::BoundaryCondition(), crimson::SolverSetupNodeTypes::MRAPoint(),
					crimson::HierarchyManager::rtOneToOne);
	hm->addRelation(crimson::VascularModelingNodeTypes::Image(), crimson::SolverSetupNodeTypes::PCMRIPoint(),
					crimson::HierarchyManager::rtOneToOne);


    QmitkNodeDescriptorManager* descriptorManager = QmitkNodeDescriptorManager::GetInstance();

    // Add icons
    descriptorManager->AddDescriptor(
        new QmitkNodeDescriptor(QObject::tr("SolverRoot"), QString(":/icons/icons/DataManagerIcon_root.png"),
                                hm->getPredicate(crimson::SolverSetupNodeTypes::SolverRoot()), descriptorManager));

    descriptorManager->AddDescriptor(
        new QmitkNodeDescriptor(QObject::tr("IBoundaryCondition"), QString(":/icons/icons/DataManagerIcon_bc.png"),
                                hm->getPredicate(crimson::SolverSetupNodeTypes::BoundaryCondition()), descriptorManager));

    descriptorManager->AddDescriptor(
        new QmitkNodeDescriptor(QObject::tr("IMaterialData"), QString(":/icons/icons/DataManagerIcon_material.png"),
                                hm->getPredicate(crimson::SolverSetupNodeTypes::Material()), descriptorManager));

    descriptorManager->AddDescriptor(
        new QmitkNodeDescriptor(QObject::tr("IBoundaryConditionSet"), QString(":/icons/icons/DataManagerIcon_bcs.png"),
                                hm->getPredicate(crimson::SolverSetupNodeTypes::BoundaryConditionSet()), descriptorManager));

    descriptorManager->AddDescriptor(
        new QmitkNodeDescriptor(QObject::tr("ISolverParametersData"), QString(":/icons/icons/DataManagerIcon_ss.png"),
                                hm->getPredicate(crimson::SolverSetupNodeTypes::SolverParameters()), descriptorManager));

    descriptorManager->AddDescriptor(
        new QmitkNodeDescriptor(QObject::tr("ISolverStudyData"), QString(":/icons/icons/DataManagerIcon_SolverStudy.png"),
                                hm->getPredicate(crimson::SolverSetupNodeTypes::SolverStudy()), descriptorManager));

    descriptorManager->AddDescriptor(
        new QmitkNodeDescriptor(QObject::tr("SolutionData"), QString(":/icons/icons/DataManagerIcon_solution.png"),
                                hm->getPredicate(crimson::SolverSetupNodeTypes::Solution()), descriptorManager));

    PluginContext = context;
}

void uk_ac_kcl_SolverSetup_Activator::stop(ctkPluginContext* context) { Q_UNUSED(context) }
