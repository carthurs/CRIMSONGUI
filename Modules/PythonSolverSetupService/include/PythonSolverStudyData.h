#pragma once

#include <vector>
#include <string>

#include <mitkCommon.h>
#include <PythonQt.h>

#include <ISolverStudyData.h>

#include "PythonSolverSetupServiceExports.h"

namespace crimson
{

class PythonSolverSetupService_EXPORT PythonSolverStudyData : public ISolverStudyData
{
public:
    mitkClassMacro(PythonSolverStudyData, ISolverStudyData);
    mitkNewMacro1Param(Self, PythonQtObjectPtr);
    itkCloneMacro(Self);
    mitkCloneMacro(Self);

    gsl::cstring_span<> getMeshNodeUID() const override;
    void setMeshNodeUID(gsl::cstring_span<> nodeUID) override;

    gsl::cstring_span<> getSolverParametersNodeUID() const override;
    void setSolverParametersNodeUID(gsl::cstring_span<> nodeUID) override;

    ImmutableValueRange<gsl::cstring_span<>> getBoundaryConditionSetNodeUIDs() const override;
    void setBoundaryConditionSetNodeUIDs(ImmutableValueRange<gsl::cstring_span<>> nodeUIDs) override;

    ImmutableValueRange<gsl::cstring_span<>> getMaterialNodeUIDs() const override;
    void setMaterialNodeUIDs(ImmutableValueRange<gsl::cstring_span<>> nodeUIDs) override;

	bool runFlowsolver() override;

    bool writeSolverSetup(const IDataProvider& dataProvider, const mitk::BaseData* vesselForestData,
                          gsl::not_null<const mitk::BaseData*> solidModelData,
                          gsl::span<const SolutionData*> solutions) override;

    std::vector<SolutionData::Pointer> loadSolution() override;
    std::vector<SolutionData::Pointer> computeMaterials(const IDataProvider& dataProvider,
                                                        const mitk::BaseData* vesselForestData,
                                                        gsl::not_null<const mitk::BaseData*> solidModelData) override;

    PythonQtObjectPtr getPythonObject() const;

protected:
    template <typename ExpectedResultT>
    ExpectedResultT _getStudyPartNodeUIDs(gsl::cstring_span<> functionName) const;
    void _setStudyPartNodeUIDs(gsl::cstring_span<> functionName, const QVariantList& args);

    PythonQtObjectPtr _createSolutionStorage(gsl::span<const SolutionData*> solutions) const;
    std::vector<SolutionData::Pointer> _loadSolutionStorage(gsl::cstring_span<> loadFunctionName, const QVariantList& args);

    QVariantList _gatherBCs(const IDataProvider& dataProvider) const;
    QVariantList _gatherMaterials(const IDataProvider& dataProvider) const;

    PythonSolverStudyData(PythonQtObjectPtr);
    virtual ~PythonSolverStudyData();

    PythonSolverStudyData(const Self& other);

    mutable PythonQtObjectPtr _pyStudyObject;
    mutable std::string _meshNodeUIDCache;
    mutable std::string _solverParametersNodeUIDCache;
    mutable std::vector<std::string> _boundaryConditionSetNodeUIDsCache;
    mutable std::vector<std::string> _materialNodeUIDsCache;
};
}
