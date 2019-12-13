#pragma once

#include <gsl.h>
#include <boost/optional.hpp>

#include <mitkPythonService.h>

#include <QObject>
#include <QSet>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVector>

#include <SolidData.h>
#include <FaceIdentifier.h>
#include <ISolverStudyData.h>
#include <MeshData.h>
#include <VesselForestData.h>
#include <PCMRIData.h>

#include "PythonSolverSetupServiceActivator.h"
#include <PythonSolverSetupManager.h>
#include "PythonSolverSetupServiceExports.h"

namespace crimson
{

class   FaceTypeQtWrapper : public QObject
{
    Q_OBJECT
public:
    Q_ENUMS(FaceType);
    enum FaceType {
        ftCapInflow = FaceIdentifier::ftCapInflow,
        ftCapOutflow = FaceIdentifier::ftCapOutflow,
        ftWall = FaceIdentifier::ftWall,

        ftUndefined = FaceIdentifier::ftUndefined
    };
};

class   ArrayDataTypeQtWrapper : public QObject
{
    Q_OBJECT
public:
    Q_ENUMS(ArrayDataType);
    enum class ArrayDataType {
        Int = 0,
        Double = 1,
    };
};

class   PyFaceIdentifier
{
public:
    static PythonQtObjectPtr cppToPy(const FaceIdentifier& cppFaceId)
    {
        auto pythonService = PythonSolverSetupServiceActivator::getPythonService();

        pythonService->Execute("import CRIMSONCore");

        QVariantList parentSolidIndices;
        for (const auto& parentSolidIndex : cppFaceId.parentSolidIndices) {
            parentSolidIndices << QString::fromStdString(parentSolidIndex);
        }

        auto result = PythonQtObjectPtr{pythonService->GetPythonManager()->mainContext().call(
            "CRIMSONCore.FaceIdentifier.FaceIdentifier",
            QVariantList{static_cast<FaceTypeQtWrapper::FaceType>(cppFaceId.faceType), parentSolidIndices})};
        Ensures(!result.isNull());
        return result;
    }

    static boost::optional<FaceIdentifier> pyToCpp(PythonQtObjectPtr pyFaceId)
    {
        FaceIdentifier cppFaceId;

        QVariant faceIdVariant = pyFaceId.getVariable("faceType");
        if (!faceIdVariant.canConvert<int>()) {
            MITK_ERROR << "Face identifier not recognized - wrong faceType (expected int)";
            return boost::none;
        }
        cppFaceId.faceType = static_cast<FaceIdentifier::FaceType>(faceIdVariant.toInt());

        QVariant parentSolidIndicesVariant = pyFaceId.getVariable("parentSolidIndices");
        if (!parentSolidIndicesVariant.canConvert<QVariantList>()) {
            MITK_ERROR << "Face identifier not recognized - wrong parentSolidIndices (expected list of strings)";
            return boost::none;
        }

        for (const auto& parentSolidIndexVariant : parentSolidIndicesVariant.toList()) {
            if (!parentSolidIndexVariant.canConvert<QString>()) {
                MITK_ERROR << "Face identifier not recognized - wrong item in parentSolidIndices (expected string)";
                return boost::none;
            }

            cppFaceId.parentSolidIndices.insert(parentSolidIndexVariant.toString().toStdString());
        }

        return cppFaceId;
    }
};

class   Utils : public QObject
{
    Q_OBJECT
public slots:
    void static_Utils_reloadAll() { crimson::PythonSolverSetupServiceActivator::reloadPythonSolverSetups(true); }

    void static_Utils_logError(QString message) { MITK_ERROR << message.toStdString(); }

    void static_Utils_logWarning(QString message) { MITK_WARN << message.toStdString(); }

    void static_Utils_logInformation(QString message) { MITK_INFO << message.toStdString(); }
};

class   SolidDataQtWrapper : public QObject
{
    Q_OBJECT
public:
	SolidDataQtWrapper()
        : _brep(nullptr)
    {
    }
	SolidDataQtWrapper(gsl::not_null<const SolidData*> brep)
        : _brep(brep)
    {
    }

