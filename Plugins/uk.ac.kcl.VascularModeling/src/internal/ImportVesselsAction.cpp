#include "ImportVesselsAction.h"
#include "VascularModelingUtils.h"
#include "VesselDrivenSlicedGeometry.h"

#include <QFileDialog>

#include <HierarchyManager.h>
#include <VascularModelingNodeTypes.h>
#include <vtkParametricSplineVesselPathData.h>
#include <mitkPlanarPolygon.h>

#include <vtkNew.h>
#include <vtkPolyDataReader.h>
#include <vtkMath.h>
#include <vtkLine.h>
#include <vtkPolyLine.h>
#include <vtkKochanekSpline.h>
#include <vtkGenericCell.h>
#include <vtkCellLocator.h>
#include <vtkCell.h>

#include "ContourTypeConversion.h"

#include <Wm5DistPoint2Ellipse2.h>

namespace detail {
    unsigned int computeSplineResolution(vtkParametricSpline* spline, double relativeAccuracy, unsigned int initialResolution = 0)
    {
        unsigned int resolution = std::max(static_cast<unsigned int>(spline->GetPoints()->GetNumberOfPoints()) * 2, initialResolution);

        bool converged = false;
        while (!converged) {
            std::array<double, 3> prevPoint;

            double u[3] = { 0, 0, 0 };
            double Du[9];
            spline->Evaluate(u, &prevPoint[0], Du);

            double paramDelta = (spline->GetMaximumU() - spline->GetMinimumU()) / (resolution - 1);

            converged = true;
            for (unsigned int i = 1; i < resolution; ++i) {
                std::array<double, 3> nextPoint;
                std::array<double, 3> nextHalfPoint;

                double nextPointParam = spline->GetMinimumU() + paramDelta * i;
                double nextHalfPointParam = nextPointParam - paramDelta / 2;

                u[0] = nextPointParam;
                spline->Evaluate(u, &nextPoint[0], Du);

                u[0] = nextHalfPointParam;
                spline->Evaluate(u, &nextHalfPoint[0], Du);

                double segmentLengthSquared = vtkMath::Distance2BetweenPoints(&prevPoint[0], &nextPoint[0]);

                double t;
                std::array<double, 3> closestPt;
                double distToHalfPointSquared = vtkLine::DistanceToLine(&nextHalfPoint[0], &prevPoint[0], &nextPoint[0], t, &closestPt[0]);

                if (t < 0) {
                    distToHalfPointSquared = vtkMath::Distance2BetweenPoints(&prevPoint[0], &nextHalfPoint[0]);
                }
                else if (t > 1) {
                    distToHalfPointSquared = vtkMath::Distance2BetweenPoints(&nextPoint[0], &nextHalfPoint[0]);
                }

                prevPoint = nextPoint;

                if (distToHalfPointSquared > segmentLengthSquared * relativeAccuracy * relativeAccuracy) {
                    converged = false;
                    resolution *= 2;
                    break;
                }
            }
        }

        return resolution;
    }

