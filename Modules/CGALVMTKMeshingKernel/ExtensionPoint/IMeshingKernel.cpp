// This will be an interface for meshing service and blueberry extension point etc.

#include <numeric>
#include <thread>
#include <valarray>

// #define CGAL_EIGEN3_ENABLED
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Polygon_mesh_processing/remesh.h>
#include <CGAL/Polygon_mesh_processing/border.h>
#include <boost/function_output_iterator.hpp>

// #include <CGAL/Polyhedron_3.h>
// #include <CGAL/extract_mean_curvature_flow_skeleton.h>
// #include <CGAL/boost/graph/split_graph_into_polylines.h>

#include <fstream>

#include "IMeshingKernel.h"

#include <VesselPathAbstractData.h>

#include <SolidData.h>
//#include <DiscreteSolidData.h>
#include <OCCBRepData.h>
#include <MeshData.h>

#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Edge.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pln.hxx>
#include <BRep_Tool.hxx>
#include <BRep_Builder.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>

#include <Geom_Curve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_Plane.hxx>
#include <GeomConvert.hxx>
#include <Geom_Line.hxx>

#include <TColStd_Array1OfReal.hxx>

#include <BSplCLib.hxx>

#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_NurbsConvert.hxx>
#include <BRepLib_FindSurface.hxx>

#include <boost/regex.hpp>
#include <boost/scope_exit.hpp>

#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <vtkPointData.h>
#include <vtkDoubleArray.h>
#include <vtkNew.h>
#include <vtkProbeFilter.h>
#include <vtkUnstructuredGridBase.h>
#include <vtkIdTypeArray.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkXMLUnstructuredGridWriter.h>
#include <vtkUnstructuredGrid.h>
#include <vtkCleanPolyData.h>
#include <vtkvmtkPolyDataSurfaceRemeshing.h>
#include <vtkvmtkBoundaryLayerGenerator.h>
#include <vtkvmtkTetGenWrapper.h>
#include "crimsonTetGenWrapper.h"
#include <vtkvmtkPolyDataToUnstructuredGridFilter.h>
#include <vtkvmtkPolyDataSizingFunction.h>
#include <vtkvmtkSurfaceProjection.h>
#include <vtkCellData.h>
#include <vtkPolyDataNormals.h>
#include <vtkGeometryFilter.h>
#include <vtkvmtkSimpleCapPolyData.h>
#include <vtkTriangleFilter.h>
#include <vtkAppendFilter.h>
#include <vtkvmtkUnstructuredGridTetraFilter.h>
#include <vtkThreshold.h>
#include <vtkIntArray.h>
#include <vtkMath.h>
#include <vtkMeshQuality.h>
#include <vtkTriangle.h>
#include <vtkCellLocator.h>
#include <vtkExtractCells.h>
#include <vtkFeatureEdges.h>

#include <Wm5ContMinSphere3.h>
#include <Wm5ApprPlaneFit3.h>

#include <boost/format.hpp>
#include <qstandardpaths.h>

#include <gsl.h>

#include <unordered_set>

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef K::Point_3 Point;
typedef CGAL::Surface_mesh<Point> Mesh;
typedef Mesh::Vertex_index Vertex_index;
typedef boost::graph_traits<Mesh>::halfedge_descriptor halfedge_descriptor;
typedef boost::graph_traits<Mesh>::edge_descriptor     edge_descriptor;
typedef boost::graph_traits<Mesh>::face_descriptor     face_descriptor;
typedef boost::graph_traits<Mesh>::vertex_descriptor   vertex_descriptor;
namespace PMP = CGAL::Polygon_mesh_processing;

// typedef CGAL::Mean_curvature_flow_skeletonization<Mesh>       Skeletonization;
// typedef Skeletonization::Skeleton                             Skeleton;
// typedef Skeleton::vertex_descriptor                           Skeleton_vertex;
// typedef Skeleton::edge_descriptor                             Skeleton_edge;

namespace crimson
{
	class MeshingTask : public crimson::async::TaskWithResult<mitk::BaseData::Pointer>
	{
	public:
		MeshingTask(const mitk::BaseData::Pointer& solid, const IMeshingKernel::GlobalMeshingParameters& params,
			const std::map<FaceIdentifier, IMeshingKernel::LocalMeshingParameters>& localParams,
			const std::map<VesselPathAbstractData::VesselPathUIDType, std::string>& vesselUIDtoNameMap)
			: solid(solid)
			, params(params)
			, localParams(localParams)
			, vesselUIDtoNameMap(vesselUIDtoNameMap)
		{
		}
		~MeshingTask() { }

		void cancel() override
		{
			crimson::async::TaskWithResult<mitk::BaseData::Pointer>::cancel();
		}

	protected:
		mitk::BaseData::Pointer solid;
		IMeshingKernel::GlobalMeshingParameters params;
		std::map<FaceIdentifier, IMeshingKernel::LocalMeshingParameters> localParams;
		std::map<VesselPathAbstractData::VesselPathUIDType, std::string> vesselUIDtoNameMap;
	};


	class OCCMeshingTask : public crimson::MeshingTask
	{
	public:
		OCCMeshingTask(const mitk::BaseData::Pointer& solid, const IMeshingKernel::GlobalMeshingParameters& params,
			const std::map<FaceIdentifier, IMeshingKernel::LocalMeshingParameters>& localParams,
			const std::map<VesselPathAbstractData::VesselPathUIDType, std::string>& vesselUIDtoNameMap)
			:MeshingTask(solid, params, localParams, vesselUIDtoNameMap) {}

		~OCCMeshingTask(){}

