#include <vtkXMLPolyDataReader.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkNew.h>

#include <boost/algorithm/string.hpp>

#include "DiscreteSolidDataIO.h"

#include <SolidData.h>
#include <DiscreteSolidData.h>
#include <IO/DiscreteDataIOMimeTypes.h>

#include <IO/IOUtilDataSerializer.h>

#include <SolidDataFaceInfoBoostIO.h>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>

#include <qstandardpaths.h>

#include <FaceIdentifierBoostIO.h>



REGISTER_IOUTILDATA_SERIALIZER(DiscreteSolidData,
	crimson::DiscreteDataIOMimeTypes::DISCRETESOLIDDATA_DEFAULT_EXTENSION(),
    crimson::DiscreteDataIOMimeTypes::DISCRETESOLIDDATA_DEFAULT_EXTENSION() + ".vtp",
    crimson::DiscreteDataIOMimeTypes::DISCRETESOLIDDATA_DEFAULT_EXTENSION() + ".faceinfo"
	)

namespace crimson {


	DiscreteSolidDataIO::DiscreteSolidDataIO()
		: AbstractFileIO(DiscreteSolidData::GetStaticNameOfClass(),
		DiscreteDataIOMimeTypes::DISCRETESOLIDDATA_MIMETYPE(),
		"Discrete solid model data")
	{
		RegisterService();
	}

	DiscreteSolidDataIO::DiscreteSolidDataIO(const DiscreteSolidDataIO& other)
		: AbstractFileIO(other)
	{
	}

	DiscreteSolidDataIO::~DiscreteSolidDataIO()
	{
	}

	std::vector< itk::SmartPointer<mitk::BaseData> > DiscreteSolidDataIO::Read()
	{
		std::vector< itk::SmartPointer<mitk::BaseData> > result;

		auto solid = DiscreteSolidData::New();

		std::ifstream faceInfoFile(GetLocalFileName() + ".faceinfo");

		if (faceInfoFile.good()) {
			// Check if new version with boost serialization
			boost::archive::xml_iarchive inArchive(faceInfoFile);

			SolidData& dataRef = *solid;
			inArchive >> BOOST_SERIALIZATION_NVP(dataRef);
		}
		else {
			mitkThrow() << "Failed to load the model face information" << GetLocalFileName();
		}

		vtkNew<vtkXMLPolyDataReader> polyDataReader;
		polyDataReader->SetFileName((GetLocalFileName() + ".vtp").c_str());

		if (polyDataReader->CanReadFile((GetLocalFileName() + ".vtp").c_str())) {
			polyDataReader->Update();
			mitk::Surface::Pointer surf = mitk::Surface::New();
			surf->SetVtkPolyData(polyDataReader->GetOutput());
			solid->setSurfaceRepresentation(surf);
		}
		else {
			mitkThrow() << "Failed to read mesh from " << GetLocalFileName();
		}

		result.push_back(solid.GetPointer());
		return result;
	}
        
	void DiscreteSolidDataIO::Write()
	{
		auto solid = static_cast<const SolidData*>(this->GetInput());

		if (!solid) {
			mitkThrow() << "Input Discrete solid data has not been set!";
		}

        // Write 'tag' file
        std::ofstream dsdFile(GetOutputLocation());
        dsdFile << "This is a stub file for CRIMSON discrete solid data";

        // Save face information
		std::ofstream faceInfoFile(GetOutputLocation() + ".faceinfo");
		boost::archive::xml_oarchive outArchive(faceInfoFile);

		auto& dataRef = *solid;
		outArchive << BOOST_SERIALIZATION_NVP(dataRef);

		// Save polygonal representation
		vtkSmartPointer<vtkXMLPolyDataWriter> pdWriter = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
		pdWriter->SetDataModeToBinary();
		pdWriter->SetInputData(solid->getSurfaceRepresentation()->GetVtkPolyData());
		pdWriter->SetFileName((GetOutputLocation() + ".vtp").c_str());
		pdWriter->Update();
	}
	
}