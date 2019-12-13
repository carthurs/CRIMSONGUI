// This will be an interface for solid model service and blueberry extension point etc.

#pragma once

#include <vector>
#include <functional>
#include <mitkBaseData.h>
#include <mitkPlanarFigure.h>

#include "SolidKernelExports.h"

#include <VesselForestData.h>
#include <VesselPathAbstractData.h>
#include <AsyncTaskWithResult.h>

#include <ImmutableRanges.h>

namespace crimson
{

class SolidKernel_EXPORT ISolidModelKernel
{
public:
    typedef std::vector<mitk::PlanarFigure::Pointer> ContourSet;

    /*! \brief   Values that represent lofting algorithms. */
    enum LoftingAlgorithm { 
        laAppSurf,  ///< Default lofting algorithm using AppSurf
        laSweep     ///< Sweeping algorithm
    };

    /*!
    * \brief   Create an asynchronous task that computes a lofted surface.
    *
    * \param   vesselPath              The vessel path.
    * \param   contours                An ordered vector of planar figures defining the contours.
    * \param   useInflowAsWall         Mark the cap face at the first contour as a part of the wall.
    * \param   useOutflowAsWall        Mark the cap face at the last contour as a part of the wall.
    * \param   algorithm               The applied lofting algorithm (loft or sweep).
    * \param   seamEdgeRoation         An angle (in degrees) to rotate the seam edge of the lofted model by.
    * \param   preview                 Create a set of contours instead of a solid model.
    * \param   interContourDistance    Distance in millimeters between the contours for the preview.
    */
    static std::shared_ptr<async::TaskWithResult<mitk::BaseData::Pointer>>
    createLoftTask(const crimson::VesselPathAbstractData* vesselPath, const ContourSet& contours, bool useInflowAsWall,
                   bool useOutflowAsWall, LoftingAlgorithm algorithm, double seamEdgeRoation, bool preview,
                   double interContourDistance);

    /*!
    * \brief   Create an asynchronous task that computes a fully blended model.
    *
    * \param   solidDatas          A map from vessel path UID's to the corresponding solid models of vessels.
    * \param   booleanOperations   The ordered boolean operations to be performed on the solid models.
    * \param   filletingInfo       The list of fillet sizes to be applied to edges created at the intersections of vessel models.
    * \param   useParallelBlending Perform the blending operation in parallel (only possible if all the operations are fuse, otherwise the flag is ignored).
    */
    static std::shared_ptr<async::TaskWithResult<mitk::BaseData::Pointer>>
    createBlendTask(const std::map<VesselForestData::VesselPathUIDType, mitk::BaseData::Pointer>& solidDatas,
                    ImmutableRefRange<VesselForestData::BooleanOperationContainerType::value_type> booleanOperations,
                    ImmutableRefRange<VesselForestData::FilletSizeInfoContainerType::value_type> filletingInfo,
                    bool useParallelBlending);

    /*!
     * \brief   Computes the intersection edge length, whether the fuse removes a face, and an index
     *  of the solid whose face was removed.
     *
     * \param   solid1  The first solid.
     * \param   solid2  The second solid.
     *
     * \return  A std::tuple&lt;double,bool,int&gt representing computed fillet size (-1 if solids do not intersect), 
     *          whether a whole face is removed, and the index of a solid whose face is removed (0 or 1) 
     */
    static std::tuple<double, bool, int> intersectionEdgeLength(mitk::BaseData::Pointer solid1, mitk::BaseData::Pointer solid2);

    /*!
     * \brief   Compute the volume of a solid model.
     */
    static mitk::ScalarType solidVolume(mitk::BaseData::Pointer solid);

    static mitk::ScalarType contourArea(mitk::PlanarFigure::Pointer figure);
};

} // namespace crimson
