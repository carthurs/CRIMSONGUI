#include "uk_ac_kcl_AsyncTaskManagerView_Activator.h"

#include "AsyncTaskManagerView.h"

#include <AsyncTaskManager.h>

ctkPluginContext* uk_ac_kcl_AsyncTaskManagerView_Activator::PluginContext;

void uk_ac_kcl_AsyncTaskManagerView_Activator::start(ctkPluginContext* context)
{
    PluginContext = context;

    BERRY_REGISTER_EXTENSION_CLASS(AsyncTaskManagerView, context)
}

void uk_ac_kcl_AsyncTaskManagerView_Activator::stop(ctkPluginContext* context)
{
    Q_UNUSED(context)
}
