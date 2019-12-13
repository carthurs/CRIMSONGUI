#pragma once 

#include "mitkCoreObjectFactory.h"
#include "PCMRIKernelExports.h"

namespace crimson {

class PCMRIKernel_EXPORT PCMRIDataCoreObjectFactory : public mitk::CoreObjectFactoryBase
{
public:
    mitkClassMacro(PCMRIDataCoreObjectFactory, CoreObjectFactoryBase);
    itkFactorylessNewMacro(PCMRIDataCoreObjectFactory);

    ~PCMRIDataCoreObjectFactory();

    mitk::Mapper::Pointer CreateMapper(mitk::DataNode* node, MapperSlotId slotId) override;
    void SetDefaultProperties(mitk::DataNode* node) override;

    // Deprecated stuff
    virtual const char* GetFileExtensions() { return ""; }
    virtual mitk::CoreObjectFactoryBase::MultimapType GetFileExtensionsMap() { return mitk::CoreObjectFactoryBase::MultimapType(); }
    virtual const char* GetSaveFileExtensions() { return ""; }
    virtual mitk::CoreObjectFactoryBase::MultimapType GetSaveFileExtensionsMap() { return mitk::CoreObjectFactoryBase::MultimapType(); }

protected:
    PCMRIDataCoreObjectFactory();
};

} // namespace crimson