		std::tuple<State, std::string> runTask() override
		{
            static const bool debug = false;
            auto writeVTU = [](vtkUnstructuredGrid* ug, const char* fn) {
                if (!debug) return;
                MITK_INFO << "writing " << fn;
                vtkNew<vtkXMLUnstructuredGridWriter> ugw;
                ugw->SetFileName((std::string("d:/kcl/") + fn + ".vtu").c_str());
                ugw->SetInputData(ug);
                ugw->Write();
            };
            auto writeVTP = [](vtkPolyData* pd, const char* fn) {
                if (!debug) return;
                MITK_INFO << "writing " << fn;
                vtkNew<vtkXMLPolyDataWriter> pdw;
                pdw->SetFileName((std::string("d:/kcl/") + fn + ".vtp").c_str());
                pdw->SetInputData(pd);
                pdw->Write();
            };
            auto coordsToVector = [](double* c) {
                return Wm5::Vector3d(c[0], c[1], c[2]);
            };

            auto brep = dynamic_cast<SolidData*>(solid.GetPointer());

            if (!brep) {
                return std::make_tuple(State_Failed, std::string("Cannot use solids defined not as OpenCASCADE data"));
            }

            stepsAddedSignal(1 + brep->getFaceIdentifierMap().getNumberOfFaceIdentifiers());

            auto faceEdgeSize = [&](int faceId) {
                const FaceIdentifier& identifier = brep->getFaceIdentifierMap().getFaceIdentifier(faceId);
                auto it = localParams.find(identifier);

                double targetLength = it == localParams.end() || !it->second.size ? *params.defaultLocalParameters.size : *it->second.size;
                if (it == localParams.end() || !it->second.sizeRelative ? *params.defaultLocalParameters.sizeRelative : *it->second.sizeRelative) {
                    targetLength *= sqrt(brep->GetGeometry()->GetBoundingBox()->GetDiagonalLength2());
                }
                return targetLength;
            };

            vtkNew<vtkPolyData> cpy;
            cpy->DeepCopy(brep->getSurfaceRepresentation()->GetVtkPolyData());
            auto pd = cpy.Get();
            //auto pd = brep->getSurfaceRepresentation()->GetVtkPolyData();

            vtkNew<vtkPolyData> remeshedPd;
            vtkNew<vtkPoints> points;

            vtkNew<vtkIntArray> faceIdArray;
            faceIdArray->Allocate(1);
            faceIdArray->SetName("Face IDs");

            remeshedPd->SetPoints(points.GetPointer());
            remeshedPd->GetCellData()->AddArray(faceIdArray.GetPointer());
            remeshedPd->Allocate();

            // Compute normals
            vtkNew<vtkCleanPolyData> c;
            c->SetInputData(pd);
            c->Update();

            vtkNew<vtkFeatureEdges> fe;
            fe->SetInputData(c->GetOutput());
            fe->FeatureEdgesOff();
            fe->Update();

            if (fe->GetOutput()->GetNumberOfCells() != 0) {
                return std::make_tuple(State_Failed, std::string("Could not achieve a watertight surface mesh. Try different fillet sizes."));
            }

            vtkNew<vtkPolyDataNormals> n;
            n->SetInputConnection(c->GetOutputPort());
            n->SetComputeCellNormals(1);
            n->SetComputePointNormals(0);
            n->SetAutoOrientNormals(1);
            n->SetFlipNormals(0);
            n->SetConsistency(1);
            n->SplittingOff();
            n->Update();

            // Remesh the surface using CGAL
            vtkPolyData* remeshInput = n->GetOutput();
            writeVTP(remeshInput, "remeshInput");
            {
                Mesh mesh;

                for (int j = 0; j < remeshInput->GetPoints()->GetNumberOfPoints(); ++j) {
                    double* coords = remeshInput->GetPoints()->GetPoint(j);
                    mesh.add_vertex(Point(coords[0], coords[1], coords[2]));
                }

                auto fimap = mesh.add_property_map<face_descriptor, int>("f:id").first;

                std::unordered_set<int> faceIdSet;

                auto arr = remeshInput->GetCellData()->GetArray("Face IDs");
                for (int k = 0; k < remeshInput->GetNumberOfCells(); ++k) {
                    vtkSmartPointer<vtkCell> cell = remeshInput->GetCell(k);
                    if (cell->GetCellType() != VTK_TRIANGLE) {
                        continue;
                    }

                    // Ensure that all faces are passed into CGAL with consistent ordering
                    Vertex_index ids[] = {Vertex_index(cell->GetPointId(0)), Vertex_index(cell->GetPointId(1)), Vertex_index(cell->GetPointId(2))};

                    Wm5::Vector3d coords[3];
                    boost::transform(ids, coords, 
                        [&](Vertex_index id) { 
                            return coordsToVector(remeshInput->GetPoint(id));
                        }
                    );

                    Wm5::Vector3d normal = coordsToVector(remeshInput->GetCellData()->GetNormals()->GetTuple(k));
                    if (normal.Dot((coords[1] - coords[0]).Cross(coords[2] - coords[1])) < 0) {
                        std::swap(ids[0], ids[1]);
                    }

                    int faceId = arr->GetTuple1(k);
                    fimap[mesh.add_face(ids[0], ids[1], ids[2])] = faceId;
                    faceIdSet.insert(faceId);
                }

                // Arrange the order of remeshing from largest edge size to smallest
                struct FaceEdgeSize {
                    int id;
                    double edgeSize;
                };
                
                std::vector<FaceEdgeSize> edgeSizes(faceIdSet.size());
                boost::transform(faceIdSet, edgeSizes.begin(), [&](int faceId) { return FaceEdgeSize{faceId, faceEdgeSize(faceId)}; });

                boost::sort(edgeSizes, [](const FaceEdgeSize& lhs, const FaceEdgeSize& rhs) { return lhs.edgeSize > rhs.edgeSize; });

                // Do the remeshing
//                int xx = 0;
                // Map to constrain the edges between different face IDs

                for (const FaceEdgeSize& edgeSize : edgeSizes) {
                    // Map to constrain the edges between different face IDs
                    auto cstmap = mesh.add_property_map<edge_descriptor, bool>("e:cst", false).first;

                    // Find all faces for this face Id
                    std::unordered_set<face_descriptor> facesForId;
                    boost::copy(
                        faces(mesh) | boost::adaptors::filtered([&](face_descriptor d) { return fimap[d] == edgeSize.id; }), 
                        std::inserter(facesForId, facesForId.end())
                    );

                    // Add a layer of faces around the edges of this faceId so that the edge is remeshed well
                    std::unordered_set<face_descriptor> additionalFacesForId;
                    for (auto he : mesh.halfedges()) {
//                         if (mesh.is_border(mesh.edge(he))) {
//                             continue;
//                         }

                        int f1Count = facesForId.count(mesh.face(he));
                        int f2Count = facesForId.count(mesh.face(mesh.opposite(he)));
                        if (f1Count + f2Count == 1) { // One of the faces is in our faceId, another one is not
                            cstmap[mesh.edge(he)] = true; // Constrain the edge

                            boost::copy(
                                mesh.faces_around_target(he) | 
                                    boost::adaptors::filtered([&](face_descriptor d) { return fimap[d] != edgeSize.id; }),
                                std::inserter(additionalFacesForId, additionalFacesForId.end())
                            );
                        }
                    }

                    // Build the full list of faces to remesh
                    boost::copy(additionalFacesForId, std::inserter(facesForId, facesForId.end()));

//                     std::ostringstream oss;
//                     oss << "d:/kcl/reminput" << xx++ << ".off";
//                     std::ofstream os(oss.str().c_str());
//                     os << mesh;
//                     os.close();

                    MITK_INFO << "Start remeshing of model face " << edgeSize.id << " (" << facesForId.size() << " faces)...";
                    PMP::isotropic_remeshing(
                        facesForId,
                        edgeSize.edgeSize,
                        mesh,
                        PMP::parameters::number_of_iterations(params.surfaceOptimizationLevel)
                        .edge_is_constrained_map(cstmap)
                        .face_patch_map(fimap)
                        .relax_constraints(true)
                    );
                    mesh.collect_garbage();
                    progressMadeSignal(1);

                    mesh.remove_property_map(cstmap);
                }
                MITK_INFO << "Done remeshing - " << mesh.number_of_vertices() << " vertices, " << mesh.number_of_faces() << " faces.";

//                 MITK_INFO << "Skeletonizing";
// 
//                 Skeleton skeleton;
//                 //CGAL::extract_mean_curvature_flow_skeleton(mesh, skeleton);
//                 Skeletonization skelMaker(mesh);
//                 skelMaker.set_quality_speed_tradeoff(0.5);
//                 skelMaker(skeleton);
// 
//                 MITK_INFO << "Done";
// 
//                 {
//                     vtkNew<vtkPolyData> pd;
//                     vtkNew<vtkPoints> points;
//                     pd->SetPoints(points.GetPointer());
//                     pd->Allocate();
// 
//                     BOOST_FOREACH (Skeleton_vertex v, vertices(skeleton))
//                         for (vertex_descriptor vd : skeleton[v].vertices) {
//                             auto p = get(CGAL::vertex_point, mesh, vd);
//                             points->InsertNextPoint(p[0], p[1], p[2]);
//                             
//                             auto p2 = skeleton[v].point;
//                             points->InsertNextPoint(p2[0], p2[1], p2[2]);
// 
//                             vtkIdType ids[2] = {points->GetNumberOfPoints() - 2, points->GetNumberOfPoints() - 1};
// 
//                             pd->InsertNextCell(VTK_LINE, 2, ids);
// 
//                             auto blSizeForFaceId = [&](int faceId) {
//                                 const FaceIdentifier& identifier = brep->getFaceIdentifierMap().getFaceIdentifier(faceId);
//                                 auto it = localParams.find(identifier);
// 
//                                 return it == localParams.end() || !it->second.propagationDistance ? *params.defaultLocalParameters.propagationDistance : *it->second.propagationDistance;
//                             };
// 
//                             double blSize = 0;
//                             int count = 0;
//                             for (face_descriptor fd : mesh.faces_around_target(mesh.halfedge(vd))) {
//                                 blSize += blSizeForFaceId(fimap[fd]);
//                                 ++count;
//                             }
//                             blSize /= count;
// 
// 
// 
//                             Wm5::Vector3d surfPoint(p[0], p[1], p[2]);
//                             Wm5::Vector3d skelPoint(p2[0], p2[1], p2[2]);
// 
//                             Wm5::Vector3d newSurfPoint = skelPoint - surfPoint;
//                             newSurfPoint.Normalize();
//                             newSurfPoint *= blSize;
//                             newSurfPoint += surfPoint;
//                             decltype(p) newP(newSurfPoint[0], newSurfPoint[1], newSurfPoint[2]);
// 
//                             put(CGAL::vertex_point, mesh, vd, newP);
//                         }
// 
//                     writeVTP(pd.Get(), "Skeleton directions");
//                 }

                // Build the VTK data structure from CGAL
                for (auto vi : mesh.vertices()) {
                    Point p = mesh.point(vi);
                    points->InsertNextPoint(p[0], p[1], p[2]);
                }

                for (auto fi : mesh.faces()) {
                    vtkIdType ids[3];

                    boost::copy(vertices_around_face(mesh.halfedge(fi), mesh), ids);

                    remeshedPd->InsertNextCell(VTK_TRIANGLE, 3, ids);
                    faceIdArray->InsertNextTuple1(fimap[fi]);
                }
            }

            writeVTP(remeshedPd.GetPointer(), "00 - remeshed");

            vtkSmartPointer<vtkUnstructuredGrid> finalResult;

            if (params.meshSurfaceOnly) {
                vtkNew<vtkvmtkPolyDataToUnstructuredGridFilter> surfaceToMesh;
                surfaceToMesh->SetInputData(remeshedPd.GetPointer());
                surfaceToMesh->Update();

                finalResult = surfaceToMesh->GetOutput();
            } else {
                bool boundaryLayers = params.defaultLocalParameters.useBoundaryLayers;
                stepsAddedSignal(boundaryLayers ? 100 : 25);

                auto runTetGen = [&](vtkPolyData* input) {
                    // *******
                    MITK_INFO << "Computing sizing function.";
                    vtkNew<vtkvmtkPolyDataSizingFunction> sizer;
                    sizer->SetSizingFunctionArrayName("VolumeSizingFunction");
                    sizer->SetInputData(input);
                    sizer->Update();

                    vtkNew<vtkvmtkPolyDataToUnstructuredGridFilter> surf2Mesh;
                    surf2Mesh->SetInputConnection(sizer->GetOutputPort());
                    surf2Mesh->Update();

                    MITK_INFO << "Running 3D mesher.";
                    vtkNew<crimsonTetGenWrapper> mesher;
                    mesher->SetInputConnection(surf2Mesh->GetOutputPort());
                    mesher->SetCellEntityIdsArrayName("Face IDs");
                    mesher->SetSizingFunctionArrayName("VolumeSizingFunction");
                    mesher->SetUseSizingFunction(1);
                    mesher->SetOrder(1);
                    mesher->SetQuality(1);
                    mesher->SetPLC(1);
                    mesher->SetNoBoundarySplit(boundaryLayers ? 1 : 0);
                    mesher->SetRemoveSliver(0);
                    mesher->SetOutputSurfaceElements(boundaryLayers ? 0 : 1);
                    mesher->SetOutputVolumeElements(1);
                    mesher->SetVerbose(1);
                    mesher->SetOptimizationLevel(params.volumeOptimizationLevel);
                    mesher->SetMinDihedral(params.minDihedralAngle);
                    mesher->SetMaxDihedral(params.maxDihedralAngle);
                    mesher->SetMaxRatio(params.maxRadiusEdgeRatio);
                    mesher->Update();

                    writeVTU(mesher->GetOutput(), "06 - mesh result");

                    return vtkSmartPointer<vtkUnstructuredGrid>(mesher->GetOutput());
                };

                ///////////////////
                if (!boundaryLayers) {
                    // Remove duplicate points produced when caps are removed
                    vtkNew<vtkCleanPolyData> cleaner;
                    cleaner->SetInputData(remeshedPd.GetPointer());
                    cleaner->Update();

                    finalResult = runTetGen(cleaner->GetOutput());
                    if (finalResult->GetNumberOfCells() == 0) {
                        return std::make_pair(State_Failed, std::string("TetGen failed, see log for details."));
                    }
                    vtkDataArray* faceIdArray = finalResult->GetCellData()->GetArray("Face IDs");
                    for (int faceId = 0; faceId < finalResult->GetNumberOfCells(); ++faceId) {
                        vtkSmartPointer<vtkCell> cell = finalResult->GetCell(faceId);
                        if (cell->GetCellDimension() == 3) {
                            faceIdArray->SetTuple1(faceId, -1);
                        }
                    }
                    progressMadeSignal(25);
                } else {
                    // We will remove caps to generate boundary layers. 

                    // Assign thickness before removing caps
                    vtkNew<vtkDoubleArray> thicknessArray;
                    thicknessArray->SetNumberOfTuples(remeshedPd->GetNumberOfPoints());
                    thicknessArray->SetName("BL thickness");
                    remeshedPd->GetPointData()->AddArray(thicknessArray.Get());

                    auto faceBLThickness = [&](int faceId) {
                        const FaceIdentifier& identifier = brep->getFaceIdentifierMap().getFaceIdentifier(faceId);
                        auto it = localParams.find(identifier);

                        return (it == localParams.end() || !it->second.thickness
                                ? *params.defaultLocalParameters.thickness
                                : *it->second.thickness);
                    };

                    vtkNew<vtkIdList> ptIdList;
                    ptIdList->SetNumberOfIds(1);
                    vtkNew<vtkIdList> cellIdList;
                    for (int i = 0; i < remeshedPd->GetNumberOfPoints(); ++i) {
                        ptIdList->SetId(0, i);
                        remeshedPd->GetCellNeighbors(-1, ptIdList.Get(), cellIdList.Get());

                        double averageThickness = 0;
                        for (int cellId = 0; cellId < cellIdList->GetNumberOfIds(); ++cellId) {
                            averageThickness += faceBLThickness(remeshedPd->GetCellData()->GetArray("Face IDs")->GetTuple1(cellIdList->GetId(cellId)));
                        }

                        thicknessArray->SetTuple1(i, averageThickness / cellIdList->GetNumberOfIds());
                    }

                    writeVTP(remeshedPd.Get(), "bl thickness");


                    // Record the plane information for restoration of ids and normal constraints.
                    struct PlaneInfo {
                        Wm5::Plane3d plane;
                        Wm5::Sphere3d sphere;

                        bool inPlane(double* coords, double tol = 0.01) const {
                            Wm5::Vector3d p(coords[0], coords[1], coords[2]);
                            return plane.DistanceTo(p) < tol && (sphere.Center - p).Length() < sphere.Radius + tol;
                        }

                        Wm5::Vector3d project(double* vec) const {
                            Wm5::Vector3d p(vec[0], vec[1], vec[2]);
                            Wm5::Vector3d projected = p - p.Dot(plane.Normal) * plane.Normal;
                            projected.Normalize();
                            return projected;
                        }
                    };

                    std::map<int, PlaneInfo> flowFacePlanes;

                    auto removeCaps = [&](vtkPolyData* polyData) {
                        polyData->BuildLinks();

                        // Find all cap triangles
                        std::unordered_set<int> capIds;
                        const FaceIdentifierMap& faceIdMap = brep->getFaceIdentifierMap();
                        for (int i = 0; i < faceIdMap.getNumberOfFaceIdentifiers(); ++i) {
                            FaceIdentifier::FaceType faceType = faceIdMap.getFaceIdentifier(i).faceType;
                            if (faceType == FaceIdentifier::ftCapInflow || faceType == FaceIdentifier::ftCapOutflow) {
                                capIds.insert(i);
                            }
                        }

                        // Remove the triangles while recording the points that belong to them
                        std::map<int, std::set<int>> pointIds;
                        vtkDataArray* faceIdArray = polyData->GetCellData()->GetArray("Face IDs");
                        for (int cellId = 0; cellId < polyData->GetNumberOfCells(); ++cellId) {
                            if (polyData->GetCell(cellId)->GetCellType() != VTK_TRIANGLE) {
                                polyData->DeleteCell(cellId);
                                continue;
                            }
                            int faceId = static_cast<int>(faceIdArray->GetTuple1(cellId) + 0.5);
                            if (capIds.count(faceId) != 0) {
                                for (int i = 0; i < polyData->GetCell(cellId)->GetNumberOfPoints(); ++i) {
                                    pointIds[faceId].insert(polyData->GetCell(cellId)->GetPointId(i));
                                }

                                polyData->DeleteCell(cellId);
                            }
                        }

                        // Fit the planes to the points of caps
                        for (const auto& faceIdToPointIds : pointIds) {
                            std::vector<Wm5::Vector3d> points(faceIdToPointIds.second.size());
                            boost::transform(faceIdToPointIds.second, points.begin(), [&](int ptId) { return coordsToVector(polyData->GetPoint(ptId)); });

                            PlaneInfo planeInfo;
                            planeInfo.plane = Wm5::OrthogonalPlaneFit3(points.size(), points.data());
                            Wm5::MinSphere3d(points.size(), points.data(), planeInfo.sphere);

                            flowFacePlanes[faceIdToPointIds.first] = planeInfo;
                        }

                        // Finally, remove the cells completely
                        polyData->RemoveDeletedCells();
                    };

                    MITK_INFO << "Removing caps before boundary layer creation";

                    removeCaps(remeshedPd.GetPointer());
                    writeVTP(remeshedPd.GetPointer(), "01 - noCaps");


                    progressMadeSignal(5);
                    if (isCancelling()) {
                        return std::make_tuple(State_Cancelled, std::string("Operation cancelled."));
                    }
                    MITK_INFO << "Computing normals";

                    // Remove duplicate points produced when caps are removed
                    vtkNew<vtkCleanPolyData> cleaner;
                    cleaner->SetInputData(remeshedPd.GetPointer());
                    cleaner->Update();

                    // Recompute normals on the remeshed surface
                    vtkNew<vtkPolyDataNormals> normals;
                    normals->SetInputConnection(cleaner->GetOutputPort());
                    normals->SetComputeCellNormals(0);
                    normals->SetAutoOrientNormals(1);
                    normals->SetFlipNormals(0);
                    normals->SetConsistency(1);
                    normals->SplittingOff();
                    normals->Update();
                    normals->GetOutput()->GetPointData()->GetNormals()->SetName("Normals");

                    // Ensure normals are in original planes for flow faces
                    for (int ptId = 0; ptId < normals->GetOutput()->GetNumberOfPoints(); ++ptId) {
                        double* coords = normals->GetOutput()->GetPoint(ptId);
                        for (const auto& faceIdToPlaneInfo : flowFacePlanes) {
                            if (faceIdToPlaneInfo.second.inPlane(coords)) {
                                double* n = normals->GetOutput()->GetPointData()->GetNormals()->GetTuple3(ptId);
                                Wm5::Vector3d projected = faceIdToPlaneInfo.second.project(n);
                                normals->GetOutput()->GetPointData()->GetNormals()->SetTuple3(ptId, projected.X(), projected.Y(), projected.Z());
                            }
                        }
                    }

                    writeVTP(normals->GetOutput(), "02 - normals");

                    progressMadeSignal(5);
                    if (isCancelling()) {
                        return std::make_tuple(State_Cancelled, std::string("Operation cancelled."));
                    }
                    MITK_INFO << "Generating boundary layers";

                    vtkNew<vtkvmtkPolyDataToUnstructuredGridFilter> surfaceToMesh;
                    surfaceToMesh->SetInputConnection(normals->GetOutputPort());
                    surfaceToMesh->Update();

                    const int InnerSurfId = surfaceToMesh->GetOutput()->GetCellData()->GetArray("Face IDs")->GetRange()[1] + 10;
                    const int SidewallId = InnerSurfId + 1;
                    const int VolumeId = SidewallId + 1;

                    vtkNew<vtkvmtkBoundaryLayerGenerator> boundaryLayer;
                    boundaryLayer->SetInputData(surfaceToMesh->GetOutput());
                    boundaryLayer->SetWarpVectorsArrayName("Normals");
                    boundaryLayer->SetCellEntityIdsArrayName("Face IDs");
                    boundaryLayer->NegateWarpVectorsOn();
                    boundaryLayer->IncludeSurfaceCellsOff();
                    boundaryLayer->IncludeSidewallCellsOn();
                    boundaryLayer->SetInnerSurfaceCellEntityId(InnerSurfId);
                    boundaryLayer->SetSidewallCellEntityId(SidewallId);
                    boundaryLayer->SetVolumeCellEntityId(VolumeId);

                    // Common boundary layer parameters
                    boundaryLayer->SetNumberOfSubLayers(params.defaultLocalParameters.numSubLayers);
                    boundaryLayer->SetSubLayerRatio(params.defaultLocalParameters.subLayerRatio);

                    boundaryLayer->SetNumberOfSubsteps(2000);
                    boundaryLayer->SetRelaxation(0.01);
                    boundaryLayer->SetLocalCorrectionFactor(0.45);
                    boundaryLayer->SetLayerThicknessArrayName("BL thickness");
                    boundaryLayer->SetConstantThickness(0);
                    boundaryLayer->SetLayerThicknessRatio(1); // Just a multiplier

                    boundaryLayer->Update();

                    writeVTU(boundaryLayer->GetOutput(), "03 - bl");

                    progressMadeSignal(25);
                    if (isCancelling()) {
                        return std::make_tuple(State_Cancelled, std::string("Operation cancelled."));
                    }
                    MITK_INFO << "Capping the internal surface of boundary layer result";

                    // Extract volume cells
                    vtkNew<vtkThreshold> blVolumeCells;
                    blVolumeCells->SetInputData(boundaryLayer->GetOutput());
                    blVolumeCells->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Face IDs");
                    blVolumeCells->ThresholdBetween(VolumeId, VolumeId);
                    blVolumeCells->Update();

                    // Extract sidewall cells
                    vtkNew<vtkThreshold> blSidewallCells;
                    blSidewallCells->SetInputData(boundaryLayer->GetOutput());
                    blSidewallCells->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Face IDs");
                    blSidewallCells->ThresholdBetween(SidewallId, SidewallId);
                    blSidewallCells->Update();

                    // Cap the inner surface
                    vtkNew<vtkGeometryFilter> meshToSurfaceInner;
                    meshToSurfaceInner->SetInputData(boundaryLayer->GetInnerSurface());
                    meshToSurfaceInner->Update();

                    const int FirstCapId = VolumeId + 10;
                    vtkNew<vtkvmtkSimpleCapPolyData> capper;
                    capper->SetInputData(meshToSurfaceInner->GetOutput());
                    capper->SetCellEntityIdsArrayName("Face IDs");
                    capper->SetCellEntityIdOffset(FirstCapId);
                    capper->Update();

                    vtkNew<vtkTriangleFilter> triangulator;
                    triangulator->SetInputData(capper->GetOutput());
                    triangulator->Update();

                    writeVTP(triangulator->GetOutput(), "04 - capped");

                    progressMadeSignal(5);
                    if (isCancelling()) {
                        return std::make_tuple(State_Cancelled, std::string("Operation cancelled."));
                    }
                    MITK_INFO << "Remesing caps";

                    // Remesh caps
                    // Assign mesh sizes for caps
                    auto assignFlowFaceMeshSizes = [&](vtkPolyData* pd, const char* arrayName) {
                        vtkNew<vtkDoubleArray> sizeArray;
                        sizeArray->SetName(arrayName);
                        sizeArray->SetNumberOfTuples(pd->GetNumberOfPoints());
                        sizeArray->FillComponent(0, 0);
                        pd->GetPointData()->AddArray(sizeArray.GetPointer());

                        vtkDataArray* faceIdArray = pd->GetCellData()->GetArray("Face IDs");
                        for (int faceId = 0; faceId < pd->GetNumberOfCells(); ++faceId) {
                            vtkSmartPointer<vtkCell> cell = pd->GetCell(faceId);
                            double cellId = faceIdArray->GetTuple1(faceId);

                            if (cellId >= FirstCapId) {
                                for (const auto& faceIdToPlaneInfo : flowFacePlanes) {
                                    if (faceIdToPlaneInfo.second.inPlane(cell->GetPoints()->GetPoint(0))) {
                                        double edgeSize = faceEdgeSize(faceIdToPlaneInfo.first);
                                        double targetArea = sqrt(3) / 4 * edgeSize * edgeSize;

                                        for (int ptId = 0; ptId < cell->GetNumberOfPoints(); ++ptId) {
                                            sizeArray->SetTuple1(cell->GetPointId(ptId), targetArea);
                                        }
                                    }
                                }
                            } 
                        }
                    };

                    assignFlowFaceMeshSizes(triangulator->GetOutput(), "Cap edge size");

                    vtkNew<vtkvmtkPolyDataSurfaceRemeshing> remeshCaps;
                    remeshCaps->SetInputData(triangulator->GetOutput());
                    remeshCaps->SetCellEntityIdsArrayName("Face IDs");
                    remeshCaps->SetPreserveBoundaryEdges(1);
                    remeshCaps->SetTargetAreaArrayName("Cap edge size");
                    remeshCaps->SetElementSizeModeToTargetAreaArray();
                    vtkNew<vtkIdList> idList;
                    idList->InsertNextId(InnerSurfId);
                    remeshCaps->SetExcludedEntityIds(idList.GetPointer());
                    remeshCaps->Update();

                    writeVTP(remeshCaps->GetOutput(), "05 - cap-remesh");

                    progressMadeSignal(25);
                    if (isCancelling()) {
                        return std::make_tuple(State_Cancelled, std::string("Operation cancelled."));
                    }

                    // Remove duplicated points after cap remeshing
                    vtkNew<vtkCleanPolyData> cleaner2;
                    cleaner2->SetInputData(remeshCaps->GetOutput());
                    cleaner2->Update();

                    // Extract remeshed caps
                    vtkNew<vtkThreshold> remeshedCapCells;
                    remeshedCapCells->SetInputData(cleaner2->GetOutput());
                    remeshedCapCells->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Face IDs");
                    remeshedCapCells->ThresholdByUpper(FirstCapId);
                    remeshedCapCells->Update();

                    vtkNew<vtkGeometryFilter> meshToSurface;
                    meshToSurface->SetInputData(cleaner2->GetOutput());
                    meshToSurface->Update();

                    vtkSmartPointer<vtkUnstructuredGrid> result = runTetGen(meshToSurface->GetOutput());
                    if (result->GetNumberOfCells() == 0) {
                        return std::make_pair(State_Failed, std::string("TetGen failed, see log for details."));
                    }

                    progressMadeSignal(25);
                    if (isCancelling()) {
                        return std::make_tuple(State_Cancelled, std::string("Operation cancelled."));
                    }
                    MITK_INFO << "Assembling final result";

                    vtkNew<vtkAppendFilter> appender;
                    appender->SetMergePoints(1);
                    appender->AddInputData(result);
                    appender->AddInputData(blVolumeCells->GetOutput());
                    appender->AddInputData(blSidewallCells->GetOutput());
                    appender->AddInputData(remeshedCapCells->GetOutput());
                    appender->AddInputData(surfaceToMesh->GetOutput()); // For surface IDs
                    appender->Update();

                    if (!appender->GetOutput()->GetCellData()->GetArray("Face IDs")) {
                        return std::make_pair(State_Failed, std::string("Please reloft/reblend before meshing."));
                    }

                    auto restoreMoreFlowFaceIds = [&](vtkUnstructuredGrid* ug) {
                        vtkDataArray* faceIdArray = ug->GetCellData()->GetArray("Face IDs");
                        for (int faceId = 0; faceId < ug->GetNumberOfCells(); ++faceId) {
                            vtkSmartPointer<vtkCell> cell = ug->GetCell(faceId);
                            double cellId = faceIdArray->GetTuple1(faceId);

                            if (cell->GetCellDimension() == 3) {
                                faceIdArray->SetTuple1(faceId, -1);
                                continue;
                            }

                            if (cellId >= FirstCapId || cellId == SidewallId) {
                                for (const auto& faceIdToPlaneInfo : flowFacePlanes) {
                                    if (faceIdToPlaneInfo.second.inPlane(cell->GetPoints()->GetPoint(0))) {
                                        faceIdArray->SetTuple1(faceId, faceIdToPlaneInfo.first);
                                        break;
                                    }
                                }
                            }
                        }
                    };
                    restoreMoreFlowFaceIds(appender->GetOutput());

                    writeVTU(appender->GetOutput(), "07 - full mesh");

                    vtkNew<vtkvmtkUnstructuredGridTetraFilter> tetrahedralizer;
                    tetrahedralizer->SetInputData(appender->GetOutput());
                    tetrahedralizer->Update();

                    writeVTU(tetrahedralizer->GetOutput(), "08 - tetrahedralized");
                    finalResult = tetrahedralizer->GetOutput();

                    progressMadeSignal(10);
                }
            }

            auto ug = mitk::UnstructuredGrid::New();
            ug->SetVtkUnstructuredGrid(finalResult);
            auto result = MeshData::New();
            result->setFaceIdentifierMap(brep->getFaceIdentifierMap());
            result->setUnstructuredGrid(ug, true);
            this->setResult(result.GetPointer());

            progressMadeSignal(1);

            return std::make_tuple(State_Finished, std::string());
		}

	};

