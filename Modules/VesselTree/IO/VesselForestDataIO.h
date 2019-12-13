#pragma once

#include <mitkAbstractFileIO.h>

#include "VesselTreeExports.h"

namespace crimson {

/*! \brief   A class handling IO of VesselForestData. */
class VesselTree_EXPORT VesselForestDataIO : public mitk::AbstractFileIO {
public:
    VesselForestDataIO();
    virtual ~VesselForestDataIO();

    std::vector< itk::SmartPointer<mitk::BaseData> > Read() override;
    void Write() override;

protected:
    VesselForestDataIO(const VesselForestDataIO&);
    AbstractFileIO* IOClone() const override { return new VesselForestDataIO(*this); }

    mitk::BaseData::Pointer Read_v0();
};


} // namespace crimson