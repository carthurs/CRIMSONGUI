#pragma once

#include <IBoundaryConditionSet.h>

#include <PythonQtObjectPtr.h>

#include <mitkCommon.h>

#include "PythonSolverSetupServiceExports.h"

namespace crimson
{

class PythonSolverSetupService_EXPORT PythonBoundaryConditionSet : public IBoundaryConditionSet
{
public:
    mitkClassMacro(PythonBoundaryConditionSet, IBoundaryConditionSet);
    mitkNewMacro1Param(Self, PythonQtObjectPtr);
    //     itkCloneMacro(Self);
    //     mitkCloneMacro(Self);

    PythonQtObjectPtr getPythonObject() const { return _pyBCSetObject; }

protected:
    friend class boost::serialization::access;

    PythonBoundaryConditionSet(PythonQtObjectPtr pyBCSetObject);
    virtual ~PythonBoundaryConditionSet() {}

    PythonBoundaryConditionSet(const Self& other) = delete;

    PythonQtObjectPtr _pyBCSetObject;

    std::vector<std::string> _boundaryConditionNames;
};

} // end namespace crimson