	class DiscreteMeshingTask : public crimson::MeshingTask
	{
	public:
		DiscreteMeshingTask(const mitk::BaseData::Pointer& solid, const IMeshingKernel::GlobalMeshingParameters& params,
			const std::map<FaceIdentifier, IMeshingKernel::LocalMeshingParameters>& localParams,
			const std::map<VesselPathAbstractData::VesselPathUIDType, std::string>& vesselUIDtoNameMap)
			:MeshingTask(solid, params, localParams, vesselUIDtoNameMap) {}

		~DiscreteMeshingTask(){}

		std::tuple<State, std::string> runTask() override
		{
			stepsAddedSignal(1);
        }
	};


    std::shared_ptr<async::TaskWithResult<mitk::BaseData::Pointer>>
		IMeshingKernel::createMeshSolidTask(const mitk::BaseData::Pointer& solid, const GlobalMeshingParameters& params,
		const std::map<FaceIdentifier, LocalMeshingParameters>& localParams,
		const std::map<VesselPathAbstractData::VesselPathUIDType, std::string>& vesselUIDtoNameMap)
	{
		return std::static_pointer_cast<crimson::async::TaskWithResult<mitk::BaseData::Pointer>>(
			std::make_shared<OCCMeshingTask>(solid, params, localParams, vesselUIDtoNameMap));
	}


