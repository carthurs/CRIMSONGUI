#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <IGESControl_Controller.hxx>
#include <IGESControl_Reader.hxx>
#include <IGESControl_Writer.hxx>
#include <STEPControl_Controller.hxx>
#include <STEPControl_Reader.hxx>
#include <STEPControl_Writer.hxx>

#include <TopExp_Explorer.hxx>
#include <Geom_Plane.hxx>
#include <TopoDS.hxx>

#include <vtkXMLPolyDataReader.h>
#include <vtkXMLPolyDataWriter.h>

#include <boost/algorithm/string.hpp>

#include "OCCBRepDataIO.h"

#include <SolidData.h>
#include <OCCBRepData.h>
#include <IO/OpenCascadeSolidKernelIOMimeTypes.h>

#include <IO/IOUtilDataSerializer.h>

#include <SolidDataFaceInfoBoostIO.h>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <BRepOffsetAPI_Sewing.hxx>
#include <GeomLib_IsPlanarSurface.hxx>

REGISTER_IOUTILDATA_SERIALIZER(OCCBRepData,
	crimson::OpenCascadeSolidKernelIOMimeTypes::OCCBREPDATA_DEFAULT_EXTENSION(),
	crimson::OpenCascadeSolidKernelIOMimeTypes::OCCBREPDATA_DEFAULT_EXTENSION() + ".vtp",
	crimson::OpenCascadeSolidKernelIOMimeTypes::OCCBREPDATA_DEFAULT_EXTENSION() + ".faceinfo"
    )

