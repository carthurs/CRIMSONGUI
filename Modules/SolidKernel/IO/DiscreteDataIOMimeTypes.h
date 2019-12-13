#pragma once

#include <mitkCustomMimeType.h>

#include <string>

namespace crimson {

class DiscreteDataIOMimeTypes
{
public:
  // Get all VesselTree Mime Types
  static std::vector<mitk::CustomMimeType*> Get();

  static std::string DISCRETESOLIDDATA_DEFAULT_EXTENSION();

  static mitk::CustomMimeType DISCRETESOLIDDATA_MIMETYPE(); 

  static std::string DISCRETESOLIDDATA_MIMETYPE_NAME();

  static std::string DISCRETESOLIDDATA_MIMETYPE_DESCRIPTION();

private:
    DiscreteDataIOMimeTypes() = delete;
    DiscreteDataIOMimeTypes(const DiscreteDataIOMimeTypes&) = delete;
};

}
