#include "uk_ac_kcl_AsyncTaskManager_Activator.h"

#include "AsyncTaskManager.h"
// #include "AsyncTaskManagerView.h"

ctkPluginContext* uk_ac_kcl_AsyncTaskManager_Activator::PluginContext;

void uk_ac_kcl_AsyncTaskManager_Activator::start(ctkPluginContext* context)
{
    PluginContext = context;
    crimson::AsyncTaskManager::init();

//    BERRY_REGISTER_EXTENSION_CLASS(AsyncTaskManagerView, context)
}

void uk_ac_kcl_AsyncTaskManager_Activator::stop(ctkPluginContext* context)
{
    Q_UNUSED(context)
    crimson::AsyncTaskManager::term();
}
