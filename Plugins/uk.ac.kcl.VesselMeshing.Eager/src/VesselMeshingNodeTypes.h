#pragma once

#include <HierarchyManager.h>
#include "uk_ac_kcl_VesselMeshing_Eager_Export.h"

namespace crimson {

struct VESSELMESHINGEAGER_EXPORT VesselMeshingNodeTypes {
    DECLARE_HIERARCHY_MANAGER_NODE_TYPE(Mesh)
    DECLARE_HIERARCHY_MANAGER_NODE_TYPE(MeshingParameters)
};

}