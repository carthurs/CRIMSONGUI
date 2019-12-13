#pragma once

#include <mitkAbstractFileIO.h>

namespace crimson {

/*! \brief    A class handling IO of SolutionData. */
class SolutionDataIO : public mitk::AbstractFileIO {
public:
    SolutionDataIO();
    virtual ~SolutionDataIO();

    std::vector< itk::SmartPointer<mitk::BaseData> > Read() override;
    void Write() override;

protected:
    SolutionDataIO(const SolutionDataIO&) = default;
    AbstractFileIO* IOClone() const override { return new SolutionDataIO(*this); }
};


} // namespace crimson