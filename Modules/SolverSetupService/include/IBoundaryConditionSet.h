#pragma once

#include "IBoundaryCondition.h"

namespace crimson
{

/*! \brief   A boundary condition set interface. */
class IBoundaryConditionSet : public mitk::BaseData
{
public:
    mitkClassMacro(IBoundaryConditionSet, BaseData);

protected:
    IBoundaryConditionSet() { mitk::BaseData::InitializeTimeGeometry(1); }
    virtual ~IBoundaryConditionSet() {}

    IBoundaryConditionSet(const Self&) = default;

    void SetRequestedRegion(const itk::DataObject*) override {}
    void SetRequestedRegionToLargestPossibleRegion() override {}
    bool RequestedRegionIsOutsideOfTheBufferedRegion() override { return true; }
    bool VerifyRequestedRegion() override { return true; }
};

} // end namespace crimson
