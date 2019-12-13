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


#include "model/circuit.h"

#include <algorithm>


namespace sapecng
{


circuit_builder::circuit_builder(circuit& circuit)
  : circuit_(circuit), has_block_(false), prefix_("")
{
  circuit_.props_.clear();

  if(circuit_.iG_)
    (circuit_.iG_)->clear();
  else
    circuit_.iG_ = new Graph;

  if(circuit_.vG_)
    (circuit_.vG_)->clear();
  else
    circuit_.vG_ = new Graph;

  circuit_.vG_o_.clear();
  circuit_.iG_o_.clear();

  replace_requested(GROUND);
  circuit_.out_ = GROUND;

  circuit_.reference_ = boost::add_vertex(*(circuit_.iG_));
  requested_.push_front(boost::add_vertex(*(circuit_.vG_)));
}


void circuit_builder::add_circuit_properties(
  std::map<std::string, std::string> map)
{
  std::map<std::string, std::string>::const_iterator i;
  for(i = map.begin(); i != map.end(); ++i)
    circuit_.props_[i->first] = i->second;
}


void circuit_builder::add_circuit_property(
  std::string name, std::string value)
{
  circuit_.props_[name] = value;
}


void circuit_builder::add_wire_component(
    std::map<std::string, std::string> props
  )
{ }


// void circuit_builder::add_out_component(
//     unsigned int v,
//     std::map<std::string, std::string> props
//   )
// {
//   replace_requested(v);
//   if(!circuit_.out_)
//     circuit_.out_ = v;
// }

void circuit_builder::add_threeD_interface_node(
    unsigned int v,
    std::map<std::string, std::string> props
  )
{
  replace_requested(v);
  if(!circuit_.out_)
    circuit_.out_ = v;
}

void circuit_builder::add_dual_component(
    abstract_builder::dual_component_type c_type,
    std::string name,
	std::vector<double> parameterValues,
    unsigned int va,
    unsigned int vb,
    std::map<std::string, std::string> props
  )
{
  replace_requested(va <= vb ? va : vb);
  replace_requested(va > vb ? va : vb);

  switch(c_type)
  {
  case abstract_builder::Component_Resistor:
    {
      add_edge(
          va, vb, va, vb, Z,
		  prefix_ + name, parameterValues.at(0),
          0, props
        );
      break;
    }
  case abstract_builder::G:
    {
      add_edge(
          va, vb, va, vb, Y,
		  prefix_ + name, parameterValues.at(0),
		  0, props
        );
      break;
    }
  case abstract_builder::Component_Inductor:
    {
      add_edge(
          va, vb, va, vb, Z,
		  prefix_ + name, parameterValues.at(0),
		  1, props
        );
      break;
    }
  case abstract_builder::Component_Capacitor:
    {
      add_edge(
          va, vb, va, vb, Y,
		  prefix_ + name, parameterValues.at(0),
		  1, props
        );
      break;
    }
  case abstract_builder::Component_VolumeTrackingPressureChamber:
	{
		add_edge(
		va, vb, va, vb, Y,
		prefix_ + name, parameterValues.at(0),
		1, props
		);
		break;
	}
  case abstract_builder::Component_Diode:
    {
      add_edge(
          va, vb, va, vb, Y,
		  prefix_ + name, parameterValues.at(0),
		  1, props
        );
      break;
    }
  case abstract_builder::V:
    {
      circuit::vertex_descriptor vdesc = boost::add_vertex(*(circuit_.iG_));
      requested_.push_front(boost::add_vertex(*(circuit_.vG_)));

      add_edge(
          vdesc, GROUND, GROUND, circuit_.reference_,
		  Y, prefix_ + name, parameterValues.at(0), 0, props
        );
      add_edge(
          vdesc, GROUND, vb, va,
          Y, "", -1., 0, props
        );
      add_edge(
          vb, va, GROUND, vdesc,
          F, "", 1., 0, props
        );

      break;
    }
  case abstract_builder::I:
    {
      add_edge(
          vb, va, GROUND, circuit_.reference_,
		  Y, prefix_ + name, parameterValues.at(0), 0, props
        );

      break;
    }
  // case abstract_builder::VM:
  //   {
  //     circuit::vertex_descriptor vdesc = boost::add_vertex(*(circuit_.iG_));
  //     requested_.push_front(boost::add_vertex(*(circuit_.vG_)));
  //     circuit::vertex_descriptor v_ = boost::add_vertex(*(circuit_.iG_));
  //     requested_.push_front(boost::add_vertex(*(circuit_.vG_)));

  //     add_edge(
  //         vdesc, vb, vb, va, Y,
  //         "", -1., 0, props
  //       );
  //     add_edge(
  //         vdesc, vb, GROUND, v_, Y,
  //         "", -1., 0, props
  //       );
  //     add_edge(
  //         GROUND, v_, vb, vdesc, F,
  //         "", 1., 0, props
  //       );

  //     if(!circuit_.out_)
  //       circuit_.out_ = v_;

  //     break;
  //   }
  // case abstract_builder::AM:
  //   {
  //     circuit::vertex_descriptor v_ = boost::add_vertex(*(circuit_.iG_));
  //     requested_.push_front(boost::add_vertex(*(circuit_.vG_)));

  //     add_edge(
  //         va, vb, GROUND, v_, Z,
  //         "", -1., 0, props
  //       );
  //     add_edge(
  //         GROUND, v_, vb, va, F,
  //         "", 1., 0, props
  //       );

  //     if(!circuit_.out_)
  //       circuit_.out_ = v_;

  //     break;
  //   }
  default:
    break;
  }
}


void circuit_builder::add_quad_component(
    abstract_builder::quad_component_type c_type,
    std::string name,
	std::vector<double> parameterValues,
	bool hasControl,
    unsigned int va,
    unsigned int vb,
    unsigned int vac,
    unsigned int vbc,
    std::map<std::string, std::string> props
  )
{
  std::vector<circuit::vertex_descriptor> nv;
  nv.push_back(va);
  nv.push_back(vb);
  nv.push_back(vac);
  nv.push_back(vbc);
  std::sort(nv.begin(), nv.end());

  replace_requested(nv.at(0));
  replace_requested(nv.at(1));
  replace_requested(nv.at(2));
  replace_requested(nv.at(3));

  switch(c_type)
  {
  case abstract_builder::VCCS:
    {
      add_edge(
          vb, va, vbc, vac, Y,
		  prefix_ + name, parameterValues.at(0),
		  0, props
        );

      break;
    }
  case abstract_builder::VCVS:
    {
      circuit::vertex_descriptor vdesc = boost::add_vertex(*(circuit_.iG_));
      requested_.push_front(boost::add_vertex(*(circuit_.vG_)));

      add_edge(
          vdesc, vbc, vbc, vac, Y,
		  prefix_ + name, parameterValues.at(0),
		  0, props
        );
      add_edge(
          vdesc, vbc, vb, va, Y,
          "", -1., 0, props
        );
      add_edge(
          vb, va, vbc, vdesc, F,
          "", 1., 0, props
        );

      break;
    }
  case abstract_builder::CCCS:
    {
      circuit::vertex_descriptor vdesc = boost::add_vertex(*(circuit_.iG_));
      requested_.push_front(boost::add_vertex(*(circuit_.vG_)));

      add_edge(
          vac, vbc, GROUND, vdesc, Z,
		  prefix_ + name, parameterValues.at(0),
		  0, props
        );
      add_edge(
          GROUND, vdesc, vbc, vac, F,
          "", 1., 0, props
        );
      add_edge(
          va, vb, GROUND, vdesc, Y,
          "", -1., 0, props
        );

      break;
    }
  case abstract_builder::CCVS:
    {
      add_edge(
          vac, vbc, vb, va, Z,
		  prefix_ + name, parameterValues.at(0),
		  0, props
        );
      add_edge(
          vb, va, vbc, vac, F,
          "", 1., 0, props
        );

      break;
    }
  case abstract_builder::AO:
    {
      add_edge(
          vb, va, vbc, vac, F,
          "", 1., 0, props
        );

      break;
    }
  case abstract_builder::n:
    {
      circuit::vertex_descriptor vvdesc = boost::add_vertex(*(circuit_.iG_));
      requested_.push_front(boost::add_vertex(*(circuit_.vG_)));
      circuit::vertex_descriptor ivdesc = boost::add_vertex(*(circuit_.iG_));
      requested_.push_front(boost::add_vertex(*(circuit_.vG_)));
      circuit::vertex_descriptor fix = boost::add_vertex(*(circuit_.iG_));
      requested_.push_front(boost::add_vertex(*(circuit_.vG_)));

      add_edge(
          vvdesc, vbc, vbc, vac, Y,
		  prefix_ + name, parameterValues.at(0),
		  0, props
        );
      add_edge(
          vvdesc, vbc, vb, fix, Y,
          "", -1., 0, props
        );
      add_edge(
          vb, fix, vbc, vvdesc, F,
          "", 1., 0, props
        );

      add_edge(
          fix, va, GROUND, ivdesc, Z,
		  prefix_ + name, parameterValues.at(0),
		  0, props
        );
      add_edge(
          GROUND, ivdesc, va, fix, F,
          "", 1., 0, props
        );
      add_edge(
          vbc, vac, GROUND, ivdesc, Y,
          "", -1., 0, props
        );

      break;
    }
  case abstract_builder::K:
    {
      circuit::vertex_descriptor afix = boost::add_vertex(*(circuit_.iG_));
      requested_.push_front(boost::add_vertex(*(circuit_.vG_)));
      circuit::vertex_descriptor bfix = boost::add_vertex(*(circuit_.iG_));
      requested_.push_front(boost::add_vertex(*(circuit_.vG_)));
      circuit::vertex_descriptor cfix = boost::add_vertex(*(circuit_.iG_));
      requested_.push_front(boost::add_vertex(*(circuit_.vG_)));
      circuit::vertex_descriptor dfix = boost::add_vertex(*(circuit_.iG_));
      requested_.push_front(boost::add_vertex(*(circuit_.vG_)));

      add_edge(
          va, afix, va, afix, Z, prefix_ + props["lp:name"],
          strtol(props["lp:value"].c_str(), 0, 10),
		  1, props
        );
      add_edge(
          vac, bfix, vac, bfix, Z, prefix_ + props["ls:name"],
          strtol(props["ls:value"].c_str(), 0, 10),
		  1, props
        );

      add_edge(
          dfix, bfix, vb, cfix, Z,
		  prefix_ + name, parameterValues.at(0),
		  1, props
        );
      add_edge(
          vb, cfix, bfix, dfix, F,
          "", 1., 1, props
        );

      add_edge(
          cfix, afix, vbc, dfix, Z,
		  prefix_ + name, parameterValues.at(0),
		  1, props
        );
      add_edge(
          vbc, dfix, afix, cfix, F,
          "", 1., 1, props
        );

      break;
    }
  default:
    break;
  }
}


void circuit_builder::flush()
{
  try_add_block();
}


void circuit_builder::try_add_block()
{
  if(!has_block_ && circuit_.out_) {
    add_edge(
        GROUND, circuit_.reference_,
        GROUND, circuit_.reference_,
        YREF, "", 1., 0,
        std::map<std::string, std::string>()
      );
    add_edge(
        circuit_.reference_, GROUND,
        GROUND, circuit_.out_,
        GREF, "", 1., 0,
        std::map<std::string, std::string>()
      );

    has_block_ = true;
  }
}


void circuit_builder::replace_requested(circuit::vertex_descriptor vertex)
{
  boost::remove_edge(
      boost::add_edge(vertex, vertex, *(circuit_.iG_)).first,
      *(circuit_.iG_)
    );
  boost::remove_edge(
      boost::add_edge(vertex, vertex, *(circuit_.vG_)).first,
      *(circuit_.vG_)
    );

  if(std::find(requested_.begin(), requested_.end(), vertex) != requested_.end()) {

    circuit::vertex_descriptor ireq = boost::add_vertex(*(circuit_.iG_));
    circuit::vertex_descriptor vreq = boost::add_vertex(*(circuit_.vG_));

    if(ireq != vreq) {
      boost::remove_vertex(ireq, *(circuit_.iG_));
      boost::remove_vertex(vreq, *(circuit_.vG_));
      return;
    }

    circuit::edge_iterator icurr, iend, vcurr, vend;
    boost::tie(icurr, iend) = boost::edges(*(circuit_.iG_));
    boost::tie(vcurr, vend) = boost::edges(*(circuit_.vG_));

    while(icurr != iend && vcurr != vend) {
      circuit::edge_descriptor iedge = *icurr;
      circuit::edge_descriptor vedge = *vcurr;

      if(boost::source(iedge, *(circuit_.iG_)) == vertex
          || boost::target(iedge, *(circuit_.iG_)) == vertex
          || boost::source(vedge, *(circuit_.vG_)) == vertex
          || boost::target(vedge, *(circuit_.vG_)) == vertex
        )
      {
        add_edge(
          (iedge.m_source == vertex ? ireq : iedge.m_source),
          (iedge.m_target == vertex ? ireq : iedge.m_target),
          (vedge.m_source == vertex ? vreq : vedge.m_source),
          (vedge.m_target == vertex ? vreq : vedge.m_target),
          get(boost::edge_type, *(circuit_.iG_), iedge),
          get(boost::edge_name, *(circuit_.iG_), iedge),
          get(boost::edge_weight, *(circuit_.iG_), iedge),
          get(boost::edge_degree, *(circuit_.iG_), iedge),
          get(boost::edge_any, *(circuit_.iG_), iedge)
        );

        boost::remove_edge(iedge, *(circuit_.iG_));
        boost::remove_edge(vedge, *(circuit_.vG_));
        circuit_.iG_o_.erase(
            std::find(circuit_.iG_o_.begin(), circuit_.iG_o_.end(), iedge)
          );
        circuit_.vG_o_.erase(
            std::find(circuit_.vG_o_.begin(), circuit_.vG_o_.end(), vedge)
          );

        boost::tie(icurr, iend) = boost::edges(*(circuit_.iG_));
        boost::tie(vcurr, vend) = boost::edges(*(circuit_.vG_));
      } else {
        ++icurr;
        ++vcurr;
      }
    }

    if(circuit_.reference_ == vertex)
      circuit_.reference_ = ireq;

    requested_.remove(vertex);
    requested_.push_front(ireq);

  }
}


void circuit_builder::add_edge(
      circuit::vertex_descriptor ivb,
      circuit::vertex_descriptor iva,
      circuit::vertex_descriptor vvb,
      circuit::vertex_descriptor vva,
      EdgeType type,
      std::string name,
      double value,
      int degree,
      std::map<std::string, std::string> props
  )
{
  circuit::edge_descriptor iedesc =
    boost::add_edge(iva, ivb, *circuit_.iG_).first;
  circuit::edge_descriptor vedesc =
    boost::add_edge(vva, vvb, *circuit_.vG_).first;
  circuit_.iG_o_.push_back(iedesc);
  circuit_.vG_o_.push_back(vedesc);

  put(boost::edge_type, *circuit_.iG_, iedesc, type);
  put(boost::edge_name, *circuit_.iG_, iedesc, name);
  put(boost::edge_weight, *circuit_.iG_, iedesc, value);
  put(boost::edge_degree, *circuit_.iG_, iedesc, degree);
  put(boost::edge_any, *circuit_.iG_, iedesc, props);

  put(boost::edge_type, *circuit_.vG_, vedesc, type);
  put(boost::edge_name, *circuit_.iG_, vedesc, name);
  put(boost::edge_weight, *circuit_.iG_, vedesc, value);
  put(boost::edge_degree, *circuit_.iG_, vedesc, degree);
  put(boost::edge_any, *circuit_.iG_, vedesc, props);
}


}
