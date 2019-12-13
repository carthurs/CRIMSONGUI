#include <usModuleActivator.h>
#include <usModuleContext.h>

#include <IO/OCCBRepDataIO.h>
#include <IO/OpenCascadeSolidKernelIOMimeTypes.h>
#include <IO/DiscreteSolidDataIO.h>
#include <IO/DiscreteDataIOMimeTypes.h>

namespace crimson
{
/**
\brief Registers services for SolidKernel module.
*/
class SolidKernelModuleActivator : public us::ModuleActivator
{
public:

    void Load(us::ModuleContext* context) override
    {
        us::ServiceProperties props;
        props[us::ServiceConstants::SERVICE_RANKING()] = 10;

        //add other readers/mime types as you write them here!

        std::vector<mitk::CustomMimeType*> mimeTypes = OpenCascadeSolidKernelIOMimeTypes::Get();
        for (std::vector<mitk::CustomMimeType*>::const_iterator mimeTypeIter = mimeTypes.begin(),
            iterEnd = mimeTypes.end(); mimeTypeIter != iterEnd; ++mimeTypeIter)
        {
            context->RegisterService(*mimeTypeIter, props);
        }

        m_OCCBRepDataIO.reset(new OCCBRepDataIO);

		mimeTypes = DiscreteDataIOMimeTypes::Get();
		for (std::vector<mitk::CustomMimeType*>::const_iterator mimeTypeIter = mimeTypes.begin(),
			iterEnd = mimeTypes.end(); mimeTypeIter != iterEnd; ++mimeTypeIter)
		{
			context->RegisterService(*mimeTypeIter, props);
		}

		m_DiscreteSolidDataIO.reset(new DiscreteSolidDataIO);
    }

    void Unload(us::ModuleContext*) override
    {
        m_OCCBRepDataIO.reset();
		m_DiscreteSolidDataIO.reset();
    }

private:

    std::unique_ptr<OCCBRepDataIO> m_OCCBRepDataIO;
	std::unique_ptr<DiscreteSolidDataIO> m_DiscreteSolidDataIO;

};

}

US_EXPORT_MODULE_ACTIVATOR(crimson::SolidKernelModuleActivator)
