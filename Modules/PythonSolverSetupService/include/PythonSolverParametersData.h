#pragma once

#include <ISolverParametersData.h>

#include <PythonQtObjectPtr.h>

#include <mitkCommon.h>

#include "QtPropertyStorage.h"

#include "PythonSolverSetupServiceExports.h"

namespace crimson
{

class PythonSolverSetupService_EXPORT PythonSolverParametersData : public ISolverParametersData
{
public:
    mitkClassMacro(PythonSolverParametersData, ISolverParametersData);
    mitkNewMacro1Param(Self, PythonQtObjectPtr);
    //     itkCloneMacro(Self);
    //     mitkCloneMacro(Self);

    PythonQtObjectPtr getPythonObject() const { return _pySolverParametersDataObject; }

    gsl::not_null<QtPropertyStorage*> getPropertyStorage() override { return &_propertyStorage; }

protected:
    PythonSolverParametersData(PythonQtObjectPtr pySolverParametersDataObject);
    virtual ~PythonSolverParametersData() {}

    PythonSolverParametersData(const Self& other) = delete;

private:
    PythonQtObjectPtr _pySolverParametersDataObject;

    QtPropertyStorage _propertyStorage;
};
}
