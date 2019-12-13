#include "uk_ac_kcl_CRIMSONApp_Activator.h"

#include "CRIMSONApplication.h"
#include "internal/Perspectives/GeometryModelingPerspective.h"
#include "internal/Perspectives/MeshingAndSolverSetupPerspective.h"

#include <mitkWorkbenchUtil.h>

namespace crimson {


ctkPluginContext* uk_ac_kcl_CRIMSONApp_Activator::PluginContext;

void uk_ac_kcl_CRIMSONApp_Activator::start(ctkPluginContext* context)
{
    BERRY_REGISTER_EXTENSION_CLASS(GeometryModelingPerspective, context)
    BERRY_REGISTER_EXTENSION_CLASS(MeshingAndSolverSetupPerspective, context)
    BERRY_REGISTER_EXTENSION_CLASS(CRIMSONApplication, context)

    PluginContext = context;

    mitk::WorkbenchUtil::SetDepartmentLogoPreference(":/KCL-logo.png", context);
}

void uk_ac_kcl_CRIMSONApp_Activator::stop(ctkPluginContext* context)
{
    Q_UNUSED(context)
}

} // namespace crimson

