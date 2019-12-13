// This will be an interface for solid model service and blueberry extension point etc.

#include <gsl.h>

#include <queue>
#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/map.hpp>

#include "ISolidModelKernel.h"
#include "internal/ContourParametrization.h"

#include "OCCBRepData.h"

#include <mitkPlanarCircle.h>
#include <mitkPlanarEllipse.h>
#include <mitkPlaneGeometry.h>

#include <vtkDistancePolyDataFilter.h>
#include <vtkPointData.h>
#include <vtkNew.h>

// OpenCASCADE
#include <OSD.hxx>
#include <Standard_ErrorHandler.hxx>

#include <TColgp_HArray1OfPnt2d.hxx>

#include <Geom2d_BSplineCurve.hxx>
#include <Geom2dConvert.hxx>
#include <Geom2dConvert_ApproxCurve.hxx>
#include <Geom2dAPI_Interpolate.hxx>
#include <Geom2dAPI_ProjectPointOnCurve.hxx>
#include <Geom_Plane.hxx>
#include <Geom_BSplineSurface.hxx>
#include <GeomFill_SectionGenerator.hxx>
#include <GeomFill_Line.hxx>
#include <GeomFill_AppSurf.hxx>

#include <GCE2d_MakeCircle.hxx>
#include <GCE2d_MakeEllipse.hxx>

#include <GProp_GProps.hxx>
#include <GCPnts_AbscissaPoint.hxx>

#include <BRepLib.hxx>
#include <BRep_Tool.hxx>
#include <BRepGProp.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepBuilderAPI_FindPlane.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepOffsetAPI_Sewing.hxx>
#include <BRepBndLib.hxx>
#include <BRepExtrema_ShapeProximity.hxx>
#include <BndLib_Add2dCurve.hxx>
#include <Bnd_Box2d.hxx>
#include <BRepTools.hxx>

#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Edge.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>

#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>

#include <Geom_BSplineCurve.hxx>
#include <GeomAdaptor_HCurve.hxx>
#include <GeomAPI_Interpolate.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <GeomConvert_ApproxCurve.hxx>
#include <TColgp_HArray1OfPnt.hxx>
#include <TColStd_HArray1OfBoolean.hxx>
#include <TColgp_Array1OfVec.hxx>
#include <BRep_Builder.hxx>
#include <BRepFill_Section.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>
#include <Bnd_Box.hxx>
#include <Message_ProgressIndicator.hxx>

#include <internal/BRepFill_PipeShellA.hxx>
#include <internal/GeomFill_GuideTrihedronAC_A.hxx>

#include <mitkContourModelSet.h>
#include <mitkContourModel.h>

#include <Wm5IntrRay3Plane3.h>

#ifdef _DEBUG
#endif // _DEBUG

#include <BSplCLib.hxx>
#include <chrono>
#include <numeric>

