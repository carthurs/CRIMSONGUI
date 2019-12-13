#include <SolutionData.h>

namespace crimson
{
SolutionData::SolutionData(vtkSmartPointer<vtkDataArray> data)
    : _data(data)
{
    InitializeTimeGeometry(1);
    Expects(data.GetPointer() != nullptr);
}

vtkDataArray* SolutionData::getArrayData() const { return _data.GetPointer(); }
}
