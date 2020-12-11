// System includes
#include <vector>
#include <cmath>

// Main include
#include "ContourTypeConversion.h"

// MITK
#include <mitkPlaneGeometry.h>

// Fitting algorithms
#include <Wm5ApprEllipseFit2.h>
#include <Wm5ContBox2.h>
#include <Wm5DistPoint2Segment2.h>

namespace crimson {

static void preparePlanarFigureConversion(mitk::PlanarFigure* to, mitk::PlanarFigure* from)
{
    to->SetPlaneGeometry(from->GetPlaneGeometry()->Clone());
    to->PlaceFigure(mitk::Point2D());
    to->SetFinalized(true);
}


void convertContourType(mitk::PlanarCircle::Pointer& to, mitk::PlanarFigure* from)
{
    if (from->GetPolyLinesSize() == 0 || from->GetPolyLine(0).size() <= 2) {
        return; // Conversion not possible with less than 2 points
    }

    to = mitk::PlanarCircle::New();

    preparePlanarFigureConversion(to, from);

    std::vector<std::pair<double, double>> points;
	std::pair<double, double> point_sum{ 0.0, 0.0 };
    for (const mitk::Point2D& polyLineElement : from->GetPolyLine(0)) {
		points.push_back(std::make_pair(polyLineElement[0], polyLineElement[1]));
		point_sum.first += polyLineElement[0];
		point_sum.second += polyLineElement[1];
    }

	const std::pair<double, double> center_of_mass{ point_sum.first / points.size(), point_sum.second / points.size() };

	double radius_sum = 0.0;
	for (const std::pair<double, double> point : points) {
		const std::pair<double, double> vector_to_point_from_centroid = { point.first - center_of_mass.first, point.second - center_of_mass.second };
		const double distance_to_point_from_centroid = sqrt(pow(vector_to_point_from_centroid.first, 2.0) + pow(vector_to_point_from_centroid.second, 2.0));
		
		radius_sum += distance_to_point_from_centroid;
	}
	const double radius = radius_sum / points.size();

    mitk::Point2D center;
    center[0] = center_of_mass.first;
    center[1] = center_of_mass.second;
    to->SetControlPoint(0, center);

    mitk::Point2D offsetPt;
    offsetPt[0] = center[0] + radius;
    offsetPt[1] = center[1];
    to->SetControlPoint(1, offsetPt);
}


void convertContourType(mitk::PlanarEllipse::Pointer& to, mitk::PlanarFigure* from)
{
    if (from->GetPolyLinesSize() == 0 || from->GetPolyLine(0).size() <= 3) {
        return; // Conversion not possible with less than 3 points
    }

    to = mitk::PlanarEllipse::New();

    preparePlanarFigureConversion(to, from);

    std::vector<double> points;
    for (const mitk::Point2D& polyLineElement : from->GetPolyLine(0)) {
        points.push_back(polyLineElement[0]);
        points.push_back(polyLineElement[1]);
    }

    mitk::Point2D center;
    mitk::Point2D majorPt;
	mitk::Point2D minorPt;


	// Just use the object bounding box as a heuristic for the ellipse. This
	// could be replaced at some point with something better.
	{
		std::vector<Wm5::Vector2d> wm5Points;
		for (size_t i = 0; i < points.size() / 2; ++i) {
			wm5Points.push_back(Wm5::Vector2d(points[2 * i], points[2 * i + 1]));
		}

		Wm5::Box2d box = Wm5::ContOrientedBox(wm5Points.size(), &wm5Points[0]);

		center[0] = box.Center.X();
		center[1] = box.Center.Y();

		Wm5::Vector2d p = box.Center + box.Axis[0] * box.Extent[0];
		majorPt[0] = p.X();
		majorPt[1] = p.Y();

		p = box.Center + box.Axis[1] * box.Extent[1];
		minorPt[0] = p.X();
		minorPt[1] = p.Y();
	}

    to->SetControlPoint(0, center);
    to->SetControlPoint(1, majorPt);
    to->SetControlPoint(2, minorPt);
}

static double maxDistancePointPoly(const mitk::Point2D& p, mitk::PlanarFigure* figure)
{
    // Compute longest distance from a point to a figure
    double dist = -1;
    for (const mitk::Point2D& polyLineElement : figure->GetPolyLine(0)) {
        dist = std::max(dist, polyLineElement.EuclideanDistanceTo(p));
    }

    return dist;
}


static double minDistancePointPoly(const mitk::Point2D& p, mitk::PlanarFigure* figure)
{
    // Compute shortest distance from a point to a figure
    double dist = std::numeric_limits<double>::max();
    mitk::Point2D prevP = *figure->GetPolyLine(0).rbegin();
    for (const mitk::Point2D& polyLineElement : figure->GetPolyLine(0)) {
        dist = std::min(dist, Wm5::DistPoint2Segment2d(Wm5::Vector2d(p[0], p[1]), 
            Wm5::Segment2d(Wm5::Vector2d(polyLineElement[0], polyLineElement[1]), Wm5::Vector2d(prevP[0], prevP[1]))).Get());
        prevP = polyLineElement;
    }

    return dist;
}                                                   

static double distancePolyToReducedPoly(mitk::PlanarFigure* originalFigure, const std::vector<mitk::Point2D>& listOfPointsToCheck)
{
    // Compute maximum of shortest distances from a set of points to a figure
    double dist = -1;

    for (const mitk::Point2D& point : listOfPointsToCheck) {
        dist = std::max(dist, minDistancePointPoly(point, originalFigure));
    }

    return dist;
}


void convertContourType(mitk::PlanarSubdivisionPolygon::Pointer& to, mitk::PlanarFigure* from)
{
    if (from->GetPolyLinesSize() == 0 || from->GetPolyLine(0).size() <= 3) {
        return; // Conversion not possible with less than 3 points
    }

    to = mitk::PlanarSubdivisionPolygon::New();

    // Experimentally determined percentages
    double p1 = 0.02, p2 = 0.05, p3 = 0.01;

    preparePlanarFigureConversion(to, from);

    // Minimum distance between control points (filter control points)
    double minDistance = maxDistancePointPoly(from->GetControlPoint(0), from) * p1;
    mitk::Point2D prevP;
    mitk::Point2D firstP = *from->GetPolyLine(0).begin();
    int ptId = 0;
    for (const mitk::Point2D& polyLineElement : from->GetPolyLine(0)) {
        mitk::Point2D p = polyLineElement;

        if (ptId == 0 || (p.EuclideanDistanceTo(prevP) > minDistance && p.EuclideanDistanceTo(firstP) > minDistance)) {
            prevP = p;
            to->SetControlPoint(ptId, p, true);
            ptId++;
        }
    }

    // Maximum allowed error for the resulting subdivision polygon)
    // It depends on both the figure and the plane geometry
    // The produced results seem "intuitively expected"
    double maxError = std::min(p2 * maxDistancePointPoly(from->GetControlPoint(0), from), 
        p3 * (from->GetPlaneGeometry()->GetBounds()[1] - from->GetPlaneGeometry()->GetBounds()[0]));

    // Keep removing control points until the figure no longer satisfies the "closeness" criterion
    unsigned int idx = 0;
    std::vector<mitk::Point2D> pointsToCheck;
    while (to->GetNumberOfControlPoints() > 4 && idx < to->GetNumberOfControlPoints()) {
        mitk::Point2D removedPoint = to->GetControlPoint(idx);
        to->RemoveControlPoint(idx);
        pointsToCheck.push_back(removedPoint);

        if (distancePolyToReducedPoly(to, pointsToCheck) > maxError) {
            to->AddControlPoint(removedPoint, idx);
            ++idx;
            pointsToCheck.clear();
        }
    }
}

} // namespace crimson


