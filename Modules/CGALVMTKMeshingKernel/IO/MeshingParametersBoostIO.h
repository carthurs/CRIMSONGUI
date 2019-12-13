#pragma once 

#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/optional.hpp>

#include <FaceIdentifierBoostIO.h>
#include <IMeshingKernel.h>
#include <MeshingParametersData.h>

namespace crimson {

    template<class Archive>
    void serialize(Archive & ar, crimson::IMeshingKernel::LocalMeshingParameters& localData, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_NVP(localData.size);
        ar & BOOST_SERIALIZATION_NVP(localData.sizeRelative);

        ar & BOOST_SERIALIZATION_NVP(localData.useBoundaryLayers);
        ar & BOOST_SERIALIZATION_NVP(localData.thickness);
        ar & BOOST_SERIALIZATION_NVP(localData.numSubLayers);
        ar & BOOST_SERIALIZATION_NVP(localData.subLayerRatio);
    }

    template<class Archive>
    void serialize(Archive & ar, crimson::IMeshingKernel::GlobalMeshingParameters& globalData, const unsigned int /*version*/)
    {
        ar & BOOST_SERIALIZATION_NVP(globalData.meshSurfaceOnly);
        ar & BOOST_SERIALIZATION_NVP(globalData.surfaceOptimizationLevel);
        ar & BOOST_SERIALIZATION_NVP(globalData.volumeOptimizationLevel);
        ar & BOOST_SERIALIZATION_NVP(globalData.maxRadiusEdgeRatio);
        ar & BOOST_SERIALIZATION_NVP(globalData.minDihedralAngle);
        ar & BOOST_SERIALIZATION_NVP(globalData.maxDihedralAngle);
        ar & BOOST_SERIALIZATION_NVP(globalData.defaultLocalParameters);
    }

    template<class Archive>
    void serialize(Archive & ar, crimson::MeshingParametersData& data, const unsigned int /*version*/)
    {
        ar & BOOST_SERIALIZATION_NVP(data.m_globalParams);
        ar & BOOST_SERIALIZATION_NVP(data.m_localParams);
    }

} // namespace crimson

BOOST_CLASS_VERSION(crimson::IMeshingKernel::LocalMeshingParameters, 1)


