#include "DiscreteSolidData.h"

#include <vtkIdList.h>
#include <vtkPolyData.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkNew.h>
#include <vtkShortArray.h>
#include <vtkCellData.h>
#include <vtkPolyDataNormals.h>
#include <vtkGetBoundaryFaces.h>
#include <vtkMassProperties.h>
#include <vtkTriangleFilter.h>
#include <vtkThreshold.h>
#include <vtkCellLocator.h>
#include <vtkFeatureEdges.h>
#include <vtkGeometryFilter.h>
#include <vtkCleanPolyData.h>

#include <vtkPolygon.h>
#include <Wm5ApprPlaneFit3.h>

#include <unordered_map>

namespace crimson
{

	DiscreteSolidData::DiscreteSolidData()
	{
	}

	DiscreteSolidData::DiscreteSolidData(const Self& other)
		: SolidData(other)
        , _edges(other._edges)
	{
	}

	DiscreteSolidData::~DiscreteSolidData() {}

    // where surface seems to be a lofted surface
    void DiscreteSolidData::fromSurface(mitk::Surface::Pointer surface) {
        // Triangulate first
        // vtkTriangleFilter "convert[s] input polygons and strips to triangles"
        //
        // I think PassVerts and PassLines is off by default, so the results of this filter will
        // only contain triangulated polygons and triangle strips
        vtkNew<vtkTriangleFilter> triangleFilter;
        triangleFilter->SetInputData(surface->GetVtkPolyData());
        triangleFilter->Update();
        surface->SetVtkPolyData(triangleFilter->GetOutput());

        // Handle face ids
        vtkDataArray* faceIdArray = surface->GetVtkPolyData()->GetCellData()->GetArray("Face IDs");

        if (!faceIdArray) {
            // Find separate faces using a feature angle and assign face IDs accordingly
            vtkNew<vtkGetBoundaryFaces> faceFilter;
            faceFilter->SetInputData(surface->GetVtkPolyData());
            faceFilter->SetFeatureAngle(45);
            faceFilter->Update();

            surface->SetVtkPolyData(faceFilter->GetOutput());
            faceIdArray = surface->GetVtkPolyData()->GetCellData()->GetArray("Regions");
            faceIdArray->SetName("Face IDs");
        } else if (faceIdArray->GetDataType() != VTK_INT || faceIdArray->GetNumberOfComponents() != 1) {
            // If face ids already exist - ensure they are in a form we expect
            vtkNew<vtkIntArray> faceIdIntArray;
            faceIdIntArray->SetNumberOfComponents(1);
            faceIdIntArray->SetNumberOfTuples(faceIdArray->GetNumberOfTuples());
            faceIdIntArray->SetName("Face IDs");

            for (int i = 0; i < faceIdArray->GetNumberOfTuples(); ++i) {
                faceIdIntArray->SetTuple1(i, faceIdArray->GetTuple1(i));
            }

            surface->GetVtkPolyData()->GetCellData()->RemoveArray("Face IDs");
            surface->GetVtkPolyData()->GetCellData()->AddArray(faceIdIntArray.GetPointer());

            faceIdArray = faceIdIntArray.GetPointer();
        }
        surface->GetVtkPolyData()->GetCellData()->SetActiveScalars("Face IDs");

        // Ensure face id array starts at 0
        int* range = static_cast<vtkIntArray*>(faceIdArray)->GetValueRange();
        for (int i = 0; i < faceIdArray->GetNumberOfTuples(); ++i) {
            faceIdArray->SetTuple1(i, faceIdArray->GetTuple1(i) - range[0]);
        }

        // Find planar faces

        // [AJM]    sorry, even if it deviates from the established style I had to split this up with newlines 
        //          to be able to read this, it just looked like (({{{[[<(({{{[<[(({{ to me
        //
        // be careful with vector<...> (std::vector, a collection of something) vs Wm5::Vector3d (one 3d point)

        auto coordsToVector = [](double* c) {
            return Wm5::Vector3d(c[0], c[1], c[2]);
        };

        // where points[cellId] = the points in that cell
        // I think the cell is... all of the polygons on the surface
        std::unordered_map<int, std::vector<Wm5::Vector3d>> points;

        for (int cellId = 0; cellId < surface->GetVtkPolyData()->GetNumberOfCells(); ++cellId) {
            // where the meaning of a "cell" is not well documented
            // "vtkPolyData represents a geometric structure consisting of vertices, lines, polygons, and/or triangle strips."
            // I am pretty sure a 'cell' here refers to a polygon, one entry in the loft's surface
            vtkSmartPointer<vtkCell> cell = surface->GetVtkPolyData()->GetCell(cellId);

            // ptID is more of a point index from the looks of it, not a "point ID".
            for (int ptId = 0; ptId < cell->GetNumberOfPoints(); ++ptId) {
                points[faceIdArray->GetTuple1(cellId)].push_back(
                    // where the vector returned is a straight conversion of a raw pointer of 3 doubles to a Wm5 3d point
                    coordsToVector(
                        // where the return value of GetPoint is literally a raw pointer to 3 doubles
                        surface->GetVtkPolyData()->GetPoints()->GetPoint(
                            // "for cell point i, return the actual point ID"
                            // so GetPointId returns the actual point ID that GetPoint needs
                            cell->GetPointId(ptId)
                        )
                    )
                );
            }
        }

        // where isFlat[cellId] = (whether all points of the cell are close enough to the surface of the plane)
        std::unordered_map<int, bool> isFlat;
        boost::transform
        (
            // iterate over points and insert the result of the lambda into the isFlat map
            points, 
            std::inserter(isFlat, isFlat.end() ), 

            []
                (
                    // map<int, vector<3d points>>::value_type
                    // reduces to pair<const key_type,mapped_type>,
                    // in this case pair<int, std::vector<Wm5::Vector3d>>
                    // where int is a cell Id and points is the list of points in that cell
                    const std::unordered_map<
                                                int, 
                                                std::vector<Wm5::Vector3d>
                                            >::value_type& idPtsPair
                ) 
            {
                const std::vector<Wm5::Vector3d>& pts = idPtsPair.second;
                Wm5::Plane3d plane = Wm5::OrthogonalPlaneFit3(pts.size(), pts.data());

                // for each point in the cell, calculate the distance from that point to the surface of the plane
                for (
                        double distance : pts | boost::adaptors::transformed
                        (
                            [&](const Wm5::Vector3d& p) 
                            { 
                                return plane.DistanceTo(p); 
                            }
                        )
                    ) 
                {
                    if (distance > 1e-3) 
                    {
                        // consider any point farther away from the surface of the plane than this to be
                        // evidence that the cell is not flat
                        return std::make_pair(idPtsPair.first, false);
                    }
                }
            
                return std::make_pair(idPtsPair.first, true);
            }
        );

        // Now create faceIdentifierMap
        for (int modelFaceIndex = 0; modelFaceIndex <= range[1] - range[0]; ++modelFaceIndex) {
            crimson::FaceIdentifier faceId;
            faceId.faceType = isFlat[modelFaceIndex] ? FaceIdentifier::ftCapOutflow : FaceIdentifier::ftWall;
            faceId.parentSolidIndices.insert("face " + std::to_string(modelFaceIndex));
            _faceIdentifierMap.setFaceIdentifierForModelFace(modelFaceIndex, faceId);
        }

        // Compute normals
        vtkNew<vtkPolyDataNormals> normals;
        normals->ComputeCellNormalsOn();
        normals->SetInputData(surface->GetVtkPolyData());
        normals->Update();

        surface->SetVtkPolyData(normals->GetOutput());

        setSurfaceRepresentation(surface);
    }