namespace crimson
{

static void setupCatchSystemSignals()
{
    static bool catchSignalsWasSetup = false;
    if (!catchSignalsWasSetup) {
        catchSignalsWasSetup = true;
        OSD::SetSignal();
    }
}

//=======================================================================
// function :  PerformPlan
// purpose  : Construct a plane of filling if exists
// RK: copied from OpenCASCADE's BRepOffsetAPI_ThruSections.cxx
//=======================================================================

static Standard_Boolean PerformPlan(const TopoDS_Wire& W, const Standard_Real presPln, TopoDS_Face& theFace)
{
    Standard_Boolean isDegen = Standard_True;
    TopoDS_Iterator iter(W);
    for (; iter.More(); iter.Next()) {
        const TopoDS_Edge& anEdge = TopoDS::Edge(iter.Value());
        if (!BRep_Tool::Degenerated(anEdge))
            isDegen = Standard_False;
    }
    if (isDegen)
        return Standard_True;

    Standard_Boolean Ok = Standard_False;
    if (!W.IsNull()) {
        BRepBuilderAPI_FindPlane Searcher(W, presPln);
        if (Searcher.Found()) {
            theFace = BRepBuilderAPI_MakeFace(Searcher.Plane(), W);
            Ok = Standard_True;
        } else // try to find another surface
        {
            BRepBuilderAPI_MakeFace MF(W);
            if (MF.IsDone()) {
                theFace = MF.Face();
                Ok = Standard_True;
            }
        }
    }

    return Ok;
}

//=============================================================================
// function :  IsSameOriented
// purpose  : Checks whether aFace is oriented to the same side as aShell or not
// RK: copied from OpenCASCADE's BRepOffsetAPI_ThruSections.cxx
//=============================================================================

static Standard_Boolean IsSameOriented(const TopoDS_Shape& aFace, const TopoDS_Shape& aShell)
{
    TopExp_Explorer Explo(aFace, TopAbs_EDGE);
    TopoDS_Shape anEdge = Explo.Current();
    TopAbs_Orientation Or1 = anEdge.Orientation();

    TopTools_IndexedDataMapOfShapeListOfShape EFmap;
    TopExp::MapShapesAndAncestors(aShell, TopAbs_EDGE, TopAbs_FACE, EFmap);

    const TopoDS_Shape& AdjacentFace = EFmap.FindFromKey(anEdge).First();
    TopoDS_Shape theEdge;
    for (Explo.Init(AdjacentFace, TopAbs_EDGE); Explo.More(); Explo.Next()) {
        theEdge = Explo.Current();
        if (theEdge.IsSame(anEdge))
            break;
    }

    TopAbs_Orientation Or2 = theEdge.Orientation();
    if (Or1 == Or2)
        return Standard_False;
    return Standard_True;
}

//=======================================================================
// function : CreateCapFaces
// purpose  : Create the inflow and outflow cap faces
//=======================================================================

static bool CreateCapFaces(TopoDS_Face& shell, const Standard_Real presPln, TopoDS_Face faces[2], TopoDS_Edge capEdges[2])
{
    int wireIndex = 0;
    TopoDS_Wire capWires[2];

    for (TopExp_Explorer Explo(shell, TopAbs_EDGE); Explo.More() && wireIndex < 2; Explo.Next()) {
        TopoDS_Edge edge = TopoDS::Edge(Explo.Current());
        BRepAdaptor_Curve adaptor(edge);
        gp_Pnt p1, p2;
        adaptor.D0(adaptor.FirstParameter(), p1);
        adaptor.D0(adaptor.LastParameter(), p2);
        if (p1.IsEqual(p2, 1e-4)) {
            capEdges[wireIndex] = TopoDS::Edge(Explo.Current());
            capWires[wireIndex++] = BRepBuilderAPI_MakeWire(TopoDS::Edge(Explo.Current()));
        }
    }

    for (int i = 0; i < 2; ++i) {
        if (!PerformPlan(capWires[i], presPln, faces[i]) || faces[i].IsNull()) {
            return false;
        }

        if (!IsSameOriented(faces[i], shell)) {
            faces[i].Reverse();
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
// Conversion of planar figures to OpenCASCADE BSpline curves
//////////////////////////////////////////////////////////////////////////

// Generic figure - use poly line and fit a BSplice curve
Handle(Geom2d_BSplineCurve) convertPlanarFigureToCurve(mitk::PlanarFigure* figure, const mitk::Point2D& centerPoint)
{
    mitk::PlanarFigure::PolyLineType polyLine = figure->GetPolyLine(0);

    Handle(TColgp_HArray1OfPnt2d) controlPts = new TColgp_HArray1OfPnt2d(1, polyLine.size());

    int ptIdx = 1;
    for (const mitk::PlanarFigure::PolyLineElement& polyLineElement : polyLine) {
        controlPts->SetValue(ptIdx++, gp_Pnt2d(polyLineElement[0] - centerPoint[0], polyLineElement[1] - centerPoint[1]));
    }

    // Interpolate through all the points
    Geom2dAPI_Interpolate interpolator(controlPts, true, 1e-7);
    interpolator.Perform();

    Bnd_Box2d bounds;
    BndLib_Add2dCurve::Add(interpolator.Curve(), 1e-7, bounds);
    Standard_Real xMin, xMax, yMin, yMax;
    bounds.Get(xMin, yMin, xMax, yMax);

    // Create a simplified approximated curve
    Geom2dConvert_ApproxCurve approximator(interpolator.Curve(), 0.01 * std::max(xMax - xMin, yMax - yMin), GeomAbs_C2, 1000,
                                           3);
    if (approximator.HasResult()) {
        approximator.Curve()->SetPeriodic();
        return approximator.Curve();
    }

    MITK_ERROR << "Approximator failed";

    // If approximation failed, return the interpolated curve
    return interpolator.Curve();
}

// Circle to BSpline curve
Handle(Geom2d_BSplineCurve) convertPlanarFigureToCurve(mitk::PlanarCircle* figure, const mitk::Point2D& centerPoint)
{
    float radius = figure->GetControlPoint(0).EuclideanDistanceTo(figure->GetControlPoint(1));
    gp_Pnt2d center(figure->GetControlPoint(0)[0] - centerPoint[0], figure->GetControlPoint(0)[1] - centerPoint[1]);
    return Geom2dConvert::CurveToBSplineCurve(GCE2d_MakeCircle(center, radius).Value(), Convert_RationalC1);
}

// Ellipse to BSpline curve
Handle(Geom2d_BSplineCurve) convertPlanarFigureToCurve(mitk::PlanarEllipse* figure, const mitk::Point2D& centerPoint)
{
    float r1 = figure->GetControlPoint(0).EuclideanDistanceTo(figure->GetControlPoint(1));
    float r2 = figure->GetControlPoint(0).EuclideanDistanceTo(figure->GetControlPoint(2));

    Handle(Geom2d_Curve) geomCurve;
    if (fabs(r1 - r2) < 1e-6 * r1) {
        // In case radii are equal, output a circle
        geomCurve =
            GCE2d_MakeCircle(
                gp_Pnt2d(figure->GetControlPoint(0)[0] - centerPoint[0], figure->GetControlPoint(0)[1] - centerPoint[1]), r1)
                .Value();
    } else {
        int majorIndex = r1 > r2 ? 1 : 2;
        int minorIndex = 3 - majorIndex;

        gp_Pnt2d pntMajor(figure->GetControlPoint(majorIndex)[0] - centerPoint[0],
                          figure->GetControlPoint(majorIndex)[1] - centerPoint[1]);
        gp_Pnt2d pntMinor(figure->GetControlPoint(minorIndex)[0] - centerPoint[0],
                          figure->GetControlPoint(minorIndex)[1] - centerPoint[1]);

        geomCurve = GCE2d_MakeEllipse(pntMajor, pntMinor, gp_Pnt2d(figure->GetControlPoint(0)[0] - centerPoint[0],
                                                                   figure->GetControlPoint(0)[1] - centerPoint[1]))
                        .Value();
    }

    return Geom2dConvert::CurveToBSplineCurve(geomCurve, Convert_RationalC1);
}

#define tryDynamicConversionFrom(Type)                                                                                         \
    do {                                                                                                                       \
        if (dynamic_cast<Type*>(figure)) {                                                                                     \
            return convertPlanarFigureToCurve(static_cast<Type*>(figure), centerPoint);                                        \
        }                                                                                                                      \
    } while (false)

Handle(Geom2d_BSplineCurve) convertAnyPlanarFigureToCurve(mitk::PlanarFigure* figure, const mitk::Point2D& centerPoint)
{
    tryDynamicConversionFrom(mitk::PlanarCircle); 
    tryDynamicConversionFrom(mitk::PlanarEllipse);

    return convertPlanarFigureToCurve(figure, centerPoint);
}

#undef tryDynamicConversionFrom

//////////////////////////////////////////////////////////////////////////
// Lofted solid creation
//////////////////////////////////////////////////////////////////////////

class LoftingTask : public crimson::async::TaskWithResult<mitk::BaseData::Pointer>
{
public:
    LoftingTask(const crimson::VesselPathAbstractData* vesselPath, const crimson::ISolidModelKernel::ContourSet& contours,
                bool useInflowAsWall, bool useOutflowAsWall, ISolidModelKernel::LoftingAlgorithm algorithm,
                double seamEdgeRotation, bool preview, double interContourDistance)
        : vesselPath(vesselPath)
        , contours(contours)
        , useInflowAsWall(useInflowAsWall)
        , useOutflowAsWall(useOutflowAsWall)
        , loftingAlgorithm(algorithm)
        , seamEdgeRotation(seamEdgeRotation)
        , preview(preview)
        , interContourDistance(interContourDistance)
    {
    }

    std::tuple<State, std::string> runTask() override
    {
        setupCatchSystemSignals();

        int progressConvert = contours.size();
        int progressLoft = 20;
        int progressMesh = 10;
        int progressLeft = progressConvert + progressLoft + progressMesh;

        stepsAddedSignal(progressLeft);

        int maxRiskContourIndex = -1;
        mitk::Point2D maxRiskPoint(0);

        // Find a contour with highest risk of self-intersection after lofting
        if (loftingAlgorithm == crimson::ISolidModelKernel::laSweep) {
            double maxRisk = 0;
            for (size_t i = 0; i < contours.size(); ++i) {
                mitk::Point2D p;
                double risk = getHighestRisk(vesselPath, contours[i], p);
                if (risk > maxRisk) {
                    maxRiskContourIndex = i;
                    maxRiskPoint = p;
                    maxRisk = risk;
                }
            }

            if (maxRisk <= 1) {
                maxRiskContourIndex = -1;
            }
        }

        // Compute the parametrization start points which define the seam edge
        std::vector<mitk::Point2D> parametrizationStartPoints = computeContourParametrizationStartPoints(
            contours, maxRiskContourIndex, maxRiskPoint, seamEdgeRotation / 180.0 * vnl_math::pi);
        Handle(TColgp_HArray1OfPnt) startPoints = new TColgp_HArray1OfPnt(1, contours.size());
        Handle(TColgp_HArray1OfPnt) centerPoints = new TColgp_HArray1OfPnt(1, contours.size());
        std::vector<TopoDS_Edge> contourEdges;
        std::vector<double> contourParameterValues;

        GeomFill_SectionGenerator aSecGenerator;

        if (isCancelling()) {
            return std::make_tuple(State_Cancelled, std::string("Operation cancelled."));
        }

        try {
            OCC_CATCH_SIGNALS;

            for (size_t i = 0; i < contours.size(); ++i) {
                const mitk::PlanarFigure::Pointer& figure = contours[i];

                // Get the point of intersection of vessel path and contour-defined geometry
                float contourParameterValue;
                figure->GetPropertyList()->GetFloatProperty("lofting.parameterValue", contourParameterValue);

                mitk::Point3D contourGeometryOrigin3D = vesselPath->getPosition(contourParameterValue);
                mitk::Point2D contourGeometryOrigin2D;
                figure->GetPlaneGeometry()->Map(contourGeometryOrigin3D, contourGeometryOrigin2D);

                // Use that point of intersection as center for the curve
                Handle(Geom2d_BSplineCurve) curve2d =
                    convertAnyPlanarFigureToCurve(figure.GetPointer(), contourGeometryOrigin2D);

                // Create the 3D edge from a 2D curve
                gp_Ax3 xyz(gp::XOY());
                Handle(Geom_Plane) xyzPlane = new Geom_Plane(xyz);
                TopoDS_Edge edge2d = BRepBuilderAPI_MakeEdge(curve2d, xyzPlane);
                BRepLib::BuildCurves3d(edge2d);

                // Get the starting point computed outside
                mitk::Point2D startPtMitk = parametrizationStartPoints[i] - contourGeometryOrigin2D;

                // Find the parametric value and the exact position of the starting point on the (potentially approximated)
                // curve
                Geom2dAPI_ProjectPointOnCurve projector(gp_Pnt2d(startPtMitk[0], startPtMitk[1]), curve2d);
                double parameterValue = projector.LowerDistanceParameter();
                gp_Pnt2d startPt = projector.NearestPoint();

                // Get the coordinates of the point slightly offset towards the parameter growth direction
                gp_Pnt2d offsetPt;
                curve2d->D0(parameterValue + 0.01 * (curve2d->LastParameter() - curve2d->FirstParameter()), offsetPt);

                // Compute the need for parameter reversal of the curve based on the sign of cross product of vectors from
                // the center of mass to the start and offset points
                mitk::Vector3D v1;
                mitk::FillVector3D(v1, startPt.X(), startPt.Y(), 0);

                mitk::Vector3D v2;
                mitk::FillVector3D(v2, offsetPt.X(), offsetPt.Y(), 0);

                bool needReverse = itk::CrossProduct(v1, v2)[2] < 0;

                // Determine if a new knot should be inserted to set the start of contour parametrization to start point
                int i1, i2;
                curve2d->LocateU(parameterValue, 1e-7, i1, i2);
                if (i1 != i2) {
                    curve2d->InsertKnot(parameterValue);
                    curve2d->LocateU(parameterValue, 1e-7, i1, i2);
                }

                // Set the parametrization start point and reverse the curve direction if necessary
                curve2d->SetOrigin(i1);
                if (needReverse) {
                    curve2d->Reverse();
                }

                TColStd_Array1OfReal knots(1, curve2d->NbKnots());
                curve2d->Knots(knots);
                BSplCLib::Reparametrize(0, 1, knots);
                curve2d->SetKnots(knots);

                // Finally, create the full 3D curve using the figure geometry information
                const mitk::PlaneGeometry* figureGeometry = figure->GetPlaneGeometry();
                gp_Ax3 cs;
                cs.SetLocation(gp_Pnt(contourGeometryOrigin3D[0], contourGeometryOrigin3D[1], contourGeometryOrigin3D[2]));
                cs.SetDirection(
                    gp_Dir(figureGeometry->GetNormal()[0], figureGeometry->GetNormal()[1], figureGeometry->GetNormal()[2]));
                cs.SetXDirection(gp_Dir(figureGeometry->GetAxisVector(0)[0], figureGeometry->GetAxisVector(0)[1],
                                        figureGeometry->GetAxisVector(0)[2]));

                Handle(Geom_Plane) p = new Geom_Plane(gp_Ax3());
                TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(curve2d, p);
                BRepLib::BuildCurves3d(edge);
                gp_Trsf transform;
                transform.SetTransformation(cs, gp_Ax3());
                edge.Location(TopLoc_Location(transform));

                double firstParam, lastParam;
                Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, firstParam, lastParam);

                if (loftingAlgorithm == crimson::ISolidModelKernel::laSweep) {
                    // Get the start point in 3D and save for interpolation
                    gp_Pnt startPoint3D;
                    curve->D0(firstParam, startPoint3D);
                    startPoints->SetValue(i + 1, startPoint3D);
                    centerPoints->SetValue(
                        i + 1, gp_Pnt(contourGeometryOrigin3D[0], contourGeometryOrigin3D[1], contourGeometryOrigin3D[2]));
                    contourEdges.push_back(edge);
                }
                contourParameterValues.push_back(contourParameterValue);

                // Add the created curve to the section generator
                aSecGenerator.AddCurve(curve);

                progressMadeSignal(1);
            }

            if (isCancelling()) {
                return std::make_tuple(State_Cancelled, std::string("Operation cancelled."));
            }

            // Contains either a shell for non-preview or a wire for preview
            TopoDS_Shape result;

            int nPreviewSections = static_cast<int>(
                ceil((*contourParameterValues.rbegin() - *contourParameterValues.begin()) / interContourDistance));
            nPreviewSections = std::max(2, nPreviewSections);

            if (loftingAlgorithm == crimson::ISolidModelKernel::laAppSurf) {
                aSecGenerator.Perform(1e-5);

                Handle(GeomFill_Line) aLine = new GeomFill_Line(contours.size());

                // Empirically determined GeomFill_AppSurf parameters
                Standard_Integer aMinDeg = 1, aMaxDeg = 2, aNbIt = 1;
                Standard_Real aTol3d = 1e-7, aTol2d = 1e-7;

                // Perform the GeomFill_AppSurf algorithm
                GeomFill_AppSurf anAlgo(aMinDeg, aMaxDeg, aTol3d, aTol2d, aNbIt);

                // THIS LINE MIGHT HELP FIGHTING THE FUSING PROBLEMS.
                // HOWEVER LEFT AS IS BEFORE THE ACTUAL MODEL FOR WHICH IT MATTERS APPEARS
                // TestData/Problem_blend_near_seamline2.mitk
                anAlgo.SetParType(Approx_Centripetal);

                anAlgo.SetContinuity(GeomAbs_C1);
                anAlgo.Perform(aLine, aSecGenerator, Standard_True);

                if (!anAlgo.IsDone()) {
                    return std::make_tuple(State_Failed, std::string("Lofting algorithm has failed."));
                }

                // Create the surface from the GeomFill_AppSurf output
                Handle(Geom_BSplineSurface) aRes;
                aRes =
                    new Geom_BSplineSurface(anAlgo.SurfPoles(), anAlgo.SurfWeights(), anAlgo.SurfUKnots(), anAlgo.SurfVKnots(),
                                            anAlgo.SurfUMults(), anAlgo.SurfVMults(), anAlgo.UDegree(), anAlgo.VDegree(), 1);

                if (preview) {
                    // Generate the preview contours by computing iso-contours of the computed surface
                    TopoDS_Wire previewWire;
                    BRep_Builder builder;
                    builder.MakeWire(previewWire);

                    double u1, u2, v1, v2;
                    aRes->Bounds(u1, u2, v1, v2);
                    for (int i = 0; i < nPreviewSections; ++i) {
                        double v = v1 + (v2 - v1) * i / (nPreviewSections - 1.0);
                        builder.Add(previewWire, BRepBuilderAPI_MakeEdge(aRes->VIso(v)));
                    }

                    result = previewWire;
                } else {
                    result = BRepBuilderAPI_MakeFace(aRes, 1e-7).Shape();
                }
            } else {
                // Interpolate through all the points
                GeomAPI_Interpolate interpolatorStartPoints(startPoints, false, 1e-7);
                interpolatorStartPoints.Perform();

                // Adjust the tangents to lie in the plane whose normal is from startPoint to centerPoint
                TColgp_Array1OfVec startPointTangentsOCC(1, contours.size());
                Handle(TColStd_HArray1OfBoolean) tangUsed = new TColStd_HArray1OfBoolean(1, contours.size());
                for (int i = startPoints->Lower(); i <= startPoints->Upper(); ++i) {
                    GeomAPI_ProjectPointOnCurve projector;
                    projector.Init(startPoints->Value(i), interpolatorStartPoints.Curve());
                    double parameterValue = projector.LowerDistanceParameter();

                    gp_Pnt p;
                    gp_Vec t;
                    interpolatorStartPoints.Curve()->D1(parameterValue, p, t);

                    gp_Vec n(startPoints->Value(i), centerPoints->Value(i));
                    n.Normalize();

                    n = t - n * n.Dot(t);
                    n.Normalize();

                    startPointTangentsOCC.SetValue(i, n);
                }

                interpolatorStartPoints.Load(startPointTangentsOCC, tangUsed);
                interpolatorStartPoints.Perform();

                // Create the sweeping spine from the vesselPath

                size_t nVesselPathPoints = 100;
                Handle(TColgp_HArray1OfPnt) vesselPathPoints =
                    new TColgp_HArray1OfPnt(1, (contours.size() - 1) * nVesselPathPoints + 1);

                // Last section might miss the end of trimmed vessel path. Therefore, extend the parameter range slightly.
                // Doesn't affect the end result.
                *contourParameterValues.rbegin() += 1e-3;

                mitk::Point3D p = vesselPath->getPosition(contourParameterValues[0]);
                vesselPathPoints->SetValue(1, gp_Pnt(p[0], p[1], p[2]));
                for (size_t i = 0; i < contourParameterValues.size() - 1; ++i) {
                    for (size_t j = 0; j < nVesselPathPoints; ++j) {
                        double param =
                            contourParameterValues[i] +
                            (contourParameterValues[i + 1] - contourParameterValues[i]) * (j + 1) / nVesselPathPoints;
                        mitk::Point3D point = vesselPath->getPosition(param);
                        vesselPathPoints->SetValue(i * nVesselPathPoints + (j + 1) + 1, gp_Pnt(point[0], point[1], point[2]));
                    }
                }

                GeomAPI_Interpolate interpolatorVesselPath(vesselPathPoints, false, 1e-7);
                interpolatorVesselPath.Perform();

                // Create a simplified approximated curve
                GeomConvert_ApproxCurve approximatorVesselPath(
                    interpolatorVesselPath.Curve(),
                    0.002 * (*contourParameterValues.rbegin() - *contourParameterValues.begin()), GeomAbs_C1, 1000, 5);

                std::vector<Handle(GeomFill_GuideTrihedronAC_A)> subCurveLaws;

                BRepFill_PipeShell shellBuilder(
                    BRepBuilderAPI_MakeWire(BRepBuilderAPI_MakeEdge(approximatorVesselPath.Curve())));
                shellBuilder.SetMaxDegree(3);
                shellBuilder.SetMaxSegments(1000);
                shellBuilder.SetForceApproxC1(Standard_False);
                shellBuilder.Set(BRepBuilderAPI_MakeWire(BRepBuilderAPI_MakeEdge(interpolatorStartPoints.Curve())),
                                 std::vector<Handle(GeomFill_GuideTrihedronAC_A)>());

                std::vector<double> placementParams;
                for (size_t i = 0; i < contourEdges.size(); ++i) {
                    TopoDS_Vertex v;
                    v.Nullify();
                    BRepFill_Section section(BRepBuilderAPI_MakeWire(contourEdges[i]), v, Standard_False, Standard_False);
                    TopoDS_Wire wire;
                    gp_Trsf trsf;
                    double abscissa;
                    double param = shellBuilder.Place(section, wire, trsf, abscissa);
                    placementParams.push_back(param);
                }

                subCurveLaws.clear();
                for (size_t i = 0; i < placementParams.size() - 1; ++i) {
                    GeomAPI_ProjectPointOnCurve projector;

                    projector.Init(startPoints->Value(i + 1), interpolatorStartPoints.Curve());
                    double parameterValue1 = projector.LowerDistanceParameter();
                    projector.Init(startPoints->Value(i + 2), interpolatorStartPoints.Curve());
                    double parameterValue2 = projector.LowerDistanceParameter();

                    Handle(GeomFill_GuideTrihedronAC_A) law = new GeomFill_GuideTrihedronAC_A(
                        new GeomAdaptor_HCurve(interpolatorStartPoints.Curve(), parameterValue1, parameterValue2));
                    law->SetCurve(
                        new GeomAdaptor_HCurve(approximatorVesselPath.Curve(), placementParams[i], placementParams[i + 1]));
                    subCurveLaws.push_back(law);
                }

                BRepFill_PipeShell shellBuilder2(BRepBuilderAPI_MakeWire(BRepBuilderAPI_MakeEdge(
                    approximatorVesselPath.Curve(), *placementParams.begin(), *placementParams.rbegin())));
                shellBuilder2.SetMaxDegree(3);
                shellBuilder2.SetMaxSegments(1000);
                shellBuilder2.SetForceApproxC1(Standard_False);
                shellBuilder2.SetTolerance(1e-1, 1e-1, 1e-1);
                shellBuilder2.Set(BRepBuilderAPI_MakeWire(BRepBuilderAPI_MakeEdge(interpolatorStartPoints.Curve())),
                                  subCurveLaws);

                for (size_t i = 0; i < contourEdges.size(); ++i) {
                    shellBuilder2.Add(BRepBuilderAPI_MakeWire(contourEdges[i]));
                }

                if (preview) {
                    TopTools_ListOfShape simulationShapes;
                    shellBuilder2.Simulate(nPreviewSections, simulationShapes);

                    TopoDS_Wire previewWire;
                    BRep_Builder builder;
                    builder.MakeWire(previewWire);

                    while (!simulationShapes.IsEmpty()) {
                        builder.Add(previewWire, TopoDS_Iterator(TopoDS::Wire(simulationShapes.First())).Value());
                        simulationShapes.RemoveFirst();
                    }

                    result = previewWire;
                } else {
                    shellBuilder2.Build();
                    result = TopoDS::Face(TopExp_Explorer(shellBuilder2.Shape(), TopAbs_FACE).Current());
                }
            }

            progressMadeSignal(progressLoft);

            // Finally, create the output MITK object
            mitk::BaseData::Pointer output;

            if (!preview) {
                // Create the solid from the outer surface by capping it
                TopoDS_Face capFaces[2];
                TopoDS_Edge capEdges[2];
                if (CreateCapFaces(TopoDS::Face(result), 1e-7, capFaces, capEdges)) {
                    BRepOffsetAPI_Sewing sewing(1e-7);

                    sewing.Add(TopoDS::Face(result));
                    sewing.Add(capFaces[0]);
                    sewing.Add(capFaces[1]);
                    sewing.Perform();
                    result = BRepBuilderAPI_MakeSolid(TopoDS::Shell(sewing.SewedShape()));
                } else {
                    MITK_ERROR << "Failed to create the cap faces";
                }

                auto data = OCCBRepData::New();
                data->setShape(result);

                // Assign face id's
                int id = 0;
                for (TopExp_Explorer faceExplorer(result, TopAbs_FACE); faceExplorer.More(); faceExplorer.Next(), ++id) {
                    FaceIdentifier faceIdentifier;
                    faceIdentifier.parentSolidIndices.insert(vesselPath->getVesselUID());
                    if (faceExplorer.Current().TShape() == capFaces[0].TShape()) {
                        faceIdentifier.faceType = useInflowAsWall ? FaceIdentifier::ftWall : FaceIdentifier::ftCapInflow;
                        data->setInflowFaceId(id);
                    } else if (faceExplorer.Current().TShape() == capFaces[1].TShape()) {
                        faceIdentifier.faceType = useOutflowAsWall ? FaceIdentifier::ftWall : FaceIdentifier::ftCapOutflow;
                        data->setOutflowFaceId(id);
                    } else {
                        faceIdentifier.faceType = FaceIdentifier::ftWall;
                    }
                    data->getFaceIdentifierMap().setFaceIdentifierForModelFace(id, faceIdentifier);
                }

                data->getSurfaceRepresentation(); // Force meshing - for execution in different thread
                output = data.GetPointer();
            } else {
                // Create contour model set for preview
                mitk::ContourModelSet::Pointer simulatedData = mitk::ContourModelSet::New();

                for (TopoDS_Iterator iter(TopoDS::Wire(result)); iter.More(); iter.Next()) {
                    mitk::ContourModel::Pointer simulatedContour = mitk::ContourModel::New();
                    Standard_Real first = 0, last = 0;
                    Handle(Geom_Curve) curve = BRep_Tool::Curve(TopoDS::Edge(iter.Value()), first, last);

                    Bnd_Box B;
                    BRepBndLib::Add(iter.Value(), B);
                    Standard_Real aXmin, aYmin, aZmin, aXmax, aYmax, aZmax;
                    B.Get(aXmin, aYmin, aZmin, aXmax, aYmax, aZmax);
                    double deflection = std::max(aXmax - aXmin, std::max(aYmax - aYmin, aZmax - aZmin)) * 0.01;

                    GeomAdaptor_Curve adaptor(curve);
                    GCPnts_QuasiUniformDeflection discretizer(adaptor, deflection);

                    for (int i = 0; i < discretizer.NbPoints() - 1; ++i) {
                        gp_Pnt p = discretizer.Value(i + 1);
                        mitk::Point3D pmitk;
                        mitk::FillVector3D(pmitk, p.X(), p.Y(), p.Z());
                        simulatedContour->AddVertex(pmitk);
                    }
                    simulatedContour->SetClosed(true);
                    simulatedData->AddContourModel(simulatedContour);
                }
                output = simulatedData.GetPointer();
            }

            progressLeft -= progressLoft;

            if (isCancelling()) {
                return std::make_tuple(State_Cancelled, std::string("Operation cancelled."));
            }

            progressMadeSignal(progressMesh);
            progressLeft -= progressMesh;

            setResult(output.GetPointer());
            return std::make_tuple(State_Finished, std::string("Lofting finished successfully."));
        } catch (...) {
            progressMadeSignal(progressLeft);

            std::string errorMessage = "Exception caught during lofting\n";
            Handle(Standard_Failure) Fail = Standard_Failure::Caught();
            std::string occErrorMessage(Fail->GetMessageString());
            if (!occErrorMessage.empty()) {
                errorMessage += "Error message: " + occErrorMessage;
            }

            return std::make_tuple(State_Failed, std::string(errorMessage));
        }
    }

    double getHighestRisk(const crimson::VesselPathAbstractData* vesselPath, mitk::PlanarFigure* figure,
                          mitk::Point2D& maxRiskPoint)
    {
        double delta = 0.1;

        float vesselParameterValue;
        figure->GetPropertyList()->GetFloatProperty("lofting.parameterValue", vesselParameterValue);

        mitk::PlanarFigure::PolyLineType polyLine = figure->GetPolyLine(0);

        mitk::Point3D figureCenter = vesselPath->getPosition(vesselParameterValue);
        mitk::Point2D figureCenter2d;
        figure->GetPlaneGeometry()->Map(figureCenter, figureCenter2d);

        double maxRisk = -1;

        for (const mitk::Point2D& p : polyLine) {
            mitk::Vector2D offset = p - figureCenter2d;

            // Offset the parameter value along the path
            double newT = vesselParameterValue + delta;
            if (newT > vesselPath->getParametricLength()) {
                newT = vesselParameterValue - delta;
            }

            mitk::Point3D newOrigin = vesselPath->getPosition(newT);

            // Get the offset frame
            mitk::Vector3D tangent = vesselPath->getTangentVector(newT);
            mitk::Vector3D normal = vesselPath->getNormalVector(newT);
            mitk::Vector3D binormal = itk::CrossProduct(tangent, normal);

            // Get the 'corresponding' position of the point in the offset frame
            mitk::Point3D newP = newOrigin + normal * offset[0] + binormal * offset[1];

            // Compute direction from the position on spline to the 'corresponding' point
            mitk::Vector3D newDirection = newP - newOrigin;
            newDirection.Normalize();

            // Find the intersection of the ray built in the offset frame with the orignal plane
            Wm5::Vector3d origin(newOrigin[0], newOrigin[1], newOrigin[2]);
            Wm5::Vector3d direction(newDirection[0], newDirection[1], newDirection[2]);

            Wm5::Ray3d ray(origin, direction);

            Wm5::Plane3d plane(Wm5::Vector3d(figure->GetPlaneGeometry()->GetNormal()[0],
                                             figure->GetPlaneGeometry()->GetNormal()[1],
                                             figure->GetPlaneGeometry()->GetNormal()[2]),
                               Wm5::Vector3d(figureCenter[0], figureCenter[1], figureCenter[2]));

            Wm5::IntrRay3Plane3d intersector(ray, plane);

            // Risk of self intersection is defined by the length of the ray segment
            // E.g. if the path doesn't curve at all, GetRayParameter() will be infinity and risk will be 0
            //      if the path is at 90 degree angle, risk will be 1, as the segment length will be equal to the offset along
            //      the spline
            double risk = 0;
            if (intersector.Find()) {
                risk = offset.GetNorm() / intersector.GetRayParameter();
            }

            if (risk > maxRisk) {
                maxRisk = risk;
                maxRiskPoint = p;
            }
        }

        return maxRisk;
    }

private:
    const crimson::VesselPathAbstractData* vesselPath;
    const crimson::ISolidModelKernel::ContourSet contours;
    bool useInflowAsWall;
    bool useOutflowAsWall;
    ISolidModelKernel::LoftingAlgorithm loftingAlgorithm;
    double seamEdgeRotation;
    bool preview;
    double interContourDistance;
};

std::shared_ptr<async::TaskWithResult<mitk::BaseData::Pointer>>
ISolidModelKernel::createLoftTask(const crimson::VesselPathAbstractData* vesselPath, const ContourSet& contours,
                                  bool useInflowAsWall, bool useOutflowAsWall, LoftingAlgorithm algorithm,
                                  double seamEdgeRoation, bool preview, double interContourDistance)
{
    return std::static_pointer_cast<crimson::async::TaskWithResult<mitk::BaseData::Pointer>>(std::make_shared<LoftingTask>(
        vesselPath, contours, useInflowAsWall, useOutflowAsWall, algorithm, seamEdgeRoation, preview, interContourDistance));
}

bool isIntersecting(OCCBRepData* shape1, OCCBRepData* shape2, double /*tolerance*/)
{
    //     BRepExtrema_DistShapeShape dst(shape1, shape2, Extrema_ExtFlag_MIN);
    //     if (dst.IsDone()) {
    //         return dst.Value() < tolerance;
    //     }

    //     try {
    //         BRepAlgoAPI_Common common(shape1, shape2);
    //         if (!common.IsDone()) {
    //             return false;
    //         }
    //         return !common.SectionEdges().IsEmpty();
    //     }
    //     catch (...) {
    //         return false;
    //     }

    bool bboxesIntersecting =
        shape1->GetGeometry()->GetBoundingBox()->GetBounds()[1] > shape2->GetGeometry()->GetBoundingBox()->GetBounds()[0] &&
        shape1->GetGeometry()->GetBoundingBox()->GetBounds()[0] < shape2->GetGeometry()->GetBoundingBox()->GetBounds()[1] &&
        shape1->GetGeometry()->GetBoundingBox()->GetBounds()[3] > shape2->GetGeometry()->GetBoundingBox()->GetBounds()[2] &&
        shape1->GetGeometry()->GetBoundingBox()->GetBounds()[2] < shape2->GetGeometry()->GetBoundingBox()->GetBounds()[3] &&
        shape1->GetGeometry()->GetBoundingBox()->GetBounds()[5] > shape2->GetGeometry()->GetBoundingBox()->GetBounds()[4] &&
        shape1->GetGeometry()->GetBoundingBox()->GetBounds()[4] < shape2->GetGeometry()->GetBoundingBox()->GetBounds()[5];

    if (!bboxesIntersecting) {
        return false;
    }

    vtkNew<vtkDistancePolyDataFilter> distFilter;
    distFilter->SetInputData(0, shape1->getSurfaceRepresentation()->GetVtkPolyData());
    distFilter->SetInputData(1, shape2->getSurfaceRepresentation()->GetVtkPolyData());
    distFilter->SetComputeSecondDistance(1);
    distFilter->Update();

    auto scalars = distFilter->GetOutput()->GetPointData()->GetScalars();
    auto scalars2 = distFilter->GetOutput(1)->GetPointData()->GetScalars();

    return (scalars->GetRange()[0] * scalars->GetRange()[1] <= 0) && (scalars2->GetRange()[0] * scalars2->GetRange()[1] <= 0);
}

class OCCProgressIndicator : public Message_ProgressIndicator
{
public:
    void Reset() override { MITK_INFO << "Reset"; }

    //! Defines method Show of Progress Indicator
    Standard_Boolean Show(const Standard_Boolean force = Standard_True) override
    {
        MITK_INFO << "Show: ";

        Standard_Real min, max, step;
        Standard_Boolean isInf;
        GetScale(min, max, step, isInf);
        MITK_INFO << "  Scale: "
                  << "max = " << max << " min = " << min << " step = " << step << " isInf = " << isInf;

        MITK_INFO << "  NbScopes = " << GetNbScopes();

        MITK_INFO << "  Position = " << GetPosition();

        MITK_INFO << "  Value = " << GetValue();

        return Standard_True;
    }

    //! Redefines method UserBreak of Progress Indicator
    virtual Standard_Boolean UserBreak() override { return _userBreak; }

    void setUserBreak(bool userBreak) { _userBreak = userBreak; }

private:
    bool _userBreak = false;
};

class BlendingTask : public crimson::async::TaskWithResult<mitk::BaseData::Pointer>
{
public:
    BlendingTask(const std::map<VesselForestData::VesselPathUIDType, mitk::BaseData::Pointer>& solidDatas,
                 ImmutableRefRange<VesselForestData::BooleanOperationContainerType::value_type> booleanOperations,
                 ImmutableRefRange<VesselForestData::FilletSizeInfoContainerType::value_type> filletingInfo,
                 bool useParallelBlending)
        : solidDatas(solidDatas)
        , booleanOperations(booleanOperations.begin(), booleanOperations.end())
        , filletingInfo(filletingInfo.begin(), filletingInfo.end())
        , useParallelBlending(useParallelBlending)
        , progressIndicator(new OCCProgressIndicator)
    {
        Expects(solidDatas.size() > 0);

        // All operands of boolean operations must be present in the solidDatas map
        Expects(std::count_if(booleanOperations.begin(), booleanOperations.end(),
                              [&solidDatas](const VesselForestData::BooleanOperationContainerType::value_type& bopInfo) {
                                  return solidDatas.find(bopInfo.vessels.first) == solidDatas.end() ||
                                         solidDatas.find(bopInfo.vessels.second) == solidDatas.end();
                              }) == 0);

        // All filleting operands must be present in the solidDatas map
        Expects(std::count_if(filletingInfo.begin(), filletingInfo.end(),
                              [&solidDatas](const VesselForestData::FilletSizeInfoContainerType::value_type& filletInfoPair) {
                                  if (solidDatas.find(filletInfoPair.first.first) == solidDatas.end()) {
                                      return true;
                                  }
                                  if (filletInfoPair.first.second == VesselForestData::InflowUID ||
                                      filletInfoPair.first.second == VesselForestData::OutflowUID) {
                                      return false;
                                  }
                                  if (solidDatas.find(filletInfoPair.first.second) == solidDatas.end()) {
                                      return true;
                                  }

                                  return false;
                              }) == 0);
    }

    void cancel() override
    {
        static_cast<OCCProgressIndicator*>(progressIndicator.Access())->setUserBreak(true);
        crimson::async::TaskWithResult<mitk::BaseData::Pointer>::cancel();
    }

    static TopoDS_Shape getSubShapeByIndex(const TopoDS_Shape& shape, const TopAbs_ShapeEnum toFind, const unsigned int index)
    {
        auto currentIndex = 0u;
        for (auto exp = TopExp_Explorer{shape, toFind}; exp.More(); exp.Next(), ++currentIndex) {
            if (index == currentIndex) {
                return exp.Current();
            }
        }

        MITK_ERROR << "Failed to find sub-shape by index";
        assert(false);
        return TopoDS_Shape();
    }

    template <typename T>
    using AttachedDataMap = std::map<Handle_TopoDS_TShape, T>;

    std::tuple<State, std::string> runTask() override
    {
        setupCatchSystemSignals();

        int toProgressIntersect = 0; // shapes.size() * (shapes.size() - 1) / 2;
        int toProgressFuse = booleanOperations.size() - 1;
        int toProgressBlend = 10;
        int toProgressMesh = 5;
        int progressLeft = toProgressIntersect + toProgressFuse + toProgressBlend + toProgressMesh;

        stepsAddedSignal(progressLeft);

        try {
            OCC_CATCH_SIGNALS;
            std::map<VesselPathAbstractData::VesselPathUIDType, OCCBRepData*> vesselUIDtoSolidMap;

            std::transform(solidDatas.begin(), solidDatas.end(),
                           std::inserter(vesselUIDtoSolidMap, vesselUIDtoSolidMap.begin()),
                           [](const decltype(solidDatas)::value_type& p) {
                               auto brepData = dynamic_cast<OCCBRepData*>(p.second.GetPointer());
                               Expects(brepData != nullptr);
                               return std::make_pair(p.first, brepData);
                           });

            auto allOperationsAreFuse =
                std::all_of(booleanOperations.begin(), booleanOperations.end(),
                            [](const VesselForestData::BooleanOperationInfo& i) { return i.bop == VesselForestData::bopFuse; });

            AttachedDataMap<double> edgeToFilletSizeMap;
            AttachedDataMap<FaceIdentifier> faceToIdMap;

            // Associate fillet sizes with edges of the original solids
            for (auto filletIter = filletingInfo.begin(); filletIter != filletingInfo.end(); ++filletIter) {
                if (vesselUIDtoSolidMap.find(filletIter->first.first) == vesselUIDtoSolidMap.end()) {
                    continue;
                }

                auto bopShape = vesselUIDtoSolidMap[filletIter->first.first];

                auto flowFaceIndex = [filletIter, bopShape]() {
                    if (filletIter->first.second == VesselForestData::InflowUID) {
                        return bopShape->inflowFaceId();
                    }
                    if (filletIter->first.second == VesselForestData::OutflowUID) {
                        return bopShape->outflowFaceId();
                    }
                    return -1;
                }();

                if (flowFaceIndex < 0) {
                    continue;
                }

                auto flowFace = getSubShapeByIndex(bopShape->getShape(), TopAbs_FACE, flowFaceIndex);

                // All the edges of a flow face need to be filleted
                for (auto exp = TopExp_Explorer{flowFace, TopAbs_EDGE}; exp.More(); exp.Next()) {
                    edgeToFilletSizeMap[exp.Current().TShape()] = filletIter->second;
                }
            }

            // Associate original face identifiers with the faces of the original solids
            for (auto& occBRepData : vesselUIDtoSolidMap | boost::adaptors::map_values) {
                auto faceIndex = 0;
                for (auto exp = TopExp_Explorer{occBRepData->getShape(), TopAbs_FACE}; exp.More(); exp.Next(), ++faceIndex) {
                    faceToIdMap[exp.Current().TShape()] =
                        occBRepData->getFaceIdentifierMap().getFaceIdentifierForModelFace(faceIndex).get();
                }
            }

            // Prioritize fuses that delete a face - allows to avoid some crashes in OCC when two vessels are blended that only
            // intersect within other vessel
            // Perform BFS

            MITK_INFO << "Optimizing fuse order";

            if (!booleanOperations.empty()) {
                auto firstIter = booleanOperations.begin();
                do {
                    firstIter =
                        std::find_if(firstIter, booleanOperations.end(), [](const VesselForestData::BooleanOperationInfo& bop) {
                            return bop.bop == VesselForestData::bopFuse;
                        });
                    auto lastIter =
                        std::find_if(firstIter, booleanOperations.end(), [](const VesselForestData::BooleanOperationInfo& bop) {
                            return bop.bop != VesselForestData::bopFuse;
                        });

                    using BBoxToPathUIDMapType =
                        std::map<float, crimson::VesselForestData::VesselPathUIDType, std::greater<float>>;
                    BBoxToPathUIDMapType bboxSizeToPathUIDMap;
                    for (auto iter = firstIter; iter != lastIter; ++iter) {
                        auto addIfNotPresent = [&bboxSizeToPathUIDMap,
                                                &vesselUIDtoSolidMap](const crimson::VesselForestData::VesselPathUIDType& uid) {
                            if (std::find_if(bboxSizeToPathUIDMap.begin(), bboxSizeToPathUIDMap.end(),
                                             [uid](const BBoxToPathUIDMapType::value_type& v) { return v.second == uid; }) ==
                                bboxSizeToPathUIDMap.end()) {
                                // Sort by highest bbox first
                                bboxSizeToPathUIDMap
                                    [vesselUIDtoSolidMap[uid]->GetGeometry()->GetBoundingBox()->GetDiagonalLength2()] = uid;
                            }
                        };

                        addIfNotPresent(iter->vessels.first);
                        addIfNotPresent(iter->vessels.second);
                    }

                    std::queue<crimson::VesselForestData::VesselPathUIDType> dfsQueue;
                    while (!bboxSizeToPathUIDMap.empty()) {
                        dfsQueue.push(bboxSizeToPathUIDMap.begin()->second);
                        bboxSizeToPathUIDMap.erase(bboxSizeToPathUIDMap.begin());

                        while (!dfsQueue.empty()) {
                            auto firstShapeUID = dfsQueue.front();
                            dfsQueue.pop();

                            // Find all the fuses deleting faces and push them to the queue
                            for (auto secondShapeUIDIter = bboxSizeToPathUIDMap.begin();
                                 secondShapeUIDIter != bboxSizeToPathUIDMap.end();) {
                                auto vesselUIDPair = std::make_pair(firstShapeUID, secondShapeUIDIter->second);
                                auto booleanOpIter = std::find_if(
                                    firstIter, lastIter,
                                    [vesselUIDPair](const VesselForestData::BooleanOperationInfo& bopInfo) {
                                        return detail::OrderIndependentVesselPairEqualTo{}(bopInfo.vessels, vesselUIDPair);
                                    });

                                if (booleanOpIter != lastIter && booleanOpIter->removesFace) {
                                    dfsQueue.push(vesselUIDPair.second);
                                    std::iter_swap(firstIter, booleanOpIter);
                                    ++firstIter;
                                    secondShapeUIDIter = bboxSizeToPathUIDMap.erase(secondShapeUIDIter);
                                } else {
                                    ++secondShapeUIDIter;
                                }
                            }
                        }
                    }
                    firstIter = lastIter;
                } while (firstIter != booleanOperations.end());
            }

            MITK_INFO << "Starting fuse";

            TopoDS_Shape fuseShape;

            if (useParallelBlending && allOperationsAreFuse) {
                if (vesselUIDtoSolidMap.size() > 1) {
                    TopTools_ListOfShape shapesList;
                    shapesList.Append(vesselUIDtoSolidMap.begin()->second->getShape());

                    TopTools_ListOfShape toolsList;
                    for (auto iter = std::next(vesselUIDtoSolidMap.begin()); iter != vesselUIDtoSolidMap.end(); ++iter) {
                        toolsList.Append(iter->second->getShape());
                    }

                    BRepAlgoAPI_Fuse fuseMaker;
                    fuseMaker.SetArguments(shapesList);
                    fuseMaker.SetTools(toolsList);
                    fuseMaker.SetRunParallel(true);
                    fuseMaker.SetProgressIndicator(progressIndicator);
                    fuseMaker.Build();

                    fuseShape = fuseMaker.Shape();

                    _updateAttachedInformationAfterBooleanOperation<TopAbs_FACE>(fuseMaker, faceToIdMap);
                    _updateAttachedInformationAfterBooleanOperation<TopAbs_EDGE>(fuseMaker, edgeToFilletSizeMap);
                } else {
                    fuseShape = vesselUIDtoSolidMap.begin()->second->getShape();
                }
            } else {
                std::map<VesselPathAbstractData::VesselPathUIDType, std::tuple<TopoDS_Shape, std::string>> currentShapes;
                std::transform(vesselUIDtoSolidMap.begin(), vesselUIDtoSolidMap.end(),
                               std::inserter(currentShapes, currentShapes.begin()),
                               [](const std::pair<VesselPathAbstractData::VesselPathUIDType, OCCBRepData*>& p) {
                                   std::string name;
                                   p.second->GetPropertyList()->GetStringProperty("name", name);
                                   return std::make_pair(p.first, std::make_tuple(p.second->getShape(), name));
                               });

                for (size_t i = 0; i < booleanOperations.size(); ++i) {
                    auto bopAndName = [this, i]() {
                        switch (booleanOperations[i].bop) {
                        case VesselForestData::bopFuse:
                            return std::make_pair(
                                std::unique_ptr<BRepAlgoAPI_BooleanOperation>(std::make_unique<BRepAlgoAPI_Fuse>()), "Fuse");
                        case VesselForestData::bopCut:
                            return std::make_pair(
                                std::unique_ptr<BRepAlgoAPI_BooleanOperation>(std::make_unique<BRepAlgoAPI_Cut>()), "Cut");
                        case VesselForestData::bopCommon:
                            return std::make_pair(
                                std::unique_ptr<BRepAlgoAPI_BooleanOperation>(std::make_unique<BRepAlgoAPI_Common>()),
                                "Common");
                        case VesselForestData::bopInvalidLast:
                        default:
                            return std::make_pair(std::unique_ptr<BRepAlgoAPI_BooleanOperation>(), "");
                        }
                    }();

                    if (!std::get<0>(bopAndName)) {
                        continue;
                    }

                    std::string names[2] = {std::get<1>(currentShapes[booleanOperations[i].vessels.first]),
                                            std::get<1>(currentShapes[booleanOperations[i].vessels.second])};

                    TopoDS_Shape shapes[2] = {std::get<0>(currentShapes[booleanOperations[i].vessels.first]),
                                              std::get<0>(currentShapes[booleanOperations[i].vessels.second])};

                    if (shapes[0] == shapes[1]) {
                        MITK_INFO << "Both arguments " << names[0] << " and " << names[1] << " already used. Skipping";
                        continue;
                    }

                    MITK_INFO << "Performing operation : " << std::get<1>(bopAndName) << " between " << names[0] << " and "
                              << names[1];

                    if (isCancelling()) {
                        return std::make_tuple(State_Cancelled, std::string("Operation cancelled."));
                    }

                    auto& bop = std::get<0>(bopAndName);

                    auto argsList = TopTools_ListOfShape{};
                    argsList.Append(shapes[0]);
                    bop->SetArguments(argsList);

                    auto toolsList = TopTools_ListOfShape{};
                    toolsList.Append(shapes[1]);
                    bop->SetTools(toolsList);

                    bop->Build();

                    _updateAttachedInformationAfterBooleanOperation<TopAbs_FACE>(*bop, faceToIdMap);
                    _updateAttachedInformationAfterBooleanOperation<TopAbs_EDGE>(*bop, edgeToFilletSizeMap);

                    auto bopResultName = std::string(std::get<1>(bopAndName)) + "(" + names[0] + ", " + names[1] + ")";

                    for (auto& vesselUIDToShapeAndName : currentShapes) {
                        if (std::get<0>(vesselUIDToShapeAndName.second) == shapes[0] ||
                            std::get<0>(vesselUIDToShapeAndName.second) == shapes[1]) {
                            vesselUIDToShapeAndName.second = std::make_tuple(bop->Shape(), bopResultName);
                        }
                    }

                    fuseShape = bop->Shape();

                    auto nSolids = 0u;
                    for (TopExp_Explorer Exp(bop->Shape(), TopAbs_SOLID); Exp.More(); Exp.Next()) {
                        ++nSolids;
                    }
                    MITK_INFO << "nSolids: " << nSolids;

                    progressMadeSignal(1);

                    --toProgressFuse;
                    --progressLeft;
                }

                // Fuse solo vessels to the result
                std::set<Handle_TopoDS_TShape> uniqueShapes;

                for (const auto& shapeAndName : currentShapes | boost::adaptors::map_values) {
                    if (!uniqueShapes.insert(std::get<0>(shapeAndName).TShape()).second) {
                        // Shape already accounted for
                        continue;
                    }

                    MITK_INFO << "Adding disconnected component " << std::get<1>(shapeAndName) << " to the result.";

                    if (fuseShape.IsNull()) {
                        fuseShape = std::get<0>(shapeAndName);
                    } else {
                        BRepAlgoAPI_Fuse bop;

                        auto argsList = TopTools_ListOfShape{};
                        argsList.Append(fuseShape);
                        bop.SetArguments(argsList);

                        auto toolsList = TopTools_ListOfShape{};
                        toolsList.Append(std::get<0>(shapeAndName));
                        bop.SetTools(toolsList);

                        bop.Build();

                        _updateAttachedInformationAfterBooleanOperation<TopAbs_FACE>(bop, faceToIdMap);
                        _updateAttachedInformationAfterBooleanOperation<TopAbs_EDGE>(bop, edgeToFilletSizeMap);

                        fuseShape = bop.Shape();
                    }
                }
            }

            if (isCancelling()) {
                return std::make_tuple(State_Cancelled, std::string("Operation cancelled."));
            }

            MITK_INFO << "Fusing complete";

            progressMadeSignal(toProgressFuse);
            progressLeft -= toProgressFuse;

            MITK_INFO << "Filleting";

            TopTools_IndexedDataMapOfShapeListOfShape edgeToFacesMap;
            TopExp::MapShapesAndAncestors(fuseShape, TopAbs_EDGE, TopAbs_FACE, edgeToFacesMap);

            // Attach fillet info to section edges
            for (auto edgeExplorer = TopExp_Explorer{fuseShape, TopAbs_EDGE}; edgeExplorer.More(); edgeExplorer.Next()) {
                auto vesselUIDs =
                    _getFaceIdentifierForShape(edgeExplorer.Current(), edgeToFacesMap, faceToIdMap).parentSolidIndices;

                if (vesselUIDs.size() == 2) {
                    auto iter = filletingInfo.find(std::make_pair(*vesselUIDs.begin(), *vesselUIDs.rbegin()));

                    if (iter != filletingInfo.end()) {
                        edgeToFilletSizeMap[edgeExplorer.Current().TShape()] = iter->second;
                    }
                }
            }

            // Assign the fillet sizes for the necessary intersection edges
            BRepFilletAPI_MakeFillet filletMaker(fuseShape);

            bool filletAdded = false;
            for (TopExp_Explorer edgeExplorer(fuseShape, TopAbs_EDGE); edgeExplorer.More(); edgeExplorer.Next()) {
                auto filletSizeIter = edgeToFilletSizeMap.find(edgeExplorer.Current().TShape());

                if (filletSizeIter == edgeToFilletSizeMap.end() || filletSizeIter->second <= 0) {
                    continue;
                }

                filletMaker.Add(filletSizeIter->second, TopoDS::Edge(edgeExplorer.Current()));
                filletAdded = true;
            }

            // Make the fillet if any needed
            auto filletedShape = fuseShape;
            if (filletAdded) {
                filletMaker.Build();

                if (filletMaker.IsDone()) {
                    filletedShape = filletMaker.Shape();
                } else {
                    auto errorString = std::string{"Blender has failed during filleting:\n"};

                    if (filletMaker.NbFaultyContours() > 0) {
                        errorString += "At edges:\n";

                        for (int i = 1; i <= filletMaker.NbFaultyContours(); ++i) {
                            for (int j = 1; j <= filletMaker.NbEdges(i); ++j) {
                                auto faceIdentifier = _getFaceIdentifierForShape(
                                    filletMaker.Edge(filletMaker.FaultyContour(i), j), edgeToFacesMap, faceToIdMap);
                                errorString +=
                                    "'" +
                                    boost::algorithm::join(_getShapeNamesList(faceIdentifier, vesselUIDtoSolidMap), "', '") +
                                    "'\n";
                            }
                        }
                    }

                    TopTools_IndexedDataMapOfShapeListOfShape vertexToFacesMap;
                    TopExp::MapShapesAndAncestors(fuseShape, TopAbs_VERTEX, TopAbs_FACE, vertexToFacesMap);

                    if (filletMaker.NbFaultyVertices() > 0) {
                        errorString += "\nAt vertices:\n";
                        for (int i = 1; i <= filletMaker.NbFaultyVertices(); ++i) {
                            auto faceIdentifier =
                                _getFaceIdentifierForShape(filletMaker.FaultyVertex(i), vertexToFacesMap, faceToIdMap);
                            errorString +=
                                "'" + boost::algorithm::join(_getShapeNamesList(faceIdentifier, vesselUIDtoSolidMap), "', '") +
                                "'\n";
                        }
                    }

                    return std::make_tuple(async::Task::State_Failed, errorString);
                }

                _generateFaceIdsForFilletedFaces(filletMaker, fuseShape, faceToIdMap);
                _updateAttachedInformationAfterFilletingOperation<TopAbs_FACE>(filletMaker, fuseShape, faceToIdMap);

                MITK_INFO << "Filleting done";
            }

            if (isCancelling()) {
                return std::make_tuple(State_Cancelled, std::string("Operation cancelled."));
            }

            progressMadeSignal(toProgressBlend);
            progressLeft -= toProgressBlend;

            MITK_INFO << "Done";
            // Create MITK output object
            auto data = OCCBRepData::New();
            data->setShape(filletedShape);

            MITK_INFO << "Assigning ID's";

            auto faceIndex = 0u;
            for (auto faceExplorer = TopExp_Explorer{data->getShape(), TopAbs_FACE}; faceExplorer.More();
                 faceExplorer.Next(), ++faceIndex) {
                data->getFaceIdentifierMap().setFaceIdentifierForModelFace(faceIndex,
                                                                           faceToIdMap[faceExplorer.Current().TShape()]);
            }

            if (isCancelling()) {
                return std::make_tuple(State_Cancelled, std::string("Operation cancelled."));
            }

            MITK_INFO << "Meshing the result";

            data->getSurfaceRepresentation();

            progressMadeSignal(toProgressMesh);
            progressLeft -= toProgressMesh;

            if (isCancelling()) {
                return std::make_tuple(State_Cancelled, std::string("Operation cancelled."));
            }

            setResult(data.GetPointer());
            return std::make_tuple(State_Finished, std::string("Blending finished successfully."));
        } catch (...) {
            progressMadeSignal(progressLeft);

            std::string errorMessage = "Exception caught during blending\n";
            Handle(Standard_Failure) Fail = Standard_Failure::Caught();
            std::string occErrorMessage(Fail->GetMessageString());
            if (!occErrorMessage.empty()) {
                errorMessage += "Error message: " + occErrorMessage;
            }

            return std::make_tuple(State_Failed, std::string(errorMessage));
        }
    }

    std::set<std::string> _getShapeNamesList(const FaceIdentifier& faceIdentifier,
                                             const std::map<std::string, OCCBRepData*>& sortedShapes)
    {
        std::set<std::string> names;
        for (auto& vesselUID : faceIdentifier.parentSolidIndices) {
            std::string name;
            sortedShapes.at(vesselUID)->GetPropertyList()->GetStringProperty("name", name);
            names.insert(name);
        }

        return names;
    }

    FaceIdentifier _getFaceIdentifierForShape(
        const TopoDS_Shape& shape, const TopTools_IndexedDataMapOfShapeListOfShape& shapeToFacesMap,
        const AttachedDataMap<FaceIdentifier>& faceIdentifierMap) // const FaceMapType& fuseShapeFaceMap, const
                                                                  // std::map<VesselPathAbstractData::VesselPathUIDType,
                                                                  // OCCBRepData*>& sortedShapes, std::set<std::string>*
                                                                  // solidsNameList = nullptr)
    {
        auto faceIdentifier = FaceIdentifier{};
        auto& facesForEdge = shapeToFacesMap.FindFromKey(shape);

        for (auto faceIter = TopTools_ListIteratorOfListOfShape{facesForEdge}; faceIter.More(); faceIter.Next()) {
            auto parentFaceIdentifierIter = faceIdentifierMap.find(faceIter.Value().TShape());

            if (parentFaceIdentifierIter == faceIdentifierMap.end()) {
                assert(false);
                MITK_WARN << "Original shape does not have any face identifiers - replacing with getShape index. The local "
                             "properties set for faces may not be preserved correctly.";
            }

            auto& parentFaceIdentifier = parentFaceIdentifierIter->second;

            std::copy(parentFaceIdentifier.parentSolidIndices.begin(), parentFaceIdentifier.parentSolidIndices.end(),
                      std::inserter(faceIdentifier.parentSolidIndices, faceIdentifier.parentSolidIndices.end()));
            faceIdentifier.faceType = parentFaceIdentifier.faceType;
        }

        return faceIdentifier;
    }

    template <TopAbs_ShapeEnum ShapeType, typename AttachedDataType>
    void _updateAttachedInformationAfterBooleanOperation(/* const */ BRepAlgoAPI_BooleanOperation& booleanOperation,
                                                         AttachedDataMap<AttachedDataType>& attachedInformationMap)
    {
        auto allShapes = booleanOperation.Tools();

        for (auto iter = TopTools_ListIteratorOfListOfShape{booleanOperation.Arguments()}; iter.More(); iter.Next()) {
            allShapes.Append(iter.Value());
        }
        _updateAttachedInformationAfterOperation<ShapeType>(allShapes, booleanOperation, attachedInformationMap);
    }

    template <TopAbs_ShapeEnum ShapeType, typename AttachedDataType>
    void _updateAttachedInformationAfterFilletingOperation(/* const */ BRepFilletAPI_MakeFillet& filletOperation,
                                                           const TopoDS_Shape& originalShape,
                                                           AttachedDataMap<AttachedDataType>& attachedInformationMap)
    {
        auto shapesList = TopTools_ListOfShape{};
        shapesList.Append(originalShape);

        _updateAttachedInformationAfterOperation<ShapeType>(shapesList, filletOperation, attachedInformationMap);
    }

    template <TopAbs_ShapeEnum ShapeType, typename AttachedDataType>
    void _updateAttachedInformationAfterOperation(const TopTools_ListOfShape& orignalShapes,
                                                  /* const */ BRepBuilderAPI_MakeShape& operation,
                                                  AttachedDataMap<AttachedDataType>& attachedInformationMap)
    {
        for (auto shapeIter = TopTools_ListIteratorOfListOfShape{orignalShapes}; shapeIter.More(); shapeIter.Next()) {
            for (auto originalShapeExplorer = TopExp_Explorer{shapeIter.Value(), ShapeType}; originalShapeExplorer.More();
                 originalShapeExplorer.Next()) {
                auto& modifiedShapeList = operation.Modified(originalShapeExplorer.Current());

                if (!modifiedShapeList.IsEmpty()) {
                    // Transfer the face identifier to all the faces originated from the original one
                    auto attachedInfoIter = attachedInformationMap.find(originalShapeExplorer.Current().TShape());

                    if (attachedInfoIter == attachedInformationMap.end()) {
                        continue;
                    }

                    for (auto modifiedFaceIter = TopTools_ListIteratorOfListOfShape{modifiedShapeList}; modifiedFaceIter.More();
                         modifiedFaceIter.Next()) {
                        attachedInformationMap[modifiedFaceIter.Value().TShape()] = attachedInfoIter->second;
                    }

                    attachedInformationMap.erase(attachedInfoIter);
                } else if (operation.IsDeleted(originalShapeExplorer.Current())) {
                    attachedInformationMap.erase(originalShapeExplorer.Current().TShape());
                }
            }
        }
    }

    void _generateFaceIdsForFilletedFaces(/* const */ BRepFilletAPI_MakeFillet& filletOperation,
                                          const TopoDS_Shape& originalShape, AttachedDataMap<FaceIdentifier>& faceIdentifierMap)
    {
        TopTools_IndexedDataMapOfShapeListOfShape edgeToFacesMap;
        TopExp::MapShapesAndAncestors(originalShape, TopAbs_EDGE, TopAbs_FACE, edgeToFacesMap);

        // Faces generated from edges
        for (TopExp_Explorer edgeExplorer(originalShape, TopAbs_EDGE); edgeExplorer.More(); edgeExplorer.Next()) {
            const TopTools_ListOfShape& filletFacesFromEdge = filletOperation.Generated(edgeExplorer.Current());

            for (TopTools_ListIteratorOfListOfShape shapeIter(filletFacesFromEdge); shapeIter.More(); shapeIter.Next()) {
                if (shapeIter.Value().ShapeType() == TopAbs_FACE) {
                    faceIdentifierMap[shapeIter.Value().TShape()] =
                        _getFaceIdentifierForShape(edgeExplorer.Current(), edgeToFacesMap, faceIdentifierMap);
                }
            }
        }

        TopTools_IndexedDataMapOfShapeListOfShape vertexToFacesMap;
        TopExp::MapShapesAndAncestors(originalShape, TopAbs_VERTEX, TopAbs_FACE, vertexToFacesMap);

        // Faces generated from vertices
        for (TopExp_Explorer vertexExplorer(originalShape, TopAbs_VERTEX); vertexExplorer.More(); vertexExplorer.Next()) {
            const TopTools_ListOfShape& filletFacesFromVertex = filletOperation.Generated(vertexExplorer.Current());

            for (TopTools_ListIteratorOfListOfShape shapeIter(filletFacesFromVertex); shapeIter.More(); shapeIter.Next()) {
                if (shapeIter.Value().ShapeType() == TopAbs_FACE) {
                    faceIdentifierMap[shapeIter.Value().TShape()] =
                        _getFaceIdentifierForShape(vertexExplorer.Current(), vertexToFacesMap, faceIdentifierMap);
                }
            }
        }
    }

private:
    const std::map<VesselPathAbstractData::VesselPathUIDType, mitk::BaseData::Pointer> solidDatas;
    std::vector<VesselForestData::BooleanOperationInfo> booleanOperations; // non-const - order may be changed
    const VesselForestData::OrderIndependentVesselPathMap<double> filletingInfo;
    bool useParallelBlending;

    Handle_Message_ProgressIndicator progressIndicator;
};

std::shared_ptr<async::TaskWithResult<mitk::BaseData::Pointer>> ISolidModelKernel::createBlendTask(
    const std::map<VesselForestData::VesselPathUIDType, mitk::BaseData::Pointer>& solidDatas,
    ImmutableRefRange<VesselForestData::BooleanOperationContainerType::value_type> booleanOperations,
    ImmutableRefRange<VesselForestData::FilletSizeInfoContainerType::value_type> filletingInfo, bool useParallelBlending)
{
    return std::static_pointer_cast<crimson::async::TaskWithResult<mitk::BaseData::Pointer>>(
        std::make_shared<BlendingTask>(solidDatas, booleanOperations, filletingInfo, useParallelBlending));
}

std::tuple<double, bool, int> ISolidModelKernel::intersectionEdgeLength(mitk::BaseData::Pointer solid1,
                                                                        mitk::BaseData::Pointer solid2)
{
    OCCBRepData* shapes[] = {dynamic_cast<OCCBRepData*>(solid1.GetPointer()), dynamic_cast<OCCBRepData*>(solid2.GetPointer())};

    if (!shapes[0] || !shapes[1]) {
        MITK_DEBUG << "Solid provided for intersection checking were not recognized!";
        return std::make_tuple(-1.0, false, -1);
    }

    bool bboxesIntersecting =
        shapes[0]->GetGeometry()->GetBoundingBox()->GetBounds()[1] >
            shapes[1]->GetGeometry()->GetBoundingBox()->GetBounds()[0] &&
        shapes[0]->GetGeometry()->GetBoundingBox()->GetBounds()[0] <
            shapes[1]->GetGeometry()->GetBoundingBox()->GetBounds()[1] &&
        shapes[0]->GetGeometry()->GetBoundingBox()->GetBounds()[3] >
            shapes[1]->GetGeometry()->GetBoundingBox()->GetBounds()[2] &&
        shapes[0]->GetGeometry()->GetBoundingBox()->GetBounds()[2] <
            shapes[1]->GetGeometry()->GetBoundingBox()->GetBounds()[3] &&
        shapes[0]->GetGeometry()->GetBoundingBox()->GetBounds()[5] >
            shapes[1]->GetGeometry()->GetBoundingBox()->GetBounds()[4] &&
        shapes[0]->GetGeometry()->GetBoundingBox()->GetBounds()[4] < shapes[1]->GetGeometry()->GetBoundingBox()->GetBounds()[5];

    if (!bboxesIntersecting) {
        return std::make_tuple(-1.0, false, -1);
    }

    try {
        OCC_CATCH_SIGNALS;

        BRepExtrema_ShapeProximity proximityDetector(shapes[0]->getShape(), shapes[1]->getShape());

        proximityDetector.Perform();

        if (proximityDetector.OverlapSubShapes1().Size() == 0) {
            return std::make_tuple(-1.0, false, -1);
        }

        BRepAlgoAPI_Fuse fuseMaker;

        TopTools_ListOfShape args;
        args.Append(shapes[0]->getShape());
        fuseMaker.SetArguments(args);

        TopTools_ListOfShape tools;
        tools.Append(shapes[1]->getShape());
        fuseMaker.SetTools(tools);

        fuseMaker.Build();

        if (fuseMaker.SectionEdges().Extent() == 0) {
            return std::make_tuple(-1.0, false, -1);
        }

        auto removesFace = false;
        auto removedFaceOwner = -1;

        for (int i = 0; i < 2; ++i) {
            for (TopExp_Explorer Exp(shapes[i]->getShape(), TopAbs_FACE); Exp.More(); Exp.Next()) {
                if (fuseMaker.IsDeleted(Exp.Current())) {
                    removesFace = true;
                    removedFaceOwner = i;
                    break;
                }
            }
        }

        auto filletSize = 0.0;

        for (TopTools_ListIteratorOfListOfShape iter(fuseMaker.SectionEdges()); iter.More(); iter.Next()) {
            BRepAdaptor_Curve adaptor(TopoDS::Edge(iter.Value()));
            filletSize += GCPnts_AbscissaPoint::Length(adaptor);
        }

        return std::make_tuple(filletSize, removesFace, removedFaceOwner);
    } catch (...) {
        return std::make_tuple(-1.0, false, -1);
    }
}

mitk::ScalarType ISolidModelKernel::solidVolume(mitk::BaseData::Pointer solid)
{
    setupCatchSystemSignals();

    if (!dynamic_cast<OCCBRepData*>(solid.GetPointer())) {
        MITK_DEBUG << "Solid provided for volume calculation not recognized!";
        return false;
    }

    try {
        OCC_CATCH_SIGNALS;
        return static_cast<OCCBRepData*>(solid.GetPointer())->getVolume();
    } catch (...) {
        return -1;
    }
}

 mitk::ScalarType ISolidModelKernel::contourArea(mitk::PlanarFigure::Pointer figure)
 {

	 std::vector< mitk::Point2D > polyLineModel(figure->GetNumberOfControlPoints());
	 for (int i = 0; i < figure->GetNumberOfControlPoints(); i++)
		 polyLineModel[i] = (figure->GetControlPoint(i));


	 mitk::Point2D centroid;
	 centroid[0] = 0; centroid[1] = 0;
	 for (int i = 0; i < polyLineModel.size(); i++){
		 centroid[0] += polyLineModel[i][0];
		 centroid[1] += polyLineModel[i][1];
	 }
	 centroid[0] /= polyLineModel.size();
	 centroid[1] /= polyLineModel.size();

	 auto curve2d = convertAnyPlanarFigureToCurve(figure.GetPointer(), centroid);

     // Create the 3D edge from a 2D curve
     gp_Ax3 xyz(gp::XOY());
     Handle(Geom_Plane) xyzPlane = new Geom_Plane(xyz);
     TopoDS_Edge edge2d = BRepBuilderAPI_MakeEdge(curve2d, xyzPlane);
     BRepLib::BuildCurves3d(edge2d);

     // Determine center of mass of the curve
     auto props = GProp_GProps();
     BRepGProp::SurfaceProperties(BRepBuilderAPI_MakeFace(BRepBuilderAPI_MakeWire(edge2d)), props);

     return props.Mass();
 }

} // namespace crimson
