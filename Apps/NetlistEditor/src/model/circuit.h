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


#ifndef CIRCUIT_H
#define CIRCUIT_H


#include "parser/parser.h"
#include "model/graph.h"


namespace sapecng
{


class circuit
{

friend class circuit_builder;

public:
  typedef boost::graph_traits<Graph>::vertex_descriptor vertex_descriptor;
  typedef boost::graph_traits<Graph>::edge_descriptor edge_descriptor;
  typedef boost::graph_traits<Graph>::vertex_iterator vertex_iterator;
  typedef boost::graph_traits<Graph>::edge_iterator edge_iterator;

  circuit():
    vG_(0),
    iG_(0),
    reference_(0),
    out_(0)
  { vG_ = new Graph; iG_ = new Graph; }

  ~circuit()
    { delete vG_; delete iG_; }

  inline const
  std::map<std::string,std::string>&
  circuitProperties() const
    { return props_; }

  inline const Graph& vGraph() const
    { return *vG_; }

  inline const
  std::vector< edge_descriptor >&
  vGraph_order() const
    { return vG_o_; }

  inline const Graph& iGraph() const
    { return *iG_; }

  inline const
  std::vector< edge_descriptor >&
  iGraph_order() const
    { return iG_o_; }

  inline vertex_descriptor referenceNode() const
    { return reference_; }

  inline vertex_descriptor outNode() const
    { return out_; }

private:
  std::map<std::string,std::string> props_;

  Graph* vG_;
  Graph* iG_;
  std::vector< edge_descriptor > vG_o_;
  std::vector< edge_descriptor > iG_o_;

  vertex_descriptor reference_;
  vertex_descriptor out_;

};



class circuit_builder: public abstract_builder
{

public:
  circuit_builder(circuit& circuit);

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

  void add_prescribed_pressure_node(
    std::string name,
    double value,
    unsigned int v,
    std::map<std::string,std::string> props =
      std::map<std::string,std::string>()
    )
  {
    assert(false);
  }

  void add_threeD_interface_node(
      unsigned int v,
      std::map<std::string,std::string> props =
        std::map<std::string,std::string>()
    );

  void add_dual_component(
      abstract_builder::dual_component_type c_type,
      std::string name,
	  std::vector<double> parameterValues,
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
    ) { }

  // void begin_userdef_component(
  //     std::string name,
  //     std::map<std::string,std::string> props =
  //       std::map<std::string,std::string>()
  //   ) { }

  // void end_userdef_component(
  //     std::string name,
  //     std::map<std::string,std::string> props =
  //       std::map<std::string,std::string>()
  //   ) { }

  void flush();

private:
  void try_add_block();
  void replace_requested(circuit::vertex_descriptor vertex);

  void add_edge(
      circuit::vertex_descriptor iva,
      circuit::vertex_descriptor ivb,
      circuit::vertex_descriptor vva,
      circuit::vertex_descriptor vvb,
      EdgeType type,
      std::string name,
      double value,
      int degree,
      std::map<std::string,std::string> props
    );

private:
  circuit& circuit_;
  std::list<circuit::vertex_descriptor> requested_;
  bool has_block_;

  std::string prefix_;
  std::vector<std::string> pstack_;

};


}


#endif // CIRCUIT_H
