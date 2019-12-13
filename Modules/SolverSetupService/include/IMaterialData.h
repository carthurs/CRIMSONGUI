#pragma once

#include "FaceData.h"

namespace crimson
{
/*! \brief   A material data interface. */
class IMaterialData : public FaceData
{
public:
    mitkClassMacro(IMaterialData, FaceData);
};

} // end namespace crimson
