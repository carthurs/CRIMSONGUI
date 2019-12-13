#include <gsl.h>

#include "PythonSolverParametersData.h"

#include "QtPropertyBrowserTools.h"

namespace crimson
{

PythonSolverParametersData::PythonSolverParametersData(PythonQtObjectPtr pySolverParametersDataObject)
    : _pySolverParametersDataObject(pySolverParametersDataObject)
{
    Expects(!pySolverParametersDataObject.isNull());
    QtPropertyBrowserTools::fillPropertiesFromPyObject(_propertyStorage.getPropertyManager(), _propertyStorage.getTopProperty(),
                                                       _pySolverParametersDataObject, "Solver parameters");
}

}
