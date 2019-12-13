#include "OCCBRepData.h"
#include <BRepBuilderAPI_Copy.hxx>

#include <vtkIdList.h>
#include <vtkPolyData.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkNew.h>
#include <vtkIntArray.h>
#include <vtkCellData.h>
#include <vtkKdTreePointLocator.h>
#include <vtkPolyDataNormals.h>

#include <NIS_Surface.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepBndLib.hxx>
#include <BRepClass_FaceClassifier.hxx>

#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>

#include <Poly_Triangulation.hxx>
#include <Poly_Triangle.hxx>
#include <BRep_Tool.hxx>
#include <TShort_Array1OfShortReal.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <Poly.hxx>

#include <Geom_Plane.hxx>
#include <gp_Pln.hxx>
#include <TopoDS_Face.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <GeomLib_IsPlanarSurface.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <Poly_Polygon3D.hxx>

#include <mitkSlicedGeometry3D.h>

#include <unordered_set>


namespace crimson
{

OCCBRepData::OCCBRepData()
{
    //OCCBRepData::InitializeTimeGeometry(1);
    //_surfaceRepresentation = mitk::Surface::New();
}

OCCBRepData::OCCBRepData(const Self& other)
    : SolidData(other)
    , _shape(BRepBuilderAPI_Copy(other._shape))
{
}

OCCBRepData::~OCCBRepData() {}

mitk::Surface::Pointer OCCBRepData::getSurfaceRepresentation() const
{
    if (!_surfaceRepresentation->GetVtkPolyData() && !_shape.IsNull()) {
        // Tesselation parameters (copied from SALOME)
        Bnd_Box B;
        BRepBndLib::Add(_shape, B);
        Standard_Real aXmin, aYmin, aZmin, aXmax, aYmax, aZmax;
        B.Get(aXmin, aYmin, aZmin, aXmax, aYmax, aZmax);
        double deflection = std::max(aXmax - aXmin, std::max(aYmax - aYmin, aZmax - aZmin)) * 0.0002;

        Standard_Real aHLRAngle = 0.5;

        // Compute the mesh representation
        BRepMesh_IncrementalMesh MESH(_shape, /*deflection, false*/0.001, true, aHLRAngle);
        //NIS_Surface surf(_shape, deflection);

        vtkNew<vtkPolyData> polyData;
        vtkNew<vtkPoints> points;
        polyData->SetPoints(points.GetPointer());

        // Face Id's
        vtkNew<vtkIntArray> faceIdArray;
        faceIdArray->Allocate(1);
        faceIdArray->SetName("Face IDs");

        polyData->GetCellData()->AddArray(faceIdArray.GetPointer());
        polyData->GetCellData()->SetActiveScalars(faceIdArray->GetName());
        polyData->Allocate();

        std::unordered_set<const Standard_Transient*> processedEdges;

        for (TopExp_Explorer faceExplorer(_shape, TopAbs_FACE); faceExplorer.More(); faceExplorer.Next()) {
            TopLoc_Location loc;
            Handle(Poly_Triangulation) triangulatedFace = BRep_Tool::Triangulation(TopoDS::Face(faceExplorer.Current()), loc);
            Poly::ComputeNormals(triangulatedFace);

            std::vector<int> localIdToGlobalId(triangulatedFace->NbNodes(), -1);
            for (TopExp_Explorer faceEdgeExplorer(faceExplorer.Current(), TopAbs_EDGE); faceEdgeExplorer.More(); faceEdgeExplorer.Next()) {
                const TopoDS_Edge& edge = TopoDS::Edge(faceEdgeExplorer.Current());
                Handle(Poly_PolygonOnTriangulation) edgeRep =
                    BRep_Tool::PolygonOnTriangulation(edge, triangulatedFace, loc);

                if (processedEdges.count(&*edge.TShape()) == 0) {
                    for (int i = 0; i < edgeRep->Nodes().Length(); ++i) {
                        int id = edgeRep->Nodes().Value(i + 1);
                        double coords[] = {
                            triangulatedFace->Nodes().Value(id).X(),
                            triangulatedFace->Nodes().Value(id).Y(),
                            triangulatedFace->Nodes().Value(id).Z()
                        };
                        points->InsertNextPoint(coords);
                    }
                    processedEdges.insert(&*edge.TShape());
                }
            }
        }

        vtkNew<vtkKdTreePointLocator> pointLocator;
        pointLocator->SetDataSet(polyData.GetPointer());
        pointLocator->BuildLocator();

        int faceIndex = 0;
        for (TopExp_Explorer faceExplorer(_shape, TopAbs_FACE); faceExplorer.More(); faceExplorer.Next(), ++faceIndex) {
            TopLoc_Location loc;
            Handle(Poly_Triangulation) triangulatedFace = BRep_Tool::Triangulation(TopoDS::Face(faceExplorer.Current()), loc);

            std::vector<int> localIdToGlobalId(triangulatedFace->NbNodes(), -1);
            for (TopExp_Explorer faceEdgeExplorer(faceExplorer.Current(), TopAbs_EDGE); faceEdgeExplorer.More(); faceEdgeExplorer.Next()) {
                Handle(Poly_PolygonOnTriangulation) edgeRep =
                    BRep_Tool::PolygonOnTriangulation(TopoDS::Edge(faceEdgeExplorer.Current()), triangulatedFace, loc);

                for (int i = 0; i < edgeRep->Nodes().Length(); ++i) {
                    int id = edgeRep->Nodes().Value(i + 1);
                    double coords[] = {
                        triangulatedFace->Nodes().Value(id).X(),
                        triangulatedFace->Nodes().Value(id).Y(),
                        triangulatedFace->Nodes().Value(id).Z()
                    };
                    localIdToGlobalId[id - 1] = pointLocator->FindClosestPoint(coords);
                }
            }

            for (int i = 0; i < triangulatedFace->NbNodes(); ++i) {
                if (localIdToGlobalId[i] == -1) {
                    points->InsertNextPoint(
                        triangulatedFace->Nodes().Value(i + 1).X(),
                        triangulatedFace->Nodes().Value(i + 1).Y(),
                        triangulatedFace->Nodes().Value(i + 1).Z()
                    );
                    localIdToGlobalId[i] = points->GetNumberOfPoints() - 1;
                }
            }

            auto vtkFaceId = 0;

            auto faceIdentifierOptional = _faceIdentifierMap.getFaceIdentifierForModelFace(faceIndex);
            if (faceIdentifierOptional) {
                vtkFaceId = _faceIdentifierMap.faceIdentifierIndex(faceIdentifierOptional.get());
            }

            for (int i = 0; i < triangulatedFace->NbTriangles(); ++i) {
                vtkIdType ids[] = {
                    localIdToGlobalId[triangulatedFace->Triangles().Value(i + 1).Value(1) - 1],
                    localIdToGlobalId[triangulatedFace->Triangles().Value(i + 1).Value(2) - 1],
                    localIdToGlobalId[triangulatedFace->Triangles().Value(i + 1).Value(3) - 1]
                };

                if (ids[0] == ids[1] || ids[0] == ids[2] || ids[1] == ids[2]) continue;

                polyData->InsertNextCell(VTK_TRIANGLE, 3, ids);

                faceIdArray->InsertNextTuple1(vtkFaceId);
            }
        }

        vtkNew<vtkPolyDataNormals> normals;
        normals->ConsistencyOn();
        normals->ComputePointNormalsOn();
        normals->ComputeCellNormalsOff();
        normals->AutoOrientNormalsOn();
        normals->SplittingOn();
        normals->SetInputData(polyData.GetPointer());
        normals->Update();

        _surfaceRepresentation = mitk::Surface::New();
        _surfaceRepresentation->SetVtkPolyData(normals->GetOutput());
    }

    return _surfaceRepresentation;
}

mitk::ScalarType OCCBRepData::getVolume() const
{
    GProp_GProps props;
    BRepGProp::VolumeProperties(getShape(), props);
    return props.Mass();
}


void OCCBRepData::setShape(const TopoDS_Shape& shape)
{
    _shape = shape;
    _surfaceRepresentation = mitk::Surface::New();

    if (!_shape.IsNull()) {
        // Compute bounding box
        Bnd_Box B;
        BRepBndLib::Add(_shape, B);
        mitk::BaseGeometry::BoundsArrayType bbox;
        B.Get(bbox[0], bbox[2], bbox[4], bbox[1], bbox[3], bbox[5]);

        this->GetTimeGeometry()->GetGeometryForTimeStep(0)->SetBounds(bbox);
        this->GetTimeGeometry()->Update();
    }
}

mitk::Vector3D OCCBRepData::getFaceNormal(const FaceIdentifier& faceId) const
{
    if (faceId.faceType != crimson::FaceIdentifier::ftCapInflow && faceId.faceType != crimson::FaceIdentifier::ftCapOutflow) {
        MITK_ERROR << "Cannot get normal for a wall";
        return mitk::Vector3D();
    }

    std::vector<int> faceIndices = _faceIdentifierMap.getModelFacesForFaceIdentifier(faceId);

    if (faceIndices.size() != 1) {
        MITK_ERROR << "Error in face normal request";
        return mitk::Vector3D();
    }

    // Find a face by index
    TopExp_Explorer faceExplorer(_shape, TopAbs_FACE);
    while (faceIndices[0]-- > 0) {
        faceExplorer.Next();
    }

    auto planeExtractor = GeomLib_IsPlanarSurface{BRep_Tool::Surface(TopoDS::Face(faceExplorer.Current()))};

    if (!planeExtractor.IsPlanar()) {
        MITK_ERROR << "Failed to extract plane from face";
        return mitk::Vector3D();
    }

    gp_Dir normal = planeExtractor.Plan().Axis().Direction();

    if (TopoDS::Face(faceExplorer.Current()).Orientation() == TopAbs_REVERSED) {
        normal.Reverse();
    }

    mitk::Vector3D result;
    result[0] = normal.X();
    result[1] = normal.Y();
    result[2] = normal.Z();

    return result;
}

double OCCBRepData::getDistanceToFaceEdge(const FaceIdentifier& faceId, const mitk::Point3D& p) const
{
    auto modelFaceIndices = _faceIdentifierMap.getModelFacesForFaceIdentifier(faceId);

    if (modelFaceIndices.size() > 1) {
        MITK_ERROR << "Trying to obtain distance to face split into multiple faces will produce wrong results!";
    }

    int faceIndex = modelFaceIndices[0];
    TopExp_Explorer faceExplorer(_shape, TopAbs_FACE);
    while (faceIndex-- > 0) {
        faceExplorer.Next();
    }

    gp_Pnt aPoint(p[0], p[1], p[2]);
    BRep_Builder aBuilder;
    TopoDS_Vertex aVertex;
    aBuilder.MakeVertex(aVertex, aPoint, Precision::Confusion());

    TopoDS_Face face = TopoDS::Face(faceExplorer.Current());

    double minDist = 1e10;
    for (TopExp_Explorer edgeExplorer(face, TopAbs_EDGE); edgeExplorer.More(); edgeExplorer.Next()) {
        TopoDS_Edge edge = TopoDS::Edge(edgeExplorer.Current());
        BRepExtrema_DistShapeShape extrema(aVertex, edge, Extrema_ExtFlag_MIN);
        if (!extrema.IsDone()) {
            MITK_ERROR << "extrema.IsDone()";
        }

        if (extrema.Value() < minDist) {
            minDist = extrema.Value();
        }
    }

    return minDist;
}

mitk::PlaneGeometry::Pointer OCCBRepData::getFaceGeometry(const FaceIdentifier& faceId, const mitk::Vector3D& referenceImageSpacing, mitk::ScalarType resliceWindowSize) const
{
	if (faceId.faceType != crimson::FaceIdentifier::ftCapInflow && faceId.faceType != crimson::FaceIdentifier::ftCapOutflow) {
		MITK_ERROR << "Cannot get PlaneGeometry for a wall";
		return mitk::PlaneGeometry::New();
	}

	std::vector<int> faceIndices = _faceIdentifierMap.getModelFacesForFaceIdentifier(faceId);

	if (faceIndices.size() != 1) {
		MITK_ERROR << "Error in face plane request";
		return mitk::PlaneGeometry::New();
	}

	// Find a face by index
	TopExp_Explorer faceExplorer(_shape, TopAbs_FACE);
	while (faceIndices[0]-- > 0) {
		faceExplorer.Next();
	}

	auto planeExtractor = GeomLib_IsPlanarSurface{ BRep_Tool::Surface(TopoDS::Face(faceExplorer.Current())) };

	if (!planeExtractor.IsPlanar()) {
		MITK_ERROR << "Failed to extract plane from face";
		return mitk::PlaneGeometry::New();
	}

	gp_Pnt origin = planeExtractor.Plan().Location();
	gp_Ax1 ax1 = planeExtractor.Plan().XAxis();
	gp_Ax1 ax2 = planeExtractor.Plan().YAxis();


	mitk::Point3D mitkOrigin;
	mitkOrigin[0] = origin.X();
	mitkOrigin[1] = origin.Y();
	mitkOrigin[2] = origin.Z();

	mitk::Vector3D v1, v2; //right, down
	
	mitk::FillVector3D(v1, ax1.Direction().X(), ax1.Direction().Y(), ax1.Direction().Z());
	mitk::FillVector3D(v2, ax2.Direction().X(), ax2.Direction().Y(), ax2.Direction().Z());


	// don't need this because it is not a stacked volume
	mitk::Vector3D spacing;
	spacing[0] = mitk::SlicedGeometry3D::CalculateSpacing(referenceImageSpacing, v1);
	spacing[1] = mitk::SlicedGeometry3D::CalculateSpacing(referenceImageSpacing, v2);
	spacing[2] = mitk::SlicedGeometry3D::CalculateSpacing(referenceImageSpacing, itk::CrossProduct(v1, v2));

	mitk::Vector2D resliceSizeInUnits;
	resliceSizeInUnits[0] = (int)(resliceWindowSize / spacing[0]);
	resliceSizeInUnits[1] = (int)(resliceWindowSize / spacing[1]);
	spacing[0] = resliceWindowSize / resliceSizeInUnits[0];
	spacing[1] = resliceWindowSize / resliceSizeInUnits[1];

	auto plane = mitk::PlaneGeometry::New();
	plane->InitializeStandardPlane(v1, v2);
	plane->SetOrigin(mitkOrigin - v1 * resliceWindowSize / 2 - v2 * resliceWindowSize / 2);

	mitk::ScalarType bounds[6] = { 0, resliceSizeInUnits[0], 0, resliceSizeInUnits[1], -0.001, 0.001 };
	plane->SetBounds(bounds);
	plane->SetReferenceGeometry(plane); // Set reference geometry to the plane geometry itself so that the image mapper and geometry mapper work correctly

	plane->SetSpacing(spacing);

	return plane;

}

} // namespace crimson
