#pragma once 

#include "mitkCoreObjectFactory.h"
#include "VesselTreeExports.h"

namespace crimson {

class VesselTree_EXPORT VesselForestCoreObjectFactory : public mitk::CoreObjectFactoryBase
{
public:
    mitkClassMacro(VesselForestCoreObjectFactory, CoreObjectFactoryBase);
    itkFactorylessNewMacro(VesselForestCoreObjectFactory);

    ~VesselForestCoreObjectFactory();

    virtual mitk::Mapper::Pointer CreateMapper(mitk::DataNode* node, MapperSlotId slotId);
    virtual void SetDefaultProperties(mitk::DataNode* node);

    // Deprecated stuff
    virtual const char* GetFileExtensions() { return ""; }
    virtual mitk::CoreObjectFactoryBase::MultimapType GetFileExtensionsMap() { return mitk::CoreObjectFactoryBase::MultimapType(); }
    virtual const char* GetSaveFileExtensions() { return ""; }
    virtual mitk::CoreObjectFactoryBase::MultimapType GetSaveFileExtensionsMap() { return mitk::CoreObjectFactoryBase::MultimapType(); }

protected:
    VesselForestCoreObjectFactory();
};

}