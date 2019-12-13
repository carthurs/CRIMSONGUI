#include "MeshDataCoreObjectFactory.h"

#include "mitkProperties.h"
#include "mitkBaseRenderer.h"
#include "mitkDataNode.h"

#include "MeshData.h"
#include "MeshDataMapper.h"

typedef std::multimap<std::string, std::string> MultimapType;

namespace crimson {

MeshDataCoreObjectFactory::MeshDataCoreObjectFactory()
: CoreObjectFactoryBase()
{
    static bool alreadyDone = false;
    if (!alreadyDone)
    {
        mitk::CoreObjectFactory::GetInstance()->RegisterExtraFactory(this);
    }

}

MeshDataCoreObjectFactory::~MeshDataCoreObjectFactory()
{
}

mitk::Mapper::Pointer MeshDataCoreObjectFactory::CreateMapper(mitk::DataNode* node, MapperSlotId id)
{
    if (!node->GetData()) {
        return nullptr;
    }

    if (dynamic_cast<MeshData*>(node->GetData())) {
        if (id == mitk::BaseRenderer::Standard3D) {
            auto mapper = MeshDataMapper3D::New();
            mapper->SetDataNode(node);
            return mapper.GetPointer();
        }
        else if (id == mitk::BaseRenderer::Standard2D) {
            auto mapper = MeshDataMapper2D::New();
            mapper->SetDataNode(node);
            return mapper.GetPointer();
        }
        
    }
    return nullptr;
}

void MeshDataCoreObjectFactory::SetDefaultProperties(mitk::DataNode* node)
{
    if (node == nullptr) {
        return;
    }

    if (dynamic_cast<MeshData*>(node->GetData())) {
        MeshDataMapper2D::SetDefaultProperties(node);
        MeshDataMapper3D::SetDefaultProperties(node);
    }
}

struct RegisterMeshDataCoreObjectFactory {
    RegisterMeshDataCoreObjectFactory()
        : m_Factory(MeshDataCoreObjectFactory::New())
    {
        mitk::CoreObjectFactory::GetInstance()->RegisterExtraFactory(m_Factory);
    }

    ~RegisterMeshDataCoreObjectFactory()
    {
        mitk::CoreObjectFactory::GetInstance()->UnRegisterExtraFactory(m_Factory);
    }

    MeshDataCoreObjectFactory::Pointer m_Factory;
};

static RegisterMeshDataCoreObjectFactory registerMeshCoreObjectFactory;

} // namespace crimson
