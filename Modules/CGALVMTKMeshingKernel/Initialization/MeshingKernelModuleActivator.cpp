#include <usModuleActivator.h>
#include <usModuleContext.h>

#include <IO/MeshDataIO.h>
#include <IO/MeshingParametersDataIO.h>
#include <IO/MeshingKernelIOMimeTypes.h>

namespace crimson {

class MeshingKernelModuleActivator : public us::ModuleActivator
{
public:
    void Load(us::ModuleContext* context) override
    {
        us::ServiceProperties props;
        props[us::ServiceConstants::SERVICE_RANKING()] = 10;

        std::vector<mitk::CustomMimeType*> mimeTypes = MeshingKernelIOMimeTypes::Get();
        for (std::vector<mitk::CustomMimeType*>::const_iterator mimeTypeIter = mimeTypes.begin(),
            iterEnd = mimeTypes.end(); mimeTypeIter != iterEnd; ++mimeTypeIter)
        {
            context->RegisterService(*mimeTypeIter, props);
        }

        m_MeshDataIO.reset(new MeshDataIO);
        m_MeshingParametersDataIO.reset(new MeshingParametersDataIO);
    }
    void Unload(us::ModuleContext* /*context*/) override
    {
        m_MeshingParametersDataIO.reset();
        m_MeshDataIO.reset();
    }

private:
    std::unique_ptr<MeshDataIO> m_MeshDataIO;
    std::unique_ptr<MeshingParametersDataIO> m_MeshingParametersDataIO;
};

}

US_EXPORT_MODULE_ACTIVATOR(crimson::MeshingKernelModuleActivator)
