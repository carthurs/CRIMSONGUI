#pragma once

#include <mitkCustomMimeType.h>

#include <string>

namespace crimson {

class SolverSetupServiceIOMimeTypes
{
public:
  // Get all VesselTree Mime Types
  static std::vector<mitk::CustomMimeType*> Get();

  static std::string SOLUTIONDATA_DEFAULT_EXTENSION();
  static mitk::CustomMimeType SOLUTIONDATA_MIMETYPE();
  static std::string SOLUTIONDATA_MIMETYPE_NAME();
  static std::string SOLUTIONDATA_MIMETYPE_DESCRIPTION();

private:
    SolverSetupServiceIOMimeTypes() = delete;
    SolverSetupServiceIOMimeTypes(const SolverSetupServiceIOMimeTypes&) = delete;
};

}
