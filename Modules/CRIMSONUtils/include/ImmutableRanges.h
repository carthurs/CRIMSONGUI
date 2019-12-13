#pragma once

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/any_range.hpp>

namespace crimson
{

template <typename T>
using ImmutableRefRange = boost::any_range<T, boost::forward_traversal_tag, const T&>;

template <typename T>
using ImmutableValueRange = boost::any_range<T, boost::forward_traversal_tag, const T>;

template <typename T, typename C>
ImmutableValueRange<T> make_transforming_immutable_range(const C& container)
{
    return container | boost::adaptors::transformed([](const typename C::value_type& v) { return T{v}; });
}

}