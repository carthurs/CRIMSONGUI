#include "uk_ac_kcl_SolverSetupPython_Activator.h"

#include <PythonSolverSetupServiceActivator.h>

#include <mitkNodePredicateDataType.h>

#include <PythonBoundaryCondition.h>
#include <PythonBoundaryConditionSet.h>
#include <PythonSolverParametersData.h>
#include <PythonSolverStudyData.h>

#include <PythonSolverSetupPreferencePage.h>

ctkPluginContext* uk_ac_kcl_SolverSetupPython_Activator::PluginContext;

void uk_ac_kcl_SolverSetupPython_Activator::start(ctkPluginContext* context)
{
    // Setup preferences
    _prefServiceTracker = std::make_unique<ctkServiceTracker<berry::IPreferencesService*>>(context);
    _prefServiceTracker->open();

    // Register listener
    auto prefs = this->_getPreferences().Cast<berry::IBerryPreferences>();
    if (prefs.IsNotNull()) {
        using PrefPage = crimson::PythonSolverSetupPreferencePage;
        if (prefs->Get(PrefPage::directoriesKeyName, "").isEmpty()) {
            prefs->Put(PrefPage::directoriesKeyName, "builtin/CRIMSONSolver");
        }

        prefs->OnChanged.AddListener(
            berry::MessageDelegate1<uk_ac_kcl_SolverSetupPython_Activator, const berry::IBerryPreferences*>(
                this, &uk_ac_kcl_SolverSetupPython_Activator::_preferencesChanged));

        _preferencesChanged(prefs.GetPointer());
    }

    BERRY_REGISTER_EXTENSION_CLASS(crimson::PythonSolverSetupPreferencePage, context)

    PluginContext = context;
}

void uk_ac_kcl_SolverSetupPython_Activator::stop(ctkPluginContext* context)
{
    _prefServiceTracker->close();
    _prefServiceTracker.reset();
    Q_UNUSED(context)
}

berry::IPreferences::Pointer uk_ac_kcl_SolverSetupPython_Activator::_getPreferences()
{
    using PrefPage = crimson::PythonSolverSetupPreferencePage;

    berry::IPreferencesService* prefService = _prefServiceTracker->getService();
    return prefService ? prefService->GetSystemPreferences()->Node(PrefPage::preferenceNodeName)
                       : berry::IPreferences::Pointer(nullptr);
}

void uk_ac_kcl_SolverSetupPython_Activator::_preferencesChanged(const berry::IBerryPreferences* prefs)
{
    using PrefPage = crimson::PythonSolverSetupPreferencePage;

    auto modulePaths = prefs->Get(PrefPage::directoriesKeyName, "").split(PrefPage::separator).filter(QRegExp{".+"});

    crimson::PythonSolverSetupServiceActivator::reloadPythonSolverSetups(modulePaths);
}

