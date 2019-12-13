#pragma once

#include <gsl.h>

#include <boost/graph/graph_traits.hpp>
#include <boost/range/adaptor/filtered.hpp>

#include "VesselForestData.h"

namespace crimson
{
struct ActiveVesselsFilter {
    ActiveVesselsFilter() = default;
    ActiveVesselsFilter(const VesselForestData* owner)
        : _owner(owner)
    {
    }

    bool operator()(const VesselForestData::VesselPathUIDType& uid) const { return _owner->getVesselUsedInBlending(uid); }

private:
    const VesselForestData* _owner = nullptr;
};

struct ActiveBooleanOperationsFilter {
    ActiveBooleanOperationsFilter() = default;
    ActiveBooleanOperationsFilter(const VesselForestData* owner)
        : _owner(owner)
    {
    }

    bool operator()(const VesselForestData::BooleanOperationInfo& bop) const
    {
        return _owner->getVesselUsedInBlending(bop.vessels.first) && _owner->getVesselUsedInBlending(bop.vessels.second);
    }

private:
    const VesselForestData* _owner;
};

struct OutEdgeIteratorFilter {
    OutEdgeIteratorFilter() = default;
    OutEdgeIteratorFilter(const VesselForestData::VesselPathUIDType* uid)
        : _uid(uid)
    {
    }

    bool operator()(const VesselForestData::BooleanOperationInfo& bop) const
    {
        return bop.vessels.first == *_uid || bop.vessels.second == *_uid;
    }

private:
    const VesselForestData::VesselPathUIDType* _uid = nullptr;
};

struct ExtractOutEdgeFromBooleanOperationInfo {
    ExtractOutEdgeFromBooleanOperationInfo() = default;
    ExtractOutEdgeFromBooleanOperationInfo(const VesselForestData::VesselPathUIDType* sourceVertexUID)
        : _sourceVertexUID(sourceVertexUID)
    {
    }

    VesselForestData::VesselPathUIDPair operator()(const VesselForestData::BooleanOperationInfo& bop) const
    {
        if (*_sourceVertexUID == bop.vessels.first) {
            return bop.vessels;
        }
        Expects(*_sourceVertexUID == bop.vessels.second);
        return std::make_pair(bop.vessels.second, bop.vessels.first);
    }

private:
    const VesselForestData::VesselPathUIDType* _sourceVertexUID;
};

} // namespace crimson

namespace boost
{
template <>
struct graph_traits<crimson::VesselForestData> {
    /////////////////////////////////
    // Graph concept
    /////////////////////////////////
    using vertex_descriptor = crimson::VesselForestData::VesselPathUIDType;
    using edge_descriptor = crimson::VesselForestData::VesselPathUIDPair;

    using directed_category = boost::undirected_tag;
    using edge_parallel_category = boost::disallow_parallel_edge_tag;

    struct traversal_category : boost::vertex_list_graph_tag, boost::incidence_graph_tag {
    };

    /////////////////////////////////
    // Incidence graph concept
    /////////////////////////////////
    using out_edge_iterator = boost::transform_iterator<
        crimson::ExtractOutEdgeFromBooleanOperationInfo,
        boost::filter_iterator<
            crimson::OutEdgeIteratorFilter,
            boost::filter_iterator<crimson::ActiveBooleanOperationsFilter,
                                   crimson::VesselForestData::BooleanOperationContainerType::const_iterator>>>;
    using degree_size_type = std::size_t;

    // out_edges(v, g)
    // source(e, g)
    // target(e, g)
    // out_degree(v, g)

    /////////////////////////////////
    // Vertex list graph concept
    /////////////////////////////////
    using vertex_iterator = boost::filter_iterator<crimson::ActiveVesselsFilter,
                                                   crimson::VesselForestData::VesselPathUIDContainerType::const_iterator>;
    using vertices_size_type = crimson::VesselForestData::VesselPathUIDContainerType::size_type;

    // vertices(g)
    // num_vertices(g)

    static vertex_descriptor null_vertex() { return {}; }
};
} // namespace boost

namespace crimson
{

inline boost::graph_traits<crimson::VesselForestData>::vertex_descriptor
source(const boost::graph_traits<crimson::VesselForestData>::edge_descriptor& e, const crimson::VesselForestData& g)
{
    return e.first;
}

inline boost::graph_traits<crimson::VesselForestData>::vertex_descriptor
target(const boost::graph_traits<crimson::VesselForestData>::edge_descriptor& e, const crimson::VesselForestData& g)
{
    return e.second;
}

inline std::pair<boost::graph_traits<crimson::VesselForestData>::out_edge_iterator,
                 boost::graph_traits<crimson::VesselForestData>::out_edge_iterator>
out_edges(const boost::graph_traits<crimson::VesselForestData>::vertex_descriptor& v, const crimson::VesselForestData& g)
{
    auto filtIter1 = boost::make_filter_iterator(crimson::ActiveBooleanOperationsFilter{&g}, g.getBooleanOperations().begin(),
                                                 g.getBooleanOperations().end());
    auto endFiltIter1 = boost::make_filter_iterator(crimson::ActiveBooleanOperationsFilter{&g}, g.getBooleanOperations().end(),
                                                    g.getBooleanOperations().end());

    auto filtIter2 = boost::make_filter_iterator(crimson::OutEdgeIteratorFilter{&v}, filtIter1, endFiltIter1);
    auto endFiltIter2 = boost::make_filter_iterator(crimson::OutEdgeIteratorFilter{&v}, endFiltIter1, endFiltIter1);

    return {boost::make_transform_iterator(filtIter2, crimson::ExtractOutEdgeFromBooleanOperationInfo{&v}),
            boost::make_transform_iterator(endFiltIter2, crimson::ExtractOutEdgeFromBooleanOperationInfo{&v})};
}

inline boost::graph_traits<crimson::VesselForestData>::degree_size_type
out_degree(const boost::graph_traits<crimson::VesselForestData>::vertex_descriptor& v, const crimson::VesselForestData& g)
{
    auto iterPair = out_edges(v, g);
    return static_cast<boost::graph_traits<crimson::VesselForestData>::degree_size_type>(
        std::distance(iterPair.first, iterPair.second));
}

inline std::pair<boost::graph_traits<crimson::VesselForestData>::vertex_iterator,
                 boost::graph_traits<crimson::VesselForestData>::vertex_iterator>
vertices(const crimson::VesselForestData& g)
{
    auto iter = boost::make_filter_iterator(crimson::ActiveVesselsFilter{&g}, g.getVessels().begin(), g.getVessels().end());
    auto endIter = boost::make_filter_iterator(crimson::ActiveVesselsFilter{&g}, g.getVessels().end(), g.getVessels().end());

    return {iter, endIter};
}

inline boost::graph_traits<crimson::VesselForestData>::vertices_size_type num_vertices(const crimson::VesselForestData& g)
{
    auto iterPair = vertices(g);
    return static_cast<boost::graph_traits<crimson::VesselForestData>::vertices_size_type>(
        std::distance(iterPair.first, iterPair.second));
}

// static void _test_vertex_list_graph_concept()
//{
//    BOOST_CONCEPT_ASSERT((boost::IncidenceGraphConcept<crimson::VesselForestData>));
//    BOOST_CONCEPT_ASSERT((boost::VertexListGraphConcept<crimson::VesselForestData>));
//}

} // namespace crimson