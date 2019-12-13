#include <mitkPythonService.h>

#include <QMessageBox>
#include <ctkPopupWidget.h>

#include <PythonSolverStudyData.h>
#include <PythonSolverSetupServiceActivator.h>

#include <PythonSolverParametersData.h>
#include <PythonBoundaryConditionSet.h>
#include <PythonBoundaryCondition.h>
#include <PythonMaterialData.h>

#include <SolverSetupUtils.h>

#include "PythonQtWrappers.h"
#include "SolverSetupPythonUtils.h"

#include <vtkIntArray.h>
#include <vtkDoubleArray.h>

#include <cstdint>

namespace crimson
{
PythonSolverStudyData::PythonSolverStudyData(PythonQtObjectPtr pyStudyObject)
    : _pyStudyObject(pyStudyObject)
{
    Expects(!pyStudyObject.isNull());
}

PythonSolverStudyData::PythonSolverStudyData(const Self& other)
    : _pyStudyObject(clonePythonObject(other._pyStudyObject))
{
}

PythonSolverStudyData::~PythonSolverStudyData() {}

template <typename ExpectedResultT>
ExpectedResultT PythonSolverStudyData::_getStudyPartNodeUIDs(gsl::cstring_span<> functionName) const
{
    auto pythonService = PythonSolverSetupServiceActivator::getPythonService();

    auto result = _pyStudyObject.call(gsl::to_QString(functionName));

    if (pythonService->PythonErrorOccured() || !result.canConvert<ExpectedResultT>()) {
        MITK_ERROR << "Failed to get node uid from solver study using " << gsl::to_string(functionName);
        return {};
    }

    return qvariant_cast<ExpectedResultT>(result);
}

void PythonSolverStudyData::_setStudyPartNodeUIDs(gsl::cstring_span<> functionName, const QVariantList& args)
{
    auto pythonService = PythonSolverSetupServiceActivator::getPythonService();

    _pyStudyObject.call(gsl::to_QString(functionName), args);

    if (pythonService->PythonErrorOccured()) {
        MITK_ERROR << "Failed to set node uid to solver study using " << gsl::to_string(functionName);
    }

    this->Modified();
}

gsl::cstring_span<> PythonSolverStudyData::getMeshNodeUID() const
{
    _meshNodeUIDCache = _getStudyPartNodeUIDs<QString>("getMeshNodeUID").toStdString();
    return _meshNodeUIDCache;
}

void PythonSolverStudyData::setMeshNodeUID(gsl::cstring_span<> nodeUID) { _setStudyPartNodeUIDs("setMeshNodeUID", QVariantList{gsl::to_QString(nodeUID)}); }

gsl::cstring_span<> PythonSolverStudyData::getSolverParametersNodeUID() const
{
    _solverParametersNodeUIDCache = _getStudyPartNodeUIDs<QString>("getSolverParametersNodeUID").toStdString();
    return _solverParametersNodeUIDCache;
}

void PythonSolverStudyData::setSolverParametersNodeUID(gsl::cstring_span<> nodeUID)
{
    _setStudyPartNodeUIDs("setSolverParametersNodeUID", QVariantList{gsl::to_QString(nodeUID)});
}

ImmutableValueRange<gsl::cstring_span<>> PythonSolverStudyData::getBoundaryConditionSetNodeUIDs() const
{
    auto resultList = _getStudyPartNodeUIDs<QVariantList>("getBoundaryConditionSetNodeUIDs");

    _boundaryConditionSetNodeUIDsCache.clear();
    _boundaryConditionSetNodeUIDsCache.reserve(resultList.size());
    std::transform(resultList.begin(), resultList.end(), std::back_inserter(_boundaryConditionSetNodeUIDsCache),
                   [](const QVariant& element) { return element.toString().toStdString(); });

    return make_transforming_immutable_range<gsl::cstring_span<>>(_boundaryConditionSetNodeUIDsCache);
}

void PythonSolverStudyData::setBoundaryConditionSetNodeUIDs(ImmutableValueRange<gsl::cstring_span<>> nodeUIDs)
{
    auto uidList = QVariantList{};
    std::transform(nodeUIDs.begin(), nodeUIDs.end(), std::back_inserter(uidList),
                   [](gsl::cstring_span<> uid) { return QVariant::fromValue(gsl::to_QString(uid)); });

    return _setStudyPartNodeUIDs("setBoundaryConditionSetNodeUIDs", QVariantList{QVariant::fromValue(uidList)});
}

ImmutableValueRange<gsl::cstring_span<>> PythonSolverStudyData::getMaterialNodeUIDs() const
{
    auto resultList = _getStudyPartNodeUIDs<QVariantList>("getMaterialNodeUIDs");

    _materialNodeUIDsCache.clear();
    _materialNodeUIDsCache.reserve(resultList.size());
    std::transform(resultList.begin(), resultList.end(), std::back_inserter(_materialNodeUIDsCache),
                   [](const QVariant& element) { return element.toString().toStdString(); });

    return make_transforming_immutable_range<gsl::cstring_span<>>(_materialNodeUIDsCache);
}

void PythonSolverStudyData::setMaterialNodeUIDs(ImmutableValueRange<gsl::cstring_span<>> nodeUIDs)
{
    auto uidList = QVariantList{};
    std::transform(nodeUIDs.begin(), nodeUIDs.end(), std::back_inserter(uidList),
                   [](gsl::cstring_span<> uid) { return QVariant::fromValue(gsl::to_QString(uid)); });

    return _setStudyPartNodeUIDs("setMaterialNodeUIDs", QVariantList{QVariant::fromValue(uidList)});
}

PythonQtObjectPtr PythonSolverStudyData::_createSolutionStorage(gsl::span<const SolutionData*> solutions) const
{
    if (solutions.size() == 0) {
        return PythonQtObjectPtr{};
    }

    auto pythonService = PythonSolverSetupServiceActivator::getPythonService();

    pythonService->Execute("import CRIMSONCore");

    auto mainContext = pythonService->GetPythonManager()->mainContext();

    auto arrayInfoObjectsMap = QVariantMap{};

    // Numpy array creation has to go directly through PythonQt because otherwise
    // it's impossible to get the PythonQtObjectPtr due to numpy.array being enumerable
    // (and thus automatically converted to QVariantList)
    auto numpyModule = PythonQt::self()->importModule("numpy");

    for (const auto& solutionPtr : solutions) {
        auto arrayData = solutionPtr->getArrayData();

        auto variantArrayData = QVariantList{};
        variantArrayData.reserve(arrayData->GetNumberOfTuples());

        // Fill the data
        for (int i = 0; i < arrayData->GetNumberOfTuples(); ++i) {
            auto component = QVariantList{};
            component.reserve(arrayData->GetNumberOfComponents());
            for (int c = 0; c < arrayData->GetNumberOfComponents(); ++c) {
                component.push_back(arrayData->GetComponent(i, c));
            }
            variantArrayData.push_back(component);
        }

        Expects(arrayData->GetDataType() == VTK_INT || arrayData->GetDataType() == VTK_DOUBLE);
        QString dtypeName = QString::fromLatin1(arrayData->GetDataType() == VTK_INT ? "i4" : "f8");

        auto numpyArrayPtr = PythonQtObjectPtr{PythonQt::self()->callAndReturnPyObject(PythonQt::self()->lookupCallable(numpyModule, "array"),
                                                                                       QVariantList{} << QVariant::fromValue(variantArrayData) << dtypeName)};

        if (numpyArrayPtr.isNull()) {
            MITK_ERROR << "Failed to create a numpy.array object";
            return PythonQtObjectPtr{};
        }

        // Create ArrayInfo object
        auto arrayInfoPtr =
            PythonQtObjectPtr{mainContext.call("CRIMSONCore.SolutionStorage.SolutionStorage.ArrayInfo",
                                               QVariantList{} << QVariant::fromValue(numpyArrayPtr))};

        if (arrayInfoPtr.isNull()) {
            MITK_ERROR << "Failed to create the SolutionStorage.ArrayInfo object";
            return PythonQtObjectPtr{};
        }

        arrayInfoObjectsMap[QString::fromLatin1(arrayData->GetName())] = QVariant::fromValue(arrayInfoPtr);
    }

    auto solutionStoragePtr = PythonQtObjectPtr{mainContext.call("CRIMSONCore.SolutionStorage.SolutionStorage", QVariantList{arrayInfoObjectsMap})};

    if (solutionStoragePtr.isNull()) {
        MITK_ERROR << "Failed to create the SolutionStorage object";
        return PythonQtObjectPtr{};
    }

    return solutionStoragePtr;
}

static VesselForestDataQtWrapper createVesselForestDataQtWrapper(const IDataProvider& dataProvider, const mitk::BaseData* vesselForestData_)
{
    if (!vesselForestData_) {
        return VesselForestDataQtWrapper{};
    }

    auto vesselForestData = static_cast<const VesselForestData*>(vesselForestData_);
    VesselForestDataQtWrapper::UIDToVesselPathDataMap uidToVesselPathDataMap;

    for (const auto& pathUID : vesselForestData->getVessels()) {
        auto vesselData = dataProvider.findDataByUID(pathUID);
        if (!vesselData) {
            MITK_WARN << "Vessel data missing";
            continue;
        }

        uidToVesselPathDataMap[pathUID] = static_cast<VesselPathAbstractData*>(vesselData);
    }

    return VesselForestDataQtWrapper{vesselForestData, uidToVesselPathDataMap};
}

QVariantList PythonSolverStudyData::_gatherBCs(const IDataProvider& dataProvider) const
{
    QVariantList pyBCObjects;
    for (const auto& bcSetUID : getBoundaryConditionSetNodeUIDs()) {
        for (const auto& bcData : dataProvider.getChildrenData(bcSetUID)) {
            Expects(dynamic_cast<IBoundaryCondition*>(bcData) != nullptr);

            auto pyBCData = dynamic_cast<PythonBoundaryCondition*>(bcData);
            if (!pyBCData) {
                MITK_ERROR << "Incorrect data types passed for writing solver setup";
                return {};
            }
			if ((gsl::to_string(pyBCData->getName()) == "Prescribed velocities (PC-MRI)" )|| (gsl::to_string(pyBCData->getName()) == "PCMRI"))
			{
				auto pySolverParametersData = dynamic_cast<const PythonSolverParametersData*>(dataProvider.findDataByUID(getSolverParametersNodeUID()));
				auto solverParametersData = static_cast<ISolverParametersData*>(dataProvider.findDataByUID(getSolverParametersNodeUID()));
				QtPropertyStorage* properties = solverParametersData->getPropertyStorage();
				auto timeStepSize = properties->getPropertyValueByName("Time step size");

				auto uid = std::string{};
				//bcData->GetStringProperty("mapping.pcmrinode", uid);
				auto pcmri = dynamic_cast<PCMRIData*>(dataProvider.findDataByUID(dynamic_cast<PythonBoundaryCondition*>(bcData)->getDataUID()));
				pcmri->setTimeStepSize(timeStepSize.toDouble());
				if (!pcmri->getMesh())
				{
					MeshData::Pointer mesh = dynamic_cast<MeshData*>(dataProvider.findDataByUID(pcmri->getMeshNodeUID()));
					pcmri->setMesh(mesh.GetPointer());

					if (gsl::to_string(getMeshNodeUID()) != pcmri->getMeshNodeUID().c_str())
					{
						QMessageBox::warning(nullptr, "Different mesh used for PC-MRI mapping.", 
							"The mesh selected for solver is different from the mesh used for PC-MRI mapping. \n Please change the selected mesh or remap the PC-MRI data.");
						QVariantList empty;
						return empty;
					}
				}
				pcmri->timeInterpolate();
				auto wrappedPCMRI = PCMRIDataQtWrapper{ pcmri };
				dynamic_cast<IBoundaryCondition*>(bcData)->setDataObject(QVariantList{ QVariant::fromValue(wrappedPCMRI) });
			}
            pyBCObjects.push_back(QVariant::fromValue(pyBCData->getPythonObject()));
        }
    }

    return pyBCObjects;
}


QVariantList PythonSolverStudyData::_gatherMaterials(const IDataProvider& dataProvider) const
{
    auto pyMaterialObjects = QVariantList{};
    for (const auto& materialUID : getMaterialNodeUIDs()) {
        auto materialData = dataProvider.findDataByUID(materialUID);

        if (!materialData) {
            continue;
        }

        auto pyMaterialData = dynamic_cast<const PythonMaterialData*>(materialData);
        if (!pyMaterialData) {
            MITK_ERROR << "Incorrect data types passed for writing solver setup";
            return {};
        }
        pyMaterialObjects.push_back(QVariant::fromValue(pyMaterialData->getPythonObject()));
    }
    return pyMaterialObjects;
}

bool PythonSolverStudyData::runFlowsolver()
{
	_pyStudyObject.call("runFlowsolver");

	return !PythonQt::self()->hadError();
}

bool PythonSolverStudyData::writeSolverSetup(const IDataProvider& dataProvider, const mitk::BaseData* vesselForestData,
                                             gsl::not_null<const mitk::BaseData*> solidModelData_, gsl::span<const SolutionData*> solutions)
{
    // Gather boundary conditions
    // Test variables
    auto pySolverParametersData = dynamic_cast<const PythonSolverParametersData*>(dataProvider.findDataByUID(getSolverParametersNodeUID()));
    auto solidModelData = dynamic_cast<const SolidData*>(solidModelData_.get());
    auto meshData = dynamic_cast<const MeshData*>(dataProvider.findDataByUID(getMeshNodeUID()));

    if (!pySolverParametersData || !solidModelData || !meshData) {
        MITK_ERROR << "Incorrect data types passed for writing solver setup";
        return false;
    }

    QVariantMap uidToFileNameMap;
    auto vesselForestDataQtWrapper = createVesselForestDataQtWrapper(dataProvider, vesselForestData);

    for (const auto& uidVesselPathDataPair : vesselForestDataQtWrapper.getUIDToVesselPathDataMap()) {
        auto name = std::string{};
        uidVesselPathDataPair.second->GetPropertyList()->GetStringProperty("name", name);
        auto simplifiedName = QString::fromStdString(name);
        simplifiedName.replace(QRegExp("\\W"), QString("_"));

        if (uidToFileNameMap.keys(simplifiedName).size() != 0) {
            MITK_WARN << "Duplicate vessel name found: " << name;
            int i = 1;
            QString simplifiedNameWithPostfix;
            do {
                simplifiedNameWithPostfix = QString("%1_%2").arg(simplifiedName).arg(i++);
            } while (uidToFileNameMap.keys(simplifiedNameWithPostfix).size() != 0);
            simplifiedName = simplifiedNameWithPostfix;
        }

        uidToFileNameMap[QString::fromStdString(uidVesselPathDataPair.first)] = QVariant::fromValue(simplifiedName);
    }

    auto vesselForestDataVariant = vesselForestData ? QVariant::fromValue(vesselForestDataQtWrapper) : QVariant{};

    auto solutionStoragePtr = _createSolutionStorage(solutions);
    auto solutionStorageVariant = !solutionStoragePtr.isNull() ? QVariant::fromValue(solutionStoragePtr) : QVariant{};

	QVariantList bcs = _gatherBCs(dataProvider);

	if (bcs.empty())
		return false;

    _pyStudyObject.call("writeSolverSetup",
		QVariantList{ vesselForestDataVariant, QVariant::fromValue(SolidDataQtWrapper{ solidModelData }),
                                     QVariant::fromValue(MeshDataQtWrapper{meshData}), QVariant::fromValue(pySolverParametersData->getPythonObject()),
									 bcs, _gatherMaterials(dataProvider), uidToFileNameMap, solutionStorageVariant });

    return !PythonQt::self()->hadError();
}

namespace detail
{
template <typename T>
T getReturnValue(PythonQtObjectPtr& pyObject, gsl::cstring_span<> functionName, const QVariantList& args = QVariantList{})
{
    auto variantResult = pyObject.call(gsl::to_QString(functionName), args);
    if (!variantResult.canConvert<T>()) {
        MITK_ERROR << "SolutionStorage." + gsl::to_string(functionName) + " returned unexpected value.";
        MITK_ERROR << "Type : " << variantResult.typeName();
        throw std::bad_cast();
    }

    return qvariant_cast<T>(variantResult);
}

template <typename T>
struct vtkDataArrayTraits {
};

template <>
struct vtkDataArrayTraits<int32_t> {
    using type = vtkIntArray;
};

template <>
struct vtkDataArrayTraits<double> {
    using type = vtkDoubleArray;
};

template <typename T>
SolutionData::Pointer makeSolutionData(const std::string& name, int nComponents, const QVariantList& componentNames, int nTuples,
                                       const QByteArray& rawDataByteArray)
{
    auto data = vtkSmartPointer<typename vtkDataArrayTraits<T>::type>::New();
    data->SetName(name.c_str());
    data->SetNumberOfComponents(nComponents);
    data->SetNumberOfTuples(nTuples);

    for (int i = 0; i < componentNames.size(); ++i) {
        if (!componentNames[i].canConvert<QString>()) {
            continue;
        }

        data->SetComponentName(i, componentNames[i].toString().toStdString().c_str());
    }

    if (rawDataByteArray.size() != nTuples * nComponents * sizeof(T)) {
        MITK_ERROR << "Data size is not equal to nTuples * nComponents for array '" << name << "'. Skipping";
        return {};
    }

    auto dataPtr = reinterpret_cast<const T*>(rawDataByteArray.data());
    for (int i = 0; i < nTuples * nComponents; ++i) {
        data->SetValue(i, dataPtr[i]);
    }

    return SolutionData::New(data.GetPointer()).GetPointer();
}
} // namespace detail

std::vector<SolutionData::Pointer> PythonSolverStudyData::_loadSolutionStorage(gsl::cstring_span<> loadFunctionName, const QVariantList& args)
{
    auto solutionStoragePtr = PythonQtObjectPtr{_pyStudyObject.call(gsl::to_QString(loadFunctionName), args)};

    if (solutionStoragePtr.isNull()) {
        return {};
    }

    auto result = std::vector<SolutionData::Pointer>{};

    using namespace detail;

    try {
        auto nArrays = getReturnValue<int>(solutionStoragePtr, "getNArrays");
        for (int i = 0; i < nArrays; ++i) {
            auto indexParamList = QVariantList{QVariant::fromValue(i)};
            auto name = getReturnValue<QString>(solutionStoragePtr, "getArrayName", indexParamList);
            auto dataType = getReturnValue<int>(solutionStoragePtr, "getArrayDataType", indexParamList);
            auto nComponents = getReturnValue<int>(solutionStoragePtr, "getArrayNComponents", indexParamList);
            auto componentNames = getReturnValue<QVariantList>(solutionStoragePtr, "getComponentNames", indexParamList);
            auto nTuples = getReturnValue<int>(solutionStoragePtr, "getArrayNTuples", indexParamList);
            auto rawDataByteArray = getReturnValue<QByteArray>(solutionStoragePtr, "getArrayData", indexParamList);

           try {
               auto solutionData = SolutionData::Pointer{};
                switch (static_cast<ArrayDataTypeQtWrapper::ArrayDataType>(dataType)) {
                case ArrayDataTypeQtWrapper::ArrayDataType::Int:
                    solutionData = makeSolutionData<int32_t>(name.toStdString(), nComponents, componentNames, nTuples, rawDataByteArray);
                    break;
                case ArrayDataTypeQtWrapper::ArrayDataType::Double:
                    solutionData = makeSolutionData<double>(name.toStdString(), nComponents, componentNames, nTuples, rawDataByteArray);
                    break;
                default:
                    MITK_ERROR << "Unknown data type " << dataType << " received for array '" << name.toStdString() << "'. Skipping";
                    continue;
                }

                if (solutionData) {
                    result.push_back(solutionData);
                }
            } catch (const std::bad_cast&) {
                MITK_ERROR << "One of the elements of the data array cannot be cast to the corresponding data type for array '" << name.toStdString()
                    << "'. Skipping";
                continue;
            }
        }
    } catch (const std::bad_cast&) {
        // Error message already output by getReturnValue()
    }
    return result;
}

std::vector<SolutionData::Pointer> PythonSolverStudyData::loadSolution()
{
    return _loadSolutionStorage("loadSolution", QVariantList{});
}

std::vector<SolutionData::Pointer> PythonSolverStudyData::computeMaterials(const IDataProvider& dataProvider,
                                                                           const mitk::BaseData* vesselForestData,
                                                                           gsl::not_null<const mitk::BaseData*> solidModelData)
{
    return _loadSolutionStorage(
        "computeMaterials",
        QVariantList{
            _gatherMaterials(dataProvider),
            vesselForestData ? QVariant::fromValue(createVesselForestDataQtWrapper(dataProvider, vesselForestData))
                             : QVariant{},
							 QVariant::fromValue(SolidDataQtWrapper{ dynamic_cast<const SolidData*>(solidModelData.get()) }),
            QVariant::fromValue(
                MeshDataQtWrapper{dynamic_cast<const MeshData*>(dataProvider.findDataByUID(getMeshNodeUID()))}),
        });
}

PythonQtObjectPtr PythonSolverStudyData::getPythonObject() const { return _pyStudyObject; }
}
