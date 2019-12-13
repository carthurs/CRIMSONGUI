#pragma once 

#include "mitkCoreObjectFactory.h"
#include "CGALVMTKMeshingKernelExports.h"

namespace crimson {

class CGALVMTKMeshingKernel_EXPORT MeshDataCoreObjectFactory : public mitk::CoreObjectFactoryBase
{
public:
    mitkClassMacro(MeshDataCoreObjectFactory, CoreObjectFactoryBase);
    itkFactorylessNewMacro(MeshDataCoreObjectFactory);

    ~MeshDataCoreObjectFactory();

    mitk::Mapper::Pointer CreateMapper(mitk::DataNode* node, MapperSlotId slotId) override;
    void SetDefaultProperties(mitk::DataNode* node) override;

    // Deprecated stuff
    const char* GetFileExtensions() override { return ""; }
    mitk::CoreObjectFactoryBase::MultimapType GetFileExtensionsMap() override { return mitk::CoreObjectFactoryBase::MultimapType(); }
    const char* GetSaveFileExtensions() override { return ""; }
    mitk::CoreObjectFactoryBase::MultimapType GetSaveFileExtensionsMap() override { return mitk::CoreObjectFactoryBase::MultimapType(); }

protected:
    MeshDataCoreObjectFactory();
};

} // namespace crimson