    class AdaptMeshTask : public crimson::async::TaskWithResult<mitk::BaseData::Pointer>
	{
		using GradientType = Eigen::Vector3d;
		using HessianType = Eigen::Matrix<double, 6, 1>;

	public:
		AdaptMeshTask(const mitk::BaseData::Pointer& originalMesh, double factor, double hmin, double hmax,
			const std::string& errorIndicatorArrayName)
			: originalMesh(dynamic_cast<MeshData*>(originalMesh.GetPointer()))
			, factor(factor)
			, hmax(hmax)
			, hmin(hmin)
			, errorIndicatorArrayName(errorIndicatorArrayName)
		{
		}
		~AdaptMeshTask() { }

		void cancel() override
		{
			crimson::async::TaskWithResult<mitk::BaseData::Pointer>::cancel();
		}

		std::tuple<State, std::string> runTask() override
		{
            stepsAddedSignal(6);

			if (originalMesh.IsNull()) {
				return std::make_tuple(State_Failed,
					std::string("Original mesh was not created with the meshing kernel other than ."));
			}

			try {
                MITK_INFO << "Computing size field";
                setSizeFieldUsingHessians();

                progressMadeSignal(1);
                if (isCancelling()) {
                    return std::make_tuple(State_Cancelled, std::string("Operation cancelled."));
                }

                auto runTetGen = [&](vtkUnstructuredGridBase* input) {
                    // *******
                    vtkNew<crimsonTetGenWrapper> mesher;
                    mesher->SetInputData(input);
                    mesher->SetCellEntityIdsArrayName("Face IDs");
                    mesher->SetSizingFunctionArrayName("Target size");
                    mesher->SetUseSizingFunction(1);
                    mesher->SetOrder(1);
                    mesher->SetQuality(1);
                    mesher->SetRefine(1);
                    mesher->SetCoarsen(0); // TetGen sometimes crashes when coarsening the outer surface.
                    mesher->SetNoBoundarySplit(0); 
                    mesher->SetRemoveSliver(0);
                    mesher->SetOutputSurfaceElements(1);
                    mesher->SetOutputVolumeElements(1);
                    mesher->SetVerbose(1);
                    mesher->Update();

                    return vtkSmartPointer<vtkUnstructuredGrid>(mesher->GetOutput());
                };

                progressMadeSignal(1);
                if (isCancelling()) {
                    return std::make_tuple(State_Cancelled, std::string("Operation cancelled."));
                }
                MITK_INFO << "Adapting mesh";

                auto newUnstructuredGrid = runTetGen(originalMesh->getUnstructuredGridRepresentation()->GetVtkUnstructuredGrid());
                originalMesh->getUnstructuredGridRepresentation()->GetVtkUnstructuredGrid()->GetPointData()->RemoveArray("Target size");


                if (newUnstructuredGrid->GetNumberOfCells() == 0) {
                    return std::make_pair(State_Failed, std::string("TetGen failed, see log for details."));
                }

				int timeStep = 0;
				originalMesh->GetPropertyList()->GetIntProperty("timeStep", timeStep);

                progressMadeSignal(1);
                if (isCancelling()) {
                    return std::make_tuple(State_Cancelled, std::string("Operation cancelled."));
                }
                MITK_INFO << "Transferring the solution";

				// Transfer solution
				vtkUnstructuredGridBase* originalUnstructuredGrid =
					originalMesh->getUnstructuredGridRepresentation()->GetVtkUnstructuredGrid();

				vtkNew<vtkProbeFilter> solutionTransferFilter;
				solutionTransferFilter->SetSourceData(originalUnstructuredGrid);
				solutionTransferFilter->SetInputData(newUnstructuredGrid);
				solutionTransferFilter->Update();

                progressMadeSignal(1);
                if (isCancelling()) {
                    return std::make_tuple(State_Cancelled, std::string("Operation cancelled."));
                }
                MITK_INFO << "Transferring the solution for nodes outside original mesh domain";

				// For nodes that were snapped to the model surface that are outside of original mesh - just find closest points
				vtkPointData* transferredPointData = solutionTransferFilter->GetOutput()->GetPointData();
				vtkDataArray* validPointMaskArray =
					transferredPointData->GetArray(solutionTransferFilter->GetValidPointMaskArrayName());
				for (int i = 0; i < newUnstructuredGrid->GetNumberOfPoints(); ++i) {
					if (validPointMaskArray->GetComponent(i, 0) != 0) {
						continue;
					}
					// Failed the solution transfer - find the closest point
					int closestPointId = originalUnstructuredGrid->FindPoint(newUnstructuredGrid->GetPoint(i));

					for (int arrayIndex = 0; arrayIndex < originalUnstructuredGrid->GetPointData()->GetNumberOfArrays();
						++arrayIndex) {
						transferredPointData->GetArray(arrayIndex)
							->SetTuple(i, originalUnstructuredGrid->GetPointData()->GetArray(arrayIndex)->GetTuple(closestPointId));
					}
				}
				transferredPointData->RemoveArray(solutionTransferFilter->GetValidPointMaskArrayName());

                for (int i = 0; i < originalUnstructuredGrid->GetCellData()->GetNumberOfArrays(); ++i) {
                    transferredPointData->RemoveArray(originalUnstructuredGrid->GetCellData()->GetArrayName(i));
                }

                newUnstructuredGrid->GetPointData()->ShallowCopy(transferredPointData);

                progressMadeSignal(1);
                if (isCancelling()) {
                    return std::make_tuple(State_Cancelled, std::string("Operation cancelled."));
                }
                MITK_INFO << "Transferring original face identifiers";

                vtkSmartPointer<vtkPolyData> pd = originalMesh->getSurfaceRepresentation()->GetVtkPolyData();

                vtkDataArray* outArray = newUnstructuredGrid->GetCellData()->GetArray("Face IDs");

                for (int i = 0; i < newUnstructuredGrid->GetNumberOfCells(); ++i) {
                    if (newUnstructuredGrid->GetCellType(i) == VTK_TETRA) {
                        outArray->SetComponent(i, 0, -1);
                    } else if (outArray->GetComponent(i, 0) == -1) {
                        outArray->SetComponent(i, 0, 0);
                    }
                }

                // Finally, create the output MITK object
                auto ug = mitk::UnstructuredGrid::New();
                ug->SetVtkUnstructuredGrid(newUnstructuredGrid);
                auto result = MeshData::New();
                result->setFaceIdentifierMap(originalMesh->getFaceIdentifierMap());
                result->setUnstructuredGrid(ug, true);
                this->setResult(result.GetPointer());

                progressMadeSignal(1);

				MITK_INFO << "Adaptation complete";

				setResult(result.GetPointer());
				return std::make_tuple(State_Finished, std::string("Adaptation complete."));
			}
			catch (...) {
				return std::make_tuple(State_Failed, std::string("Unhandled exception caught"));
			}
		}

