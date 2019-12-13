#include "PCMRIDataCoreObjectFactory.h"

#include "mitkBaseRenderer.h"
#include "mitkDataNode.h"

#include "PCMRIData.h"
#include "PCMRIDataMapper.h"

typedef std::multimap<std::string, std::string> MultimapType;

namespace crimson {

PCMRIDataCoreObjectFactory::PCMRIDataCoreObjectFactory()
: CoreObjectFactoryBase()
{
    static bool alreadyDone = false;
    if (!alreadyDone)
    {
        mitk::CoreObjectFactory::GetInstance()->RegisterExtraFactory(this);
        alreadyDone = true;
    }
}

PCMRIDataCoreObjectFactory::~PCMRIDataCoreObjectFactory()
{
}

mitk::Mapper::Pointer PCMRIDataCoreObjectFactory::CreateMapper(mitk::DataNode* node, MapperSlotId id)
{
    if (!node->GetData()) {
        return nullptr;
    }

    if (dynamic_cast<PCMRIData*>(node->GetData())) {
        if (id == mitk::BaseRenderer::Standard3D) {
            auto mapper = PCMRIDataMapper3D::New();
            mapper->SetDataNode(node);
            return mapper.GetPointer();
        }
        else if (id == mitk::BaseRenderer::Standard2D) {
            auto mapper = PCMRIDataMapper2D::New();
            mapper->SetDataNode(node);
            return mapper.GetPointer();
        }
        
    }
    return nullptr;
}

void PCMRIDataCoreObjectFactory::SetDefaultProperties(mitk::DataNode* node)
{
    if (node == nullptr) {
        return;
    }

    if (dynamic_cast<PCMRIData*>(node->GetData())) {
        PCMRIDataMapper2D::SetDefaultProperties(node);
        PCMRIDataMapper3D::SetDefaultProperties(node);
    }
}

struct RegisterPCMRIDataCoreObjectFactory {
    RegisterPCMRIDataCoreObjectFactory()
        : m_Factory(PCMRIDataCoreObjectFactory::New())
    {
        mitk::CoreObjectFactory::GetInstance()->RegisterExtraFactory(m_Factory);
    }

    ~RegisterPCMRIDataCoreObjectFactory()
    {
        mitk::CoreObjectFactory::GetInstance()->UnRegisterExtraFactory(m_Factory);
    }

    PCMRIDataCoreObjectFactory::Pointer m_Factory;
};

static RegisterPCMRIDataCoreObjectFactory registerPCMRICoreObjectFactory;

} // namespace crimson
