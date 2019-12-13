#include <usModuleActivator.h>
#include <usModuleContext.h>

#include <IO/SolutionDataIO.h>
#include <IO/SolverSetupServiceIOMimeTypes.h>

namespace crimson {

class SolverSetupServiceModuleActivator : public us::ModuleActivator
{
public:
    void Load(us::ModuleContext* context) override
    {
        us::ServiceProperties props;
        props[us::ServiceConstants::SERVICE_RANKING()] = 10;

        std::vector<mitk::CustomMimeType*> mimeTypes = SolverSetupServiceIOMimeTypes::Get();
        for (std::vector<mitk::CustomMimeType*>::const_iterator mimeTypeIter = mimeTypes.begin(),
            iterEnd = mimeTypes.end(); mimeTypeIter != iterEnd; ++mimeTypeIter)
        {
            context->RegisterService(*mimeTypeIter, props);
        }

        m_SolutionDataIO.reset(new SolutionDataIO);
    }
    void Unload(us::ModuleContext* /*context*/) override
    {
        m_SolutionDataIO.reset();
    }

private:
    std::unique_ptr<SolutionDataIO> m_SolutionDataIO;
};

}

US_EXPORT_MODULE_ACTIVATOR(crimson::SolverSetupServiceModuleActivator)