    void DiscreteSolidData::setSurfaceRepresentation(mitk::Surface::Pointer surface) {
        this->GetTimeGeometry()->GetGeometryForTimeStep(0)->SetBounds(surface->GetTimeGeometry()->GetGeometryForTimeStep(0)->GetBounds());
        this->GetTimeGeometry()->Update();

        _surfaceRepresentation = surface;

        _edges.clear();
        // Find and cache all the face edges
        for (int faceIdIndex = 0; faceIdIndex < _faceIdentifierMap.getNumberOfFaceIdentifiers(); ++faceIdIndex) {
            vtkNew<vtkCleanPolyData> cleaner;
            cleaner->SetInputData(_surfaceRepresentation->GetVtkPolyData());
            cleaner->Update();

            vtkNew<vtkThreshold> faceCells;
            faceCells->SetInputConnection(cleaner->GetOutputPort());
            faceCells->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Face IDs");
            faceCells->ThresholdBetween(faceIdIndex, faceIdIndex);

            vtkNew<vtkGeometryFilter> meshToSurface;
            meshToSurface->SetInputConnection(faceCells->GetOutputPort());

            vtkNew<vtkFeatureEdges> edges;
            edges->BoundaryEdgesOn();
            edges->FeatureEdgesOff();
            edges->NonManifoldEdgesOff();
            edges->ManifoldEdgesOff();
            edges->SetInputConnection(meshToSurface->GetOutputPort());

            edges->Update();

            _edges.push_back(edges->GetOutput());
        }
    }

