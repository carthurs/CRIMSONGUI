#include "PythonBoundaryCondition.h"

#include "SolverSetupPythonUtils.h"

namespace crimson
{

PythonBoundaryCondition::PythonBoundaryCondition(PythonQtObjectPtr pyBCObject)
    : _impl(pyBCObject)
{
}

//PythonBoundaryCondition::PythonBoundaryCondition(PythonQtObjectPtr pyBCObject, mitk::BaseData::Pointer data)
//	: _impl(pyBCObject), _dataObject(data)
//{
//}

bool PythonBoundaryCondition::isUnique()
{
    auto isUniqueVariant = getClassVariable(_impl.getPythonObject(), "unique");
    if (!isUniqueVariant.isValid()) {
        MITK_ERROR << "Uniqueness flag not found for boundary condition " << gsl::to_string(_impl.getName());
        return false;
    }
    if (!isUniqueVariant.canConvert<bool>()) {
        MITK_ERROR << "Cannot convert uniqueness flag to boolean for boundary condition " << gsl::to_string(_impl.getName());
        return false;
    }
    return isUniqueVariant.toBool();
}

}