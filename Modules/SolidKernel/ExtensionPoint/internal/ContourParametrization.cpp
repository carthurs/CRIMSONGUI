#include "internal/ContourParametrization.h"

#include <Wm5ConvexHull2.h>
#include <Wm5DistPoint2Line2.h>
#include <Wm5IntrLine2Segment2.h>
#include <Wm5ContBox2.h>

namespace crimson
{
/*!
 * \brief   Subdivide the contour into convex and concave parts. Concave part is a part that
 *  doesn't belong to the convex hull.
 */
static std::vector<int> classifyContour(const std::vector<mitk::Point2D>& contour, double tolerancePercentage = 0.1)
{
    std::vector<int> partStarts;

    if (contour.empty()) {
        return partStarts;
    }

    std::vector<Wm5::Vector2d> wm5Points;
    for (const mitk::Point2D& point : contour) {
        wm5Points.push_back(Wm5::Vector2d(point[0], point[1]));
    }

    Wm5::ConvexHull2d hull(wm5Points.size(), &wm5Points[0], 1e-9, false, Wm5::Query::QT_RATIONAL);

    // TODO: We can use hull to compute bbox - see other minimal bbox algorithm from wm5
    Wm5::Box2d bbox = Wm5::ContOrientedBox(wm5Points.size(), &wm5Points[0]);

    double minDistanceForConcaveParts = tolerancePercentage * std::min(bbox.Extent[0], bbox.Extent[1]);

    partStarts.push_back(hull.GetIndices()[0]);

    bool reversedOrder = false;

    int startIndex, endIndex;
    for (int i = 0; i < hull.GetNumSimplices(); ++i) {
        startIndex = hull.GetIndices()[i];
        endIndex = hull.GetIndices()[(i + 1) % hull.GetNumSimplices()];

        if ((endIndex != (startIndex + 1) % static_cast<int>(wm5Points.size()) &&
             startIndex != (endIndex + 1) % static_cast<int>(wm5Points.size()))) {
            double maxD = -1;
            Wm5::Vector2d direction = wm5Points[endIndex] - wm5Points[startIndex];
            direction.Normalize();
            Wm5::Line2d line(wm5Points[startIndex], direction);
            for (int ptId = startIndex; ptId != endIndex;
                 ptId = (reversedOrder ? ptId - 1 + wm5Points.size() : ptId + 1) % wm5Points.size()) {
                maxD = std::max(maxD, Wm5::DistPoint2Line2d(wm5Points[ptId], line).Get());
            }

            if (maxD > minDistanceForConcaveParts) {
                // push concave part
                partStarts.push_back(startIndex);
                partStarts.push_back(endIndex);
            }

            continue;
        }

        if (i == hull.GetNumSimplices() - 1 && partStarts.size() > 1) {
            // Close the loop
            partStarts[0] = partStarts[partStarts.size() - 1];
            partStarts.pop_back();
        }

        if (startIndex == (endIndex + 1) % static_cast<int>(wm5Points.size())) {
            reversedOrder = true;
        }
    }

    if (reversedOrder && partStarts.size() > 1) {
        std::reverse(partStarts.begin(), partStarts.end());
        std::rotate(partStarts.begin(), std::next(partStarts.begin(), partStarts.size() - 2), partStarts.end());
    }

    return partStarts;
}

/*!
 * \brief   Gets center of subcontour by index.
 */
static mitk::Point2D getCenterOfSubcontour(const std::vector<mitk::Point2D>& contour, const std::vector<int>& classification,
                                           int subcontourId)
{
    double subcontourLength = 0;
    int subcontourEndPointId =
        (classification[(subcontourId + 1) % classification.size()] - 1 + contour.size()) % contour.size();
    for (int i = classification[subcontourId]; i != subcontourEndPointId; i = (i + 1) % contour.size()) {
        subcontourLength += contour[i].EuclideanDistanceTo(contour[(i + 1) % contour.size()]);
    }

    double curSubcontourLength = 0;
    for (int i = classification[subcontourId]; i != subcontourEndPointId; i = (i + 1) % contour.size()) {
        double currEdgeLength = contour[i].EuclideanDistanceTo(contour[(i + 1) % contour.size()]);
        curSubcontourLength += currEdgeLength;

        if (curSubcontourLength > subcontourLength / 2) {
            double t = (curSubcontourLength - subcontourLength / 2) / currEdgeLength;

            mitk::Vector2D resultV =
                contour[i].GetVectorFromOrigin() * t + contour[(i + 1) % contour.size()].GetVectorFromOrigin() * (1 - t);
            mitk::Point2D result;
            result[0] = resultV[0];
            result[1] = resultV[1];
            return result;
        }
    }

    assert(false);
    return mitk::Point2D();
}

/*!
 * \brief   Gets center of longest subcontour.
 */
static mitk::Point2D getCenterOfLongestSubcontour(const std::vector<mitk::Point2D>& contour,
                                                  const std::vector<int>& classification)
{
    double maxSubcontourLength = -1;
    int maxSubcontourId = 0;
    for (size_t subcontourId = 0; subcontourId < classification.size(); subcontourId++) {
        double subcontourLength = 0;
        int subcontourEndPointId =
            (classification[(subcontourId + 1) % classification.size()] - 1 + contour.size()) % contour.size();
        for (int i = classification[subcontourId]; i != subcontourEndPointId; i = (i + 1) % contour.size()) {
            subcontourLength += contour[i].EuclideanDistanceTo(contour[(i + 1) % contour.size()]);
        }

        if (subcontourLength > maxSubcontourLength) {
            maxSubcontourLength = subcontourLength;
            maxSubcontourId = subcontourId;
        }
    }

    return getCenterOfSubcontour(contour, classification, maxSubcontourId);
}

/*!
 * \brief   Finds an intersection of a contour with a ray from figure center in a particular direction. 
 *          
 * \return Intersection subcontour index and intersection point.
 */
static std::pair<int, mitk::Point2D> findIntersectionByDirection(const std::vector<mitk::Point2D>& contour,
                                                                 const std::vector<int>& classification,
                                                                 const mitk::Point2D& figureCenter,
                                                                 const mitk::Vector2D& direction)
{
    // Find the IDs of intersections
    Wm5::Line2d line(Wm5::Vector2d(figureCenter[0], figureCenter[1]), Wm5::Vector2d(direction[0], direction[1]));

    double maxDistanceToIntersection = -1;
    int maxIntersectionSubcontourId = -1;
    mitk::Point2D maxIntersectionPoint;

    double minDistanceToIntersection = std::numeric_limits<double>::max();
    int minIntersectionSubcontourId = -1;
    mitk::Point2D minIntersectionPoint;

    for (size_t subcontourId = 0; subcontourId < classification.size(); subcontourId++) {
        int subcontourEndPointId =
            (classification[(subcontourId + 1) % classification.size()] - 1 + contour.size()) % contour.size();
        for (int i = classification[subcontourId]; i != subcontourEndPointId; i = (i + 1) % contour.size()) {
            Wm5::Segment2d segment(Wm5::Vector2d(contour[i][0], contour[i][1]),
                                   Wm5::Vector2d(contour[(i + 1) % contour.size()][0], contour[(i + 1) % contour.size()][1]));

            Wm5::IntrLine2Segment2d intersector(line, segment);
            intersector.SetIntervalThreshold(1e-2);
            if (intersector.Find() && intersector.GetQuantity() == 1) {
                mitk::Point2D intersectionPoint;
                intersectionPoint[0] = intersector.GetPoint().X();
                intersectionPoint[1] = intersector.GetPoint().Y();

                double distance = figureCenter.EuclideanDistanceTo(intersectionPoint);

                if (direction * (intersectionPoint - figureCenter) > 0) {
                    if (distance > maxDistanceToIntersection) {
                        maxIntersectionSubcontourId = subcontourId;
                        maxDistanceToIntersection = distance;
                        maxIntersectionPoint = intersectionPoint;
                    }
                } else {
                    if (distance < minDistanceToIntersection) {
                        minIntersectionSubcontourId = subcontourId;
                        minDistanceToIntersection = distance;
                        minIntersectionPoint = intersectionPoint;
                    }
                }
            }
        }
    }

    if (maxIntersectionSubcontourId != -1) {
        return std::make_pair(maxIntersectionSubcontourId, maxIntersectionPoint);
    }

    assert(minIntersectionSubcontourId != -1);
    return std::make_pair(minIntersectionSubcontourId, minIntersectionPoint);
}

/*!
 * \brief   Computes a contour figure center.
 */
static mitk::Point2D getFigureCenter(const std::vector<mitk::Point2D>& contour)
{
    mitk::Point2D center(0);
    mitk::ScalarType factor = 1.0 / contour.size();
    for (const mitk::Point2D& point : contour) {
        center += point.GetVectorFromOrigin() * factor;
    }
    return center;
}

std::vector<mitk::Point2D> computeContourParametrizationStartPoints(const ISolidModelKernel::ContourSet& contours,
                                                                    int highestRiskContourIndex,
                                                                    const mitk::Point2D& highertRiskPoint, double seamAngle)
{
    std::vector<std::vector<mitk::Point2D>> contourPolyLines(contours.size());

    std::vector<std::vector<int>> contourClassifications(contours.size());

    for (size_t i = 0; i < contours.size(); ++i) {
        if (contours[i]->GetPolyLinesSize() == 0) {
            // Point section
            continue;
        }

        contourPolyLines[i] = contours[i]->GetPolyLine(0);
        contourClassifications[i] = classifyContour(contourPolyLines[i]);
    }

    // Find the first contour that requires the strict parameter value selection - i.e. a contour with at least one concave part
    std::vector<mitk::Point2D> parametrizationStartPts;
    std::vector<mitk::Vector2D> directions;

    mitk::Vector2D direction;
    direction[0] = 0;
    direction[1] = 1;
    bool directionInitialized = false;
    for (size_t contourId = 0; contourId < contourClassifications.size(); ++contourId) {
        mitk::Point2D parametrizationStartPt;
        if (contourClassifications[contourId].size() > 1) {
            // Non-convex part
            mitk::Point2D figureCenter = getFigureCenter(contourPolyLines[contourId]);
            if (!directionInitialized) {
                // First non-convex contour - select center of longest subcontour as the first direction
                parametrizationStartPt =
                    getCenterOfLongestSubcontour(contourPolyLines[contourId], contourClassifications[contourId]);

                direction = parametrizationStartPt - figureCenter;
                direction.Normalize();
                for (size_t i = 0; i < contourId; ++i) {
                    directions[i] = direction;
                }
                directionInitialized = true;
            } else {
                if (contourClassifications[contourId].size() == 0) {
                    // Point section
                    parametrizationStartPt = contours[contourId]->GetControlPoint(0) + direction * 1e-3;
                } else {
                    // Convex section
                    std::pair<int, mitk::Point2D> intersection = findIntersectionByDirection(
                        contourPolyLines[contourId], contourClassifications[contourId], figureCenter, direction);
                    parametrizationStartPt = getCenterOfSubcontour(contourPolyLines[contourId],
                                                                   contourClassifications[contourId], intersection.first);
                }
            }
            direction = parametrizationStartPt - figureCenter;
            direction.Normalize();
        }
        parametrizationStartPts.push_back(parametrizationStartPt);
        directions.push_back(direction);
    }

//#define STARTPOINTS_DEBUG
#ifdef STARTPOINTS_DEBUG
    auto pointSet = mitk::PointSet::New();
#endif // STARTPOINTS_DEBUG

    int prevNonConvex = -1;
    int nextNonConvex;

    // Interpolate the direction for convex contours which are in-between the non-convex ones
    for (size_t contourId = 0; contourId < contourClassifications.size(); ++contourId) {
        // If contour is convex, then it will have only one subcontour
        if (contourClassifications[contourId].size() > 1) {
            nextNonConvex = contourId;
            if (prevNonConvex != -1) {
                for (int convexContourId = prevNonConvex + 1; convexContourId < nextNonConvex; ++convexContourId) {
                    double t = (double)(convexContourId - prevNonConvex) / (nextNonConvex - prevNonConvex);
                    directions[convexContourId] = directions[prevNonConvex] * (1.0 - t) + directions[nextNonConvex] * t;
                    directions[convexContourId].Normalize();
                }
            }
            prevNonConvex = contourId;
        }
    }

    double angle = seamAngle;

    if (highestRiskContourIndex >= 0) {
        // Rotate start points according to highest risk point
        mitk::Vector2D d = (highertRiskPoint - getFigureCenter(contourPolyLines[highestRiskContourIndex]));
        d.Normalize();
        angle += atan2(d[1], d[0]) - atan2(directions[highestRiskContourIndex][1], directions[highestRiskContourIndex][0]);
    }

    auto transform = itk::AffineTransform<double, 2>::New();
    transform->Rotate2D(angle);
    std::transform(directions.begin(), directions.end(), directions.begin(),
                   [transform](const mitk::Vector2D& dir) { return transform->TransformVector(dir); });

    // Compute the parametrization start points using the directions computed above
    for (size_t contourId = 0; contourId < contourClassifications.size(); ++contourId) {
        // if (contourClassifications[contourId].size() == 1) {
        // Convex section
        parametrizationStartPts[contourId] =
            findIntersectionByDirection(contourPolyLines[contourId], contourClassifications[contourId],
                                        getFigureCenter(contourPolyLines[contourId]), directions[contourId])
                .second;
        /*}
        else */ if (contourClassifications[contourId].size() == 0) {
            // Point section
            parametrizationStartPts[contourId] = contours[contourId]->GetControlPoint(0) + directions[contourId] * 1e-3;
        }

#ifdef STARTPOINTS_DEBUG
        mitk::Point3D pt3D;
        contours[contourId]->GetGeometry2D()->Map(
            parametrizationStartPts[contourId] /*getFigureCenter(contourPolyLines[contourId])*/, pt3D);
        pointSet->InsertPoint(pointSet->GetSize(), pt3D);
#endif // STARTPOINTS_DEBUG
    }

#ifdef STARTPOINTS_DEBUG
    auto node = mitk::DataNode::New();
    node->SetData(pointSet);
    node->SetName("Starts");
    node->SetColor(0, 1, 0);
    GetDataStorage()->Add(node, currentVesselPath());
#endif // STARTPOINTS_DEBUG

    return parametrizationStartPts;
}

} // namespace crimson
