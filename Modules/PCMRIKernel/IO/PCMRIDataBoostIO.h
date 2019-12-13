#pragma once 

#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/complex.hpp>

#include <PCMRIData.h>
#include <Multi_ArraySerialize.h>

namespace crimson {

    template<class Archive>
    void serialize(Archive & ar, crimson::TimeInterpolationParameters& parameters, const unsigned int /*version*/)
    {
        ar & BOOST_SERIALIZATION_NVP(parameters._cardiacFrequency);
        ar & BOOST_SERIALIZATION_NVP(parameters._normal);
        ar & BOOST_SERIALIZATION_NVP(parameters._nOriginal);
        ar & BOOST_SERIALIZATION_NVP(parameters._nControlPoints);
    }

    template<class Archive>
    void serialize(Archive & ar, crimson::VisualizationParameters& parameters, const unsigned int /*version*/)
    {
        ar & BOOST_SERIALIZATION_NVP(parameters._maxIndex);
        ar & BOOST_SERIALIZATION_NVP(parameters._scaleFactor);
    }

    template<class Archive>
    void serialize(Archive & ar, crimson::PCMRIData& pcmriData, const unsigned int /*version*/)
    {
        ar & BOOST_SERIALIZATION_NVP(pcmriData._mappedPCMRIvalues);
        ar & BOOST_SERIALIZATION_NVP(pcmriData._meshNodeUID);
        ar & BOOST_SERIALIZATION_NVP(pcmriData._faceIdentifierIndex);
        ar & BOOST_SERIALIZATION_NVP(pcmriData._parameters);
        ar & BOOST_SERIALIZATION_NVP(pcmriData._parametersVis);
    }

} // namespace crimson
