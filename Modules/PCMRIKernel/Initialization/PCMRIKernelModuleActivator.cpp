#include <usModuleActivator.h>
#include <usModuleContext.h>

#include <IO/PCMRIDataIO.h>
#include <IO/PCMRIKernelIOMimeTypes.h>

namespace crimson
{
/**
\brief Registers services for PCMRIKernel module.
*/
class PCMRIKernelModuleActivator : public us::ModuleActivator
{
public:

    void Load(us::ModuleContext* context) override
    {

        us::ServiceProperties props;
        props[us::ServiceConstants::SERVICE_RANKING()] = 10;

        //add other readers/mime types as you write them here!

        std::vector<mitk::CustomMimeType*> mimeTypes = PCMRIKernelIOMimeTypes::Get();
        for (std::vector<mitk::CustomMimeType*>::const_iterator mimeTypeIter = mimeTypes.begin(),
            iterEnd = mimeTypes.end(); mimeTypeIter != iterEnd; ++mimeTypeIter)
        {
            context->RegisterService(*mimeTypeIter, props);
        }

        m_PCMRIDataIO.reset(new PCMRIDataIO);

    }

    void Unload(us::ModuleContext*) override
    {
        m_PCMRIDataIO.reset();
    }

private:

    std::unique_ptr<PCMRIDataIO> m_PCMRIDataIO;
	
};

}

US_EXPORT_MODULE_ACTIVATOR(crimson::PCMRIKernelModuleActivator)
