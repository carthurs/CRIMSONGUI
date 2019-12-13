#include "MeshDataMapper.h"

#include "MeshData.h"

namespace crimson
{

MeshDataMapper3D::MeshDataMapper3D() {}

MeshDataMapper3D::~MeshDataMapper3D() {}

const mitk::UnstructuredGrid* MeshDataMapper3D::GetInput()
{
    auto mesh = dynamic_cast<MeshData*>(GetDataNode()->GetData());

    if (!mesh) {
        return nullptr;
    }

    return mesh->getUnstructuredGridRepresentation();
}

MeshDataMapper2D::MeshDataMapper2D() {}

MeshDataMapper2D::~MeshDataMapper2D() {}

const mitk::Surface* MeshDataMapper2D::GetInput() const
{
    auto brep = dynamic_cast<MeshData*>(GetDataNode()->GetData());

    if (!brep) {
        return nullptr;
    }

    return brep->getSurfaceRepresentation();
}

} // namespace crimson
