#pragma once

#include <HierarchyManager.h>
#include "uk_ac_kcl_SolverSetup_Export.h"

namespace crimson {

struct SOLVERSETUP_EXPORT SolverSetupNodeTypes {
    static const char* solverRootPropertyName;
    static const char* adaptationDataPropertyName;
    DECLARE_HIERARCHY_MANAGER_NODE_TYPE(SolverRoot)
    DECLARE_HIERARCHY_MANAGER_NODE_TYPE(SolverParameters)
    DECLARE_HIERARCHY_MANAGER_NODE_TYPE(BoundaryConditionSet)
    DECLARE_HIERARCHY_MANAGER_NODE_TYPE(BoundaryCondition)
    DECLARE_HIERARCHY_MANAGER_NODE_TYPE(Material)
    DECLARE_HIERARCHY_MANAGER_NODE_TYPE(SolverStudy)
    DECLARE_HIERARCHY_MANAGER_NODE_TYPE(Solution)
    DECLARE_HIERARCHY_MANAGER_NODE_TYPE(AdaptationData)
	DECLARE_HIERARCHY_MANAGER_NODE_TYPE(PCMRIData)
	DECLARE_HIERARCHY_MANAGER_NODE_TYPE(MRAPoint)
	DECLARE_HIERARCHY_MANAGER_NODE_TYPE(PCMRIPoint)

};

}