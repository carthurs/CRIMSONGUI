#pragma once

#include <mitkBaseData.h>
#include <mitkVector.h>

#include <vtkSmartPointer.h>
#include <vtkPolyData.h>

#include "VesselTreeExports.h"

namespace crimson {

/*! \brief   An interface class for vessel path data. */
class VesselTree_EXPORT VesselPathAbstractData : public mitk::BaseData
{
public:
    mitkClassMacro(VesselPathAbstractData, mitk::BaseData);

    typedef mitk::Point3D PointType;
    typedef mitk::Vector3D VectorType;
    typedef mitk::ScalarType ParameterType;
    typedef size_t IdType;

    /*! \brief   Encapsulates the result of a closest point query. */
    struct ClosestPointQueryResult {
        enum {
            CLOSEST_POINT_NONE, ///< Could not find any points (e.g. curve is empty)
            CLOSEST_POINT_ENDPOINT, ///< The closest point is one of endpoints
            CLOSEST_POINT_CURVE ///< The closest point is along the curve
        } resultType;

        PointType closestPoint; ///< The closest point

        IdType controlPointId;  ///< If resultType == CLOSEST_POINT_ENDPOINT - the index of endpoint
                                ///  If resultType == CLOSEST_POINT_CURVE - the closest point is between controlPointId and controlPointId + 1
        ParameterType t;    ///< The arc-length along the vessel path to the closest point 
    };

public:

    typedef std::string VesselPathUIDType;
    static const char* VesselUIDPropertyKey;    ///< The property key for the storage of the VesselPathUID

    /*! \brief Get unique identifier of the vessel */
    VesselPathUIDType getVesselUID() const;

    //////////////////////////////////////////////////////////////////////////
    // Parametric access interface
    //////////////////////////////////////////////////////////////////////////

    /*!
     * \brief   Computes the position at the arc-length 't' along the vessel path.
     *          The valid parameter values are in range [0, getParametricLength()].
     */
    virtual PointType getPosition(ParameterType t) const = 0;

    /*!
     * \brief   Computes the tangent vector (first derivative)
     */
    virtual VectorType getTangentVector(ParameterType t) const = 0;

    /*!
     * \brief   Computes the normal vector, the binormal can be computed by taking
     *  a cross product of tangent and normal vectors.
     */
    virtual VectorType getNormalVector(ParameterType t) const = 0;

    /*!
     * \brief   Gets the parametric length of the curve.
     */
    virtual ParameterType getParametricLength() const = 0;

    /*!
     * \brief   Gets the closest point on the curve.
     */
    virtual ClosestPointQueryResult getClosestPoint(const PointType& p) const = 0;

    /*!
     * \brief   Gets the vessel path as vtkPolyData.
     */
    virtual vtkSmartPointer<vtkPolyData> getPolyDataRepresentation() const = 0;

    //////////////////////////////////////////////////////////////////////////
    // Control points access
    //////////////////////////////////////////////////////////////////////////
    /*!
     * \brief   Gets number of control points.
     */
    virtual IdType controlPointsCount() const = 0;

    /*!
     * \brief   Gets position of a control point.
     *
     * \param   id  Control point index.
     */
    virtual PointType getControlPoint(IdType id) const = 0;

    /*!
     * \brief   Sets control point.
     *
     * \param   id  Control point index
     * \param   pos Position to set.
     *
     * \return  false if the index is out of range.
     */
    virtual bool setControlPoint(IdType id, const PointType& pos) = 0;

    /*!
     * \brief   Sets all the control points.
     *
     * \param   controlPoints   The control points.
     */
    virtual bool setControlPoints(const std::vector<PointType>& controlPoints) = 0;

    /*!
     * \brief   Adds a control point to the end of the vessel path.
     *
     * \param   pos The position of the new control point.
     *
     * \return  An index of the new control point.
     */
    virtual IdType addControlPoint(const PointType& pos) = 0;

    /*!
     * \brief   Inserts a control point to a particular index.
     *
     * \param   id  The index to insert control point before.
     * \param   pos The position of the new control point.
     *
     * \return  An index of the new control point.
     */
    virtual IdType addControlPoint(IdType id, const PointType& pos) = 0;

    /*!
     * \brief   Removes the control point at index 'id'.
     */
    virtual bool removeControlPoint(IdType id) = 0;

    /*!
     * \brief   Gets the parameter value along the vessel path for a control point with index 'id'.
     */
    virtual ParameterType getControlPointParameterValue(IdType id) const = 0;

public:
    //////////////////////////////////////////////////////////////////////////
    // Events
    //////////////////////////////////////////////////////////////////////////
    itkEventMacro(VesselPathEvent, itk::AnyEvent);
    itkEventMacro(AllControlPointsSetEvent, VesselPathEvent);

    class ControlPointEvent : public VesselPathEvent {
    public:
        typedef ControlPointEvent Self;
        typedef VesselPathEvent Superclass;

        // @path is the pointer to the path whose insertion/removal caused the change of vessel forest size
        ControlPointEvent() : _id(0) {}
        virtual ~ControlPointEvent() {}
        virtual const char * GetEventName() const { return "ControlPointEvent"; }
        virtual bool CheckEvent(const ::itk::EventObject* e) const { return dynamic_cast<const Self*>(e); }
        virtual ::itk::EventObject* MakeObject() const { return new Self(); }
        void SetControlPointId(IdType id) { _id = id; }
        IdType GetControlPointId() const { return _id; }

    private:
        IdType _id;
        void operator=(const Self&) = delete;
    };

    itkEventMacro(ControlPointInsertEvent, ControlPointEvent);
    itkEventMacro(ControlPointRemoveEvent, ControlPointEvent);
    itkEventMacro(ControlPointModifiedEvent, ControlPointEvent);

public:
    void UpdateOutputInformation() override;
    void SetRequestedRegionToLargestPossibleRegion() override {}
    bool RequestedRegionIsOutsideOfTheBufferedRegion() override { return false; }
    bool VerifyRequestedRegion() override { return true; }
    void SetRequestedRegion(const itk::DataObject *) override {}

    void ExecuteOperation(mitk::Operation* operation) override;

protected:
    VesselPathAbstractData();
    virtual ~VesselPathAbstractData();

    VesselPathAbstractData(const Self& other);

    // purposely not implemented
    static Pointer New();

private:
    void _generateVesselUID();
};

}