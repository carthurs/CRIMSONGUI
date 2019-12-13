#pragma once

#include <gsl.h>

#include <mitkPythonService.h>
#include "PythonSolverSetupServiceActivator.h"

namespace crimson
{

inline QVariant getClassVariable(PythonQtObjectPtr obj, QString variableName)
{
    return PythonQtObjectPtr{obj.getVariable("__class__")}.getVariable(variableName);
}

inline PythonQtObjectPtr clonePythonObject(PythonQtObjectPtr object)
{
    Expects(!object.isNull());

    auto pythonService = PythonSolverSetupServiceActivator::getPythonService();
    pythonService->Execute("import copy");

    auto result = PythonQtObjectPtr{
        pythonService->GetPythonManager()->mainContext().call("copy.deepcopy", QVariantList{QVariant::fromValue(object)})};

    Ensures(!result.isNull());
    return result;
}
} // namespace crimson