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


#include "parser/ir_circuit.h"

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>

#include "utility/strings.h"


namespace sapecng
{


	void ir_parser::parse_internal(abstract_builder& builder)
	{
		if (ptree_.get_child_optional("circuit")) {
			parse_rec(builder, &(ptree_.get_child("circuit")));
		}
	}


	void ir_parser::parse_rec(
		abstract_builder& builder,
		boost::property_tree::ptree* head
		)
	{
		try {
			std::map<std::string, std::string> cmap;
			if (head->get_child_optional("props")) {
				BOOST_FOREACH(
					boost::property_tree::ptree::value_type& cprop,
					head->get_child("props")
					)
					cmap.insert(std::pair<std::string, std::string>
					(
					cprop.second.get("name", "property"),
					cprop.second.get("value", "value")
					)
					);
				builder.add_circuit_properties(cmap);
			}

			// if (head->get_child_optional("out")) {
			// 	boost::property_tree::ptree& o =
			// 		head->get_child("out");

			// 	unsigned int node =
			// 		boost::lexical_cast<unsigned int>(o.get("node", "0"));

			// 	std::map<std::string, std::string> imap;
			// 	if (o.get_child_optional("props")) {
			// 		BOOST_FOREACH(
			// 			boost::property_tree::ptree::value_type& iprop,
			// 			o.get_child("props")
			// 			)
			// 			imap.insert(std::pair<std::string, std::string>
			// 			(
			// 			iprop.second.get("name", "property"),
			// 			iprop.second.get("value", "value")
			// 			)
			// 			);
			// 	}

			// 	builder.add_out_component(node, imap);
			// }

			if (head->get_child_optional("threeDInterface")) {
				boost::property_tree::ptree& o =
					head->get_child("threeDInterface");

				unsigned int node =
					boost::lexical_cast<unsigned int>(o.get("node", "0"));

				std::map<std::string, std::string> imap;
				if (o.get_child_optional("props")) {
					BOOST_FOREACH(boost::property_tree::ptree::value_type& iprop,	o.get_child("props"))
          {
						imap.insert(std::pair<std::string, std::string>	(iprop.second.get("name", "property"), iprop.second.get("value", "value")));
          }
				}

				builder.add_threeD_interface_node(node, imap);
			}

			if (head->get_child_optional(predefinedStrings::prescribedPressureNodesCollectiveName))
      {

		// iterate over all the prescribed pressure nodes
		BOOST_FOREACH(boost::property_tree::ptree::value_type& iterator, head->get_child(predefinedStrings::prescribedPressureNodesCollectiveName))
        {
			// Extract the tree for this prescribed pressure node from the iterator
			boost::property_tree::ptree prescribedPressureNodeInfo = iterator.second;

			unsigned int node = boost::lexical_cast<unsigned int>(prescribedPressureNodeInfo.get("node", "0"));
			double value = boost::lexical_cast<double>(prescribedPressureNodeInfo.get("value", "none"));
			std::string name = boost::lexical_cast<std::string>(prescribedPressureNodeInfo.get("name", "none"));

  				std::map<std::string, std::string> imap;
				if (prescribedPressureNodeInfo.get_child_optional("props")) {
					BOOST_FOREACH(boost::property_tree::ptree::value_type& iprop, prescribedPressureNodeInfo.get_child("props"))
  					{
  						imap.insert(std::pair<std::string, std::string>(iprop.second.get("name", "property"), iprop.second.get("value", "value")));
  					}
  				}
				builder.add_prescribed_pressure_node(name, value, node, imap);
        }
			}

			if (head->get_child_optional("components")) {
				BOOST_FOREACH(
					boost::property_tree::ptree::value_type& singleComponentsProperties,
					head->get_child("components")
					)
				{
					char id = singleComponentsProperties.second.get("id", "none")[0];
					std::string name = boost::lexical_cast<std::string>(singleComponentsProperties.second.get("name", "none"));
					
					std::vector<double> componentParameters;
					double primaryParameterOfComponenet = boost::lexical_cast<double>(singleComponentsProperties.second.get("value", "none"));
					componentParameters.push_back(primaryParameterOfComponenet);

					// not all components have initial volumes, so make this optional:
					boost::optional<std::string> initialVolume = singleComponentsProperties.second.get_optional<std::string>("initialVolume");
					if (initialVolume) {
						componentParameters.push_back(boost::lexical_cast<double>(*initialVolume));
					}

					
					bool hasControl = false; // This isn't actually used any more.
					//bool hasControl =
					//        boost::lexical_cast<int>(singleComponentsProperties.second.get("hasControl", "none"));

					std::vector<unsigned int> nodes;
					BOOST_FOREACH(
						boost::property_tree::ptree::value_type& node,
						singleComponentsProperties.second.get_child("nodes")
						)
						nodes.push_back(boost::lexical_cast<unsigned int>(
						node.second.get_value<std::string>()));

					std::map<std::string, std::string> imap;
					if (singleComponentsProperties.second.get_child_optional("props")) {
						BOOST_FOREACH(
							boost::property_tree::ptree::value_type& iprop,
							singleComponentsProperties.second.get_child("props")
							)
							imap.insert(std::pair<std::string, std::string>
							(
							iprop.second.get("name", "property"),
							iprop.second.get("value", "value")
							)
							);
					}

					switch (id)
					{
					case 'R':
						builder.add_dual_component(
							abstract_builder::Component_Resistor,
							name, componentParameters,
							nodes.at(0), nodes.at(1),
							imap
							);
						break;
					case 'L':
						builder.add_dual_component(
							abstract_builder::Component_Inductor,
							name, componentParameters,
							nodes.at(0), nodes.at(1),
							imap
							);
						break;
					case 'G':
						builder.add_dual_component(
							abstract_builder::G,
							name, componentParameters,
							nodes.at(0), nodes.at(1),
							imap
							);
						break;
					case 'C':
						builder.add_dual_component(
							abstract_builder::Component_Capacitor,
							name, componentParameters,
							nodes.at(0), nodes.at(1),
							imap
							);
						break;
					case 't':
						builder.add_dual_component(
							abstract_builder::Component_VolumeTrackingPressureChamber,
							name, componentParameters,
							nodes.at(0), nodes.at(1),
							imap
							);
						break;
					case 'D':
						builder.add_dual_component(
							abstract_builder::Component_Diode,
							name, componentParameters,
							nodes.at(0), nodes.at(1),
							imap
							);
						break;
					case 'V':
						builder.add_dual_component(
							abstract_builder::V,
							name, componentParameters,
							nodes.at(0), nodes.at(1),
							imap
							);
						break;
					case 'I':
						builder.add_dual_component(
							abstract_builder::I,
							name, componentParameters,
							nodes.at(0), nodes.at(1),
							imap
							);
						break;
					// case 'v':
					// 	builder.add_dual_component(
					// 		abstract_builder::VM,
					// 		name, 1.,
					// 		nodes.at(0), nodes.at(1),
					// 		imap
					// 		);
					// 	break;
					// case 'a':
					// 	builder.add_dual_component(
					// 		abstract_builder::AM,
					// 		name, 1.,
					// 		nodes.at(0), nodes.at(1),
					// 		imap
					// 		);
					// 	break;
					case 'H':
						builder.add_quad_component(
							abstract_builder::VCCS,
							name, componentParameters, hasControl,
							nodes.at(0), nodes.at(1),
							nodes.at(2), nodes.at(3),
							imap
							);
						break;
					case 'E':
						builder.add_quad_component(
							abstract_builder::VCVS,
							name, componentParameters, hasControl,
							nodes.at(0), nodes.at(1),
							nodes.at(2), nodes.at(3),
							imap
							);
						break;
					case 'F':
						builder.add_quad_component(
							abstract_builder::CCCS,
							name, componentParameters, hasControl,
							nodes.at(0), nodes.at(1),
							nodes.at(2), nodes.at(3),
							imap
							);
						break;
					case 'Y':
						builder.add_quad_component(
							abstract_builder::CCVS,
							name, componentParameters, hasControl,
							nodes.at(0), nodes.at(1),
							nodes.at(2), nodes.at(3),
							imap
							);
						break;
					case 'A':
						// not sure what happened here - this component had a hard-coded "1." passed,
						// so during the conversion to vectors of parameters (to support >1 parameter per component generically),
						// I've just maintainted old "functionality" by constructing a vector containing 1.0 in-place.
						builder.add_quad_component(
							abstract_builder::AO,
							name, std::vector<double>{1.0}, false,
							nodes.at(0), nodes.at(1),
							nodes.at(2), nodes.at(3),
							imap
							);
						break;
					case 'n':
						builder.add_quad_component(
							abstract_builder::n,
							name, componentParameters, hasControl,
							nodes.at(0), nodes.at(1),
							nodes.at(2), nodes.at(3),
							imap
							);
						break;
					case 'K':
						builder.add_quad_component(
							abstract_builder::K,
							name, componentParameters, hasControl,
							nodes.at(0), nodes.at(1),
							nodes.at(2), nodes.at(3),
							imap
							);
						break;
					}
				}
			}

			if (head->get_child_optional("wires")) {
				BOOST_FOREACH(
					boost::property_tree::ptree::value_type& w,
					head->get_child("wires")
					)
				{
					std::map<std::string, std::string> imap;
					if (w.second.get_child_optional("props")) {
						BOOST_FOREACH(
							boost::property_tree::ptree::value_type& iprop,
							w.second.get_child("props")
							)
							imap.insert(std::pair<std::string, std::string>
							(
							iprop.second.get("name", "property"),
							iprop.second.get("value", "value")
							)
							);
					}

					builder.add_wire_component(imap);
				}
			}

			if (head->get_child_optional("unknows")) {
				BOOST_FOREACH(
					boost::property_tree::ptree::value_type& u,
					head->get_child("unknows")
					)
				{
					std::map<std::string, std::string> imap;
					if (u.second.get_child_optional("props")) {
						BOOST_FOREACH(
							boost::property_tree::ptree::value_type& iprop,
							u.second.get_child("props")
							)
							imap.insert(std::pair<std::string, std::string>
							(
							iprop.second.get("name", "property"),
							iprop.second.get("value", "value")
							)
							);
					}

					builder.add_unknow_component(imap);
				}
			}

			// if (head->get_child_optional("userdefs")) {
			// 	BOOST_FOREACH(
			// 		boost::property_tree::ptree::value_type& ud,
			// 		head->get_child("userdefs")
			// 		)
			// 	{
			// 		std::string name =
			// 			boost::lexical_cast<std::string>(ud.second.get("name", "none"));

			// 		std::map<std::string, std::string> imap;
			// 		if (ud.second.get_child_optional("props")) {
			// 			BOOST_FOREACH(
			// 				boost::property_tree::ptree::value_type& iprop,
			// 				ud.second.get_child("props")
			// 				)
			// 				imap.insert(std::pair<std::string, std::string>
			// 				(
			// 				iprop.second.get("name", "property"),
			// 				iprop.second.get("value", "value")
			// 				)
			// 				);
			// 		}

			// 		builder.begin_userdef_component(name, imap);
			// 		parse_rec(builder, &(ud.second.get_child("def")));
			// 		builder.end_userdef_component(name, imap);
			// 	}
			// }

			builder.flush();
		}
		catch (const boost::bad_lexical_cast& blc) {
			throw read_error() << what(std::string(blc.what()));
		}
	}


