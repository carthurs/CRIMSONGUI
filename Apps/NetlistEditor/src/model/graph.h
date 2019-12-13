/*
    SapecNG - Next Generation Symbolic Analysis Program for Electric Circuit
    Copyright (C) 2009, Michele Caini

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef GRAPH_H
#define GRAPH_H


#include "boost-sapecng/edge_properties.hpp"
#include <boost/graph/adjacency_list.hpp>


namespace sapecng
{


enum EdgeType
{
  YREF, GREF,
  Z, Y, F,
};


typedef
boost::adjacency_list
  <
    boost::vecS,        // OutEdgeList
    boost::vecS,        // VertexList
    boost::undirectedS, // Directed
    boost::no_property, // VertexProperties
    boost::property< boost::edge_type_t, EdgeType,
      boost::property< boost::edge_name_t, std::string,
      boost::property< boost::edge_weight_t, double,
      boost::property< boost::edge_degree_t, int,
      boost::property< boost::edge_any_t, std::map< std::string, std::string >
        > > > > >,    // EdgeProperties
    boost::no_property, // GraphProperties
    boost::listS        // EdgeList
  >
Graph
;


}


#endif // GRAPH_H