    template<typename SplineType>
    vtkSmartPointer<vtkParametricSpline> fitPolyLineWithSpline(vtkPolyLine* polyLine, double maxVertexDistance)
    {
        vtkSmartPointer<vtkPoints> splinePoints = vtkPoints::New();
        splinePoints->Resize(polyLine->GetNumberOfPoints());

        vtkNew<vtkParametricSpline> spline;
        spline->SetXSpline(vtkNew<SplineType>().Get());
        spline->SetYSpline(vtkNew<SplineType>().Get());
        spline->SetZSpline(vtkNew<SplineType>().Get());
        spline->SetPoints(splinePoints);

        unsigned int splineResolution = 0;

        vtkNew<vtkParametricFunctionSource> splineSource;
        splineSource->SetParametricFunction(spline.Get());

        vtkNew<vtkCellLocator> cellLocator;
        vtkNew<vtkGenericCell> cell;

        vtkIdType dirtyDistanceRangeStart = 0;
        vtkIdType dirtyDistanceRangeEnd = polyLine->GetNumberOfPoints() - 1;

        std::set<unsigned int> controlPointIndices{ 0, static_cast<unsigned int>(polyLine->GetNumberOfPoints() - 1) };

        std::vector<double> distancesSquaredToIntermediateSpline(polyLine->GetNumberOfPoints(), 0.0);

        do {
            double maxDistanceSquared = 0;

            splinePoints->SetNumberOfPoints(controlPointIndices.size());

            vtkIdType index = 0;
            for (int i : controlPointIndices) {
                splinePoints->SetPoint(index++, polyLine->GetPoints()->GetPoint(polyLine->GetPointId(i)));
            }

            spline->Modified();

            splineResolution = computeSplineResolution(spline.Get(), 0.1, splineResolution);

            splineSource->SetUResolution(splineResolution);
            splineSource->Update();
            cellLocator->SetDataSet(splineSource->GetOutput());
            cellLocator->ForceBuildLocator();

            // Update distances for potentially affected segments
            for (int i = dirtyDistanceRangeStart; i <= dirtyDistanceRangeEnd; ++i) {
                if (controlPointIndices.find(i) != controlPointIndices.end()) {
                    continue;
                }

                double closestPt[3];
                vtkIdType cellId;
                int subId;
                cellLocator->FindClosestPoint(polyLine->GetPoints()->GetPoint(polyLine->GetPointId(i)), closestPt, cell.Get(), cellId, subId, distancesSquaredToIntermediateSpline[i]);
            }

            auto maxElementIterator = std::max_element(distancesSquaredToIntermediateSpline.begin(), distancesSquaredToIntermediateSpline.end());

            if (*maxElementIterator < maxVertexDistance * maxVertexDistance) {
                break;
            }

            auto newIter = controlPointIndices.insert(std::distance(distancesSquaredToIntermediateSpline.begin(), maxElementIterator)).first;

            *maxElementIterator = 0;

            ptrdiff_t deltaBack = std::min(static_cast<ptrdiff_t>(3), std::distance(controlPointIndices.begin(), newIter));
            ptrdiff_t deltaForward = std::min(static_cast<ptrdiff_t>(3), std::distance(newIter, controlPointIndices.end()) - 1);

            auto dirtyDistanceRangeStartIter = newIter;
            std::advance(dirtyDistanceRangeStartIter, -deltaBack);
            dirtyDistanceRangeStart = *dirtyDistanceRangeStartIter;

            auto dirtyDistanceRangeEndIter = newIter;
            std::advance(dirtyDistanceRangeEndIter, deltaForward);
            dirtyDistanceRangeEnd = *dirtyDistanceRangeEndIter;
        } while (controlPointIndices.size() <= static_cast<size_t>(polyLine->GetNumberOfPoints()));

        return spline.Get();
    }

