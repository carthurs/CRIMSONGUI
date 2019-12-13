#include "DiscreteDataIOMimeTypes.h"

#include <mitkIOMimeTypes.h>

namespace crimson
{

std::vector<mitk::CustomMimeType*> DiscreteDataIOMimeTypes::Get()
{
    std::vector<mitk::CustomMimeType*> mimeTypes;

    // order matters here (descending rank for mime types)

    mimeTypes.push_back(DISCRETESOLIDDATA_MIMETYPE().Clone());

    return mimeTypes;
}

// Mime Types
std::string DiscreteDataIOMimeTypes::DISCRETESOLIDDATA_DEFAULT_EXTENSION() { return "dsd"; }
mitk::CustomMimeType DiscreteDataIOMimeTypes::DISCRETESOLIDDATA_MIMETYPE()
{
    mitk::CustomMimeType mimeType(DISCRETESOLIDDATA_MIMETYPE_NAME());
    std::string category = "Discrete solid model";
    mimeType.SetComment("Solid models");
    mimeType.SetCategory(category);
    mimeType.AddExtension(DISCRETESOLIDDATA_DEFAULT_EXTENSION());
    return mimeType;
}

// Names
std::string DiscreteDataIOMimeTypes::DISCRETESOLIDDATA_MIMETYPE_NAME()
{
    static std::string name = mitk::IOMimeTypes::DEFAULT_BASE_NAME() + "." + DISCRETESOLIDDATA_DEFAULT_EXTENSION();
    return name;
}

}