    mitk::Surface::Pointer DiscreteSolidData::getSurfaceRepresentation() const
	{
        return _surfaceRepresentation;
	}

	mitk::ScalarType DiscreteSolidData::getVolume() const
	{
        if (!_surfaceRepresentation) {
            return 0;
        }

        vtkNew<vtkMassProperties> massProps;
        massProps->SetInputData(_surfaceRepresentation->GetVtkPolyData());
        return massProps->GetVolume();
	}

	mitk::Vector3D DiscreteSolidData::getFaceNormal(const FaceIdentifier& faceId) const
	{
		if (faceId.faceType != crimson::FaceIdentifier::ftCapInflow && faceId.faceType != crimson::FaceIdentifier::ftCapOutflow) {
			MITK_ERROR << "Cannot get normal for a wall";
			return mitk::Vector3D();
		}

        int faceIdentifierIndex = _faceIdentifierMap.faceIdentifierIndex(faceId);
        vtkPolyData* pd = _surfaceRepresentation->GetVtkPolyData();
        vtkDataArray* faceIdArray = pd->GetCellData()->GetArray("Face IDs");
        for (vtkIdType i = 0; i < pd->GetNumberOfCells(); ++i) {
            if (faceIdArray->GetTuple1(i) == faceIdentifierIndex) {
                mitk::Vector3D result;
                double* normal = pd->GetCellData()->GetNormals()->GetTuple3(i);
                mitk::FillVector3D(result, normal[0], normal[1], normal[2]);
                return result;
            }
        }

        MITK_ERROR << "Could not find a triangle with a face identifier " << faceIdentifierIndex;
        return mitk::Vector3D();
	}

	double DiscreteSolidData::getDistanceToFaceEdge(const FaceIdentifier& faceId, const mitk::Point3D& p) const
	{
        int faceIdentifierIndex = _faceIdentifierMap.faceIdentifierIndex(faceId);
        vtkPolyData* pd = _surfaceRepresentation->GetVtkPolyData();
        vtkDataArray* faceIdArray = pd->GetCellData()->GetArray("Face IDs");

        vtkNew<vtkCellLocator> loc;
        loc->SetDataSet(_edges[faceIdentifierIndex]);
        loc->BuildLocator();

        double pt[3] = {p[0], p[1], p[2]};
        double closestPt[3];
        vtkIdType id;
        int subId;
        double dist2;

        loc->FindClosestPoint(pt, closestPt, id, subId, dist2);
        return sqrt(dist2);
	}

	mitk::PlaneGeometry::Pointer DiscreteSolidData::getFaceGeometry(const FaceIdentifier& faceId, const mitk::Vector3D& referenceImageSpacing, mitk::ScalarType resliceWindowSize) const
	{
        int faceIdentifierIndex = _faceIdentifierMap.faceIdentifierIndex(faceId);
        vtkPolyData* pd = _surfaceRepresentation->GetVtkPolyData();
        vtkDataArray* faceIdArray = pd->GetCellData()->GetArray("Face IDs");
        for (vtkIdType i = 0; i < pd->GetNumberOfCells(); ++i) {
            if (faceIdArray->GetTuple1(i) == faceIdentifierIndex) {
                vtkSmartPointer<vtkCell> triangle = pd->GetCell(i);

                double p1[3], p2[3], p3[3];

                pd->GetPoint(triangle->GetPointId(0), p1);
                pd->GetPoint(triangle->GetPointId(1), p2);
                pd->GetPoint(triangle->GetPointId(2), p3);

                mitk::Vector3D v1, v2;
                auto plane = mitk::PlaneGeometry::New();
                mitk::FillVector3D(v1, p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2]);
                mitk::FillVector3D(v2, p3[0] - p1[0], p3[1] - p1[1], p3[2] - p1[2]);
                plane->InitializeStandardPlane(v1, v2);

                plane->SetReferenceGeometry(plane);

                return plane;
            }
        }

        MITK_ERROR << "Could not find a triangle with a face identifier " << faceIdentifierIndex;
        return {};
	}

} // namespace crimson
