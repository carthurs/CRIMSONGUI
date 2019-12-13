#include "SolidDataCoreObjectFactory.h"

#include "mitkBaseRenderer.h"
#include "mitkDataNode.h"

#include "SolidData.h"
#include "SolidDataMapper.h"

typedef std::multimap<std::string, std::string> MultimapType;

namespace crimson {

SolidDataCoreObjectFactory::SolidDataCoreObjectFactory()
: CoreObjectFactoryBase()
{
    static bool alreadyDone = false;
    if (!alreadyDone)
    {
        mitk::CoreObjectFactory::GetInstance()->RegisterExtraFactory(this);
        alreadyDone = true;
    }
}

SolidDataCoreObjectFactory::~SolidDataCoreObjectFactory()
{
}

mitk::Mapper::Pointer SolidDataCoreObjectFactory::CreateMapper(mitk::DataNode* node, MapperSlotId id)
{
    if (!node->GetData()) {
        return nullptr;
    }

    if (dynamic_cast<SolidData*>(node->GetData())) {
        if (id == mitk::BaseRenderer::Standard3D) {
            auto mapper = SolidDataMapper3D::New();
            mapper->SetDataNode(node);
            return mapper.GetPointer();
        }
        else if (id == mitk::BaseRenderer::Standard2D) {
            auto mapper = SolidDataMapper2D::New();
            mapper->SetDataNode(node);
            return mapper.GetPointer();
        }
        
    }
    return nullptr;
}

void SolidDataCoreObjectFactory::SetDefaultProperties(mitk::DataNode* node)
{
    if (node == nullptr) {
        return;
    }

    if (dynamic_cast<SolidData*>(node->GetData())) {
        SolidDataMapper2D::SetDefaultProperties(node);
        SolidDataMapper3D::SetDefaultProperties(node);
    }
}

struct RegisterSolidDataCoreObjectFactory {
    RegisterSolidDataCoreObjectFactory()
        : m_Factory(SolidDataCoreObjectFactory::New())
    {
        mitk::CoreObjectFactory::GetInstance()->RegisterExtraFactory(m_Factory);
    }

    ~RegisterSolidDataCoreObjectFactory()
    {
        mitk::CoreObjectFactory::GetInstance()->UnRegisterExtraFactory(m_Factory);
    }

    SolidDataCoreObjectFactory::Pointer m_Factory;
};

static RegisterSolidDataCoreObjectFactory registerSolidCoreObjectFactory;

} // namespace crimson
