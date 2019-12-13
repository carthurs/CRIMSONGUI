#pragma once

#include <map>
#include <string>

#include <ISolverSetupManager.h>
#include <QObject>

#include <PythonQtObjectPtr.h>

#include "PythonSolverSetupServiceExports.h"

namespace crimson
{

class MeshData;
class PythonSolverParametersData;

class PythonSolverSetupService_EXPORT PythonSolverSetupManager : public ISolverSetupManager
{
public:
    gsl::cstring_span<> getName() const override { return _name; }

    ImmutableValueRange<gsl::cstring_span<>> getBoundaryConditionSetNameList() const override;
    IBoundaryConditionSet::Pointer createBoundaryConditionSet(gsl::cstring_span<> name) override;

    ImmutableValueRange<gsl::cstring_span<>>
    getBoundaryConditionNameList(gsl::not_null<IBoundaryConditionSet*> ownerBCSet) const override;
    IBoundaryCondition::Pointer createBoundaryCondition(gsl::not_null<IBoundaryConditionSet*> ownerBCSet,
                                                        gsl::cstring_span<> name) override;

    ImmutableValueRange<gsl::cstring_span<>> getMaterialNameList() const override;
    IMaterialData::Pointer createMaterial(gsl::cstring_span<> name) override;

    ImmutableValueRange<gsl::cstring_span<>> getSolverParametersNameList() const override;
    ISolverParametersData::Pointer createSolverParameters(gsl::cstring_span<> name) override;

    ImmutableValueRange<gsl::cstring_span<>> getSolverStudyNameList() const override;
    ISolverStudyData::Pointer createSolverStudy(gsl::cstring_span<> name) override;

    PythonSolverSetupManager(PythonQtObjectPtr pySolverSetupManagerObject, gsl::cstring_span<> name);
    virtual ~PythonSolverSetupManager() {}

private:
    mutable PythonQtObjectPtr _pySolverSetupManagerObject;
    std::string _name;

    std::vector<std::string> _boundaryConditionSetNames;
    mutable std::map<IBoundaryConditionSet*, std::vector<std::string>> _boundaryConditionNames;
    std::vector<std::string> _solverParametersNames;
    std::vector<std::string> _solverStudyNames;
    std::vector<std::string> _materialNames;

    template <typename... Args>
    std::vector<std::string> _getClassNames(gsl::cstring_span<> functionName, Args&&... args) const;

    template <class T, typename... Args>
    typename T::Pointer _createSolverObject(gsl::cstring_span<> functionName, Args&&... args);
	template <class T, typename... Args>
	typename T::Pointer _createSolverObjectWithCObject(gsl::cstring_span<> functionName, mitk::BaseData::Pointer data, Args&&... args);
};

} // end namespace crimson
