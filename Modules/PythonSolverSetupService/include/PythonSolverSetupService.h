#pragma once

#include <string>
#include <vector>

#include <usServiceRegistration.h>

#include <ISolverSetupService.h>

#include "PythonSolverSetupServiceExports.h"

namespace crimson
{

class PythonSolverSetupService_EXPORT PythonSolverSetupService : public ISolverSetupService
{
public:
    PythonSolverSetupService(gsl::cstring_span<> modulePath, bool reloadModules = false);
    ~PythonSolverSetupService();

    PythonSolverSetupService(const PythonSolverSetupService&) = delete;
    PythonSolverSetupService& operator=(const PythonSolverSetupService&) = delete;

    gsl::not_null<ISolverSetupManager*> getSolverSetupManager() const override { return _solverSetupManager.get(); }

private:
    std::string _modulePath;
    std::unique_ptr<ISolverSetupManager> _solverSetupManager;
    us::ServiceRegistration<PythonSolverSetupService> _serviceRegistration;
};
}