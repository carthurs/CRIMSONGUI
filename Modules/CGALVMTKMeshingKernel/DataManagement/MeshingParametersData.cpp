#include "MeshingParametersData.h"

namespace crimson {

MeshingParametersData::MeshingParametersData()
{
    MeshingParametersData::InitializeTimeGeometry(1);
}

MeshingParametersData::~MeshingParametersData()
{
}

void MeshingParametersData::UpdateOutputInformation()
{
    if (this->GetSource()) {
        this->GetSource()->UpdateOutputInformation();
    }
}

void MeshingParametersData::PrintSelf(std::ostream& os, itk::Indent indent) const
{
    mitk::BaseData::PrintSelf(os, indent);
}

} // namespace crimson
