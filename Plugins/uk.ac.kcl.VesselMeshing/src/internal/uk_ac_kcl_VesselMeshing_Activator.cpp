#include "uk_ac_kcl_VesselMeshing_Activator.h"

#include "VesselMeshingView.h"
#include "MeshExplorationView.h"
#include "MeshAction.h"
#include "ShowMeshInformationAction.h"
#include "ConvertToDiscreteModelAction.h"

ctkPluginContext* uk_ac_kcl_VesselMeshing_Activator::PluginContext;

void uk_ac_kcl_VesselMeshing_Activator::start(ctkPluginContext* context)
{
    BERRY_REGISTER_EXTENSION_CLASS(VesselMeshingView, context)
    BERRY_REGISTER_EXTENSION_CLASS(MeshExplorationView, context)
    BERRY_REGISTER_EXTENSION_CLASS(MeshAction, context)
    BERRY_REGISTER_EXTENSION_CLASS(ShowMeshInformationAction, context)
    BERRY_REGISTER_EXTENSION_CLASS(ConvertToDiscreteModelAction, context)

    PluginContext = context;
}

void uk_ac_kcl_VesselMeshing_Activator::stop(ctkPluginContext* context)
{
    Q_UNUSED(context)
}
