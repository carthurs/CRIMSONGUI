#pragma once

#include <mitkSurfaceVtkMapper2D.h>
#include <mitkSurfaceVtkMapper3D.h>

#include "PCMRIKernelExports.h"

//////////////////////////////////////////////////////////////////////////
// PCMRI data mappers are just subclasses of surface mappers 
// which provide a surface extracted from a PCMRI as input
//////////////////////////////////////////////////////////////////////////

namespace crimson {
    

class PCMRIKernel_EXPORT PCMRIDataMapper3D : public mitk::SurfaceVtkMapper3D {
public:
    mitkClassMacro(PCMRIDataMapper3D, mitk::SurfaceVtkMapper3D);

    itkFactorylessNewMacro(Self)
    itkCloneMacro(Self)

//might need to be overriden in subclasses
    const mitk::Surface* GetInput() override;
    void CalculateTimeStep(mitk::BaseRenderer*) override { }
    
    static void SetDefaultProperties(mitk::DataNode* node, mitk::BaseRenderer* renderer = NULL, bool overwrite = false);

protected:
    PCMRIDataMapper3D();
    virtual ~PCMRIDataMapper3D();

    mitk::Surface::ConstPointer m_Surface = nullptr;
};

class PCMRIKernel_EXPORT PCMRIDataMapper2D : public mitk::SurfaceVtkMapper2D {
public:
    mitkClassMacro(PCMRIDataMapper2D, mitk::SurfaceVtkMapper2D);

    itkFactorylessNewMacro(Self)
    itkCloneMacro(Self)

    const mitk::Surface* GetInput() const override;
    void CalculateTimeStep(mitk::BaseRenderer*) override { }

protected:
    PCMRIDataMapper2D();
    virtual ~PCMRIDataMapper2D();
};

} // namespace crimson
