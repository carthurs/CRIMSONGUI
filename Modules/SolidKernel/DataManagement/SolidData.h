#pragma once

#include <mitkBaseData.h>
#include <mitkSurface.h>
#include <mitkPlaneGeometry.h>

#include "FaceIdentifier.h"

#include "SolidKernelExports.h"

#include <boost/serialization/version.hpp>

namespace boost {
namespace serialization  {
class access;
}
}

namespace crimson {

class SolidDataIO;

/*!
 * \brief   SolidData represents an abstract solid model class.
 */
class SolidKernel_EXPORT SolidData : public mitk::BaseData
{
public:
    mitkClassMacro(SolidData, BaseData);
    //itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);
    //mitkCloneMacro(Self);

public:
	
    ////////////////////////////////////////////////////////////
    // mitk::BaseData interface implementation
    ////////////////////////////////////////////////////////////
    void UpdateOutputInformation() override;
    void SetRequestedRegionToLargestPossibleRegion() override {}
    bool RequestedRegionIsOutsideOfTheBufferedRegion() override { return false; }
    bool VerifyRequestedRegion() override { return true; }
    void SetRequestedRegion(const itk::DataObject *) override {}
    void PrintSelf(std::ostream& os, itk::Indent indent) const override;
	

     /*!
     * \brief   Gets the model representation as an mitk::Surface (primarily for rendering)
     */
    virtual mitk::Surface::Pointer getSurfaceRepresentation() const =0;

    /*!
     * \brief   Get the model volume.
     */
    virtual mitk::ScalarType getVolume() const=0;

    /*!
     * \brief   Gets the FaceIdentifierMap (see FaceIdentifierMap documentation for details)
     */
    FaceIdentifierMap& getFaceIdentifierMap() { return _faceIdentifierMap; }

    /*!
     * \brief   Gets the FaceIdentifierMap (see FaceIdentifierMap documentation for details)
     */
    const FaceIdentifierMap& getFaceIdentifierMap() const { return _faceIdentifierMap; }

    /*!
     * \brief   Gets the normal to the face defined by a FaceIdentifier. Works for planar faces only.
     */
    virtual mitk::Vector3D getFaceNormal(const FaceIdentifier& faceId) const=0;

    /*!
     * \brief   Gets the distance from a 3D point to the edge of the face. Works for non-subdivided
     *  faces only (i.e. faceId.parentSolidIndices.size() == 1)
     */
    virtual double getDistanceToFaceEdge(const FaceIdentifier& faceId, const mitk::Point3D& pos) const=0;

    ///@{ 
    /*!
     * \brief   Sets the face index of the inflow face as defined by the OpenCASCADE's traversal order
     *  over the shape's faces (for lofted models only)
     */
    void setInflowFaceId(int faceIndex) { _inflowFaceId = faceIndex; }

    /*!
     * \brief   Sets the face index of the outflow face as defined by the OpenCASCADE's traversal
     *  order over the shape's faces (for lofted models only)
     */
    void setOutflowFaceId(int faceIndex) { _outflowFaceId = faceIndex; }

    /*!
     * \brief   Gets the face index of the inflow face as defined by the OpenCASCADE's traversal order
     *  over the shape's faces (for lofted models only)
     */
    int inflowFaceId() const { return _inflowFaceId; }

    /*!
     * \brief   Gets the face index of the outflow face as defined by the OpenCASCADE's traversal
     *  order over the shape's faces (for lofted models only)
     */
    int outflowFaceId() const { return _outflowFaceId; }

	/*!
	* \brief   Returns PlaneGeometry for the face given by FaceIdentifier
	*/
	virtual mitk::PlaneGeometry::Pointer getFaceGeometry(const FaceIdentifier& faceId, const mitk::Vector3D& referenceImageSpacing, mitk::ScalarType resliceWindowSize) const = 0;
    ///@} 
protected:
    SolidData();
    virtual ~SolidData();

    SolidData(const Self& other);

    //friend class OCCBRepDataIO;
    template<class Archive>
    friend void serialize(Archive & ar, crimson::SolidData& data, const unsigned int version);
    friend class boost::serialization::access;

    mutable mitk::Surface::Pointer _surfaceRepresentation = nullptr;

    int _inflowFaceId = -1;
    int _outflowFaceId = -1;

    FaceIdentifierMap _faceIdentifierMap;
};

} // namespace crimson

BOOST_CLASS_VERSION(crimson::SolidData, 1)