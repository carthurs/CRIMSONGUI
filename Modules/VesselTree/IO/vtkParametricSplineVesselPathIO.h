#pragma once

#include <mitkAbstractFileIO.h>

#include "VesselTreeExports.h"

namespace crimson {

/*! \brief   A class handling IO of vtkParametricSplineVesselPathData. */
class VesselTree_EXPORT vtkParametricSplineVesselPathIO : public mitk::AbstractFileIO {
public:
    vtkParametricSplineVesselPathIO();
    virtual ~vtkParametricSplineVesselPathIO();

    std::vector< itk::SmartPointer<mitk::BaseData> > Read() override;
    void Write() override;

protected:
    vtkParametricSplineVesselPathIO(const vtkParametricSplineVesselPathIO&);
    AbstractFileIO* IOClone() const override { return new vtkParametricSplineVesselPathIO(*this); }
};


} // namespace crimson