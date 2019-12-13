#include <usModuleActivator.h>
#include <usModuleContext.h>

#include <IO/VesselForestDataIO.h>
#include <IO/vtkParametricSplineVesselPathIO.h>
#include <IO/VesselTreeIOMimeTypes.h>

namespace crimson
{
/**
\brief Registers services for VesselTree module.
*/
class VesselTreeModuleActivator : public us::ModuleActivator
{
public:

    void Load(us::ModuleContext* context) override
    {
        us::ServiceProperties props;
        props[us::ServiceConstants::SERVICE_RANKING()] = 10;

        std::vector<mitk::CustomMimeType*> mimeTypes = VesselTreeIOMimeTypes::Get();
        for (std::vector<mitk::CustomMimeType*>::const_iterator mimeTypeIter = mimeTypes.begin(),
            iterEnd = mimeTypes.end(); mimeTypeIter != iterEnd; ++mimeTypeIter)
        {
            context->RegisterService(*mimeTypeIter, props);
        }

        m_VesselForestDataIO.reset(new VesselForestDataIO);
        m_vtkParametricSplineVesselPathIO.reset(new vtkParametricSplineVesselPathIO);
    }

    void Unload(us::ModuleContext*) override
    {
        m_VesselForestDataIO.reset();
        m_vtkParametricSplineVesselPathIO.reset();
    }

private:

    std::unique_ptr<VesselForestDataIO> m_VesselForestDataIO;
    std::unique_ptr<vtkParametricSplineVesselPathIO> m_vtkParametricSplineVesselPathIO;

};

}

US_EXPORT_MODULE_ACTIVATOR(crimson::VesselTreeModuleActivator)
