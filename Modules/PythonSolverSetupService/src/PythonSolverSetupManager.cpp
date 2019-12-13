#include <PythonQt.h>

#include <mitkPythonService.h>

#include <PythonSolverSetupManager.h>
#include <PythonSolverParametersData.h>
#include <PythonSolverStudyData.h>
#include <PythonBoundaryConditionSet.h>
#include <PythonBoundaryCondition.h>
#include <PythonMaterialData.h>

#include <SolverSetupUtils.h>

#include "PythonQtWrappers.h"

#include "PythonSolverSetupServiceActivator.h"

namespace crimson
{

PythonSolverSetupManager::PythonSolverSetupManager(PythonQtObjectPtr pySolverSetupManagerObject, gsl::cstring_span<> name)
    : _pySolverSetupManagerObject(pySolverSetupManagerObject)
    , _name(gsl::to_string(name))
    , _boundaryConditionSetNames(_getClassNames("getBoundaryConditionSetNames"))
    , _solverParametersNames(_getClassNames("getSolverParametersNames"))
    , _solverStudyNames(_getClassNames("getSolverStudyNames"))
    , _materialNames(_getClassNames("getMaterialNames"))
{
    Expects(!pySolverSetupManagerObject.isNull());
}

template <typename... Args>
std::vector<std::string> PythonSolverSetupManager::_getClassNames(gsl::cstring_span<> functionName, Args&&... args) const
{
    auto resultVariant =
        _pySolverSetupManagerObject.call(gsl::to_QString(functionName), QVariantList{QVariant::fromValue(args)...}).toList();

    std::vector<std::string> result;
    std::transform(resultVariant.begin(), resultVariant.end(), std::back_inserter(result),
                   [](const QVariant& name) { return name.toString().toStdString(); });

    return result;
}

template <class T, typename... Args>
typename T::Pointer PythonSolverSetupManager::_createSolverObject(gsl::cstring_span<> functionName, Args&&... args)
{
    auto pythonService = PythonSolverSetupServiceActivator::getPythonService();

    auto pyBCObject =
        _pySolverSetupManagerObject.call(gsl::to_QString(functionName), QVariantList{QVariant::fromValue(args)...});

    if (pythonService->PythonErrorOccured() || !pyBCObject.canConvert<PythonQtObjectPtr>()) {
        return typename T::Pointer();
    }

    return T::New(qvariant_cast<PythonQtObjectPtr>(pyBCObject)).GetPointer();
}

template <class T, typename... Args>
typename T::Pointer PythonSolverSetupManager::_createSolverObjectWithCObject(gsl::cstring_span<> functionName, mitk::BaseData::Pointer data, Args&&... args)
{
	auto pythonService = PythonSolverSetupServiceActivator::getPythonService();

	auto pyBCObject =
		_pySolverSetupManagerObject.call(gsl::to_QString(functionName), QVariantList{ QVariant::fromValue(args)... });

	if (pythonService->PythonErrorOccured() || !pyBCObject.canConvert<PythonQtObjectPtr>()) {
		return typename T::Pointer();
	}

	return T::New(qvariant_cast<PythonQtObjectPtr>(pyBCObject),data.GetPointer()).GetPointer();
}

ImmutableValueRange<gsl::cstring_span<>> PythonSolverSetupManager::getSolverStudyNameList() const
{
    return make_transforming_immutable_range<gsl::cstring_span<>>(_solverStudyNames);
}

ISolverStudyData::Pointer PythonSolverSetupManager::createSolverStudy(gsl::cstring_span<> name)
{
    return _createSolverObject<PythonSolverStudyData>("createSolverStudy", gsl::to_QString(name)).GetPointer();
}

ImmutableValueRange<gsl::cstring_span<>> PythonSolverSetupManager::getSolverParametersNameList() const
{
    return make_transforming_immutable_range<gsl::cstring_span<>>(_solverParametersNames);
}

ISolverParametersData::Pointer PythonSolverSetupManager::createSolverParameters(gsl::cstring_span<> name)
{
    return _createSolverObject<PythonSolverParametersData>("createSolverParameters", gsl::to_QString(name)).GetPointer();
}

ImmutableValueRange<gsl::cstring_span<>> PythonSolverSetupManager::getBoundaryConditionSetNameList() const
{
    return make_transforming_immutable_range<gsl::cstring_span<>>(_boundaryConditionSetNames);
}

IBoundaryConditionSet::Pointer PythonSolverSetupManager::createBoundaryConditionSet(gsl::cstring_span<> name)
{
    return _createSolverObject<PythonBoundaryConditionSet>("createBoundaryConditionSet", gsl::to_QString(name)).GetPointer();
}

ImmutableValueRange<gsl::cstring_span<>> PythonSolverSetupManager::getMaterialNameList() const
{
    return make_transforming_immutable_range<gsl::cstring_span<>>(_materialNames);
}

IMaterialData::Pointer PythonSolverSetupManager::createMaterial(gsl::cstring_span<> name)
{
    return _createSolverObject<PythonMaterialData>("createMaterial", gsl::to_QString(name)).GetPointer();
}


ImmutableValueRange<gsl::cstring_span<>>
PythonSolverSetupManager::getBoundaryConditionNameList(gsl::not_null<IBoundaryConditionSet*> ownerBCSet) const
{
    auto bcNamesIter = _boundaryConditionNames.find(ownerBCSet);
    if (bcNamesIter == _boundaryConditionNames.end()) {
        auto pythonOwnerBCSet = dynamic_cast<PythonBoundaryConditionSet*>(ownerBCSet.get());
        Expects(pythonOwnerBCSet != nullptr);

        bcNamesIter =
            _boundaryConditionNames.emplace(ownerBCSet.get(), std::move(_getClassNames("getBoundaryConditionNames",
                                                                                       pythonOwnerBCSet->getPythonObject())))
                .first;
    }

    return make_transforming_immutable_range<gsl::cstring_span<>>(bcNamesIter->second);
}

IBoundaryCondition::Pointer PythonSolverSetupManager::createBoundaryCondition(gsl::not_null<IBoundaryConditionSet*> ownerBCSet,
                                                                              gsl::cstring_span<> name)
{
    auto pythonOwnerBCSet = dynamic_cast<PythonBoundaryConditionSet*>(ownerBCSet.get());
    Expects(pythonOwnerBCSet != nullptr);

	
		IBoundaryCondition::Pointer BC = dynamic_cast<IBoundaryCondition*>(_createSolverObject<PythonBoundaryCondition>("createBoundaryCondition", gsl::to_QString(name),
			pythonOwnerBCSet->getPythonObject())
			.GetPointer());
		return BC;
}
}
