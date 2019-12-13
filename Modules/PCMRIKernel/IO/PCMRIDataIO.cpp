#include <vtkXMLPolyDataReader.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkNew.h>
#include <vtkPointData.h>

#include <boost/algorithm/string.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/array.hpp>

#include "PCMRIDataIO.h"
#include <PCMRIDataBoostIO.h>

#include <PCMRIData.h>
#include <IO/PCMRIKernelIOMimeTypes.h>

#include <IO/IOUtilDataSerializer.h>


REGISTER_IOUTILDATA_SERIALIZER(PCMRIData,
	crimson::PCMRIKernelIOMimeTypes::PCMRIDATA_DEFAULT_EXTENSION(),
	crimson::PCMRIKernelIOMimeTypes::PCMRIDATA_DEFAULT_EXTENSION() + ".vtp"
    )

namespace crimson {

PCMRIDataIO::PCMRIDataIO()
	: AbstractFileIO(PCMRIData::GetStaticNameOfClass(), 
	PCMRIKernelIOMimeTypes::PCMRIDATA_MIMETYPE(),
    "PCMRI data")
{
    RegisterService();
}

PCMRIDataIO::PCMRIDataIO(const PCMRIDataIO& other)
	: AbstractFileIO(other)
{
}

PCMRIDataIO::~PCMRIDataIO()
{
}


std::vector< itk::SmartPointer<mitk::BaseData> > PCMRIDataIO::Read()
{
    std::vector< itk::SmartPointer<mitk::BaseData> > result;

	itk::SmartPointer<crimson::PCMRIData> brep = (itk::SmartPointer<crimson::PCMRIData>) PCMRIData::New();

    std::ifstream PCMRIFile(GetLocalFileName());
    boost::archive::xml_iarchive inArchive(PCMRIFile);
    auto& dataRef = *brep;
    inArchive >> BOOST_SERIALIZATION_NVP(dataRef);

    // Read polygonal representation
    vtkSmartPointer<vtkXMLPolyDataReader> pdReader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
    pdReader->SetFileName((GetLocalFileName() + ".vtp").c_str());

    if (pdReader->CanReadFile((GetLocalFileName() + ".vtp").c_str()) != 0) {
        pdReader->Update();
        dynamic_cast<PCMRIData *>(brep.GetPointer())->_surfaceRepresentation = mitk::Surface::New();
		dynamic_cast<PCMRIData *>(brep.GetPointer())->_surfaceRepresentation->SetVtkPolyData(pdReader->GetOutput());
    }

    result.push_back(brep.GetPointer());
    return result;
}

void PCMRIDataIO::Write()
{
    auto pcmriData = static_cast<const PCMRIData*>(this->GetInput());

    if (!pcmriData) {
        mitkThrow() << "Input pcmri data has not been set!";
    }

    if (!pcmriData->getSurfaceRepresentation()) {
        mitkThrow() << "Empty mesh surface data within PCMRIData";
    }

    //Save PCMRIData
    std::ofstream outStream(GetOutputLocation());
    boost::archive::xml_oarchive outArchive(outStream);

    auto& dataRef = *pcmriData;
    outArchive << BOOST_SERIALIZATION_NVP(dataRef);

    //Save mesh surface polydata
    vtkNew<vtkXMLPolyDataWriter> pdWriter;
    pdWriter->SetDataModeToBinary();
	pdWriter->SetInputData(pcmriData->getSurfaceRepresentation()->GetVtkPolyData());
    pdWriter->SetFileName((GetOutputLocation() + ".vtp").c_str());
    pdWriter->Update();
}

}