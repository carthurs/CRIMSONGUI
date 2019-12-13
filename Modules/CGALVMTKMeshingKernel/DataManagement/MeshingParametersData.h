#pragma once

#include <mitkBaseData.h>

#include "IMeshingKernel.h"

#include "CGALVMTKMeshingKernelExports.h"
namespace boost {
    namespace serialization {
        class access;
    }
}

namespace crimson {

/*!
 * \brief   MeshingParametersData containes the local and global meshing parameters which can be
 *  associated with an mitk::DataNode This allows to preserve the user-assigned meshing
 *  parameters after re-lofting and re-blending the model.
 */
class CGALVMTKMeshingKernel_EXPORT MeshingParametersData : public mitk::BaseData
{
    friend class boost::serialization::access;
public:
    mitkClassMacro(MeshingParametersData, BaseData);
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);
    mitkCloneMacro(Self);

public:
    void UpdateOutputInformation() override;
    void SetRequestedRegionToLargestPossibleRegion() override {}
    bool RequestedRegionIsOutsideOfTheBufferedRegion() override { return false; }
    bool VerifyRequestedRegion() override { return true; }
    void SetRequestedRegion(const itk::DataObject *) override {}

    void PrintSelf(std::ostream& os, itk::Indent indent) const override;

    crimson::IMeshingKernel::GlobalMeshingParameters& globalParameters() { this->Modified(); return m_globalParams; }
    std::map<crimson::FaceIdentifier, crimson::IMeshingKernel::LocalMeshingParameters>& localParameters() { this->Modified(); return m_localParams; }

protected:
    MeshingParametersData();
    virtual ~MeshingParametersData();

    MeshingParametersData(const Self& other) = default;

    crimson::IMeshingKernel::GlobalMeshingParameters m_globalParams;
    std::map<crimson::FaceIdentifier, crimson::IMeshingKernel::LocalMeshingParameters> m_localParams;

     template<class Archive>
     friend void serialize(Archive & ar, crimson::MeshingParametersData& data, const unsigned int version);

};

} // namespace crimson
