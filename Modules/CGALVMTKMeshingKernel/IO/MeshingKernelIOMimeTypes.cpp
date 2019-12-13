#include "MeshingKernelIOMimeTypes.h"

#include <mitkIOMimeTypes.h>

namespace crimson
{

std::vector<mitk::CustomMimeType*> MeshingKernelIOMimeTypes::Get()
{
    std::vector<mitk::CustomMimeType*> mimeTypes;

    // order matters here (descending rank for mime types)

    mimeTypes.push_back(MESHDATA_MIMETYPE().Clone());
    mimeTypes.push_back(MESHINGPARAMETERSDATA_MIMETYPE().Clone());

    return mimeTypes;
}

// Mime Types
std::string MeshingKernelIOMimeTypes::MESHDATA_DEFAULT_EXTENSION() { return "ocms"; }
mitk::CustomMimeType MeshingKernelIOMimeTypes::MESHDATA_MIMETYPE()
{
    mitk::CustomMimeType mimeType(MESHDATA_MIMETYPE_NAME());
    std::string category = "CRIMSON mesh (open source)";
    mimeType.SetComment("FEM meshes (open source)");
    mimeType.SetCategory(category);
    mimeType.AddExtension(MESHDATA_DEFAULT_EXTENSION());
    return mimeType;
}

std::string MeshingKernelIOMimeTypes::MESHINGPARAMETERSDATA_DEFAULT_EXTENSION() { return "ompd"; }
mitk::CustomMimeType MeshingKernelIOMimeTypes::MESHINGPARAMETERSDATA_MIMETYPE()
{
    mitk::CustomMimeType mimeType(MESHINGPARAMETERSDATA_MIMETYPE_NAME());
    std::string category = "Meshing parameters data (open source)";
    mimeType.SetComment("Meshing parameters datas (open source)");
    mimeType.SetCategory(category);
    mimeType.AddExtension(MESHINGPARAMETERSDATA_DEFAULT_EXTENSION());
    return mimeType;
}

// Names
std::string MeshingKernelIOMimeTypes::MESHDATA_MIMETYPE_NAME()
{
    static std::string name = mitk::IOMimeTypes::DEFAULT_BASE_NAME() + "." + MESHDATA_DEFAULT_EXTENSION();
    return name;
}

std::string MeshingKernelIOMimeTypes::MESHINGPARAMETERSDATA_MIMETYPE_NAME()
{
    static std::string name = mitk::IOMimeTypes::DEFAULT_BASE_NAME() + "." + MESHINGPARAMETERSDATA_DEFAULT_EXTENSION();
    return name;
}

}