		// Build a linear system using the coordinates of the nodes
		Eigen::Matrix4d buildSystem(vtkCell* cell)
		{
            auto ug = originalMesh->getUnstructuredGridRepresentation()->GetVtkUnstructuredGrid();
            Eigen::Matrix4d m;

			for (int i = 0; i < 4; ++i) {
				double X[3];
                ug->GetPoint(cell->GetPointId(i), X);

				m(i, 0) = 1;
				for (int j = 0; j < 3; ++j) {
					m(i, j + 1) = X[j];
				}
			}

			return m;
		}

		// reconstruct the element gradient
		GradientType elementGradient(vtkCell* tet)
		{
			// build the linear system
			Eigen::Matrix4d matrix = buildSystem(tet);

			// get the field vals
			Eigen::Vector4d rhs;
			auto errorIndicatorData =
				static_cast<vtkDoubleArray*>(originalMesh->getPointData()->GetArray(errorIndicatorArrayName.c_str()));
			for (int i = 0; i < 4; i++) {
				rhs(i) = errorIndicatorData->GetComponent(tet->GetPointId(i), 0);
			}

			return matrix.colPivHouseholderQr()
				.solve(rhs)
				.tail<3>(); // Solve the system and take the solution from the last 3 elements
		}

		// compute the element gradient
		HessianType elementHessian(vtkCell* tet)
		{
			// build the linear system
			Eigen::Matrix4d matrix = buildSystem(tet);

			auto solverMethod = matrix.colPivHouseholderQr();

			// get the field vals
			Eigen::Vector4d hessianComponents[3];
            for (int gradientCoord = 0; gradientCoord < 3; ++gradientCoord) {
				Eigen::Vector4d rhs;
				for (int i = 0; i < 4; i++) {
					rhs(i) = nodalGradients[tet->GetPointId(i)](gradientCoord); // speed component
				}
				hessianComponents[gradientCoord] = solverMethod.solve(rhs);
			}

			HessianType hessian;
			hessian(0) = hessianComponents[0](1); // xx
			hessian(1) = hessianComponents[0](2); // xy
			hessian(2) = hessianComponents[0](3); // xz
			hessian(3) = hessianComponents[1](2); // yy
			hessian(4) = hessianComponents[1](3); // yz
			hessian(5) = hessianComponents[2](3); // zz

			return hessian;
		}