    mitk::PlanarFigure::Pointer tryConvertToSimpleFigures(const mitk::PlanarPolygon::Pointer& poly, double relativeAccuracy = 0.03)
    {
        mitk::PlanarCircle::Pointer circle = mitk::PlanarCircle::New();
        crimson::convertContourType(circle, poly.GetPointer());

        bool circleConversionSuccessful = true;
        double circleRadius = (circle->GetControlPoint(1) - circle->GetControlPoint(0)).GetNorm();
        for (unsigned int i = 0; i < poly->GetNumberOfControlPoints(); ++i) {
            double distanceToCircle = abs((poly->GetControlPoint(i) - circle->GetControlPoint(0)).GetNorm() - circleRadius);
            if (distanceToCircle > relativeAccuracy * circleRadius) {
                circleConversionSuccessful = false;
                break;
            }
        }

        //if (circleConversionSuccessful) {
        //    return circle.GetPointer();
        //}

        mitk::PlanarEllipse::Pointer ellipse = mitk::PlanarEllipse::New();
        crimson::convertContourType(ellipse, poly.GetPointer());

        Wm5::Vector2d center{ ellipse->GetControlPoint(0)[0], ellipse->GetControlPoint(0)[1] };
        Wm5::Vector2d p1{ ellipse->GetControlPoint(1)[0], ellipse->GetControlPoint(1)[1] };
        p1 -= center;
        double r1 = p1.Normalize();
        Wm5::Vector2d p2{ ellipse->GetControlPoint(2)[0], ellipse->GetControlPoint(2)[1] };
        p2 -= center;
        double r2 = p2.Normalize();

        Wm5::Ellipse2d ellipse2(center, p1, p2, r1, r2);

        bool ellipseConversionSuccessful = true;
        //for (unsigned int i = 0; i < poly->GetNumberOfControlPoints(); ++i) {
        //    double distanceToEllipse = 
        //        Wm5::DistPoint2Ellipse2d(Wm5::Vector2d(poly->GetControlPoint(i)[0], poly->GetControlPoint(i)[1]), ellipse2).Get();
        //    if (distanceToEllipse > relativeAccuracy * std::max(r1, r2)) {
        //        ellipseConversionSuccessful = false;
        //        break;
        //    }
        //}

        if (ellipseConversionSuccessful) {
            return ellipse.GetPointer();
        }

        return nullptr;
    }

}

ImportVesselsAction::ImportVesselsAction()
{
}

ImportVesselsAction::~ImportVesselsAction()
{
}


