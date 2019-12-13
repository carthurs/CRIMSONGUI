#pragma once

#include <mitkCustomMimeType.h>

#include <string>

namespace crimson {

class PCMRIKernelIOMimeTypes
{
public:
  // Get all VesselTree Mime Types
  static std::vector<mitk::CustomMimeType*> Get();

  static std::string PCMRIDATA_DEFAULT_EXTENSION();

  static mitk::CustomMimeType PCMRIDATA_MIMETYPE(); 

  static std::string PCMRIDATA_MIMETYPE_NAME();

  static std::string PCMRIDATA_MIMETYPE_DESCRIPTION();

private:
    PCMRIKernelIOMimeTypes() = delete;
    PCMRIKernelIOMimeTypes(const PCMRIKernelIOMimeTypes&) = delete;
};

}
