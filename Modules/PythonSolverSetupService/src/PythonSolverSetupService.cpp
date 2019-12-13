#include <QDir>
#include <QFileInfo>
#include <QVariant>

#include <PythonQt.h>
#include <mitkPythonService.h>

#include <PythonSolverSetupManager.h>
#include <PythonSolverSetupService.h>

#include "PythonSolverSetupServiceActivator.h"

namespace crimson
{

PythonSolverSetupService::PythonSolverSetupService(gsl::cstring_span<> modulePath, bool reloadModules)
    : _modulePath(gsl::to_string(modulePath))
{
    auto pythonService = PythonSolverSetupServiceActivator::getPythonService();

    pythonService->Execute("import sys");

    auto dirInfo = QFileInfo{_modulePath.c_str()};

    if (dirInfo.fileName().isEmpty()) {
        _modulePath = dirInfo.path().toStdString();
        dirInfo = QFileInfo{dirInfo.path()};
    }

    if (dirInfo.path() != "builtin") {
        if (!dirInfo.exists()) {
            throw std::runtime_error{"Directory containing Python solver setup scripts doesn't exist: " + _modulePath};
        }

        if (!dirInfo.isDir()) {
            throw std::runtime_error{_modulePath + " is not a directory containing Python solver setup scripts"};
        }

        pythonService->Execute(QString{"sys.path.insert(0, r'%1')"}.arg(dirInfo.absolutePath()).toStdString());
    }

    auto moduleName = dirInfo.fileName();

    pythonService->Execute(QString{"import %1"}.arg(moduleName).toStdString());
    if (pythonService->PythonErrorOccured()) {
        throw std::runtime_error{"Failed to import solver setup package from " + _modulePath};
    }

    if (reloadModules) {
        pythonService->Execute(QString{"%1 = reload(%1)"}.arg(moduleName).toStdString());
        pythonService->Execute(QString{"%1.reloadAll()"}.arg(moduleName).toStdString());
    }

    // Find solver setup managers
    auto solverSetupManagerVariant = pythonService->GetPythonManager()->executeString(
        QString{"%1.getSolverSetupManager()"}.arg(moduleName), ctkAbstractPythonManager::EvalInput);
    if (pythonService->PythonErrorOccured() || !solverSetupManagerVariant.canConvert<QVariantList>() ||
        solverSetupManagerVariant.toList().size() != 2) {
        throw std::runtime_error{"Solver setup manager not found in " + moduleName.toStdString() +
                                 ". Package-level getSolverSetupManager() must return a pair (name, class)."};
    }

    auto solverSetupManagerNameAndClass = solverSetupManagerVariant.toList();
    auto classObject = PythonQtObjectPtr{solverSetupManagerNameAndClass[1]};

    if (classObject.isNull()) {
        throw std::runtime_error{"Skipped a value returned by getSolverSetupManager() - not a class in " +
                                 moduleName.toStdString()};
    }

    auto solverManagerObject = PythonQtObjectPtr{classObject.call()};

    if (classObject.isNull()) {
        throw std::runtime_error{"Failed to create a solver setup manager returned by getSolverSetupManager() in " +
                                 moduleName.toStdString()};
    }

    _solverSetupManager = std::make_unique<PythonSolverSetupManager>(
        solverManagerObject, gsl::as_temp_span(solverSetupManagerNameAndClass[0].toString().toStdString()));

    _serviceRegistration = us::GetModuleContext()->RegisterService<ISolverSetupService>(this);
    MITK_INFO << "Successfully loaded python solver setup " << moduleName.toStdString() << " from " << gsl::to_string(modulePath);
}

PythonSolverSetupService::~PythonSolverSetupService()
{
    _serviceRegistration.Unregister();

    auto pythonService = PythonSolverSetupServiceActivator::getPythonService();
    auto dirInfo = QFileInfo{_modulePath.c_str()};

    if (dirInfo.path() != "builtin") {
        pythonService->Execute(QString("sys.path.remove(r'%1')").arg(dirInfo.absolutePath()).toStdString());
        pythonService->Execute(QString("del %1").arg(dirInfo.fileName()).toStdString());
    }
}
}
