#include <boost/algorithm/string.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>

#include "MeshingParametersDataIO.h"
#include "MeshingParametersBoostIO.h"

#include <MeshingParametersData.h>
#include <IO/MeshingKernelIOMimeTypes.h>

#include <IO/IOUtilDataSerializer.h>

REGISTER_IOUTILDATA_SERIALIZER(MeshingParametersData, crimson::MeshingKernelIOMimeTypes::MESHINGPARAMETERSDATA_DEFAULT_EXTENSION())

namespace crimson {

MeshingParametersDataIO::MeshingParametersDataIO()
    : AbstractFileIO(MeshingParametersData::GetStaticNameOfClass(),
    MeshingKernelIOMimeTypes::MESHINGPARAMETERSDATA_MIMETYPE(),
    "Meshing parameters data")
{
    RegisterService();
}

MeshingParametersDataIO::MeshingParametersDataIO(const MeshingParametersDataIO& other)
    : AbstractFileIO(other)
{
}

MeshingParametersDataIO::~MeshingParametersDataIO()
{
}

std::vector< itk::SmartPointer<mitk::BaseData> > MeshingParametersDataIO::Read()
{
    std::vector< itk::SmartPointer<mitk::BaseData> > result;

    std::istream* inStream = GetInputStream();
    std::shared_ptr<std::istream> fileInStream;
    if (!inStream) {
        fileInStream.reset(new std::ifstream(GetInputLocation()));
        inStream = fileInStream.get();
    }
    boost::archive::xml_iarchive inArchive(*inStream);

    auto data = MeshingParametersData::New();
    auto& dataRef = *data;

    inArchive >> BOOST_SERIALIZATION_NVP(dataRef);

    result.push_back(data.GetPointer());
    return result;
}

void MeshingParametersDataIO::Write()
{
    auto data = static_cast<const MeshingParametersData*>(this->GetInput());

    if (!data) {
        MITK_ERROR << "Input MeshingParameters data has not been set!";
        return;
    }

    std::ostream* outStream = GetOutputStream();
    std::shared_ptr<std::ostream> fileOutStream;
    if (!outStream) {
        fileOutStream.reset(new std::ofstream(GetOutputLocation()));
        outStream = fileOutStream.get();
    }


    auto& dataRef = *data;
    boost::archive::xml_oarchive out(*outStream);
    out << BOOST_SERIALIZATION_NVP(dataRef);
}

}