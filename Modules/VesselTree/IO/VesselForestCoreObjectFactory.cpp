#include "VesselForestCoreObjectFactory.h"

#include "mitkBaseRenderer.h"
#include "mitkDataNode.h"

#include "vtkParametricSplineVesselPathData.h"
#include "vtkParametricSplineVesselPathVtkMapper3D.h"

typedef std::multimap<std::string, std::string> MultimapType;

namespace crimson {

VesselForestCoreObjectFactory::VesselForestCoreObjectFactory()
    : CoreObjectFactoryBase()
{
    static bool alreadyDone = false;
    if (!alreadyDone)
    {
        mitk::CoreObjectFactory::GetInstance()->RegisterExtraFactory(this);
        alreadyDone = true;
    }
}

VesselForestCoreObjectFactory::~VesselForestCoreObjectFactory()
{
}

mitk::Mapper::Pointer VesselForestCoreObjectFactory::CreateMapper(mitk::DataNode* node, MapperSlotId /*id*/)
{
    if (!node->GetData()) {
        return nullptr;
    }

    if (dynamic_cast<vtkParametricSplineVesselPathData*>(node->GetData())) {
        //if (id == mitk::BaseRenderer::Standard3D) {
        auto mapper = vtkParametricSplineVesselPathVtkMapper3D::New();
        mapper->SetDataNode(node);
        return mapper.GetPointer();
        //}
    }
    return nullptr;
}

void VesselForestCoreObjectFactory::SetDefaultProperties(mitk::DataNode* node)
{
    if (node == nullptr) {
        return;
    }

    if (dynamic_cast<vtkParametricSplineVesselPathData*>(node->GetData())) {
        vtkParametricSplineVesselPathVtkMapper3D::SetDefaultProperties(node);
    }
}

struct RegisterVesselForestCoreObjectFactory {
    RegisterVesselForestCoreObjectFactory()
        : m_Factory(VesselForestCoreObjectFactory::New())
    {
        mitk::CoreObjectFactory::GetInstance()->RegisterExtraFactory(m_Factory);
    }

    ~RegisterVesselForestCoreObjectFactory()
    {
        mitk::CoreObjectFactory::GetInstance()->UnRegisterExtraFactory(m_Factory);
    }

    VesselForestCoreObjectFactory::Pointer m_Factory;
};

static RegisterVesselForestCoreObjectFactory registerVesselForestCoreObjectFactory;

}