		// Compute values for all elements in the mesh
		template <typename DataType, typename Functor>
		void computeElementValues(std::vector<DataType>& outputElementValues, Functor&& computeFunction)
		{
			outputElementValues.resize(originalMesh->getNElements());
            auto ug = originalMesh->getUnstructuredGridRepresentation()->GetVtkUnstructuredGrid();
            for (int i = 0; i < originalMesh->getNElements(); ++i) {
                vtkSmartPointer<vtkCell> tet = ug->GetCell(i);
                Expects(tet->GetCellType() == VTK_TETRA);

                outputElementValues[i] = computeFunction(tet.GetPointer());
            }
		}

		// Compute values at nodes using element values using volume-wieghted averaging
		template <typename DataType>
		void computeNodalValuesFromElementValues(std::vector<DataType>& outputNodalValues,
			const std::vector<DataType>& elementValues)
		{
			outputNodalValues.resize(originalMesh->getNNodes());
            auto ug = originalMesh->getUnstructuredGridRepresentation()->GetVtkUnstructuredGrid();
            Expects(ug->GetNumberOfPoints() == originalMesh->getNNodes());

			// loop over vertices and get patch of elements
            vtkNew<vtkIdList> pointIdList;
            pointIdList->SetNumberOfIds(1);

            vtkNew<vtkIdList> neighborCells;
            for (int vertexId = 0; vertexId < ug->GetNumberOfPoints(); ++vertexId) {
                pointIdList->SetId(0, vertexId);
                ug->GetCellNeighbors(-1, pointIdList.Get(), neighborCells.Get());

                double patchVolume = 0;

                DataType patchValue;
                patchValue.fill(0);

                // loop over elements and recover gradient at vertex
                for (int cellId = 0; cellId < neighborCells->GetNumberOfIds(); cellId++) {
                    vtkSmartPointer<vtkCell> cell = ug->GetCell(neighborCells->GetId(cellId));
                    if (cell->GetCellType() != VTK_TETRA) {
                        continue;
                    }

                    double tetVolume = vtkMeshQuality::TetVolume(cell);
                    patchVolume += tetVolume;

                    // get element gradient or hessian for each element in the patch at vertex
                    patchValue += elementValues[neighborCells->GetId(cellId)] * tetVolume;
                }

                // attach the recovered gradient to the vertex
                outputNodalValues[vertexId] = patchValue / patchVolume;
            }
		}

