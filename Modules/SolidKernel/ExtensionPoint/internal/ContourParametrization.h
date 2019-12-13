#pragma once

#include "ISolidModelKernel.h"

namespace crimson {

/*!
 * \brief   Calculates the contour parametrization start points.
 *
 * \param   contours                The contours.
 * \param   highestRiskContourIndex The index of a contour where risk of self-intersection is highest.
 * \param   highestRiskPoint        The point at the highest risk contour where risk of self-intersection is highest.
 * \param   seamAngle               The rotation angle for the seam edge (in radians).
 */
extern std::vector<mitk::Point2D> computeContourParametrizationStartPoints(const ISolidModelKernel::ContourSet& contours,
                                                                           int highestRiskContourIndex,
                                                                           const mitk::Point2D& highestRiskPoint,
                                                                           double seamAngle);
} // namespace crimson