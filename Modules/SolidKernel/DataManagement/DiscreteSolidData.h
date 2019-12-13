#pragma once

#include <array>

#include <SolidData.h>
#include <mitkSurface.h>
#include <mitkUnstructuredGrid.h>

#include "FaceIdentifier.h"

#include "SolidKernelExports.h"

#include <boost/serialization/version.hpp>

namespace boost {
namespace serialization  {
class access;
}
}

namespace crimson {

class DiscreteSolidDataIO;

/*!
 * \brief   DiscreteSolidData represents a solid model stored as a pDiscreteModel
 *  shape.
 */
class SolidKernel_EXPORT DiscreteSolidData : public SolidData
{
public:
    mitkClassMacro(DiscreteSolidData, SolidData);
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);
    mitkCloneMacro(Self);

public:
    /*!
    * \brief   Convert a normal surface to a solid data (detect flat faces, etc.)
    */
    void fromSurface(mitk::Surface::Pointer surface);

    /*!
    * \brief   Sets the model representation as an mitk::Surface
    */
    void setSurfaceRepresentation(mitk::Surface::Pointer surface);

    /*!
     * \brief   Gets the model representation as an mitk::Surface
     */
    mitk::Surface::Pointer getSurfaceRepresentation() const override;

    /*!
     * \brief   Get the model volume.
     */
    mitk::ScalarType getVolume() const override;

      /*!
     * \brief   Gets the normal to the face defined by a FaceIdentifier. Works for planar faces only.
     */
    mitk::Vector3D getFaceNormal(const FaceIdentifier& faceId) const override;

    /*!
     * \brief   Gets the distance from a 3D point to the edge of the face. Works for non-subdivided
     *  faces only (i.e. faceId.parentSolidIndices.size() == 1)
     */
    double getDistanceToFaceEdge(const FaceIdentifier& faceId, const mitk::Point3D& pos) const override;

	/*!
	* \brief   Returns PlaneGeometry for the face given by FaceIdentifier
	*/
	mitk::PlaneGeometry::Pointer getFaceGeometry(const FaceIdentifier& faceId, const mitk::Vector3D& referenceImageSpacing, mitk::ScalarType resliceWindowSize) const override;


protected:
    DiscreteSolidData();
    virtual ~DiscreteSolidData();

    DiscreteSolidData(const Self& other);

    std::vector<vtkSmartPointer<vtkPolyData>> _edges;

private:
    friend class DiscreteSolidDataIO;
};

} // namespace crimson

//BOOST_CLASS_VERSION(crimson::DiscreteSolidData, 1)