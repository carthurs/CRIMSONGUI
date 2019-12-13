#include "uk_ac_kcl_SolverSetupView_Activator.h"

#include "SolverSetupView.h"
#include "MeshAdaptView.h"
#include "ResliceView.h"

ctkPluginContext* uk_ac_kcl_SolverSetupView_Activator::PluginContext;

void uk_ac_kcl_SolverSetupView_Activator::start(ctkPluginContext* context)
{
    BERRY_REGISTER_EXTENSION_CLASS(SolverSetupView, context)
    BERRY_REGISTER_EXTENSION_CLASS(MeshAdaptView, context)
	BERRY_REGISTER_EXTENSION_CLASS(ResliceView, context)

    PluginContext = context;
}

void uk_ac_kcl_SolverSetupView_Activator::stop(ctkPluginContext* context)
{
    Q_UNUSED(context)
}
