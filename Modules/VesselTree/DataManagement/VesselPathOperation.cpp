#include "VesselPathOperation.h"

namespace crimson {

VesselPathOperation::VesselPathOperation(mitk::OperationType operationType, PointType point, IdType index, mitk::ScalarType tension)
: mitk::Operation(operationType)
, m_Point(point)
, m_Index(index)
, m_Tension(tension)
{}

VesselPathOperation::~VesselPathOperation()
{
}

auto VesselPathOperation::GetPoint() const -> PointType
{
    return m_Point;
}

auto VesselPathOperation::GetIndex() const -> IdType
{
    return m_Index;
}

mitk::ScalarType VesselPathOperation::GetTension() const
{
    return m_Tension;
}

}