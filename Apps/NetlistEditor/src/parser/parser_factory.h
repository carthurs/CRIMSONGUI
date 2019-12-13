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


#ifndef PARSER_FACTORY_H
#define PARSER_FaCTORY_H


#include <parser/parser.h>
#include <parser/ir_circuit.h>
#include <parser/crc_circuit.h>


namespace sapecng
{



class builder_factory
{

public:
  enum b_type
    { INFO, XML, CRC };

  static abstract_builder* builder(
      builder_factory::b_type type,
      std::basic_ostream<
          boost::property_tree::ptree::key_type::value_type
        >& stream
    )
  {
    switch(type)
    {
    case INFO:
      return new info_builder(stream);
    case XML:
      return new xml_builder(stream);
    case CRC:
      return new crc_builder(stream);
    default:
      break;
    }

    return 0;
  }

  static abstract_builder* builder(
      std::string type,
      std::basic_ostream<
          boost::property_tree::ptree::key_type::value_type
        >& stream
    )
  {
    if(type == "info")
      return builder(INFO, stream);

    if(type == "xml")
      return builder(XML, stream);

    if(type == "dat")
      return builder(CRC, stream);

    return 0;
  }

};



class parser_factory
{

public:
  enum p_type
    { INFO, XML };

  static abstract_parser* parser(
      parser_factory::p_type type,
      std::basic_istream<
          boost::property_tree::ptree::key_type::value_type
        >& stream
    )
  {
    switch(type)
    {
    case INFO:
      return new info_parser(stream);
    case XML:
      return new xml_parser(stream);
    default:
      break;
    }

    return 0;
  }

  static abstract_parser* parser(
      std::string type,
      std::basic_istream<
          boost::property_tree::ptree::key_type::value_type
        >& stream
    )
  {
    if(type == "info")
      return parser(INFO, stream);

    if(type == "xml")
      return parser(XML, stream);

    return 0;
  }

};



}


#endif // PARSER_FACTORY_H
