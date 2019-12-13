#include "SolverSetupServiceIOMimeTypes.h"

#include <mitkIOMimeTypes.h>

namespace crimson
{

std::vector<mitk::CustomMimeType*> SolverSetupServiceIOMimeTypes::Get()
{
    std::vector<mitk::CustomMimeType*> mimeTypes;

    // order matters here (descending rank for mime types)

    mimeTypes.push_back(SOLUTIONDATA_MIMETYPE().Clone());

    return mimeTypes;
}

// Mime Types
std::string SolverSetupServiceIOMimeTypes::SOLUTIONDATA_DEFAULT_EXTENSION() { return "soln"; }
mitk::CustomMimeType SolverSetupServiceIOMimeTypes::SOLUTIONDATA_MIMETYPE()
{
    mitk::CustomMimeType mimeType(SOLUTIONDATA_MIMETYPE_NAME());
    std::string category = "Solution data";
    mimeType.SetCategory(category);
    mimeType.AddExtension(SOLUTIONDATA_DEFAULT_EXTENSION());
    return mimeType;
}

// Names
std::string SolverSetupServiceIOMimeTypes::SOLUTIONDATA_MIMETYPE_NAME()
{
    static std::string name = mitk::IOMimeTypes::DEFAULT_BASE_NAME() + "." + SOLUTIONDATA_DEFAULT_EXTENSION();
    return name;
}

}
