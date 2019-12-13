#include "SolidDataMapper.h"

#include "SolidData.h"

namespace crimson {

SolidDataMapper3D::SolidDataMapper3D()
{
}

SolidDataMapper3D::~SolidDataMapper3D()
{

}

const mitk::Surface* SolidDataMapper3D::GetInput()
{
    auto brep = dynamic_cast<SolidData*>(GetDataNode()->GetData());

    if (!brep) {
        return nullptr;
    }

    return brep->getSurfaceRepresentation();
}

void SolidDataMapper3D::SetDefaultProperties(mitk::DataNode* node, mitk::BaseRenderer* renderer /*= NULL*/, bool overwrite /*= false*/)
{
    mitk::SurfaceVtkMapper3D::SetDefaultProperties(node, renderer, overwrite);
    node->AddProperty("color.selectable", mitk::ColorProperty::New(0.1f, 0.9f, 0.2f), renderer, overwrite);
}


SolidDataMapper2D::SolidDataMapper2D()
{

}

SolidDataMapper2D::~SolidDataMapper2D()
{

}

const mitk::Surface* SolidDataMapper2D::GetInput() const
{
    auto brep = dynamic_cast<SolidData*>(GetDataNode()->GetData());

    if (!brep) {
        return nullptr;
    }

    return brep->getSurfaceRepresentation();
}

} // namespace crimson
