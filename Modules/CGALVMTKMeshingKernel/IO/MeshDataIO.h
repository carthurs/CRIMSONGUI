#pragma once

#include <mitkAbstractFileIO.h>

namespace crimson {

/*! \brief    A class handling IO of MeshData. */
class MeshDataIO : public mitk::AbstractFileIO {
public:
    MeshDataIO();
    virtual ~MeshDataIO();

    std::vector< itk::SmartPointer<mitk::BaseData> > Read() override;
    void Write() override;

protected:
    MeshDataIO(const MeshDataIO&);
    AbstractFileIO* IOClone() const override { return new MeshDataIO(*this); }
};


} // namespace crimson