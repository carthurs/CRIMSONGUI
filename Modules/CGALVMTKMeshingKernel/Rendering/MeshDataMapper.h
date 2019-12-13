#pragma once

#include <mitkSurfaceVtkMapper2D.h>
#include <mitkUnstructuredGridVtkMapper3D.h>

#include "CGALVMTKMeshingKernelExports.h"

//////////////////////////////////////////////////////////////////////////
// Mesh data mappers are just subclasses of surface mappers 
// which provide a surface extracted from Mesh as input
//////////////////////////////////////////////////////////////////////////

namespace crimson {


class CGALVMTKMeshingKernel_EXPORT MeshDataMapper3D : public mitk::UnstructuredGridVtkMapper3D {
public:
    mitkClassMacro(MeshDataMapper3D, mitk::UnstructuredGridVtkMapper3D);

    itkFactorylessNewMacro(Self)
    itkCloneMacro(Self)

    const mitk::UnstructuredGrid* GetInput() override;
    void CalculateTimeStep(mitk::BaseRenderer*) override { }

protected:
    MeshDataMapper3D();
    virtual ~MeshDataMapper3D();
};

class CGALVMTKMeshingKernel_EXPORT MeshDataMapper2D : public mitk::SurfaceVtkMapper2D {
public:
    mitkClassMacro(MeshDataMapper2D, mitk::SurfaceVtkMapper2D);

    itkFactorylessNewMacro(Self)
    itkCloneMacro(Self)

    const mitk::Surface* GetInput() const override;
    void CalculateTimeStep(mitk::BaseRenderer*) override { }

protected:
    MeshDataMapper2D();
    virtual ~MeshDataMapper2D();
};

} // namespace crimson
