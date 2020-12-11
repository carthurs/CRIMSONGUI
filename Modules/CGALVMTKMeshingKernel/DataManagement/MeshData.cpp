#include "MeshData.h"

#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkIdList.h>
#include <vtkDoubleArray.h>
#include <vtkShortArray.h>
#include <vtkPolyDataNormals.h>
#include <vtkUnstructuredGrid.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkExtractCells.h>
#include <vtkGeometryFilter.h>
#include <vtkMeshQuality.h>
#include <vtkTriangle.h>

#include <Wm5Vector3.h>

#include <boost/container/static_vector.hpp>

#include <unordered_map>
#include <unordered_set>
#include <numeric>

namespace crimson
{

struct TriangleFace {
    TriangleFace(const std::array<int, 3>& ids)
    : _ids(ids) {}

    bool operator<(const TriangleFace& rhs) const { 
        auto sorted = [](std::array<int, 3> ids) {
            std::sort(ids.begin(), ids.end());
            return ids;
        };
        return sorted(_ids) < sorted(rhs._ids); 
    }

    std::array<int, 3> _ids;
};

MeshData::MeshData() { MeshData::InitializeTimeGeometry(1); }

MeshData::MeshData(const Self& other)
    : mitk::BaseData(other)
{
    MeshData::InitializeTimeGeometry(1);

    if (other._unstructuredGridRepresentation) {
        this->_unstructuredGridRepresentation = other._unstructuredGridRepresentation->Clone();
        this->_firstTriangleCellId = other._firstTriangleCellId;
        this->_nFaces = other._nFaces;
        this->_nEdges = other._nEdges;
    }
    if (other._surfaceRepresentation) {
        this->_surfaceRepresentation = other._surfaceRepresentation->Clone();
    }
}

MeshData::~MeshData() {}

void MeshData::UpdateOutputInformation()
{
    if (this->GetSource()) {
        this->GetSource()->UpdateOutputInformation();
    }
}

void MeshData::PrintSelf(std::ostream& os, itk::Indent indent) const { mitk::BaseData::PrintSelf(os, indent); }

mitk::Surface::Pointer MeshData::getSurfaceRepresentation() const
{
    if (!_surfaceRepresentation) {
        vtkUnstructuredGridBase* ug = getUnstructuredGridRepresentation()->GetVtkUnstructuredGrid();

        vtkNew<vtkExtractCells> extractCellsFilter;
        extractCellsFilter->SetInputData(ug);
        extractCellsFilter->AddCellRange(_firstTriangleCellId, ug->GetNumberOfCells());

        vtkNew<vtkGeometryFilter> geometryFilter;
        geometryFilter->SetInputConnection(extractCellsFilter->GetOutputPort());
        geometryFilter->Update();

        _surfaceRepresentation = mitk::Surface::New();
        _surfaceRepresentation->SetVtkPolyData(geometryFilter->GetOutput());
    }

    return _surfaceRepresentation;
}

mitk::Surface::Pointer MeshData::getSurfaceRepresentationForFace(crimson::FaceIdentifier faceId) const
{
    vtkUnstructuredGridBase* ug = getUnstructuredGridRepresentation()->GetVtkUnstructuredGrid();

    vtkDataArray* array = ug->GetCellData()->GetArray("Face IDs");

    std::vector<int> nodeIds = getNodeIdsForFace(faceId);

    std::vector<vtkIdType> cellIds;

    int faceIdentifierIndex = _faceIdentifierMap.faceIdentifierIndex(faceId);
    for (int i = _firstTriangleCellId; i < array->GetNumberOfTuples(); ++i) {
        if (faceIdentifierIndex == array->GetTuple1(i)) {
            cellIds.push_back(i);

            vtkSmartPointer<vtkCell> cell = ug->GetCell(i);
            for (int j = 0; j < cell->GetNumberOfPoints(); ++j) {
                if (boost::find(nodeIds, cell->GetPointId(j)) == nodeIds.end()) {
                    nodeIds.push_back(cell->GetPointId(j));
                }
            }
        }
    }

    vtkNew<vtkPolyData> pd;
    vtkNew<vtkPoints> points;

    for (int id : nodeIds) {
        points->InsertNextPoint(ug->GetPoint(id));
    }

    pd->SetPoints(points.Get());
    pd->Allocate();

    for (vtkIdType cellId : cellIds) {
        vtkSmartPointer<vtkCell> cell = ug->GetCell(cellId);
        vtkIdType pointIds[3];
        for (int j = 0; j < cell->GetNumberOfPoints(); ++j) {
            auto it = boost::find(nodeIds, cell->GetPointId(j));
            pointIds[j] = it - nodeIds.begin();
        }
        pd->InsertNextCell(VTK_TRIANGLE, 3, pointIds);
    }

    auto result = mitk::Surface::New();
    result->SetVtkPolyData(pd.Get());

	return result;
}

mitk::UnstructuredGrid::Pointer MeshData::getUnstructuredGridRepresentation() const
{
    return _unstructuredGridRepresentation;
}


void MeshData::setUnstructuredGrid(mitk::UnstructuredGrid::Pointer data, bool fixOrdering) {
    _unstructuredGridRepresentation = data;

    // All tetrahedra come in the list of cells first. Find the partition point.
    vtkUnstructuredGridBase* ug = data->GetVtkUnstructuredGrid();
    for (int i = 0; i < ug->GetNumberOfCells(); ++i) {
        if (ug->GetCellType(i) != VTK_TETRA) {
            _firstTriangleCellId = i;
            break;
        }
    }

    if (!ug->GetCellData()->GetArray("originalFaceIds")) {
        vtkNew<vtkIntArray> originalFaceIds;
        originalFaceIds->SetName("originalFaceIds");
        originalFaceIds->SetNumberOfTuples(ug->GetNumberOfCells());
        originalFaceIds->FillComponent(0, -1);

        int globalFaceId = 0;
        for (int i = _firstTriangleCellId; i < ug->GetNumberOfCells(); ++i) {
            vtkCell* cell = ug->GetCell(i);
            originalFaceIds->SetTuple1(i, globalFaceId++);
        }

        ug->GetCellData()->AddArray(originalFaceIds.GetPointer());
    }

    if (fixOrdering) {
        // Fix node ordering of faces
        for (int i = _firstTriangleCellId; i < ug->GetNumberOfCells(); ++i) {
            // Get the tetrahedron this face belongs to
            vtkSmartPointer<vtkCell> faceCell = ug->GetCell(i);
            vtkNew<vtkIdList> neighbors;
            ug->GetCellNeighbors(i, faceCell->GetPointIds(), neighbors.GetPointer());

            if (neighbors->GetNumberOfIds() == 0) {
                break; // No volume mesh
            }

            vtkSmartPointer<vtkCell> tetCell = ug->GetCell(neighbors->GetId(0));

            Wm5::Vector3d facePoints[3];
            Wm5::Vector3d tipPoint;

            boost::container::static_vector<vtkIdType, 3> facePointIds;
            for (int j = 0; j < 3; ++j) {
                facePointIds.push_back(faceCell->GetPointId(j));
                ug->GetPoint(facePointIds.back(), facePoints[j]);
            }

            for (int j = 0; j < 4; ++j) {
                vtkIdType id = tetCell->GetPointId(j);
                if (boost::find(facePointIds, id) == facePointIds.end()) {
                    ug->GetPoint(id, tipPoint);
                    break;
                }
            }

            if ((facePoints[1] - facePoints[0]).Cross(facePoints[2] - facePoints[0]).Dot(tipPoint - facePoints[0]) > 0) {
                // The normal would be pointing inside the tetrahedron. Fix this by swapping two indices
                std::swap(facePointIds[1], facePointIds[2]);
                ug->ReplaceCell(i, 3, facePointIds.data());
            }
        }

        // Fix ordering of nodes for tetrahedra
        for (int i = 0; i < _firstTriangleCellId; ++i) {
            vtkSmartPointer<vtkCell> tet = ug->GetCell(i);

            Wm5::Vector3d v[4];
            for (int j = 0; j < 4; ++j) {
                tet->GetPoints()->GetPoint(j, v[j]);
            }

            if (((v[1] - v[0]).Cross(v[2] - v[0])).Dot(v[3] - v[0]) > 0) {
                vtkIdType ids[4] = {tet->GetPointId(0), tet->GetPointId(2), tet->GetPointId(1), tet->GetPointId(3)};
                ug->ReplaceCell(i, 4, ids);
            }
        }
    }

    std::set<std::pair<int, int>> edges;
    std::set<TriangleFace> faces;

    for (int i = 0; i < _firstTriangleCellId; ++i) {
        vtkSmartPointer<vtkCell> tet = ug->GetCell(i);
        for (int edgeId = 0; edgeId < tet->GetNumberOfEdges(); ++edgeId) {
            vtkSmartPointer<vtkCell> edge = tet->GetEdge(edgeId);
            std::pair<int, int> vertIds(edge->GetPointId(0), edge->GetPointId(1));
            if (vertIds.first > vertIds.second) {
                std::swap(vertIds.first, vertIds.second);
            }
            edges.insert(vertIds);
        }

        for (int faceId = 0; faceId < tet->GetNumberOfFaces(); ++faceId) {
            vtkSmartPointer<vtkCell> face = tet->GetFace(faceId);
            faces.insert(TriangleFace{{face->GetPointId(0), face->GetPointId(1), face->GetPointId(2)}});
        }
    }

    _nFaces = faces.size();
    _nEdges = edges.size();
}

std::vector<int> MeshData::getBoundaryNodeIdsForFace(const FaceIdentifier& faceId) const
{
    vtkUnstructuredGridBase* ug = getUnstructuredGridRepresentation()->GetVtkUnstructuredGrid();
    vtkDataArray* array = ug->GetCellData()->GetArray("Face IDs");

    int faceIdentifierIndex = _faceIdentifierMap.faceIdentifierIndex(faceId);
    std::unordered_map<int, std::unordered_set<int>> nodeIndicesAndNeightbors;

    vtkNew<vtkIdList> singlePointIdList;
    singlePointIdList->SetNumberOfIds(1);
    vtkNew<vtkIdList> pointNeighbors;
    for (int cellId = _firstTriangleCellId; cellId < array->GetNumberOfTuples(); ++cellId) {
        if (faceIdentifierIndex == array->GetTuple1(cellId)) {
            vtkSmartPointer<vtkIdList> pointIds = ug->GetCell(cellId)->GetPointIds();

            std::vector<int> edgePoints;
            for (int cellPointId = 0; cellPointId < pointIds->GetNumberOfIds(); ++cellPointId) {
                int ptId = pointIds->GetId(cellPointId);
                singlePointIdList->SetId(0, ptId);
                ug->GetCellNeighbors(cellId, singlePointIdList.Get(), pointNeighbors.Get());

                auto neighIdRng = boost::make_iterator_range_n(pointNeighbors->GetPointer(0), pointNeighbors->GetNumberOfIds());
                if (boost::find_if(
                        neighIdRng, 
                        [&](vtkIdType neighId) {
                            return ug->GetCellType(neighId) == VTK_TRIANGLE && array->GetTuple1(neighId) != faceIdentifierIndex;
                        }
                    ) != neighIdRng.end()) {
                    edgePoints.push_back(ptId);
                }
            }

            for (int edgePointId : edgePoints) {
                for (int otherEdgePointId : edgePoints) {
                    if (edgePointId != otherEdgePointId) {
                        nodeIndicesAndNeightbors[edgePointId].insert(otherEdgePointId);
                    }
                }
            }
        }
    }

    if (nodeIndicesAndNeightbors.empty()) {
        return {};
    }

    // Now order the points... Hello PCMRI
    std::vector<int> orderedNodeIds; 
    orderedNodeIds.push_back(nodeIndicesAndNeightbors.begin()->first);
    int prevNodeId = -1;

    while (orderedNodeIds.size() < nodeIndicesAndNeightbors.size()) {
        for (int neighId : nodeIndicesAndNeightbors[orderedNodeIds.back()]) {
            if (neighId == prevNodeId) {
                continue;
            }
            prevNodeId = orderedNodeIds.back();
            orderedNodeIds.push_back(neighId);
            break;
        }
    }

    return orderedNodeIds;
}

std::vector<int> MeshData::getNodeIdsForFace(const FaceIdentifier& faceId) const
{
    vtkUnstructuredGridBase* ug = getUnstructuredGridRepresentation()->GetVtkUnstructuredGrid();
    vtkDataArray* array = ug->GetCellData()->GetArray("Face IDs");

    // index of face Id to find (the one that was passed in as a parameter)
    int faceIdentifierIndex = _faceIdentifierMap.faceIdentifierIndex(faceId);

    // where item nodeIndicies[nodeIndex] is present iff (???)
    std::unordered_set<int> nodeIndices;

    vtkNew<vtkIdList> singlePointIdList;
    singlePointIdList->SetNumberOfIds(1);
    vtkNew<vtkIdList> pointNeighbors;

    // for each faceId
    for (int cellId = _firstTriangleCellId; cellId < array->GetNumberOfTuples(); ++cellId) {
        // if this is the face we want to find
        if (faceIdentifierIndex == array->GetTuple1(cellId)) {
            vtkIdList* pointIds = ug->GetCell(cellId)->GetPointIds();

            boost::copy
            (
                // source data (compound filter):
                // start with the first point pointed to by the pointer and go to the last pointID
                boost::make_iterator_range_n(pointIds->GetPointer(0), pointIds->GetNumberOfIds()) |
                // use only pointIDs that aren't already in nodeIndicies
                boost::adaptors::filtered([&](int ptId) { return nodeIndices.count(ptId) == 0; }) |

                // ???
                boost::adaptors::filtered
                (
                    [&](int ptId) 
                    {
                        // seems like this could have been a special case outside of 
                        // the copy functions to save on processing time / improve readability
                        if (faceId.faceType == FaceIdentifier::ftWall) {
                            return true;
                        }

                        singlePointIdList->SetId(0, ptId);

                        // I think this is finding all cells that are adjacent to the current one
                        ug->GetCellNeighbors(cellId, singlePointIdList.Get(), pointNeighbors.Get());
                        auto neighIdRng = boost::make_iterator_range_n(pointNeighbors->GetPointer(0), pointNeighbors->GetNumberOfIds());

                        // return whether the first matching entry is the end()
                        return boost::find_if
                        (
                            // return the first neighboring entry that is a triangle and a wall
                            neighIdRng, 
                            [&](vtkIdType neighId) 
                            {
                                return  ug->GetCellType(neighId) == VTK_TRIANGLE && 
                                        _faceIdentifierMap.getFaceIdentifier(array->GetTuple1(neighId)).faceType == FaceIdentifier::ftWall;
                            }
                        ) == neighIdRng.end();
                    }
                ),
                // destination data
                std::inserter(nodeIndices, nodeIndices.end())
            );
        }
    }

    return {nodeIndices.begin(), nodeIndices.end()};
}

auto MeshData::getMeshFaceInfoForFace(const FaceIdentifier& faceId) const -> std::vector<MeshFaceInfo>
{
    std::vector<MeshFaceInfo> out;

    vtkUnstructuredGridBase* ug = getUnstructuredGridRepresentation()->GetVtkUnstructuredGrid();
    vtkDataArray* faceIdArray = ug->GetCellData()->GetArray("Face IDs");
    vtkDataArray* globalFaceIdArray = ug->GetCellData()->GetArray("originalFaceIds");

    int faceIdentifierIndex = _faceIdentifierMap.faceIdentifierIndex(faceId);
    for (int cellId = _firstTriangleCellId; cellId < ug->GetNumberOfCells(); ++cellId) {
        if (faceIdentifierIndex == faceIdArray->GetTuple1(cellId)) {
            MeshFaceInfo meshFaceInfo;
            meshFaceInfo.globalFaceId = globalFaceIdArray->GetTuple1(cellId);

            vtkSmartPointer<vtkCell> cell = ug->GetCell(cellId);

            vtkNew<vtkIdList> idList;
            idList->InsertNextId(cell->GetPointId(0));
            idList->InsertNextId(cell->GetPointId(1));
            idList->InsertNextId(cell->GetPointId(2));
            boost::copy(boost::make_iterator_range_n(idList->GetPointer(0), 3), meshFaceInfo.nodeIds);

            //get the neighbors of the cell
            vtkNew<vtkIdList> neighborCellIds;

            ug->GetCellNeighbors(cellId, idList.Get(), neighborCellIds.Get());
            if (neighborCellIds->GetNumberOfIds() > 0) {
                meshFaceInfo.elementId = neighborCellIds->GetId(0);
            }

            out.push_back(meshFaceInfo);
        }
    }

    return out;
}

std::vector<int> MeshData::getAdjacentElements(int elementIndex) const
{
    vtkUnstructuredGridBase* ug = getUnstructuredGridRepresentation()->GetVtkUnstructuredGrid();
    vtkCell* cell = ug->GetCell(elementIndex);

    std::vector<int> adjacentElements;
    for (int face = 0; face < cell->GetNumberOfFaces(); ++face) {
        vtkSmartPointer<vtkCell> faceCell = cell->GetFace(face);

        vtkNew<vtkIdList> idList;
        idList->InsertNextId(faceCell->GetPointId(0));
        idList->InsertNextId(faceCell->GetPointId(1));
        idList->InsertNextId(faceCell->GetPointId(2));

        //get the neighbors of the cell
        vtkNew<vtkIdList> neighborCellIds;

        ug->GetCellNeighbors(elementIndex, idList.Get(), neighborCellIds.Get());
        for (int i = 0; i < neighborCellIds->GetNumberOfIds(); ++i) {
            if (ug->GetCellType(neighborCellIds->GetId(i)) == VTK_TETRA) {
                adjacentElements.push_back(neighborCellIds->GetId(i));
            }
        }
    }

    return adjacentElements;
}

int MeshData::getNNodes() const { return _unstructuredGridRepresentation->GetVtkUnstructuredGrid()->GetNumberOfPoints(); }

int MeshData::getNEdges() const { return _nEdges; }

int MeshData::getNFaces() const { return _nFaces; }

int MeshData::getNElements() const { return _firstTriangleCellId; }

std::string MeshData::getDataNodeName() const
{
	std::string nodeName;
	GetPropertyList()->GetStringProperty("name", nodeName);
	return nodeName;
}

mitk::Point3D MeshData::getNodeCoordinates(int nodeIndex) const
{
    return getUnstructuredGridRepresentation()->GetVtkUnstructuredGrid()->GetPoint(nodeIndex);
}

std::vector<int> MeshData::getElementNodeIds(int elementIndex) const
{
    vtkCell* cell = getUnstructuredGridRepresentation()->GetVtkUnstructuredGrid()->GetCell(elementIndex);
    vtkIdList* ids = cell->GetPointIds();

    return {ids->GetPointer(0), ids->GetPointer(ids->GetNumberOfIds())};
}

std::vector<double> MeshData::getElementAspectRatios()
{
    vtkNew<vtkMeshQuality> qualFilter;
    qualFilter->SetTetQualityMeasureToAspectRatio();
    qualFilter->SetInputData(_unstructuredGridRepresentation->GetVtkUnstructuredGrid());
    qualFilter->Update();

    vtkDataArray* tetQualArray = qualFilter->GetOutput()->GetCellData()->GetArray("Quality");

    std::vector<double> result(_firstTriangleCellId);
    for (int i = 0; i < result.size(); ++i) {
        result[i] = tetQualArray->GetTuple1(i);
    }

    return result;
}

vtkPointData* MeshData::getPointData() const
{
    this->Modified();
    return getUnstructuredGridRepresentation()->GetVtkUnstructuredGrid()->GetPointData();
}

double MeshData::calculateArea(const FaceIdentifier& faceId)
{
    vtkIdType faceIdIndex = _faceIdentifierMap.faceIdentifierIndex(faceId);
    vtkUnstructuredGridBase* ug = getUnstructuredGridRepresentation()->GetVtkUnstructuredGrid();
    vtkDataArray* faceIdArray = ug->GetCellData()->GetArray("Face IDs");

    double area = 0;
    for (vtkIdType i = _firstTriangleCellId; i < ug->GetNumberOfCells(); ++i) {
        if (faceIdArray->GetTuple1(i) == faceIdIndex) {
            vtkSmartPointer<vtkCell> cell = ug->GetCell(i);
            area += static_cast<vtkTriangle*>(cell.Get())->ComputeArea();
        }
    }
    return area;
}

} // namespace crimson
