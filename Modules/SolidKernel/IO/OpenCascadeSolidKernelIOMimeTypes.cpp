#include "OpenCascadeSolidKernelIOMimeTypes.h"

#include <mitkIOMimeTypes.h>

namespace crimson
{

std::vector<mitk::CustomMimeType*> OpenCascadeSolidKernelIOMimeTypes::Get()
{
    std::vector<mitk::CustomMimeType*> mimeTypes;

    // order matters here (descending rank for mime types)

    mimeTypes.push_back(OCCBREPDATA_MIMETYPE().Clone());

    return mimeTypes;
}

// Mime Types
std::string OpenCascadeSolidKernelIOMimeTypes::OCCBREPDATA_DEFAULT_EXTENSION() { return "brep"; }
mitk::CustomMimeType OpenCascadeSolidKernelIOMimeTypes::OCCBREPDATA_MIMETYPE()
{
    mitk::CustomMimeType mimeType(OCCBREPDATA_MIMETYPE_NAME());
    std::string category = "OpenCascade solid model";
    mimeType.SetComment("Solid models");
    mimeType.SetCategory(category);
    mimeType.AddExtension(OCCBREPDATA_DEFAULT_EXTENSION());
    mimeType.AddExtension("igs");
    mimeType.AddExtension("iges");
    mimeType.AddExtension("stp");
    mimeType.AddExtension("step");
    return mimeType;
}

// Names
std::string OpenCascadeSolidKernelIOMimeTypes::OCCBREPDATA_MIMETYPE_NAME()
{
    static std::string name = mitk::IOMimeTypes::DEFAULT_BASE_NAME() + "." + OCCBREPDATA_DEFAULT_EXTENSION();
    return name;
}

}
