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
        } else {
            mitkThrow() << "Failed to load the model face information" << GetLocalFileName();
        }

        vtkNew<vtkXMLPolyDataReader> polyDataReader;
        polyDataReader->SetFileName((GetLocalFileName() + ".vtp").c_str());

        if (polyDataReader->CanReadFile((GetLocalFileName() + ".vtp").c_str())) {
            polyDataReader->Update();
            mitk::Surface::Pointer surf = mitk::Surface::New();
            surf->SetVtkPolyData(polyDataReader->GetOutput());
            solid->setSurfaceRepresentation(surf);
        } else {
            mitkThrow() << "Failed to read mesh from " << GetLocalFileName();
        }

        result.push_back(solid.GetPointer());
        return result;
        
// 		itk::SmartPointer<crimson::SolidData> brep = (itk::SmartPointer<crimson::SolidData>) DiscreteSolidData::New();
// 
// 		pProgress progress = Progress_new();
// 		Progress_setDefaultCallback(progress);
// 
// 		try {			
// 			
// 			meshsim_shared_ptr<pDiscreteModel> model = nullptr;  // pointer to our Discrete model
// 
// 			if (boost::algorithm::iends_with(GetLocalFileName(), ".stl")) {
// 
// 
// 				//Import mesh from stl and build a model from that mesh
// 				meshsim_shared_ptr<pMesh> newMesh = make_meshsim_shared(M_new(0, 0)); //mesh to load
// 				char solidName[200]; // array to hold name of solid, if present
// 				int finished = 0;
// 
// 				if (!M_importSolidFromSTLFile(newMesh.get(), GetLocalFileName().c_str(), finished, solidName, progress)) {
// 					mitkThrow() << "Failed to read " << GetLocalFileName() << "!";
// 				}
// 
// 				// check the input mesh for intersections
// 				// this call must occur before the discrete model is created
// 				if (MS_checkMeshIntersections(newMesh.get(), 0, progress)) {
// 					mitkThrow() << "There are intersections in the input mesh" << endl;
// 				}
// 
// 				// create the Discrete model
// 				model = make_meshsim_shared(DM_createFromMesh(newMesh.get(), 1, progress)); // pointer to the temporary model
// 				if (!model) { //check for error
// 					mitkThrow() << "Error creating Discrete model from mesh" << endl;
// 				}
// 
// 				// define the Discrete model
// 				DM_findEdgesByFaceNormals(model.get(), 45, progress);
// 				DM_eliminateDanglingEdges(model.get(), progress);
// 				if (DM_completeTopology(model.get(), progress)) { //check for error
// 					mitkThrow() << "Error completing Discrete model topology" << endl;
// 				}
// 
// 				QString pathName = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
// 				GM_write(model.get(), (pathName + "/discreteModel.smd").toStdString().c_str(), 0, progress); // save the discrete model FOR DEBUGGING PURPOSES
// 
// 
// 				// If a solid model is imported from an external tool, create a face identifier map from scratch
// 				int discreteFaceIndex = 0; //numbering has to start from zero
// 
// 				meshsim_unique_ptr<GFIter> fIter = make_meshsim_unique(GM_faceIter(model.get()));  // initialize the iterator
// 				pGFace modelFace;
// 
// 				while (modelFace = GFIter_next(fIter.get())){ // get next face
// 
// 					crimson::FaceIdentifier faceId;
// 
// 					pPList edges = GF_edges(modelFace);
// 					int size = PList_size(edges);
// 
// 					// Label all the planar faces as outflow and all curved faces as wall
// 
// 					faceId.faceType = (size == 1) ? FaceIdentifier::ftCapOutflow : FaceIdentifier::ftWall;
// 
// 					faceId.parentSolidIndices.insert("face " + std::to_string(discreteFaceIndex));
// 
// 					brep->getFaceIdentifierMap().setFaceIdentifierForModelFace(discreteFaceIndex, faceId);
// 					discreteFaceIndex++;
// 
// 					int faceID = GEN_tag(modelFace); //check for debugging purposes
// 				}
// 			}
// 
// 			else if (boost::algorithm::iends_with(GetLocalFileName(), ".smd")) {
// 
// 				model = make_meshsim_shared((pDiscreteModel)GM_load(GetLocalFileName().c_str(), 0, progress));
// 
// 				if (!model) {
// 					mitkThrow() << "Failed to read " << GetLocalFileName() << "!";
// 				}
// 
// 				// Read face info
// 				std::ifstream faceInfoFile(GetLocalFileName() + ".faceinfo");
// 
// 				if (faceInfoFile.good()) {
// 
// 					try {
// 						// Check if new version with boost serialization
// 						boost::archive::xml_iarchive inArchive(faceInfoFile);
// 
// 						auto& dataRef = *brep;
// 						inArchive >> BOOST_SERIALIZATION_NVP(dataRef);
// 					}
// 					catch (const boost::archive::archive_exception&) {
// 						faceInfoFile.clear();
// 						faceInfoFile.seekg(0);
// 
// 						int nFaces;
// 						faceInfoFile >> nFaces;
// 
// 						for (int i = 0; i < nFaces; ++i) {
// 							crimson::FaceIdentifier faceId;
// 
// 							int nParentSolids;
// 							faceInfoFile >> nParentSolids;
// 							for (int j = 0; j < nParentSolids; ++j) {
// 								std::string parentSolidId;
// 								faceInfoFile >> parentSolidId;
// 								faceId.parentSolidIndices.insert(parentSolidId);
// 							}
// 
// 							int fit;
// 							faceInfoFile >> fit;
// 							faceId.faceType = static_cast<FaceIdentifier::FaceType>(fit);
// 
// 							brep->getFaceIdentifierMap().setFaceIdentifierForModelFace(i, faceId);
// 						}
// 					}
// 				}
// 
// 				// Read polygonal representation
// 				vtkSmartPointer<vtkXMLPolyDataReader> pdReader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
// 				pdReader->SetFileName((GetLocalFileName() + ".vtp").c_str());
// 
// 				if (pdReader->CanReadFile((GetLocalFileName() + ".vtp").c_str()) != 0) {
// 					pdReader->Update();
// 					dynamic_cast<DiscreteSolidData *>(brep.GetPointer())->_surfaceRepresentation = mitk::Surface::New();
// 					dynamic_cast<DiscreteSolidData *>(brep.GetPointer())->_surfaceRepresentation->SetVtkPolyData(pdReader->GetOutput());
// 				}
// 
// 			}
// 	
// 			dynamic_cast<DiscreteSolidData *>(brep.GetPointer())->setModel(model);
// 
// 			result.push_back(brep.GetPointer());
// 			return result;
// 		}
// 		catch (pSimError err) {
// 			MITK_ERROR << "SimModSuite error caught:" << endl;
// 			MITK_ERROR << "  Error code: " << SimError_code(err) << endl;
// 			MITK_ERROR << "  Error string: " << SimError_toString(err) << endl;
// 			SimError_delete(err);
// 			return result;
// 		}
		//catch (...) {
		//	MITK_ERROR << "Unhandled exception caught" << endl;
		//	return result;
		//}
		
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