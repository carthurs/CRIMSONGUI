#include "PCMRIKernelIOMimeTypes.h"

#include <mitkIOMimeTypes.h>

namespace crimson
{

std::vector<mitk::CustomMimeType*> PCMRIKernelIOMimeTypes::Get()
{
    std::vector<mitk::CustomMimeType*> mimeTypes;

    // order matters here (descending rank for mime types)

    mimeTypes.push_back(PCMRIDATA_MIMETYPE().Clone());

    return mimeTypes;
}

// Mime Types
std::string PCMRIKernelIOMimeTypes::PCMRIDATA_DEFAULT_EXTENSION() { return "pcmri"; }
mitk::CustomMimeType PCMRIKernelIOMimeTypes::PCMRIDATA_MIMETYPE()
{
    mitk::CustomMimeType mimeType(PCMRIDATA_MIMETYPE_NAME());
    std::string category = "PCMRI Data";
    mimeType.SetComment("Patient-specific flow profile");
    mimeType.SetCategory(category);
    mimeType.AddExtension(PCMRIDATA_DEFAULT_EXTENSION());

    return mimeType;
}

// Names
std::string PCMRIKernelIOMimeTypes::PCMRIDATA_MIMETYPE_NAME()
{
    static std::string name = mitk::IOMimeTypes::DEFAULT_BASE_NAME() + "." + PCMRIDATA_DEFAULT_EXTENSION();
    return name;
}

}