		// simple average over a patch surrounding the vertex
		void smoothHessians()
		{
			std::vector<HessianType> smoothedHessians(nodalHessians.size());

			// keep the vals in memory before finally setting them
            auto ug = originalMesh->getUnstructuredGridRepresentation()->GetVtkUnstructuredGrid();

            // loop over vertices and get patch of elements
            vtkNew<vtkIdList> pointIdList;
            pointIdList->SetNumberOfIds(1);

            vtkNew<vtkIdList> neighborCells;
            for (int vertexId = 0; vertexId < ug->GetNumberOfPoints(); ++vertexId) {
                pointIdList->SetId(0, vertexId);
                ug->GetCellNeighbors(-1, pointIdList.Get(), neighborCells.Get());

                std::unordered_set<vtkIdType> allIds;
                std::unordered_set<vtkIdType> surfaceIds;

                allIds.insert(vertexId);
                for (int cellId = 0; cellId < neighborCells->GetNumberOfIds(); cellId++) {
                    vtkSmartPointer<vtkCell> cell = ug->GetCell(neighborCells->GetId(cellId));
                    for (int neighVertexId = 0; neighVertexId < cell->GetNumberOfPoints(); ++neighVertexId) {
                        (cell->GetCellType() == VTK_TETRA ? allIds : surfaceIds).insert(cell->GetPointId(neighVertexId));
                    }
                }

                for (vtkIdType id : surfaceIds) {
                    allIds.erase(id);
                }

                if (allIds.empty()) {
// 					MITK_WARN << "\nerror in SmoothHessians: there is a boundary vertex whose\n"
// 						<< "neighbors are exclusively classified on model faces/edges/vertices\n"
// 						<< "and NOT in the interior.\n";
					smoothedHessians[vertexId] = nodalHessians[vertexId];
					continue;
				}

                HessianType averageHessian;
                averageHessian.fill(0);

                for (vtkIdType id : allIds) {
                    averageHessian += nodalHessians[id];
                }

                smoothedHessians[vertexId] = averageHessian / allIds.size();
			}

			nodalHessians = smoothedHessians;
		}

		// relative interpolation error along an edge
		double E_error(vtkIdType v1, vtkIdType v2, const Eigen::Matrix3d& H)
		{
			Eigen::Vector3d vertexCoords[2];

            auto ug = originalMesh->getUnstructuredGridRepresentation()->GetVtkUnstructuredGrid();

            ug->GetPoint(v1, &vertexCoords[0](0));
            ug->GetPoint(v2, &vertexCoords[1](0));

			Eigen::Vector3d edgeVector = vertexCoords[1] - vertexCoords[0];

			double localError = 0;
			for (int i = 0; i < 3; i++) {
				for (int j = 0; j < 3; j++) {
					localError += H(i, j) * edgeVector(i) * edgeVector(j);
				}
			}

			return abs(localError);
		}

		// max relative interpolation error at a vertex
		double maxLocalError(vtkIdType vertexId, const Eigen::Matrix3d& H)
		{
			double maxLocE = 0;

            auto ug = originalMesh->getUnstructuredGridRepresentation()->GetVtkUnstructuredGrid();

            vtkNew<vtkIdList> pointIdList;
            pointIdList->SetNumberOfIds(1);

            vtkNew<vtkIdList> neighborCells;

            pointIdList->SetId(0, vertexId);
            ug->GetCellNeighbors(-1, pointIdList.Get(), neighborCells.Get());

            std::unordered_set<vtkIdType> allIds;
            for (int cellId = 0; cellId < neighborCells->GetNumberOfIds(); cellId++) {
                vtkSmartPointer<vtkCell> cell = ug->GetCell(neighborCells->GetId(cellId));
                for (int otherVertexId = 0; otherVertexId < cell->GetNumberOfPoints(); ++otherVertexId) {
                    allIds.insert(cell->GetPointId(otherVertexId));
                }
            }
            allIds.erase(vertexId);

			for (vtkIdType otherId : allIds) {
				maxLocE = std::max(maxLocE, E_error(vertexId, otherId, H));
			}

			return maxLocE;
		}

