#include "PCMRIDataMapper.h"

#include "PCMRIData.h"

namespace crimson {

PCMRIDataMapper3D::PCMRIDataMapper3D()
{
}

PCMRIDataMapper3D::~PCMRIDataMapper3D()
{

}

const mitk::Surface* PCMRIDataMapper3D::GetInput()
{
    auto brep = dynamic_cast<PCMRIData*>(GetDataNode()->GetData());

    if (!brep) {
        return nullptr;
    }

    return brep->getSurfaceRepresentation();
}

void PCMRIDataMapper3D::SetDefaultProperties(mitk::DataNode* node, mitk::BaseRenderer* renderer /*= NULL*/, bool overwrite /*= false*/)
{
    mitk::SurfaceVtkMapper3D::SetDefaultProperties(node, renderer, overwrite);
    node->AddProperty("color.selectable", mitk::ColorProperty::New(0.1f, 0.9f, 0.2f), renderer, overwrite);
}


PCMRIDataMapper2D::PCMRIDataMapper2D()
{

}

PCMRIDataMapper2D::~PCMRIDataMapper2D()
{

}

const mitk::Surface* PCMRIDataMapper2D::GetInput() const
{
    auto brep = dynamic_cast<PCMRIData*>(GetDataNode()->GetData());

    if (!brep) {
        return nullptr;
    }

    return brep->getSurfaceRepresentation();
}

} // namespace crimson
