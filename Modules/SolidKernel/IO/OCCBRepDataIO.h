#pragma once

#include <mitkAbstractFileIO.h>

#include <TopoDS_Shape.hxx>

namespace crimson {

/*! \brief    A class handling IO of OCCBRepData. */
class OCCBRepDataIO : public mitk::AbstractFileIO {
public:
    OCCBRepDataIO();
    virtual ~OCCBRepDataIO();
    std::vector< itk::SmartPointer<mitk::BaseData> > Read() override;
    void Write() override;

protected:
    OCCBRepDataIO(const OCCBRepDataIO&);
    AbstractFileIO* IOClone() const override { return new OCCBRepDataIO(*this); }

    TopoDS_Shape trySewImportedShape(const TopoDS_Shape& importedShape);
};


} // namespace crimson