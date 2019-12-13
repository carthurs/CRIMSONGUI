#include "VesselTreeIOMimeTypes.h"

#include <mitkIOMimeTypes.h>

namespace crimson
{

std::vector<mitk::CustomMimeType*> VesselTreeIOMimeTypes::Get()
{
    std::vector<mitk::CustomMimeType*> mimeTypes;

    // order matters here (descending rank for mime types)

    mimeTypes.push_back(VESSELTREE_MIMETYPE().Clone());
    mimeTypes.push_back(VTKPARAMETRICSPLINEVESSELPATH_MIMETYPE().Clone());

    return mimeTypes;
}

// Mime Types
std::string VesselTreeIOMimeTypes::VESSELTREE_DEFAULT_EXTENSION() { return "vfd"; }
mitk::CustomMimeType VesselTreeIOMimeTypes::VESSELTREE_MIMETYPE()
{
    mitk::CustomMimeType mimeType(VESSELTREE_MIMETYPE_NAME());
    std::string category = "Vessel Tree File";
    mimeType.SetComment("Vessel Trees");
    mimeType.SetCategory(category);
    mimeType.AddExtension(VESSELTREE_DEFAULT_EXTENSION());
    return mimeType;
}

std::string VesselTreeIOMimeTypes::VTKPARAMETRICSPLINEVESSELPATH_DEFAULT_EXTENSION() { return "vps"; }
mitk::CustomMimeType VesselTreeIOMimeTypes::VTKPARAMETRICSPLINEVESSELPATH_MIMETYPE()
{
    mitk::CustomMimeType mimeType(VTKPARAMETRICSPLINEVESSELPATH_MIMETYPE_NAME());
    std::string category = "VTK Parametric spline vessel path";
    mimeType.SetComment("Vessel Paths");
    mimeType.SetCategory(category);
    mimeType.AddExtension(VTKPARAMETRICSPLINEVESSELPATH_DEFAULT_EXTENSION());
    return mimeType;
}
// Names
std::string VesselTreeIOMimeTypes::VESSELTREE_MIMETYPE_NAME()
{
    static std::string name = mitk::IOMimeTypes::DEFAULT_BASE_NAME() + "." + VESSELTREE_DEFAULT_EXTENSION();
    return name;
}

std::string VesselTreeIOMimeTypes::VTKPARAMETRICSPLINEVESSELPATH_MIMETYPE_NAME()
{
    static std::string name = mitk::IOMimeTypes::DEFAULT_BASE_NAME() + "." + VTKPARAMETRICSPLINEVESSELPATH_DEFAULT_EXTENSION();
    return name;
}

}
