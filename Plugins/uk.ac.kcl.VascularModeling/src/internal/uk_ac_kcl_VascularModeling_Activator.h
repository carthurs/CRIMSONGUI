#pragma once

#include <ctkPluginActivator.h>

class uk_ac_kcl_VascularModeling_Activator : public QObject, public ctkPluginActivator
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "uk_ac_kcl_VascularModeling")
    Q_INTERFACES(ctkPluginActivator)

public:

    void start(ctkPluginContext* context) override;
    void stop(ctkPluginContext* context) override;

    static ctkPluginContext* GetPluginContext() { return PluginContext; }

private:

    static ctkPluginContext* PluginContext;
}; // uk_ac_kcl_VascularModeling_Activator