	SolidDataQtWrapper(const SolidDataQtWrapper& other)
        : _brep(other._brep)
    {
    }

public slots:

    QVector<double> getFaceNormal(PythonQtObjectPtr pyFaceIdentifier) const
    {
        Expects(_brep != nullptr);
        auto cppFaceIdOptional = PyFaceIdentifier::pyToCpp(pyFaceIdentifier);

        if (!cppFaceIdOptional) {
            return {};
        }

        auto normal = _brep->getFaceNormal(cppFaceIdOptional.get());
        return QVector<double>{normal[0], normal[1], normal[2]};
    }

    double getDistanceToFaceEdge(PythonQtObjectPtr pyFaceIdentifier, double x, double y, double z) const
    {
        Expects(_brep != nullptr);
        auto cppFaceIdOptional = PyFaceIdentifier::pyToCpp(pyFaceIdentifier);

        if (!cppFaceIdOptional) {
            return 0;
        }

        auto pos = mitk::Point3D{};
        pos[0] = x;
        pos[1] = y;
        pos[2] = z;
        return _brep->getDistanceToFaceEdge(cppFaceIdOptional.get(), pos);
    }

    PythonQtObjectPtr getFaceIdentifierForModelFace(int faceIndex) const
    {
        Expects(_brep != nullptr);
        auto identifierOpt = _brep->getFaceIdentifierMap().getFaceIdentifierForModelFace(faceIndex);
        return identifierOpt ? PyFaceIdentifier::cppToPy(identifierOpt.get()) : nullptr;
    }

    int getNumberOfModelFaces() const
    {
        Expects(_brep != nullptr);
        return _brep->getFaceIdentifierMap().getNumberOfModelFaces();
    }

    int getNumberOfFaceIdentifiers() const
    {
        Expects(_brep != nullptr);
        return _brep->getFaceIdentifierMap().getNumberOfFaceIdentifiers();
    }

    int faceIdentifierIndex(PythonQtObjectPtr pyFaceIdentifier) const
    {
        Expects(_brep != nullptr);

        auto cppFaceIdOptional = PyFaceIdentifier::pyToCpp(pyFaceIdentifier);

        if (!cppFaceIdOptional) {
            return 0;
        }

        return _brep->getFaceIdentifierMap().faceIdentifierIndex(cppFaceIdOptional.get());
    }

    PythonQtObjectPtr getFaceIdentifier(int uniqueFaceId) const
    {
        Expects(_brep != nullptr);
        return PyFaceIdentifier::cppToPy(_brep->getFaceIdentifierMap().getFaceIdentifier(uniqueFaceId));
    }

    QVector<int> getModelFacesForFaceIdentifier(PythonQtObjectPtr pyFaceIdentifier) const
    {
        Expects(_brep != nullptr);
        auto cppFaceIdOptional = PyFaceIdentifier::pyToCpp(pyFaceIdentifier);

        if (!cppFaceIdOptional) {
            return {};
        }

        return QVector<int>::fromStdVector(_brep->getFaceIdentifierMap().getModelFacesForFaceIdentifier(cppFaceIdOptional.get()));
    }

private:
	const SolidData* _brep;
};

class   MeshDataQtWrapper : public QObject
{
    Q_OBJECT
public:
    MeshDataQtWrapper()
        : _mesh(nullptr)
    {
    }
    MeshDataQtWrapper(gsl::not_null<const MeshData*> mesh)
        : _mesh(mesh)
    {
    }

    MeshDataQtWrapper(const MeshDataQtWrapper& other)
        : _mesh(other._mesh)
    {
    }

public slots:

    int getNNodes() const
    {
        Expects(_mesh != nullptr);
        return _mesh->getNNodes();
    }

    int getNEdges() const
    {
        Expects(_mesh != nullptr);
        return _mesh->getNEdges();
    }