	ir_builder::ir_builder() : head_(0)
	{
		boost::property_tree::ptree cc;
		head_ = &(ptree_.add_child("circuit", cc));
	}


	void ir_builder::add_circuit_properties(std::map<std::string, std::string> map)
	{
		boost::property_tree::ptree cprop;

		typedef std::map<std::string, std::string>::const_iterator ci;
		for (ci i = map.begin(); i != map.end(); ++i) {
			cprop.put("name", i->first);
			cprop.put("value", i->second);
			head_->add_child("props.prop", cprop);
		}
	}


	void ir_builder::add_circuit_property(std::string name, std::string value)
	{
		boost::property_tree::ptree cprop;
		cprop.put("name", name);
		cprop.put("value", value);
		head_->add_child("props.prop", cprop);
	}


	void ir_builder::add_wire_component(
		std::map<std::string, std::string> props
		)
	{
		boost::property_tree::ptree cc;

		if (!props.empty()) {
			boost::property_tree::ptree cprop;

			typedef std::map<std::string, std::string>::const_iterator ci;
			for (ci i = props.begin(); i != props.end(); ++i)
			{
				cprop.put("name", i->first);
				cprop.put("value", i->second);
				cc.add_child("props.prop", cprop);
			}
		}

		head_->add_child("wires.wire", cc);
	}


