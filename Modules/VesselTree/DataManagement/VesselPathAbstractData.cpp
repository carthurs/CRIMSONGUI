#include "VesselPathAbstractData.h"
#include "VesselPathOperation.h"

#include <mitkInteractionConst.h>
#include <mitkUIDGenerator.h>

namespace crimson {

VesselPathAbstractData::VesselPathAbstractData()
{
    // Currently only single-time-step geometries are supported
    VesselPathAbstractData::InitializeTimeGeometry(1);
    _generateVesselUID();
}

VesselPathAbstractData::VesselPathAbstractData(const Self& other)
    : mitk::BaseData(other)
{
    _generateVesselUID();
}

VesselPathAbstractData::~VesselPathAbstractData()
{
}

const char* VesselPathAbstractData::VesselUIDPropertyKey = "PathUID";
void VesselPathAbstractData::_generateVesselUID()
{
    this->GetPropertyList()->SetStringProperty(VesselUIDPropertyKey, mitk::UIDGenerator("PATH_").GetUID().c_str());
}

auto VesselPathAbstractData::getVesselUID() const -> VesselPathUIDType
{
    std::string uid;
    bool rc = this->GetPropertyList()->GetStringProperty(VesselUIDPropertyKey, uid);
    assert(rc);
    // Avoid unused variable warning
    (void)rc;

    return uid;
}

void VesselPathAbstractData::UpdateOutputInformation()
{
    if (this->GetSource()) {
        this->GetSource()->UpdateOutputInformation();
    }
}


#define CheckOperationTypeMacro(OperationType, operation, newOperationName) \
    OperationType *newOperationName = dynamic_cast<OperationType *>(operation); \
    if (newOperationName == NULL)\
        {\
        itkWarningMacro("Recieved wrong type of operation!"); \
        return; \
        }

void VesselPathAbstractData::ExecuteOperation(mitk::Operation* operation)
{
    CheckOperationTypeMacro(VesselPathOperation, operation, vesselPathOp);
    switch (operation->GetOperationType()) {
    case mitk::OpNOTHING:
        break;

    case mitk::OpINSERT://inserts the point at the given position and selects it.
        this->addControlPoint(vesselPathOp->GetIndex(), vesselPathOp->GetPoint());
        break;

    case mitk::OpMOVE://moves the point given by index
        this->setControlPoint(vesselPathOp->GetIndex(), vesselPathOp->GetPoint());
        break;

    case mitk::OpREMOVE://removes the point at given by position
        this->removeControlPoint(vesselPathOp->GetIndex());
        break;

    default:
        itkWarningMacro("VesselPathAbstractData could not understrand the operation. Please check!");
        break;
    }

    mitk::OperationEndEvent endevent(operation);
    static_cast<const itk::Object*>(this)->InvokeEvent(endevent);
}

}