    int getNFaces() const
    {
        Expects(_mesh != nullptr);
        return _mesh->getNFaces();
    }

    int getNElements() const
    {
        Expects(_mesh != nullptr);
        return _mesh->getNElements();
    }

    QVector<double> getNodeCoordinates(int nodeIndex) const
    {
        Expects(_mesh != nullptr);
        auto coords = _mesh->getNodeCoordinates(nodeIndex);
        return QVector<double>{coords[0], coords[1], coords[2]};
    }

    QVector<int> getElementNodeIds(int elementIndex) const
    {
        Expects(_mesh != nullptr);
        return QVector<int>::fromStdVector(_mesh->getElementNodeIds(elementIndex));
    }

    QVector<int> getAdjacentElements(int elementIndex) const
    {
        Expects(_mesh != nullptr);
        return QVector<int>::fromStdVector(_mesh->getAdjacentElements(elementIndex));
    }

    QVector<int> getNodeIdsForFace(PythonQtObjectPtr pyFaceIdentifier) const
    {
        Expects(_mesh != nullptr);
        auto cppFaceIdOptional = PyFaceIdentifier::pyToCpp(pyFaceIdentifier);

        if (!cppFaceIdOptional) {
            return {};
        }

        return QVector<int>::fromStdVector(_mesh->getNodeIdsForFace(cppFaceIdOptional.get()));
    }

    QVariantList getMeshFaceInfoForFace(PythonQtObjectPtr pyFaceIdentifier) const // TODO: QVector<QVector<int>>
    {
        Expects(_mesh != nullptr);
        auto cppFaceIdOptional = PyFaceIdentifier::pyToCpp(pyFaceIdentifier);

        if (!cppFaceIdOptional) {
            return {};
        }

        auto faceInfos = _mesh->getMeshFaceInfoForFace(cppFaceIdOptional.get());

        auto out = QVariantList{};
        out.reserve(faceInfos.size());
        std::transform(
            faceInfos.begin(), faceInfos.end(), std::back_inserter(out),
            [](const crimson::MeshData::MeshFaceInfo& info) {
                return QVariant::fromValue(QVector<int>{info.elementId, info.globalFaceId, info.nodeIds[0], info.nodeIds[1], info.nodeIds[2]});
            });
        return out;
    }

private:
    const MeshData* _mesh;
};

class   VesselForestDataQtWrapper : public QObject
{
    Q_OBJECT
public:
    using UIDToVesselPathDataMap = std::unordered_map<std::string, const VesselPathAbstractData*>;

    VesselForestDataQtWrapper()
        : _vesselForest(nullptr)
    {
    }
    VesselForestDataQtWrapper(gsl::not_null<const VesselForestData*> data, UIDToVesselPathDataMap uidToVesselPathDataMap)
        : _vesselForest(data)
        , _uidToVesselPathDataMap(std::move(uidToVesselPathDataMap))
    {
    }

    VesselForestDataQtWrapper(const VesselForestDataQtWrapper& other)
        : _vesselForest(other._vesselForest)
        , _uidToVesselPathDataMap(other._uidToVesselPathDataMap)
    {
    }
//    VesselForestDataQtWrapper(const VesselForestDataQtWrapper&) = default;

    const UIDToVesselPathDataMap& getUIDToVesselPathDataMap() const { return _uidToVesselPathDataMap; }

public slots:
    QVariantMap getActiveConnectedComponentsMap() const
    {
        Expects(_vesselForest != nullptr);

        auto result = QVariantMap{};
        for (const auto& uidComponentIndexPair : _vesselForest->computeActiveConnectedComponents()) {
            result[QString::fromStdString(uidComponentIndexPair.first)] = uidComponentIndexPair.second;
        }

        return result;
    }

