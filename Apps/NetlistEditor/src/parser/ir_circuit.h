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


#ifndef IR_CIRCUIT_H
#define IR_CIRCUIT_H


#include "parser/parser.h"
#include "exception/sapecngexception.h"

#include <stack>
#include <iostream>
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>


namespace sapecng
{


class ir_parser: public abstract_parser
{

public:
  ir_parser() { }

protected:
  void parse_internal(abstract_builder& builder);

private:
  void parse_rec(
      abstract_builder& builder,
      boost::property_tree::ptree* head
    );

protected:
  typedef
  boost::error_info<struct tag_what, std::string>
  what;

  boost::property_tree::ptree ptree_;

};


class info_parser: public ir_parser
{

public:
  info_parser(
      std::basic_istream<
          boost::property_tree::ptree::key_type::value_type
        >& stream
    ): stream_(stream) { }

  void parse(abstract_builder& builder)
  {
    try {
      boost::property_tree::info_parser::read_info(stream_, ptree_);
      parse_internal(builder);
    } catch(const boost::property_tree::info_parser::info_parser_error& ipe) {
      throw stream_read_error() << what(std::string(ipe.what()));
    } catch(const sapecng_exception& se) {
      se << what("info parser unknow error");
      throw;
    }
  }

private:
  std::basic_istream<
      boost::property_tree::ptree::key_type::value_type
    >& stream_;

};


class xml_parser: public ir_parser
{

public:
  xml_parser(
      std::basic_istream<
          boost::property_tree::ptree::key_type::value_type
        >& stream
    ): stream_(stream) { }

  void parse(abstract_builder& builder)
  {
    try {
      boost::property_tree::xml_parser::read_xml(stream_, ptree_);
      parse_internal(builder);
    } catch(const boost::property_tree::xml_parser::xml_parser_error& xpe) {
      throw stream_read_error() << what(std::string(xpe.what()));
    } catch(const sapecng_exception& se) {
      se << what("xml parser unknown error");
      throw;
    }
  }

private:
  std::basic_istream<
      boost::property_tree::ptree::key_type::value_type
    >& stream_;

};



class ir_builder: public abstract_builder
{

public:
  ir_builder();

  void add_circuit_properties(std::map<std::string,std::string> map);
  void add_circuit_property(std::string name, std::string value);

  void add_wire_component(
      std::map<std::string,std::string> props =
        std::map<std::string,std::string>()
    );

  // void add_out_component(
  //     unsigned int v,
  //     std::map<std::string,std::string> props =
  //       std::map<std::string,std::string>()
  //   );

  void add_threeD_interface_node(
      unsigned int v,
        std::map<std::string,std::string> props =
          std::map<std::string,std::string>()
    );

  void add_prescribed_pressure_node(
	  std::string name,
    double value,
    unsigned int v,
	  std::map<std::string, std::string> props =
	  std::map<std::string, std::string>()
	  );

  void add_dual_component(
      abstract_builder::dual_component_type c_type,
      std::string name,
      std::vector<double> value,
      unsigned int va,
      unsigned int vb,
      std::map<std::string,std::string> props =
        std::map<std::string,std::string>()
    );

  void add_quad_component(
      abstract_builder::quad_component_type c_type,
      std::string name,
      std::vector<double> parameterValues,
      bool hasControl,
      unsigned int va,
      unsigned int vb,
      unsigned int vac,
      unsigned int vbc,
      std::map<std::string,std::string> props =
        std::map<std::string,std::string>()
    );

  void add_unknow_component(
      std::map<std::string,std::string> props =
        std::map<std::string,std::string>()
    );

  // void begin_userdef_component(
  //     std::string name,
  //     std::map<std::string,std::string> props =
  //       std::map<std::string,std::string>()
  //   );

  // void end_userdef_component(
  //     std::string name,
  //     std::map<std::string,std::string> props =
  //       std::map<std::string,std::string>()
  //   );

protected:
  typedef
  boost::error_info<struct tag_what, std::string>
  what;

  boost::property_tree::ptree ptree_;
  boost::property_tree::ptree* head_;
  std::stack<boost::property_tree::ptree*> stack_;

private:
  void add_item(
      char id,
      std::string name,
      std::vector<double> parameters,
      std::vector<unsigned int> nodes,
      std::map<std::string,std::string> props =
        std::map<std::string,std::string>()
    );

};


class info_builder: public ir_builder
{

public:
  info_builder(
      std::basic_ostream<
          boost::property_tree::ptree::key_type::value_type
        >& stream
    ): stream_(stream) { }

  void flush()
  {
    try {
      boost::property_tree::info_parser::write_info(stream_, ptree_);
    } catch(const boost::property_tree::info_parser::info_parser_error& ipe) {
      throw stream_write_error() << what(std::string(ipe.what()));
    }
  }

private:
  std::basic_ostream<
      boost::property_tree::ptree::key_type::value_type
    >& stream_;

};


class xml_builder: public ir_builder
{

public:
  xml_builder(
      std::basic_ostream<
          boost::property_tree::ptree::key_type::value_type
        >& stream
    ): stream_(stream) { }

  void flush()
  {
    try {
      boost::property_tree::xml_parser::write_xml(stream_, ptree_);
    } catch(const boost::property_tree::xml_parser::xml_parser_error& xpe) {
      throw stream_write_error() << what(std::string(xpe.what()));
    }
  }

private:
  std::basic_ostream<
      boost::property_tree::ptree::key_type::value_type
    >& stream_;

};



}


#endif // IR_CIRCUIT_H