		void setSizeFieldUsingHessians()
		{
			MITK_INFO << "Computing per-element Hessians";
			computeHessians();

			double tol = 1.e-12;
			double totalError = 0;                                     // total error for all vertices
			double localErrorMax = 0.;                                 // max local error
			double localErrorMin = std::numeric_limits<double>::max(); // min local error

			int hminCount = 0;
			int hmaxCount = 0;
			int bothHminHmaxCount = 0;
			int bdryNumNodes = 0;

			std::vector<std::tuple<Eigen::Vector3d, Eigen::Matrix3d>> eigenValuesVectors(nodalHessians.size());

            auto ug = originalMesh->getUnstructuredGridRepresentation()->GetVtkUnstructuredGrid();

            vtkNew<vtkDoubleArray> arr;
            arr->SetNumberOfTuples(ug->GetNumberOfPoints());
            arr->SetName("Target size");

            for (int vertexIndex = 0; vertexIndex < ug->GetNumberOfPoints(); ++vertexIndex) {
                // Hessian in the symmetric matrix form
                Eigen::Matrix3d hessian;

                hessian(0, 0) = nodalHessians[vertexIndex](0);
                hessian(0, 1) = hessian(1, 0) = nodalHessians[vertexIndex](1);
                hessian(0, 2) = hessian(2, 0) = nodalHessians[vertexIndex](2);
                hessian(1, 1) = nodalHessians[vertexIndex](3);
                hessian(1, 2) = hessian(2, 1) = nodalHessians[vertexIndex](4);
                hessian(2, 2) = nodalHessians[vertexIndex](5);

                // compute eigen values and eigen vectors
                Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> eigensolver(hessian);

                if (eigensolver.info() != Eigen::Success) {
                    MITK_WARN << "Failed to find eigenvectors for the hessian for node " << vertexIndex;
                    continue;
                }

                auto eigenValuesAbsolute = eigensolver.eigenvalues().cwiseAbs();

                if (eigenValuesAbsolute.maxCoeff() < tol) {
                    eigenValuesVectors[vertexIndex] = std::make_tuple(eigenValuesAbsolute, Eigen::Matrix3d::Zero());

                    MITK_WARN << "Zero maximum eigenvalue for node " << vertexIndex;
                    continue;
                }

                eigenValuesVectors[vertexIndex] = std::make_tuple(eigenValuesAbsolute, eigensolver.eigenvectors());

                // estimate relative interpolation error
                // needed for scaling metric field (mesh size field)
                // to get an idea refer Appendix A in Li's thesis
                double localError = maxLocalError(vertexIndex, hessian);
                totalError += localError;
                localErrorMax = std::max(localErrorMax, localError);
                localErrorMin = std::min(localErrorMin, localError);
            }

            originalMesh->getPointData()->AddArray(arr.GetPointer());

			double averageError = totalError / originalMesh->getNNodes();
			MITK_INFO << "Info on relative interpolation error: ";
			MITK_INFO << "   total: " << totalError;
			MITK_INFO << "   mean:  " << averageError;
			MITK_INFO << "   max local: " << localErrorMax;
			MITK_INFO << "   min local: " << localErrorMin;

			double targetLocalError = averageError * factor;

			MITK_INFO << " towards uniform local error distribution of " << targetLocalError;
			MITK_INFO << "   with max edge length=" << hmax << " min edge length=" << hmin;

            for (int vertexIndex = 0; vertexIndex < ug->GetNumberOfPoints(); ++vertexIndex) {
				double tol2 = 0.01 * hmax;
				double tol3 = 0.01 * hmin;

				Eigen::Vector3d eigenValues;
				Eigen::Matrix3d eigenVectors;
				std::tie(eigenValues, eigenVectors) = eigenValuesVectors[vertexIndex];

				double directionalSize[3];

				for (int j = 0; j < 3; j++) {
					if (eigenValues(j) < tol)
						directionalSize[j] = hmax;
					else {
						directionalSize[j] = std::max(hmin, std::min(hmax, sqrt(targetLocalError / eigenValues(j))));
					}
				}

                arr->SetTuple1(vertexIndex, *std::min_element(directionalSize, directionalSize + 3));

				bool foundHmin = false;
				bool foundHmax = false;
				for (int j = 0; j < 3; j++) {
					if (abs(directionalSize[j] - hmax) <= tol2) {
						foundHmax = true;
					}
					if (abs(directionalSize[j] - hmin) <= tol3) {
						foundHmin = true;
					}
				}
				if (foundHmin) {
					hminCount++;
				}
				if (foundHmax) {
					hmaxCount++;
				}
				if (foundHmin && foundHmax) {
					bothHminHmaxCount++;
				}
			}

			MITK_INFO << "Nodes with hmin into effect : " << hminCount << endl;
			MITK_INFO << "Nodes with hmax into effect : " << hmaxCount << endl;
			MITK_INFO << "Nodes with both hmin/hmax into effect : " << bothHminHmaxCount << endl;
			MITK_INFO << "(No nodes are ignored in boundary layer for CGALVMTK mesher)" << endl << endl;

			nodalHessians.clear();
		}

		void computeHessians()
		{
			std::vector<GradientType> elementGradients;
			computeElementValues<GradientType>(elementGradients, [this](vtkCell* r) { return elementGradient(r); });
			computeNodalValuesFromElementValues(nodalGradients, elementGradients);
			elementGradients.clear(); // Element gradients are no longer needed

            // vtkNew<vtkDoubleArray> arr;
            // arr->SetNumberOfComponents(3);
            // arr->SetNumberOfTuples(nodalGradients.size());
            // arr->SetName("grad");
            // 
            // for (int i = 0; i < nodalGradients.size(); ++i) {
            //     for (int j = 0; j < 3; ++j) {
            //         arr->SetComponent(i, j, nodalGradients[i](j));
            //     }
            // }
            // originalMesh->getPointData()->AddArray(arr.GetPointer());

			std::vector<HessianType> elementHessians;
			computeElementValues<HessianType>(elementHessians, [this](vtkCell* r) { return elementHessian(r); });
			computeNodalValuesFromElementValues(nodalHessians, elementHessians);
			elementHessians.clear(); // Element hessians are no longer needed
			nodalGradients.clear();  // Nodal gradients are no longer needed

			smoothHessians();

			// Now the nodal hessians are ready to use in the adaptation procedure
		}
	private:
		MeshData::Pointer originalMesh;

		double factor;
		double hmax;
		double hmin;
		std::string errorIndicatorArrayName;

		std::vector<GradientType> nodalGradients;
		std::vector<HessianType> nodalHessians;
	};

    std::shared_ptr<async::TaskWithResult<mitk::BaseData::Pointer>>
		IMeshingKernel::createAdaptMeshTask(const mitk::BaseData::Pointer& originalMesh, double factor, double hmin, double hmax,
		const std::string& errorIndicatorArrayName)
	{
    	return std::static_pointer_cast<crimson::async::TaskWithResult<mitk::BaseData::Pointer>>(
		    std::make_shared<AdaptMeshTask>(originalMesh, factor, hmin, hmax, errorIndicatorArrayName));
	}

} // namespace crimson
