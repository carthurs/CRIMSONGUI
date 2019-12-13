#pragma once

#include <gsl.h>

#include <mitkCommon.h>
#include <mitkBaseData.h>

#include <vtkSmartPointer.h>
#include <vtkDataArray.h>

#include "SolverSetupServiceExports.h"

namespace crimson
{

/*! \brief   A class used for storing the simulation results and materials. */
class SolverSetupService_EXPORT SolutionData : public mitk::BaseData
{
public:
    mitkClassMacro(SolutionData, mitk::BaseData);
    mitkNewMacro1Param(Self, vtkSmartPointer<vtkDataArray>);
    //     itkCloneMacro(Self);
    //     mitkCloneMacro(Self);

    /*!
     * \brief   Gets the data in form of a vtkDataArray.
     */
    vtkDataArray* getArrayData() const;

protected:
    SolutionData(vtkSmartPointer<vtkDataArray> data);
    virtual ~SolutionData() {}

    SolutionData(const Self& other) = delete;

private:
    vtkSmartPointer<vtkDataArray> _data;

    void SetRequestedRegion(const itk::DataObject*) override {}
    void SetRequestedRegionToLargestPossibleRegion() override {}
    bool RequestedRegionIsOutsideOfTheBufferedRegion() override { return true; }
    bool VerifyRequestedRegion() override { return true; }
};
}
