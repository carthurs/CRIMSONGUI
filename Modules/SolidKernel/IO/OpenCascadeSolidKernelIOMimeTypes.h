#pragma once

#include <mitkCustomMimeType.h>

#include <string>

namespace crimson {

class OpenCascadeSolidKernelIOMimeTypes
{
public:
  // Get all VesselTree Mime Types
  static std::vector<mitk::CustomMimeType*> Get();

  static std::string OCCBREPDATA_DEFAULT_EXTENSION();

  static mitk::CustomMimeType OCCBREPDATA_MIMETYPE(); 

  static std::string OCCBREPDATA_MIMETYPE_NAME();

  static std::string OCCBREPDATA_MIMETYPE_DESCRIPTION();

private:
    OpenCascadeSolidKernelIOMimeTypes() = delete;
    OpenCascadeSolidKernelIOMimeTypes(const OpenCascadeSolidKernelIOMimeTypes&) = delete;
};

}