namespace crimson {

static size_t getNSolidsInShape(const TopoDS_Shape& shape)
{
    size_t nSolids = 0;
    for (TopExp_Explorer Exp(shape, TopAbs_SOLID); Exp.More(); Exp.Next()) {
        ++nSolids;
    }
    return nSolids;
}

OCCBRepDataIO::OCCBRepDataIO()
	: AbstractFileIO(OCCBRepData::GetStaticNameOfClass(),
	OpenCascadeSolidKernelIOMimeTypes::OCCBREPDATA_MIMETYPE(),
    "OpenCascade solid model data")
{
    RegisterService();
}

OCCBRepDataIO::OCCBRepDataIO(const OCCBRepDataIO& other)
    : AbstractFileIO(other)
{
}

OCCBRepDataIO::~OCCBRepDataIO()
{
}

TopoDS_Shape OCCBRepDataIO::trySewImportedShape(const TopoDS_Shape& importedShape)
{
    if (getNSolidsInShape(importedShape) == 0) {
        MITK_INFO << "File " << GetLocalFileName() << " does not contain any solids - attempting to sew all faces to a solid";
        BRepOffsetAPI_Sewing faceSewer(1e-4);
        TopExp_Explorer explorer(importedShape, TopAbs_FACE);
        while (explorer.More())
        {
            faceSewer.Add(explorer.Current());
            explorer.Next();
        }
        faceSewer.Perform();

        if (getNSolidsInShape(faceSewer.SewedShape()) == 0) {
            mitkThrow() << "Sewing faces in file " << GetLocalFileName() << " did not produce a solid. Import aborted.";
        }

        return faceSewer.SewedShape();
    } else {
        return importedShape;
    }
}

std::vector< itk::SmartPointer<mitk::BaseData> > OCCBRepDataIO::Read()
{
    std::vector< itk::SmartPointer<mitk::BaseData> > result;

	itk::SmartPointer<crimson::SolidData> brep = (itk::SmartPointer<crimson::SolidData>) OCCBRepData::New();

    TopoDS_Shape newShape;
    if (boost::algorithm::iends_with(GetLocalFileName(), ".brep")) {
        BRep_Builder builder;
        if (!BRepTools::Read(newShape, GetLocalFileName().c_str(), builder)) {
            mitkThrow() << "Failed to read " << GetLocalFileName() << "!";
        }
    }
    else if (boost::algorithm::iends_with(GetLocalFileName(), ".igs") || boost::algorithm::iends_with(GetLocalFileName(), ".iges")) {
        IGESControl_Controller::Init();
        IGESControl_Reader reader;
        if (reader.ReadFile(GetLocalFileName().c_str()) != IFSelect_RetDone) {
            mitkThrow() << "Failed to read " << GetLocalFileName() << "!";
        }
        reader.TransferRoots();

        newShape = trySewImportedShape(reader.OneShape());
    }
    else if (boost::algorithm::iends_with(GetLocalFileName(), ".stp") || boost::algorithm::iends_with(GetLocalFileName(), ".step")) {
        STEPControl_Controller::Init();
        STEPControl_Reader reader;
        if (reader.ReadFile(GetLocalFileName().c_str()) != IFSelect_RetDone) {
            mitkThrow() << "Failed to read " << GetLocalFileName() << "!";
        }
        reader.TransferRoots();

        newShape = trySewImportedShape(reader.OneShape());
    }

	dynamic_cast<OCCBRepData *>(brep.GetPointer())->setShape(newShape);
 
    // Read face info
    std::ifstream faceInfoFile(GetLocalFileName() + ".faceinfo");

    if (faceInfoFile.good()) {

        try {
            // Check if new version with boost serialization
            boost::archive::xml_iarchive inArchive(faceInfoFile);

            auto& dataRef = *brep;
            inArchive >> BOOST_SERIALIZATION_NVP(dataRef);
        }
        catch (const boost::archive::archive_exception&) {
            faceInfoFile.clear();
            faceInfoFile.seekg(0);

            int nFaces;
            faceInfoFile >> nFaces;

            for (int i = 0; i < nFaces; ++i) {
                crimson::FaceIdentifier faceId;

                int nParentSolids;
                faceInfoFile >> nParentSolids;
                for (int j = 0; j < nParentSolids; ++j) {
                    std::string parentSolidId;
                    faceInfoFile >> parentSolidId;
                    faceId.parentSolidIndices.insert(parentSolidId);
                }

                int fit;
                faceInfoFile >> fit;
                faceId.faceType = static_cast<FaceIdentifier::FaceType>(fit);

                brep->getFaceIdentifierMap().setFaceIdentifierForModelFace(i, faceId);
            }
        }
    }
    else {
        // If a solid model is imported from an external tool, create a face identifier map from scratch

        int occFaceIndex = 0;
        for (TopExp_Explorer faceExplorer(newShape, TopAbs_FACE); faceExplorer.More(); faceExplorer.Next(), occFaceIndex++) {
            crimson::FaceIdentifier faceId;
            
            const TopoDS_Face& occFace = TopoDS::Face(faceExplorer.Current());
            
            // Label all the planar faces as outflow and all curved faces as wall
            faceId.faceType = GeomLib_IsPlanarSurface{BRep_Tool::Surface(occFace)}.IsPlanar() ? FaceIdentifier::ftCapOutflow : FaceIdentifier::ftWall;

            faceId.parentSolidIndices.insert("face " + std::to_string(occFaceIndex));

            brep->getFaceIdentifierMap().setFaceIdentifierForModelFace(occFaceIndex, faceId);
        }
    }

    // Read polygonal representation
    vtkSmartPointer<vtkXMLPolyDataReader> pdReader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
    pdReader->SetFileName((GetLocalFileName() + ".vtp").c_str());

    if (pdReader->CanReadFile((GetLocalFileName() + ".vtp").c_str()) != 0) {
        pdReader->Update();
		dynamic_cast<OCCBRepData *>(brep.GetPointer())->_surfaceRepresentation = mitk::Surface::New();
		dynamic_cast<OCCBRepData *>(brep.GetPointer())->_surfaceRepresentation->SetVtkPolyData(pdReader->GetOutput());
    }

    result.push_back(brep.GetPointer());
    return result;
}

void OCCBRepDataIO::Write()
{
    auto brep = static_cast<const SolidData*>(this->GetInput());

    if (!brep) {
        mitkThrow() << "Input OCCBRep data has not been set!";
    }

    if (boost::algorithm::iends_with(GetOutputLocation(), ".brep")) {
		if (!BRepTools::Write(static_cast<const OCCBRepData*>(brep)->getShape(), GetOutputLocation().c_str())) {
            mitkThrow() << "Failed to write " << GetOutputLocation();
        }
    }
    else if (boost::algorithm::iends_with(GetOutputLocation(), ".igs") || boost::algorithm::iends_with(GetOutputLocation(), ".iges")) {
        IGESControl_Controller::Init();
        IGESControl_Writer ICW;
		ICW.AddShape(static_cast<const OCCBRepData*>(brep)->getShape());
        ICW.ComputeModel();
        if (!ICW.Write(GetOutputLocation().c_str())) {
            mitkThrow() << "Failed to write " << GetOutputLocation();
        }
    }
    else if (boost::algorithm::iends_with(GetOutputLocation(), ".stp") || boost::algorithm::iends_with(GetOutputLocation(), ".step")) {
        STEPControl_Controller::Init();
        STEPControl_Writer SCW;
		SCW.Transfer(static_cast<const OCCBRepData*>(brep)->getShape(), STEPControl_AsIs);
        if (!SCW.Write(GetOutputLocation().c_str())) {
            mitkThrow() << "Failed to write " << GetOutputLocation();
        }
    }

    // Save face information
    std::ofstream faceInfoFile(GetOutputLocation() + ".faceinfo");
    boost::archive::xml_oarchive outArchive(faceInfoFile);

    auto& dataRef = *brep;
    outArchive << BOOST_SERIALIZATION_NVP(dataRef);

    // Save polygonal representation
    vtkSmartPointer<vtkXMLPolyDataWriter> pdWriter = vtkSmartPointer<vtkXMLPolyDataWriter>::New();
    pdWriter->SetDataModeToBinary();
    pdWriter->SetInputData(brep->getSurfaceRepresentation()->GetVtkPolyData());
    pdWriter->SetFileName((GetOutputLocation() + ".vtp").c_str());
    pdWriter->Update();
}

}