	// void ir_builder::add_out_component(
	// 	unsigned int v,
	// 	std::map<std::string, std::string> props
	// 	)
	// {
	// 	boost::property_tree::ptree cc;

	// 	cc.put("node", v);

	// 	if (!props.empty()) {
	// 		boost::property_tree::ptree cprop;

	// 		typedef std::map<std::string, std::string>::const_iterator ci;
	// 		for (ci i = props.begin(); i != props.end(); ++i)
	// 		{
	// 			cprop.put("name", i->first);
	// 			cprop.put("value", i->second);
	// 			cc.add_child("props.prop", cprop);
	// 		}
	// 	}

	// 	head_->put_child("out", cc);
	// }

void ir_builder::add_prescribed_pressure_node(
	std::string name,
  double value,
  unsigned int v,
	std::map < std::string, std::string > props
	)
{
	boost::property_tree::ptree cc;

	cc.put("node", v);
  cc.put("name", name);
  cc.put("value", value);

	if (!props.empty())
	{
		boost::property_tree::ptree cprop;

		typedef std::map<std::string, std::string>::const_iterator ci;
		for (ci property = props.begin(); property != props.end(); property++)
		{
			cprop.put("name", property->first);
			cprop.put("value", property->second);
			cc.add_child("props.prop",cprop);
		}
	}

  head_->add_child("prescribedPressureNodes.prescribedPressureNode", cc);

}

void ir_builder::add_threeD_interface_node(
    unsigned int v,
    std::map<std::string, std::string> props
  )
{
  boost::property_tree::ptree cc;

  cc.put("node", v);

  if(!props.empty()) {
    boost::property_tree::ptree cprop;

    typedef std::map<std::string, std::string>::const_iterator ci;      
    for(ci i = props.begin(); i != props.end(); ++i)
    {
      cprop.put("name", i->first);
      cprop.put("value", i->second);
      cc.add_child("props.prop", cprop);
    }
  }

  head_->put_child("threeDInterface", cc);
}

void ir_builder::add_dual_component(
    abstract_builder::dual_component_type c_type,
    std::string name,
    std::vector<double> value,
    unsigned int va,
    unsigned int vb,
    std::map<std::string, std::string> props
  )
{
  std::vector<unsigned int> nodes;
  nodes.push_back(va);
  nodes.push_back(vb);

  switch(c_type)
  {
  case abstract_builder::Component_Resistor:
    {
		add_item('R', name, value, nodes, props);
      break;
    }
  case abstract_builder::G:
    {
		add_item('G', name, value, nodes, props);
      break;
    }
  case abstract_builder::Component_Inductor:
    {
		add_item('L', name, value, nodes, props);
      break;
    }
  case abstract_builder::Component_Capacitor:
    {
		add_item('C', name, value, nodes, props);
      break;
    }
  case abstract_builder::Component_VolumeTrackingPressureChamber:
  {
	  add_item('t', name, value, nodes, props);
	  break;
  }
  case abstract_builder::Component_Diode:
    {
		add_item('D', name, value, nodes, props);
      break;
    }
  case abstract_builder::V:
    {
		add_item('V', name, value, nodes, props);
      break;
    }
  case abstract_builder::I:
    {
		add_item('I', name, value, nodes, props);
      break;
    }
  // case abstract_builder::VM:
  //   {
		// add_item('v', name, value, nodes, props);
  //     break;
  //   }
  // case abstract_builder::AM:
  //   {
		// add_item('a', name, value, nodes, props);
  //     break;
  //   }
  default:
    break;
  }
}


void ir_builder::add_quad_component(
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
  std::vector<unsigned int> nodes;
  nodes.push_back(va);
  nodes.push_back(vb);
  nodes.push_back(vac);
  nodes.push_back(vbc);

  switch(c_type)
  {
  case abstract_builder::VCCS:
    {
		add_item('H', name, parameterValues, nodes, props);
      break;
    }
  case abstract_builder::VCVS:
    {
		add_item('E', name, parameterValues, nodes, props);
      break;
    }
  case abstract_builder::CCCS:
    {
		add_item('F', name, parameterValues, nodes, props);
      break;
    }
  case abstract_builder::CCVS:
    {
		add_item('Y', name, parameterValues, nodes, props);
      break;
    }
  case abstract_builder::AO:
    {
		add_item('A', name, parameterValues, nodes, props);
      break;
    }
  case abstract_builder::n:
    {
		add_item('n', name, parameterValues, nodes, props);
      break;
    }
  case abstract_builder::K:
    {
		add_item('K', name, parameterValues, nodes, props);
      break;
    }
  default:
    break;
  }
}


void ir_builder::add_unknow_component(
    std::map<std::string, std::string> props
  )
{
  boost::property_tree::ptree cc;

  if(!props.empty()) {
    boost::property_tree::ptree cprop;

    typedef std::map<std::string, std::string>::const_iterator ci;
    for(ci i = props.begin(); i != props.end(); ++i) {
      cprop.put("name", i->first);
      cprop.put("value", i->second);
      cc.add_child("props.prop", cprop);
    }
  }

  head_->add_child("unknows.unknow", cc);
}


// void ir_builder::begin_userdef_component(
//     std::string name,
//     std::map<std::string,std::string> props
//   )
// {
//   boost::property_tree::ptree ud;

//   ud.put("name", name);

//   if(!props.empty()) {
//     boost::property_tree::ptree cprop;

//     typedef std::map<std::string, std::string>::const_iterator ci;
//     for(ci i = props.begin(); i != props.end(); ++i)
//     {
//       cprop.put("name", i->first);
//       cprop.put("value", i->second);
//       ud.add_child("props.prop", cprop);
//     }
//   }

//   boost::property_tree::ptree* foo = &(head_->add_child("userdefs.userdef", ud));

//   stack_.push(head_);
//   boost::property_tree::ptree def;
//   head_ = &(foo->add_child("def", def));
// }


// void ir_builder::end_userdef_component(
//     std::string name,
//     std::map<std::string,std::string> props
//   )
// {
//   head_ = stack_.top();
//   stack_.pop();
// }


void ir_builder::add_item(
    char id,
    std::string name,
    std::vector<double> parameters,
    std::vector<unsigned int> nodes,
    std::map<std::string, std::string> props
  )
{
  boost::property_tree::ptree cc;

  cc.put("id", id);
  cc.put("name", name);
  cc.put("value", parameters.at(0));
  if (parameters.size() > 1)
  {
	  // this currently just supports the volumeTrackingPressureChamber's initialVolume parameter
	  cc.put(predefinedStrings::initialVolumeXmlFieldName, parameters.at(1));
  }

  typedef std::vector<unsigned int>::const_iterator ci;
  for(ci i = nodes.begin(); i != nodes.end(); ++i)
    cc.add("nodes.node", *i);

  if(!props.empty()) {
    boost::property_tree::ptree cprop;

    typedef std::map<std::string, std::string>::const_iterator ci;
    for(ci i = props.begin(); i != props.end(); ++i)
    {
      cprop.put("name", i->first);
      cprop.put("value", i->second);
      cc.add_child("props.prop", cprop);
    }
  }

  head_->add_child("components.component", cc);
}


}
