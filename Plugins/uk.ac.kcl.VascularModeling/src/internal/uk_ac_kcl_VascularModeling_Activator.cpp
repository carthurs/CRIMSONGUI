#include "uk_ac_kcl_VascularModeling_Activator.h"

#include "VesselPathPlanningView.h"
#include "VesselDrivenResliceView.h"
#include "ContourModelingView.h"
#include "VesselBlendingView.h"

#include "LoftAction.h"
#include "BlendAction.h"
#include "ShowHideContoursAction.h"
#include "ExportVesselsAction.h"
#include "ImportVesselsAction.h"

#include "ReparentAction.h"

ctkPluginContext* uk_ac_kcl_VascularModeling_Activator::PluginContext;

void uk_ac_kcl_VascularModeling_Activator::start(ctkPluginContext* context)
{
    BERRY_REGISTER_EXTENSION_CLASS(VesselPathPlanningView, context)
    BERRY_REGISTER_EXTENSION_CLASS(VesselDrivenResliceView, context)
    BERRY_REGISTER_EXTENSION_CLASS(ContourModelingView, context)
    BERRY_REGISTER_EXTENSION_CLASS(VesselBlendingView, context)

    BERRY_REGISTER_EXTENSION_CLASS(LoftAction, context)
    BERRY_REGISTER_EXTENSION_CLASS(BlendAction, context)
    BERRY_REGISTER_EXTENSION_CLASS(ShowContoursAction, context)
    BERRY_REGISTER_EXTENSION_CLASS(HideContoursAction, context)
    BERRY_REGISTER_EXTENSION_CLASS(ExportVesselsAction, context)
    BERRY_REGISTER_EXTENSION_CLASS(ImportVesselsAction, context)

    BERRY_REGISTER_EXTENSION_CLASS(ReparentAction, context)

    PluginContext = context;
}

void uk_ac_kcl_VascularModeling_Activator::stop(ctkPluginContext* context)
{
    Q_UNUSED(context)
}