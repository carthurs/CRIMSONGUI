#pragma once

#include <mitkAbstractFileIO.h>
#include <mitkIOMimeTypes.h>

#include <PythonQt.h>

#include "PythonSolverSetupServiceActivator.h"

namespace crimson
{

template <typename SolverSetupObject> class SolverSetupPythonObjectIO : public mitk::AbstractFileIO
{
public:
    SolverSetupPythonObjectIO(const mitk::CustomMimeType& mimeType, us::ModuleContext* context)
        : AbstractFileIO(SolverSetupObject::GetStaticNameOfClass(), mimeType, mimeType.GetCategory())
    {
        RegisterService(context);
    }

    virtual ~SolverSetupPythonObjectIO() {}

    std::vector<itk::SmartPointer<mitk::BaseData>> Read() override
    {
        std::vector<itk::SmartPointer<mitk::BaseData>> result;

        auto pythonServiceManager = PythonSolverSetupServiceActivator::getPythonService()->GetPythonManager();

        pythonServiceManager->executeString("import CRIMSONCore");

        auto module = PythonQtObjectPtr{pythonServiceManager->getVariable("CRIMSONCore")};
        module = PythonQtObjectPtr{module.getVariable("IO")};

        auto pyObjectPtr = PythonQtObjectPtr{module.call("loadFromFile", QVariantList{QString::fromStdString(GetInputLocation())})};

        if (pythonServiceManager->pythonErrorOccured() || pyObjectPtr.isNull()) {
            mitkThrow() << "Failed to load python object from " << GetInputLocation();
        }

        return {SolverSetupObject::New(pyObjectPtr).GetPointer()};
    }

    void Write() override
    {
        auto data = dynamic_cast<const SolverSetupObject*>(this->GetInput());

        if (!data) {
            mitkThrow() << "Input " << SolverSetupObject::GetStaticNameOfClass() << " data has not been set!";
        }

        auto pythonServiceManager = PythonSolverSetupServiceActivator::getPythonService()->GetPythonManager();

        pythonServiceManager->executeString("import CRIMSONCore");

        auto module = PythonQtObjectPtr{pythonServiceManager->getVariable("CRIMSONCore")};
        module = PythonQtObjectPtr{module.getVariable("IO")};

        module.call("saveToFile", QVariantList{QVariant::fromValue(data->getPythonObject()),
                                               QVariant::fromValue(QString::fromStdString(GetOutputLocation()))});

        if (pythonServiceManager->pythonErrorOccured()) {
            mitkThrow() << "Failed to write python object to " << GetOutputLocation();
        }
    }

protected:
    SolverSetupPythonObjectIO(const SolverSetupPythonObjectIO& other)
        : AbstractFileIO(other)
    {
    }
    AbstractFileIO* IOClone() const override { return new SolverSetupPythonObjectIO(*this); }
};

} // namespace crimson