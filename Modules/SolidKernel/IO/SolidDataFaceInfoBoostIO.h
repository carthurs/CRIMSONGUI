#pragma once 

#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>

#include <FaceIdentifierBoostIO.h>
#include <SolidData.h>

namespace crimson {

    template<class Archive>
    void serialize(Archive & ar, crimson::SolidData& data, const unsigned int version)
    {
        if (version == 0) {
            ar & boost::serialization::make_nvp("data._faceIdentifiers", data._faceIdentifierMap._faceIdentifiers);
            ar & boost::serialization::make_nvp("data._uniqueIdentifiers", data._faceIdentifierMap._uniqueIdentifiers);
        } else {
            ar & BOOST_SERIALIZATION_NVP(data._faceIdentifierMap);
        }
        ar & BOOST_SERIALIZATION_NVP(data._inflowFaceId);
        ar & BOOST_SERIALIZATION_NVP(data._outflowFaceId);
    }

} // namespace crimson
