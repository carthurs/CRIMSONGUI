#pragma once

#include "FaceData.h"
#include "mitkBaseData.h"

namespace crimson
{

/*! \brief   A boundary condition interface. */
class IBoundaryCondition : public FaceData
{
public:
    mitkClassMacro(IBoundaryCondition, FaceData);

    /*!
     * \brief   Currently unused uniqueness flag.
     */
    virtual bool isUnique() = 0;


};

} // end namespace crimson
