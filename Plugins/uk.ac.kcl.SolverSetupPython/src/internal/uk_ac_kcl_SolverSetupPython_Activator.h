#pragma once

#include <memory>

#include <ctkPluginActivator.h>

#include <berryIPreferencesService.h>
#include <berryIPreferences.h>
#include <berryIBerryPreferences.h>
#include <ctkServiceTracker.h>

class uk_ac_kcl_SolverSetupPython_Activator : public QObject, public ctkPluginActivator
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "uk_ac_kcl_SolverSetupPython")
    Q_INTERFACES(ctkPluginActivator)

public:
    void start(ctkPluginContext* context) override;
    void stop(ctkPluginContext* context) override;

    static ctkPluginContext* GetPluginContext() { return PluginContext; }

private:
    berry::IPreferences::Pointer _getPreferences();
    void _preferencesChanged(const berry::IBerryPreferences*);

    static ctkPluginContext* PluginContext;

    std::unique_ptr<ctkServiceTracker<berry::IPreferencesService*>> _prefServiceTracker;
}; // uk_ac_kcl_VascularModeling_Activator

