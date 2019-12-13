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


#ifndef EDGE_PROPERTIES_H
#define EDGE_PROPERTIES_H


#include <boost/graph/properties.hpp>


namespace boost
{


struct any_t
{
  typedef edge_property_tag kind;
};


struct type_t
{
  typedef edge_property_tag kind;
};


struct degree_t
{
  typedef edge_property_tag kind;
};


struct symbolic_t
{
  typedef edge_property_tag kind;
};


enum edge_any_t { edge_any };
enum edge_type_t { edge_type };
enum edge_degree_t { edge_degree };


BOOST_INSTALL_PROPERTY(edge, any);
BOOST_INSTALL_PROPERTY(edge, type);
BOOST_INSTALL_PROPERTY(edge, degree);


}


#endif // EDGE_PROPERTIES_H
