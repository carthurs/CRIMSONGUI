#pragma once

#include <gsl.h>

#include <mitkBaseData.h>

namespace crimson
{
class QtPropertyStorage;

/*! \brief   A solver parameters data interface. */
class ISolverParametersData : public mitk::BaseData
{
public:
    mitkClassMacro(ISolverParametersData, BaseData);

    /*!
    * \brief   Gets property storage for the SolverParametersData properties.
    */
    virtual gsl::not_null<QtPropertyStorage*> getPropertyStorage() = 0;

protected:
    ISolverParametersData() { mitk::BaseData::InitializeTimeGeometry(1); }
    virtual ~ISolverParametersData() {}

    ISolverParametersData(const Self&) = default;

    void SetRequestedRegion(const itk::DataObject*) override {}
    void SetRequestedRegionToLargestPossibleRegion() override {}
    bool RequestedRegionIsOutsideOfTheBufferedRegion() override { return true; }
    bool VerifyRequestedRegion() override { return true; }
};
}
