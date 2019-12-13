#pragma once

#include <HierarchyManager.h>
#include "uk_ac_kcl_VascularModeling_Eager_Export.h"

namespace crimson {

struct VASCULARMODELINGEAGER_EXPORT VascularModelingNodeTypes {
    DECLARE_HIERARCHY_MANAGER_NODE_TYPE(Image)
    DECLARE_HIERARCHY_MANAGER_NODE_TYPE(VesselTree)
    DECLARE_HIERARCHY_MANAGER_NODE_TYPE(VesselPath)
    DECLARE_HIERARCHY_MANAGER_NODE_TYPE(Solid)
	DECLARE_HIERARCHY_MANAGER_NODE_TYPE(Surface)
    DECLARE_HIERARCHY_MANAGER_NODE_TYPE(Loft)
    DECLARE_HIERARCHY_MANAGER_NODE_TYPE(LoftPreview)
    DECLARE_HIERARCHY_MANAGER_NODE_TYPE(Blend)
    DECLARE_HIERARCHY_MANAGER_NODE_TYPE(BlendPreview)
    DECLARE_HIERARCHY_MANAGER_NODE_TYPE(Contour)
    DECLARE_HIERARCHY_MANAGER_NODE_TYPE(ContourSegmentationImage)
    DECLARE_HIERARCHY_MANAGER_NODE_TYPE(ContourReferenceImage)
};

}