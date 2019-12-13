#pragma once 

#include "mitkOperation.h"

#include <VesselTreeExports.h>
#include "VesselPathAbstractData.h"

namespace crimson {

class VesselTree_EXPORT VesselPathOperation : public mitk::Operation
{
public:
    typedef VesselPathAbstractData::PointType PointType;
    typedef VesselPathAbstractData::IdType IdType;

    /*!
     * \brief   Operation that handles all actions on one Point.
     *
     * \param   operationType   The type of the operation (see mitkOperation.h; e.g. move or add; Information for StateMachine::ExecuteOperation());
     * \param   point           The information of the point to add or is the information to change a point into.
     * \param   id              The position in a list which describes the element to change
     * \param   tension         The spline tension.
     */
    VesselPathOperation(mitk::OperationType operationType, PointType point, IdType id = 0, mitk::ScalarType tension = -1);

    virtual ~VesselPathOperation();

    PointType GetPoint() const;
    IdType GetIndex() const;
    mitk::ScalarType GetTension() const;

private:
    PointType m_Point;

    IdType m_Index;
    mitk::ScalarType m_Tension;
};

}

