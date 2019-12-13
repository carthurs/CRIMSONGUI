#include "uk_ac_kcl_HierarchyManager_Activator.h"

#include "HierarchyManager.h"

ctkPluginContext* uk_ac_kcl_HierarchyManager_Activator::PluginContext;

void uk_ac_kcl_HierarchyManager_Activator::start(ctkPluginContext* context)
{
    PluginContext = context;
    crimson::HierarchyManager::init();
}

void uk_ac_kcl_HierarchyManager_Activator::stop(ctkPluginContext* context)
{
    Q_UNUSED(context)
    crimson::HierarchyManager::term();
}
