#include <QFileInfo>
#include <QDir>

#include <mitkPythonService.h>

#include "PythonQtWrappers.h"
#include "PythonSolverSetupServiceActivator.h"

#include <PythonSolverSetupService.h>
#include <PythonSolverParametersData.h>
#include <PythonSolverStudyData.h>
#include <PythonBoundaryConditionSet.h>
#include <PythonBoundaryCondition.h>
#include <PythonMaterialData.h>

#include <SolverSetupPythonObjectIO.h>
#include <IO/IOUtilDataSerializer.h>

// TODO: remove duplication with CREATE_SOLVERSETUPOBJECT_MIMETYPE_AND_IO
REGISTER_IOUTILDATA_SERIALIZER(PythonSolverParametersData, "pyssd")
REGISTER_IOUTILDATA_SERIALIZER(PythonSolverStudyData, "pystudy")
REGISTER_IOUTILDATA_SERIALIZER(PythonBoundaryConditionSet, "pybcs");
REGISTER_IOUTILDATA_SERIALIZER(PythonBoundaryCondition, "pybc");
REGISTER_IOUTILDATA_SERIALIZER(PythonMaterialData, "pymat");

namespace crimson
{
class SolverSetupObjectMimeType : public mitk::CustomMimeType
{
public:
    SolverSetupObjectMimeType(const std::string& extension, const std::string& description)
    {
        SetName(mitk::IOMimeTypes::DEFAULT_BASE_NAME() + "." + extension);
        SetCategory(description);
        AddExtension(extension);
    }

    virtual ~SolverSetupObjectMimeType() {}
};

#define CREATE_SOLVERSETUPOBJECT_MIMETYPE_AND_IO(type, IOType, extension, description)                                         \
    mimeType = std::static_pointer_cast<mitk::CustomMimeType>(                                                                 \
        std::make_shared<crimson::SolverSetupObjectMimeType>(extension, description));                                         \
    _mimeTypes.push_back(mimeType);                                                                                            \
    _ioClasses.push_back(                                                                                                      \
        std::static_pointer_cast<mitk::AbstractFileIO>(std::make_shared<IOType<type>>(*mimeType, moduleContext)))

PythonSolverSetupServiceActivator* PythonSolverSetupServiceActivator::_instance = nullptr;

PythonSolverSetupServiceActivator::PythonSolverSetupServiceActivator()
{
    Expects(_instance == nullptr);
    _instance = this;
}

PythonSolverSetupServiceActivator::~PythonSolverSetupServiceActivator() { _instance = nullptr; }

void PythonSolverSetupServiceActivator::Load(us::ModuleContext* moduleContext)
{
    // Setup the python environment
    Expects(!_pythonServiceTracker);
    _pythonServiceTracker.reset(
        new us::ServiceTracker<mitk::IPythonService>(moduleContext, us::LDAPFilter("(Name=PythonService)")));
    _pythonServiceTracker->Open();

    std::shared_ptr<mitk::CustomMimeType> mimeType;

    CREATE_SOLVERSETUPOBJECT_MIMETYPE_AND_IO(crimson::PythonSolverParametersData, crimson::SolverSetupPythonObjectIO, "pyssd",
                                             "Python Solver Parameters Data");
    CREATE_SOLVERSETUPOBJECT_MIMETYPE_AND_IO(crimson::PythonSolverStudyData, crimson::SolverSetupPythonObjectIO, "pystudy",
                                             "Python Solver Study");
    CREATE_SOLVERSETUPOBJECT_MIMETYPE_AND_IO(crimson::PythonBoundaryConditionSet, crimson::SolverSetupPythonObjectIO, "pybcs",
                                             "Python BC set");
    CREATE_SOLVERSETUPOBJECT_MIMETYPE_AND_IO(crimson::PythonBoundaryCondition, crimson::SolverSetupPythonObjectIO, "pybc",
                                             "Python Boundary Condition");
    CREATE_SOLVERSETUPOBJECT_MIMETYPE_AND_IO(crimson::PythonMaterialData, crimson::SolverSetupPythonObjectIO, "pymat",
                                             "Python Material");

    us::ServiceProperties props;
    props[us::ServiceConstants::SERVICE_RANKING()] = 10;

    for (const auto& mimeTypePtr : _mimeTypes) {
        moduleContext->RegisterService(mimeTypePtr.get(), props);
    }
}

void PythonSolverSetupServiceActivator::Unload(us::ModuleContext*)
{
    Expects(_pythonServiceTracker.get() != nullptr);
    _solverSetupServices.clear();
    _pythonServiceTracker->Close();
    _pythonServiceTracker.reset();
    _mimeTypes.clear();
    _ioClasses.clear();
}

void PythonSolverSetupServiceActivator::reloadPythonSolverSetups(const QStringList& modulePaths)
{
    Expects(_instance != nullptr);

    _instance->_modulePaths = modulePaths;
    reloadPythonSolverSetups();
}

void PythonSolverSetupServiceActivator::reloadPythonSolverSetups(bool reloadModules)
{
    Expects(_instance != nullptr);

    static bool classesRegistered = false;
    if (!classesRegistered) {
        classesRegistered = true;
        // Force load the python module
        mitk::IPythonService::ForceLoadModule();

        // Register wrapper classes
        PythonQt::self()->registerCPPClass("FaceType", "", "CRIMSON", PythonQtCreateObject<crimson::FaceTypeQtWrapper>);
        PythonQt::self()->registerCPPClass("ArrayDataType", "", "CRIMSON", PythonQtCreateObject<crimson::ArrayDataTypeQtWrapper>);
        PythonQt::self()->registerCPPClass("Utils", "", "CRIMSON", PythonQtCreateObject<crimson::Utils>);
		PythonQt::self()->registerClass(&crimson::SolidDataQtWrapper::staticMetaObject, "CRIMSON");
        PythonQt::self()->registerClass(&crimson::MeshDataQtWrapper::staticMetaObject, "CRIMSON");
        PythonQt::self()->registerClass(&crimson::VesselForestDataQtWrapper::staticMetaObject, "CRIMSON");
		PythonQt::self()->registerClass(&crimson::PCMRIDataQtWrapper::staticMetaObject, "CRIMSON");
    }

    _instance->_solverSetupServices.clear();

    for (const auto& path : _instance->_modulePaths) {
        try {
            auto service = std::make_unique<PythonSolverSetupService>(gsl::as_temp_span(path.toStdString()), reloadModules);
            _instance->_solverSetupServices.push_back(std::move(service));
        } catch (const std::runtime_error& e) {
            MITK_ERROR << e.what();
        }
    }
}

mitk::PythonService* PythonSolverSetupServiceActivator::getPythonService()
{
    Expects(_instance != nullptr && _instance->_pythonServiceTracker.get() != nullptr);
    return static_cast<mitk::PythonService*>(_instance->_pythonServiceTracker->GetService());
}
}

US_EXPORT_MODULE_ACTIVATOR(crimson::PythonSolverSetupServiceActivator)
