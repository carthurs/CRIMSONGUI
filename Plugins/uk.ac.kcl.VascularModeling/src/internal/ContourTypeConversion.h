#pragma once

#include <type_traits>

#include <mitkPlanarFigure.h>
#include <mitkPlanarCircle.h>
#include <mitkPlanarEllipse.h>
#include <mitkPlanarSubdivisionPolygon.h>

namespace crimson
{

//////////////////////////////////////////////////////////////////////////
// Default conversions
//////////////////////////////////////////////////////////////////////////

// The default conversion of a contour type to itself which simply clones the contour
template <typename ContourToTypePointer, typename Dummy = typename std::enable_if<std::is_base_of<
                                             mitk::PlanarFigure, typename ContourToTypePointer::ObjectType>::value>::type>
void convertContourType(ContourToTypePointer& to, typename ContourToTypePointer::ObjectType* from)
{
    to = from;
}

// The default conversion from arbitrary planar figure. Sets "to" to nullptr unless overloaded
template <typename ContourToTypePointer, typename Dummy = typename std::enable_if<std::is_base_of<
                                             mitk::PlanarFigure, typename ContourToTypePointer::ObjectType>::value>::type>
void convertContourType(ContourToTypePointer& to, mitk::PlanarFigure* /*from*/)
{
    to = nullptr;
}

//////////////////////////////////////////////////////////////////////////
// Circle conversions
//////////////////////////////////////////////////////////////////////////

// Fit a circle to a polyline defining the planar figure
extern void convertContourType(mitk::PlanarCircle::Pointer& to, mitk::PlanarFigure* from);

//////////////////////////////////////////////////////////////////////////
// Ellipse conversions
//////////////////////////////////////////////////////////////////////////

// Fit an ellipse to a polyline defining the planar figure
extern void convertContourType(mitk::PlanarEllipse::Pointer& to, mitk::PlanarFigure* from);

//////////////////////////////////////////////////////////////////////////
// Subdivision polygon conversions
//////////////////////////////////////////////////////////////////////////

// Fit a subdivision polygon to a polyline defining the planar figure
extern void convertContourType(mitk::PlanarSubdivisionPolygon::Pointer& to, mitk::PlanarFigure* from);

//////////////////////////////////////////////////////////////////////////
// Selection of the conversion operation relying on the compiler to select
// the correct function
//////////////////////////////////////////////////////////////////////////

// Define the contour type using RTTI and try to convert the contour to the target type using this information
#define tryDynamicConversionFrom(Type)                                                                                         \
    if (dynamic_cast<Type*>(from)) {                                                                                           \
        convertContourType(out, static_cast<Type*>(from));                                                                     \
        return out;                                                                                                            \
    }

template <typename ContourToType,
          typename Dummy = typename std::enable_if<std::is_base_of<mitk::PlanarFigure, ContourToType>::value>::type>
typename ContourToType::Pointer tryConvertContourType(mitk::PlanarFigure* from)
{
    typename ContourToType::Pointer out;

    tryDynamicConversionFrom(mitk::PlanarCircle) tryDynamicConversionFrom(mitk::PlanarEllipse)
        tryDynamicConversionFrom(mitk::PlanarSubdivisionPolygon) tryDynamicConversionFrom(mitk::PlanarPolygon)

        // If the type of "from" contour is unknown - try converting directly from planar figure
        convertContourType(out, from);
    return out;
}

#undef tryDynamicConversionFrom

} // namespace crimson
