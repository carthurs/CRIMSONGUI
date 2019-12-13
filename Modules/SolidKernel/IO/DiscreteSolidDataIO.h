#pragma once

#include <mitkAbstractFileIO.h>


namespace crimson {

/*! \brief    A class handling IO of DiscreteSolidData. */
class DiscreteSolidDataIO : public mitk::AbstractFileIO {
public:
    DiscreteSolidDataIO();
    virtual ~DiscreteSolidDataIO();
    std::vector< itk::SmartPointer<mitk::BaseData> > Read() override;
	void Write() override; 

protected:
    DiscreteSolidDataIO(const DiscreteSolidDataIO&);
    AbstractFileIO* IOClone() const override { return new DiscreteSolidDataIO(*this); }

};


} // namespace crimson