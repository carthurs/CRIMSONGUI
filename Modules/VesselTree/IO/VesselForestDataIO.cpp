#include <tinyxml.h>

#include "VesselForestDataIO.h"

#include <VesselForestData.h>
#include <IO/VesselTreeIOMimeTypes.h>

#include <IO/IOUtilDataSerializer.h>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>

REGISTER_IOUTILDATA_SERIALIZER(VesselForestData, crimson::VesselTreeIOMimeTypes::VESSELTREE_DEFAULT_EXTENSION())

namespace crimson {

VesselForestDataIO::VesselForestDataIO()
    : AbstractFileIO(VesselForestData::GetStaticNameOfClass(),
    VesselTreeIOMimeTypes::VESSELTREE_MIMETYPE(),
    "Vessel tree data")
{
    RegisterService();
}

VesselForestDataIO::VesselForestDataIO(const VesselForestDataIO& other)
    : AbstractFileIO(other)
{
}

VesselForestDataIO::~VesselForestDataIO()
{
}

std::vector< itk::SmartPointer<mitk::BaseData> > VesselForestDataIO::Read()
{
    std::vector< itk::SmartPointer<mitk::BaseData> > result;

    try {
        std::istream* inStream = GetInputStream();
        std::shared_ptr<std::istream> fileInStream;
        if (!inStream) {
            fileInStream.reset(new std::ifstream(GetInputLocation()));
            inStream = fileInStream.get();
        }
        boost::archive::xml_iarchive inArchive(*inStream);

        auto vesselForest = VesselForestData::New();
        auto& vesselForestRef = *vesselForest;

        inArchive >> BOOST_SERIALIZATION_NVP(vesselForestRef);

        result.emplace_back(vesselForest.GetPointer());
        return result;
    }
    catch (...) {}

    // Try legacy reader if the boost reader failed
    try {
        result.emplace_back(Read_v0().GetPointer());
        return result;
    } catch (...) {}

    mitkThrow() << "Failed to read vessel tree";
}

void VesselForestDataIO::Write()
{
    auto vesselForest = static_cast<const VesselForestData*>(this->GetInput());

    if (!vesselForest) {
        MITK_ERROR << "Input " << VesselForestData::GetStaticNameOfClass() << " data has not been set!";
        return;
    }

    std::ostream* outStream = GetOutputStream();
    std::shared_ptr<std::ostream> fileOutStream;
    if (!outStream) {
        fileOutStream.reset(new std::ofstream(GetOutputLocation()));
        outStream = fileOutStream.get();
    }

    auto& vesselForestRef = *vesselForest;
    boost::archive::xml_oarchive out(*outStream);
    out << BOOST_SERIALIZATION_NVP(vesselForestRef);
}




mitk::BaseData::Pointer VesselForestDataIO::Read_v0()
{
    // LEGACY READER

    auto vesselForest = VesselForestData::New();

    TiXmlDocument doc(GetLocalFileName());
    if (!doc.LoadFile()) {
        mitkThrow() << "Failed to load the vessel forest data from file '" << GetLocalFileName() << "'.";
    }

    TiXmlHandle hDoc(&doc);

    auto rootElement = hDoc.FirstChildElement().Element();

    if (!rootElement) {
        mitkThrow() << "Failed to parse vessel forest data in file '" << GetLocalFileName() << "'.";
    }

    TiXmlHandle hRoot(rootElement);


    // Read the vessel path UID's
    for (auto vesselPathElement = hRoot.FirstChild("VesselPaths").FirstChild().Element(); vesselPathElement; vesselPathElement = vesselPathElement->NextSiblingElement()) {
        std::string uid;
        if (vesselPathElement->QueryStringAttribute("UID", &uid) != TIXML_SUCCESS) {
            MITK_WARN << "Failed to read vessel path UID in file '" << GetLocalFileName() << "'. Ignoring.";
            continue;
        }

        vesselForest->_vesselUIDs.insert(uid);
        vesselForest->_vesselUsedInBlending[uid] = true;
    }

    for (auto filletSizeElement = hRoot.FirstChild("FilletSizes").FirstChild().Element(); filletSizeElement; filletSizeElement = filletSizeElement->NextSiblingElement()) {
        bool success = true;
        std::string vessel1uid;
        success = success && (filletSizeElement->QueryStringAttribute("Vessel1UID", &vessel1uid) == TIXML_SUCCESS);
        std::string vessel2uid;
        success = success && (filletSizeElement->QueryStringAttribute("Vessel2UID", &vessel2uid) == TIXML_SUCCESS);
        double filletSize;
        success = success && (filletSizeElement->QueryDoubleAttribute("Fillet", &filletSize) == TIXML_SUCCESS);
        int removesFace = 0;
        // Support for older version
        // success = success && (filletSizeElement->QueryDoubleAttribute("Fillet", &filletSize) == TIXML_SUCCESS);
        filletSizeElement->QueryIntAttribute("RemovesFace", &removesFace);

        if (!success) {
            MITK_WARN << "Failed to read fillet size in file '" << GetLocalFileName() << "'. Ignoring.";
            continue;
        }

        if (filletSize >= 0) {
            VesselForestData::BooleanOperationInfo info;
            info.vessels = std::make_pair(vessel1uid, vessel2uid);
            info.bop = VesselForestData::bopFuse;
            info.removesFace = removesFace;
            if (vessel2uid != VesselForestData::InflowUID && vessel2uid != VesselForestData::OutflowUID) {
                vesselForest->_booleanOperations.emplace_back(info);
            }
            vesselForest->_filletSizes[info.vessels] = filletSize;
        }
    }

    for (auto useInBlendingElement = hRoot.FirstChild("UseInBlendingRoot").FirstChild().Element(); useInBlendingElement; useInBlendingElement = useInBlendingElement->NextSiblingElement()) {
        bool success = true;
        std::string vesseluid;
        success = success && (useInBlendingElement->QueryStringAttribute("VesselUID", &vesseluid) == TIXML_SUCCESS);
        int use = 1;
        success = success && (useInBlendingElement->QueryIntAttribute("Use", &use) == TIXML_SUCCESS);

        if (!success) {
            MITK_WARN << "Failed to read fillet size in file '" << GetLocalFileName() << "'. Ignoring.";
            continue;
        }

        vesselForest->_vesselUsedInBlending[vesseluid] = (bool)use;
    }

    return vesselForest.GetPointer();
}


}