#pragma once 

#include "mitkCoreObjectFactory.h"
#include "SolidKernelExports.h"

namespace crimson {

class SolidKernel_EXPORT SolidDataCoreObjectFactory : public mitk::CoreObjectFactoryBase
{
public:
    mitkClassMacro(SolidDataCoreObjectFactory, CoreObjectFactoryBase);
    itkFactorylessNewMacro(SolidDataCoreObjectFactory);

    ~SolidDataCoreObjectFactory();

    mitk::Mapper::Pointer CreateMapper(mitk::DataNode* node, MapperSlotId slotId) override;
    void SetDefaultProperties(mitk::DataNode* node) override;

    // Deprecated stuff
    virtual const char* GetFileExtensions() { return ""; }
    virtual mitk::CoreObjectFactoryBase::MultimapType GetFileExtensionsMap() { return mitk::CoreObjectFactoryBase::MultimapType(); }
    virtual const char* GetSaveFileExtensions() { return ""; }
    virtual mitk::CoreObjectFactoryBase::MultimapType GetSaveFileExtensionsMap() { return mitk::CoreObjectFactoryBase::MultimapType(); }

protected:
    SolidDataCoreObjectFactory();
};

} // namespace crimson
