#include "SolverSetupNodeTypes.h"

const char* crimson::SolverSetupNodeTypes::solverRootPropertyName = "SolverSetup.SolverType";
const char* crimson::SolverSetupNodeTypes::adaptationDataPropertyName = "SolverSetup.AdaptationData";

DEFINE_HIERARCHY_MANAGER_NODE_TYPE(crimson::SolverSetupNodeTypes, SolverRoot)
DEFINE_HIERARCHY_MANAGER_NODE_TYPE(crimson::SolverSetupNodeTypes, SolverParameters)
DEFINE_HIERARCHY_MANAGER_NODE_TYPE(crimson::SolverSetupNodeTypes, BoundaryConditionSet)
DEFINE_HIERARCHY_MANAGER_NODE_TYPE(crimson::SolverSetupNodeTypes, BoundaryCondition)
DEFINE_HIERARCHY_MANAGER_NODE_TYPE(crimson::SolverSetupNodeTypes, Material)
DEFINE_HIERARCHY_MANAGER_NODE_TYPE(crimson::SolverSetupNodeTypes, SolverStudy)
DEFINE_HIERARCHY_MANAGER_NODE_TYPE(crimson::SolverSetupNodeTypes, Solution)
DEFINE_HIERARCHY_MANAGER_NODE_TYPE(crimson::SolverSetupNodeTypes, AdaptationData)
DEFINE_HIERARCHY_MANAGER_NODE_TYPE(crimson::SolverSetupNodeTypes, PCMRIData)
DEFINE_HIERARCHY_MANAGER_NODE_TYPE(crimson::SolverSetupNodeTypes, MRAPoint)
DEFINE_HIERARCHY_MANAGER_NODE_TYPE(crimson::SolverSetupNodeTypes, PCMRIPoint)