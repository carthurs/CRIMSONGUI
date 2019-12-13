#include <mitkPythonService.h>

#include "PythonBoundaryConditionSet.h"

namespace crimson
{

PythonBoundaryConditionSet::PythonBoundaryConditionSet(PythonQtObjectPtr pyBCSetObject)
    :_pyBCSetObject(pyBCSetObject)
{
    Expects(!pyBCSetObject.isNull());
}

} // end namespace crimson

