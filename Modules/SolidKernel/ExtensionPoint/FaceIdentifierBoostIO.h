#pragma once 

#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/optional.hpp>

#include <FaceIdentifier.h>

namespace crimson {

    template<class Archive>
    void serialize(Archive & ar, crimson::FaceIdentifier& faceId, const unsigned int /*version*/)
    {
        ar & BOOST_SERIALIZATION_NVP(faceId.faceType);
        ar & BOOST_SERIALIZATION_NVP(faceId.parentSolidIndices);
    }

    template<class Archive>
    void serialize(Archive & ar, crimson::FaceIdentifierMap& faceIdMap, const unsigned int /*version*/)
    {
        ar & BOOST_SERIALIZATION_NVP(faceIdMap._faceIdentifiers);
        ar & BOOST_SERIALIZATION_NVP(faceIdMap._uniqueIdentifiers);
    }

} // namespace crimson