void ImportVesselsAction::Run(const QList<mitk::DataNode::Pointer> &selectedNodes)
{
    for (const mitk::DataNode::Pointer& node : selectedNodes) {
        auto hm = crimson::HierarchyManager::getInstance();

        if (!hm->getPredicate(crimson::VascularModelingNodeTypes::VesselTree())->CheckNode(node)) {
            continue;
        }

        QStringList vesselPathFileNames = QFileDialog::getOpenFileNames(nullptr, "Select vessel paths to import", QString(), "VTK vessel paths (*.vtk)");

        if (vesselPathFileNames.isEmpty()) {
            return;
        }

        for (const QString& fileName : vesselPathFileNames) {
            MITK_INFO << "Importing vessel path from " << fileName.toStdString();

            vtkNew<vtkPolyDataReader> reader;
            reader->SetFileName(fileName.toLatin1());
            reader->Update();

            vtkPolyData* polyData = reader->GetOutput();

            if (polyData->GetNumberOfCells() == 0) {
                MITK_ERROR << "No cells found. Skipping " << fileName.toStdString();
                continue;
            }
            
            // Get the vessel path
            vtkCell* vesselPathCell = polyData->GetCell(0);
            if (vesselPathCell->GetCellType() != VTK_POLY_LINE && vesselPathCell->GetCellType() != VTK_LINE) {
                MITK_ERROR << "Unexpected cell type. Skipping " << fileName.toStdString();
                continue;
            }

            auto vesselPath = crimson::vtkParametricSplineVesselPathData::New();

            MITK_INFO << "Fitting spline for the vessel path";
            vtkSmartPointer<vtkParametricSpline> spline =
                detail::fitPolyLineWithSpline<vtkKochanekSpline>(static_cast<vtkPolyLine*>(vesselPathCell), 0.05);

            std::vector<crimson::VesselPathAbstractData::PointType> controlPoints;
            for (vtkIdType index = 0; index < spline->GetPoints()->GetNumberOfPoints(); ++index) {
                crimson::VesselPathAbstractData::PointType point3D;
                mitk::vtk2itk(spline->GetPoints()->GetPoint(index), point3D);
                controlPoints.push_back(point3D);
            }

            vesselPath->setControlPoints(controlPoints);

            auto vesselPathNode = mitk::DataNode::New();
            vesselPathNode->SetData(vesselPath);
            vesselPathNode->SetName(QFileInfo(fileName).baseName().toStdString());

            auto randomColor = QColor::fromHsv(qrand() % 360, 255, 210);
            vesselPathNode->SetColor(randomColor.redF(), randomColor.greenF(), randomColor.blueF());


            crimson::HierarchyManager::getInstance()->addNodeToHierarchy(node, crimson::VascularModelingNodeTypes::VesselTree(), 
                vesselPathNode, crimson::VascularModelingNodeTypes::VesselPath());

            // Initialize sliced geometry for the vessel path to position the contours
            mitk::ScalarType paramDelta;
            mitk::Vector3D referenceImageSpacing;
            unsigned int timeSteps;

            std::tie(paramDelta, referenceImageSpacing, timeSteps) = crimson::VascularModelingUtils::getResliceGeometryParameters(vesselPathNode);

            auto slicedGeometry = crimson::VesselDrivenSlicedGeometry::New();
            slicedGeometry->InitializedVesselDrivenSlicedGeometry(vesselPath.GetPointer(), paramDelta, referenceImageSpacing, 50);

            // Read the contours
            for (vtkIdType contourCellId = 1; contourCellId < polyData->GetNumberOfCells(); ++contourCellId) {
                MITK_INFO << "Importing countour " << contourCellId << "/" << polyData->GetNumberOfCells() - 1;

                vtkCell* contourCell = polyData->GetCell(contourCellId);
                if (contourCell->GetCellType() != VTK_POLY_LINE) {
                    MITK_WARN << "Unexpected cell type for contour in " << fileName.toStdString() << ". Skipping.";
                    continue;
                }

                std::vector<mitk::Point3D> contourPoints;
                for (vtkIdType index = 0; index < contourCell->GetPointIds()->GetNumberOfIds(); ++index) {
                    if (index == contourCell->GetPointIds()->GetNumberOfIds() && 
                        contourCell->GetPointId(index) == contourCell->GetPointId(0)) {
                        break;
                    }

                    crimson::VesselPathAbstractData::PointType point3D;
                    mitk::vtk2itk(polyData->GetPoints()->GetPoint(contourCell->GetPointId(index)), point3D);
                    contourPoints.push_back(point3D);
                }

                auto planarPolygon = mitk::PlanarPolygon::New();

                // Find figure center
				mitk::Point3D center(0);
				center = vesselPath->getControlPoint(contourCellId - 1);
     /*           mitk::ScalarType factor = 1.0 / contourPoints.size();
                for (const mitk::Point3D& p : contourPoints) {
                    center += p.GetVectorFromOrigin() * factor;
                }*/

                // Get the geometry for the planar figure
                mitk::PlaneGeometry* contourGeometry = slicedGeometry->GetPlaneGeometry(slicedGeometry->findSliceByPoint(center));
                planarPolygon->SetClosed(true);
                planarPolygon->PlaceFigure(mitk::Point2D(0));
                planarPolygon->SetPlaneGeometry(contourGeometry);

                for (size_t i = 0; i < contourPoints.size(); ++i) {
                    mitk::Point2D controlPoint;
                    contourGeometry->Map(contourPoints[i], controlPoint);
                    planarPolygon->SetControlPoint(i, controlPoint, true);
                }

                planarPolygon->SetFinalized(true);

                // Try to convert to simple figures
                mitk::PlanarFigure::Pointer planarFigure = detail::tryConvertToSimpleFigures(planarPolygon);

                auto contourNode = mitk::DataNode::New();
                contourNode->SetData(planarFigure.IsNotNull() ? planarFigure.GetPointer() : planarPolygon.GetPointer());

                crimson::VascularModelingUtils::setDefaultContourNodeProperties(contourNode, false);
                if (planarFigure.IsNull()) {
                    contourNode->SetBoolProperty("planarfigure.drawcontrolpoints", false);
                } else {
                    contourNode->SetBoolProperty("lofting.interactiveContour", true);
                }

                crimson::HierarchyManager::getInstance()->addNodeToHierarchy(vesselPathNode, crimson::VascularModelingNodeTypes::VesselPath(), contourNode, crimson::VascularModelingNodeTypes::Contour());
            }
            MITK_INFO << "Done importing from " << fileName.toStdString();
        }
    }
}
