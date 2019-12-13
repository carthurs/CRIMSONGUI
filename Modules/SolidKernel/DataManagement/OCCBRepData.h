#pragma once

#include <SolidData.h>
#include <mitkSurface.h>
#include <TopoDS_Shape.hxx>

#include "FaceIdentifier.h"

#include "SolidKernelExports.h"

#include <boost/serialization/version.hpp>

namespace boost {
namespace serialization  {
class access;
}
}

namespace crimson {

class OCCBRepDataIO;

/*!
 * \brief   OCCBRepData represents a solid model stored as an OpenCASCADE boundary representation
 *  shape.
 */
class SolidKernel_EXPORT OCCBRepData : public SolidData
{
public:
    mitkClassMacro(OCCBRepData, SolidData);
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);
    mitkCloneMacro(Self);

public:

    ///@{ 
    /*!
     * \brief   Sets the OpenCASCADE representation.
     */
    void setShape(const TopoDS_Shape& shape);

    /*!
     * \brief   Gets the OpenCASCADE representation.
     */
    const TopoDS_Shape& getShape() const { return _shape;  }
    ///@} 

    /*!
     * \brief   Gets the model representation as an mitk::Surface (primarily for rendering)
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
    OCCBRepData();
    virtual ~OCCBRepData();

    OCCBRepData(const Self& other);

private:
    friend class OCCBRepDataIO;
    //template<class Archive>
	//friend void serialize(Archive & ar, crimson::OCCBRepData& data, const unsigned int version);
    //friend class boost::serialization::access;

    TopoDS_Shape _shape;

};

} // namespace crimson

//BOOST_CLASS_VERSION(crimson::OCCBRepData, 1)