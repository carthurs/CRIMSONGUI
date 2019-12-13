#include "SolidData.h"

#include <vtkIdList.h>
#include <vtkPolyData.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkNew.h>
#include <vtkShortArray.h>
#include <vtkCellData.h>

namespace crimson
{

SolidData::SolidData()
{
    SolidData::InitializeTimeGeometry(1);
    _surfaceRepresentation = mitk::Surface::New();
}

SolidData::SolidData(const Self& other)
    : mitk::BaseData(other)
    , _surfaceRepresentation(other._surfaceRepresentation->Clone())
    , _faceIdentifierMap(other._faceIdentifierMap)
    , _inflowFaceId(other._inflowFaceId)
    , _outflowFaceId(other._outflowFaceId)
{
}

SolidData::~SolidData() {}

void SolidData::UpdateOutputInformation()
{
    if (this->GetSource()) {
        this->GetSource()->UpdateOutputInformation();
    }
}

void SolidData::PrintSelf(std::ostream& os, itk::Indent indent) const
{
    os << indent << "Volume: " << getVolume() / 1000.0 << " ml";
    mitk::BaseData::PrintSelf(os, indent);
}


} // namespace crimson
