#pragma once

#include <mitkCustomMimeType.h>

#include <string>

namespace crimson {

class MeshingKernelIOMimeTypes
{
public:
  // Get all VesselTree Mime Types
  static std::vector<mitk::CustomMimeType*> Get();

  static std::string MESHDATA_DEFAULT_EXTENSION();
  static std::string MESHINGPARAMETERSDATA_DEFAULT_EXTENSION();

  static mitk::CustomMimeType MESHDATA_MIMETYPE();
  static mitk::CustomMimeType MESHINGPARAMETERSDATA_MIMETYPE();

  static std::string MESHDATA_MIMETYPE_NAME();
  static std::string MESHINGPARAMETERSDATA_MIMETYPE_NAME();

  static std::string MESHDATA_MIMETYPE_DESCRIPTION();
  static std::string MESHINGPARAMETERSDATA_MIMETYPE_DESCRIPTION();

private:
    MeshingKernelIOMimeTypes() = delete;
    MeshingKernelIOMimeTypes(const MeshingKernelIOMimeTypes&) = delete;
};

}
