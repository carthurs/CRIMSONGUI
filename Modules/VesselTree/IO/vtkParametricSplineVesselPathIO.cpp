#include <tinyxml.h>

#include "vtkParametricSplineVesselPathIO.h"

#include <vtkParametricSplineVesselPathData.h>
#include <IO/VesselTreeIOMimeTypes.h>

#include <IO/IOUtilDataSerializer.h>

#include <vtkNew.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkPolyLine.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>

REGISTER_IOUTILDATA_SERIALIZER(vtkParametricSplineVesselPathData, crimson::VesselTreeIOMimeTypes::VTKPARAMETRICSPLINEVESSELPATH_DEFAULT_EXTENSION())

namespace crimson {

vtkParametricSplineVesselPathIO::vtkParametricSplineVesselPathIO()
    : AbstractFileIO(vtkParametricSplineVesselPathData::GetStaticNameOfClass(),
    VesselTreeIOMimeTypes::VTKPARAMETRICSPLINEVESSELPATH_MIMETYPE(),
    "Vessel tree data")
{
    RegisterService();
}

vtkParametricSplineVesselPathIO::vtkParametricSplineVesselPathIO(const vtkParametricSplineVesselPathIO& other)
    : AbstractFileIO(other)
{
}

vtkParametricSplineVesselPathIO::~vtkParametricSplineVesselPathIO()
{
}

std::vector< itk::SmartPointer<mitk::BaseData> > vtkParametricSplineVesselPathIO::Read()
{
    std::vector< itk::SmartPointer<mitk::BaseData> > result;

    auto vesselPath = vtkParametricSplineVesselPathData::New();

    vtkNew<vtkXMLPolyDataReader> reader;
    reader->SetFileName(GetLocalFileName().c_str());
    reader->Update();

    vtkPolyData* polyData = reader->GetOutput();

    if (!polyData) {
        mitkThrow() << "Failed to load file " << GetLocalFileName() << ".";
    }

    if (polyData->GetNumberOfCells() == 0) {
        mitkThrow() << "Failed to find the vessel path description in file " << GetLocalFileName() << ".";
    }

    if (polyData->GetNumberOfCells() != 1) {
        MITK_WARN << "More than one cell found in file " << GetLocalFileName() << ". Reading only the first one.";
    }

    vtkPoints* points = polyData->GetPoints();
    vtkCell* cell = polyData->GetCell(0);

    if (cell->GetCellType() != VTK_POLY_LINE && cell->GetCellType() != VTK_LINE) {
        mitkThrow() << "Unexpected non-polyline cell in vessel tree data";
    }

    vtkIdList* pointIds = cell->GetPointIds();
    std::vector<VesselPathAbstractData::PointType> controlPoints;
    for (vtkIdType i = 0; i < pointIds->GetNumberOfIds(); ++i) {
        double* posPtr = points->GetPoint(pointIds->GetId(i));
        controlPoints.push_back(VesselPathAbstractData::PointType(posPtr));
    }

    vesselPath->setControlPoints(controlPoints);

    vtkDoubleArray* tensionArray = static_cast<vtkDoubleArray*>(polyData->GetCellData()->GetArray("tension"));
    if (tensionArray) {
        vesselPath->setTension(tensionArray->GetTuple1(0));
    }

    result.push_back(vesselPath.GetPointer());
    return result;
}

void vtkParametricSplineVesselPathIO::Write()
{
    auto vesselPath = static_cast<const vtkParametricSplineVesselPathData*>(this->GetInput());

    if (!vesselPath) {
        mitkThrow() << "Input vessel forest data has not been set!";
    }

    // Create temporary vtkPolyData representation
    vtkNew<vtkPolyData> polyData;
    vtkNew<vtkPoints> points;

    polyData->SetPoints(points.GetPointer());
    polyData->Allocate(1);

    vtkNew<vtkIdList> polyLine;

    for (VesselPathAbstractData::IdType controlPointId = 0; controlPointId < vesselPath->controlPointsCount(); ++controlPointId) {
        VesselPathAbstractData::PointType pos = vesselPath->getControlPoint(controlPointId);
        polyLine->InsertNextId(points->InsertNextPoint(pos[0], pos[1], pos[2]));
    }

    double tension = vesselPath->getTension();

    vtkNew<vtkDoubleArray> tensionArray;
    tensionArray->SetNumberOfTuples(1);
    tensionArray->SetTuple(0, &tension);
    tensionArray->SetName("tension");

    polyData->GetCellData()->AddArray(tensionArray.GetPointer());

    polyData->InsertNextCell(VTK_POLY_LINE, polyLine.GetPointer());

    vtkNew<vtkXMLPolyDataWriter> writer;
    writer->SetInputData(polyData.GetPointer());
    writer->SetFileName(GetOutputLocation().c_str());
    writer->SetDataModeToBinary();
    writer->Write();
}



}