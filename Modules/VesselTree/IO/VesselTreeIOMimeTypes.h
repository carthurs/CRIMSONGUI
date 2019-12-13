#pragma once

#include <mitkCustomMimeType.h>
#include "VesselTreeExports.h"


#include <string>

namespace crimson {

class VesselTree_EXPORT VesselTreeIOMimeTypes
{
public:
  // Get all VesselTree Mime Types
  static std::vector<mitk::CustomMimeType*> Get();

  static std::string VESSELTREE_DEFAULT_EXTENSION();
  static std::string VTKPARAMETRICSPLINEVESSELPATH_DEFAULT_EXTENSION();

  static mitk::CustomMimeType VESSELTREE_MIMETYPE(); 
  static mitk::CustomMimeType VTKPARAMETRICSPLINEVESSELPATH_MIMETYPE(); 

  static std::string VESSELTREE_MIMETYPE_NAME();
  static std::string VTKPARAMETRICSPLINEVESSELPATH_MIMETYPE_NAME();

  static std::string VESSELTREE_MIMETYPE_DESCRIPTION();
  static std::string VTKPARAMETRICSPLINEVESSELPATH_MIMETYPE_DESCRIPTION();

private:
    VesselTreeIOMimeTypes() = delete;
    VesselTreeIOMimeTypes(const VesselTreeIOMimeTypes&) = delete;
};

}
