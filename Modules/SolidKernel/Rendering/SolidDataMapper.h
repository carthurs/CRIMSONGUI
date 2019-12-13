#pragma once

#include <mitkSurfaceVtkMapper2D.h>
#include <mitkSurfaceVtkMapper3D.h>

#include "SolidKernelExports.h"

//////////////////////////////////////////////////////////////////////////
// Solid data mappers are just subclasses of surface mappers 
// which provide a surface extracted from a solid as input
//////////////////////////////////////////////////////////////////////////

namespace crimson {

class SolidKernel_EXPORT SolidDataMapper3D : public mitk::SurfaceVtkMapper3D {
public:
    mitkClassMacro(SolidDataMapper3D, mitk::SurfaceVtkMapper3D);

    itkFactorylessNewMacro(Self)
    itkCloneMacro(Self)

//might need to be overriden in subclasses
    const mitk::Surface* GetInput() override;
    void CalculateTimeStep(mitk::BaseRenderer*) override { }
    
    static void SetDefaultProperties(mitk::DataNode* node, mitk::BaseRenderer* renderer = NULL, bool overwrite = false);

protected:
    SolidDataMapper3D();
    virtual ~SolidDataMapper3D();

    mitk::Surface::ConstPointer m_Surface = nullptr;
};

class SolidKernel_EXPORT SolidDataMapper2D : public mitk::SurfaceVtkMapper2D {
public:
    mitkClassMacro(SolidDataMapper2D, mitk::SurfaceVtkMapper2D);

    itkFactorylessNewMacro(Self)
    itkCloneMacro(Self)

    const mitk::Surface* GetInput() const override;
    void CalculateTimeStep(mitk::BaseRenderer*) override { }

protected:
    SolidDataMapper2D();
    virtual ~SolidDataMapper2D();
};

} // namespace crimson