    QVariantList getClosestPoint(PythonQtObjectPtr pyFaceIdentifier, double x, double y, double z) const
    {
        Expects(_vesselForest != nullptr);

        auto closestPathInfo = _getClosestVesselPath(pyFaceIdentifier, x, y, z);
        if (!closestPathInfo.vesselPath) {
            return {};
        }

        return QVariantList{closestPathInfo.distance, closestPathInfo.t};
    }

    QVector<double> getVesselPathCoordinateFrame(PythonQtObjectPtr pyFaceIdentifier, double x, double y, double z) const
    {
        auto closestPathInfo = _getClosestVesselPath(pyFaceIdentifier, x, y, z);
        if (!closestPathInfo.vesselPath) {
            return{};
        }

        auto pt = closestPathInfo.vesselPath->getPosition(closestPathInfo.t);
        auto tangent = closestPathInfo.vesselPath->getTangentVector(closestPathInfo.t);
        auto normal = closestPathInfo.vesselPath->getNormalVector(closestPathInfo.t);

        return QVector<double>{pt[0], pt[1], pt[2], tangent[0], tangent[1], tangent[2], normal[0], normal[1], normal[2]};
    }

private:
    struct ClosestVesselPathInfo {
        const VesselPathAbstractData* vesselPath = nullptr;
        double distance = 0;
        double t = 0;
    };

    ClosestVesselPathInfo _getClosestVesselPath(PythonQtObjectPtr pyFaceIdentifier, double x, double y, double z) const
    {
        auto cppFaceIdOptional = PyFaceIdentifier::pyToCpp(pyFaceIdentifier);

        if (!cppFaceIdOptional) {
            return {};
        }

        auto&& parentSolidIndices = cppFaceIdOptional->parentSolidIndices;

        auto result = ClosestVesselPathInfo{};

        auto pos = mitk::Point3D{};
        mitk::FillVector3D(pos, x, y, z);
        for (const auto& vesselUID : parentSolidIndices) {
            auto closestPointRequestResult = _uidToVesselPathDataMap.at(vesselUID)->getClosestPoint(pos);

            auto d = closestPointRequestResult.closestPoint.EuclideanDistanceTo(pos);
            if (result.vesselPath == nullptr || d < result.distance) {
                result.vesselPath = _uidToVesselPathDataMap.at(vesselUID);
                result.t = closestPointRequestResult.t;
                result.distance = d;
            }
        }

        return result;
    }


    const VesselForestData* _vesselForest;
    UIDToVesselPathDataMap _uidToVesselPathDataMap;
};

class   PCMRIDataQtWrapper : public QObject
{
	Q_OBJECT
public:
	PCMRIDataQtWrapper()
		: _pcmriData(nullptr)
	{
	}
	PCMRIDataQtWrapper(gsl::not_null<const PCMRIData*> pcmriData)
		: _pcmriData(pcmriData)
	{
	}

	PCMRIDataQtWrapper(const PCMRIDataQtWrapper& other)
		: _pcmriData(other._pcmriData)
	{
	}

public slots:

	QVector<double> getTimepoints() const
	{
		Expects(_pcmriData != nullptr);
		return QVector<double>::fromStdVector(_pcmriData->getTimepoints());
	}

	QVector<double> getFlowWaveform() const
	{
		Expects(_pcmriData != nullptr);
		return QVector<double>::fromStdVector(_pcmriData->getFlowWaveform());
	}

	QVector<double> getSingleMappedPCMRIvector(int pointIndex, int timepointIndex) const
	{
		Expects(_pcmriData != nullptr);
		auto coords = _pcmriData->getSingleMappedPCMRIvector(pointIndex, timepointIndex);
		return QVector<double>{coords[0], coords[1], coords[2]};
	}

private:
	const PCMRIData* _pcmriData;
};
}

Q_DECLARE_METATYPE(crimson::SolidDataQtWrapper)
Q_DECLARE_METATYPE(crimson::MeshDataQtWrapper)
Q_DECLARE_METATYPE(crimson::VesselForestDataQtWrapper)
Q_DECLARE_METATYPE(crimson::PCMRIDataQtWrapper)
