#pragma once

#include <memory>
#include <vector>

#include <QStringList>

#include <usModuleActivator.h>
#include <usModuleContext.h>

#include <usServiceTracker.h>

#include <mitkCustomMimeType.h>
#include <mitkAbstractFileIO.h>
#include <mitkIPythonService.h>

#include "PythonSolverSetupServiceExports.h"

namespace mitk
{
class PythonService;
}

namespace crimson
{
class ISolverSetupService;

class PythonSolverSetupService_EXPORT PythonSolverSetupServiceActivator : public us::ModuleActivator
{
public:
    PythonSolverSetupServiceActivator();
    ~PythonSolverSetupServiceActivator();

    void Load(us::ModuleContext* context) override;
    void Unload(us::ModuleContext* context) override;

    static mitk::PythonService* getPythonService();

    static void reloadPythonSolverSetups(const QStringList& modulePaths);
    static void reloadPythonSolverSetups(bool reloadModules = false);

private:
    std::vector<std::shared_ptr<mitk::CustomMimeType>> _mimeTypes;
    std::vector<std::shared_ptr<mitk::AbstractFileIO>> _ioClasses;

    std::unique_ptr<us::ServiceTracker<mitk::IPythonService>> _pythonServiceTracker;
    std::vector<std::unique_ptr<ISolverSetupService>> _solverSetupServices;

    QStringList _modulePaths;

    static PythonSolverSetupServiceActivator* _instance;
};
}