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


#include "parser/crc_circuit.h"
#include "gui/editor/PrescribedFlowComponentMetadata.h"


namespace sapecng
{


// void crc_builder::add_out_component(
//     unsigned int v,
//     std::map<std::string, std::string> props
//   )
// {
//   out_ = v;
// }

void crc_builder::add_threeD_interface_node(
    unsigned int v,
    std::map<std::string, std::string> props
  )
{
  out_ = v;
}

void crc_builder::add_dual_component(
    abstract_builder::dual_component_type c_type,
    std::string name,
    std::vector<double> parameterValues,
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
      if(name.size() == 0 || name.at(0) != 'R')
        name = "R_" + name;

	  add_item(name, parameterValues, nodes);

      break;
    }
  case abstract_builder::G:
    {
      if(name.size() == 0 || name.at(0) != 'G')
        name = "G_" + name;

	  add_item(name, parameterValues, nodes);

      break;
    }
  case abstract_builder::Component_Inductor:
    {
      if(name.size() == 0 || name.at(0) != 'L')
        name = "L_" + name;

	  add_item(name, parameterValues, nodes);

      break;
    }
  case abstract_builder::Component_Capacitor:
    {
      if(name.size() == 0 || name.at(0) != 'C')
        name = "C_" + name;

	  add_item(name, parameterValues, nodes);

      break;
    }
  case abstract_builder::Component_Diode:
    {
      if(name.size() == 0 || name.at(0) != 'D')
        name = "D_" + name;

	  add_item(name, parameterValues, nodes);

      break;
    }
  case abstract_builder::Component_VolumeTrackingPressureChamber:
  {
	  if (name.size() == 0 || name.at(0) != 't')
		  name = "t_" + name;

	  add_item(name, parameterValues, nodes);

	  break;
  }
  case abstract_builder::V:
    {
      if(name.size() == 0 || name.at(0) != 'V')
        name = "V_" + name;

	  add_item(name, parameterValues, nodes);

      break;
    }
  case abstract_builder::I:
    {
      if(name.size() == 0 || name.at(0) != 'I')
        name = "I_" + name;

	  add_item(name, parameterValues, nodes);

      break;
    }
  default:
    break;
  }
}


void crc_builder::add_quad_component(
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
      if(name.size() == 0 || name.at(0) != 'H')
        name = "H_" + name;

	  add_item(name, parameterValues, nodes);

      break;
    }
  case abstract_builder::VCVS:
    {
      if(name.size() == 0 || name.at(0) != 'E')
        name = "E_" + name;

	  add_item(name, parameterValues, nodes);

      break;
    }
  case abstract_builder::CCCS:
    {
      if(name.size() == 0 || name.at(0) != 'F')
        name = "F_" + name;

	  add_item(name, parameterValues, nodes);

      break;
    }
  case abstract_builder::CCVS:
    {
      if(name.size() == 0 || name.at(0) != 'Y')
        name = "Y_" + name;

	  add_item(name, parameterValues, nodes);

      break;
    }
  case abstract_builder::AO:
    {
      if(name.size() == 0 || name.at(0) != 'A')
        name = "A_" + name;

      stream_ << name << " ";
      for(std::vector<unsigned int>::size_type i = 0; i < nodes.size(); ++i)
        stream_ << nodes[i] << " ";
      stream_ << std::endl;

      break;
    }
  case abstract_builder::n:
    {
      if(name.size() == 0 || name.at(0) != 'n')
        name = "n_" + name;

	  add_item(name, parameterValues, nodes);

      break;
    }
  case abstract_builder::K:
    {
      if(name.size() == 0 || name.at(0) != 'K')
        name = "K_" + name;

      std::vector<unsigned int> lp_nodes, ls_nodes;
      lp_nodes.push_back(va);
      lp_nodes.push_back(vb);
      ls_nodes.push_back(vac);
      ls_nodes.push_back(vbc);

	  add_item(props["lp:name"], std::vector<double> {std::stod(props["lp:value"].c_str(), NULL)}, lp_nodes);
	  add_item(props["ls:name"], std::vector<double> {std::stod(props["ls:value"].c_str(), NULL)}, ls_nodes);

      stream_ << name << " " << props["lp:name"] << " " << props["ls:name"];
	  stream_ << " " << parameterValues.at(0) << std::endl;

      break;
    }
  default:
    break;
  }
}

void crc_builder::writeFileHeaderComments()
{
	stream_ << "# List of components in a format similar to that for netlist." << std::endl;
	stream_ << "# Hash - commented lines are ignored." << std::endl;
	
}

void crc_builder::writeBoundaryConditionIndexComment()
{
	stream_ << "### Begin " << "PLACEHOLDER-TH" << " netlist boundary condition model" << std::endl;
}

void crc_builder::writeNumberOfComponents(const int numberOfComponents)
{
	stream_ << "# Number of Components" << std::endl;
	stream_ << numberOfComponents << std::endl;
}

void crc_builder::writeIndexOfNodeAt3DInterface()
{
  stream_ << "# Index of node at 3D interface:" << std::endl;
  stream_ << out_ << std::endl;
}


void crc_builder::flush()
{
  //stream_ << ".END" << std::endl;
}

void crc_builder::writePrescribedPressureNodesToFile(const std::vector<int> prescribedPressureNodeIndices, const std::vector<double> prescribedPressures)
{
	stream_ << "# Number of prescribed pressure nodes:" << std::endl;
	stream_ << prescribedPressureNodeIndices.size() << std::endl;
	stream_ << "# Indices of nodes with prescribed pressures:" << std::endl;
	for (auto nodeIndex = prescribedPressureNodeIndices.begin(); nodeIndex != prescribedPressureNodeIndices.end(); nodeIndex++)
	{
		stream_ << *nodeIndex << std::endl;
	}

	stream_ << "# Prescribed pressure values / scalings (dependent on types, given by next component):" << std::endl;
	for (auto nodePressurePrescription = prescribedPressures.begin(); nodePressurePrescription != prescribedPressures.end(); nodePressurePrescription++)
	{
		stream_ << *nodePressurePrescription << std::endl;
	}

	stream_ << "# Prescribed pressure types (f=fixed to value given in previous line, l=left ventricular pressure, scaled by value given in previous line):" << std::endl;
	for (int node = 0; node < prescribedPressureNodeIndices.size(); node++)
	{
		// The nodal prescriptions are all just fixed ("f") pressures for now:
		stream_ << "f" << std::endl;
	}
}

void crc_builder::writePrescribedComponentFlowsToFile(const int indexOfComponentAt3DInterface, const std::vector<qsapecng::PrescribedFlowComponentMetadata> prescribedFlowComponentsMetadata)
{
	// This should be expanded when we start supporting more than one
	// prescribed flow:
	stream_ << "# Number of prescribed flows:" << std::endl;
	int numberOfPrescribedFlows = prescribedFlowComponentsMetadata.size() + 1; // +1 for the one at the 3D interface
	stream_ << numberOfPrescribedFlows << std::endl;
	stream_ << "# Indices of components with prescribed flows" << std::endl;
	stream_ << indexOfComponentAt3DInterface << std::endl;
	for (auto componentInfo : prescribedFlowComponentsMetadata) {
		stream_ << componentInfo.getComponentIndex() << std::endl;
	}
	stream_ << "# Values of prescribed flows(3D interface set to - 1; this value is irrelevant and unused by the code):" << std::endl;
	stream_ << "-1.0e0" << std::endl;
	for (auto componentInfo : prescribedFlowComponentsMetadata) {
		stream_ << componentInfo.getPrescribedFlowValue() << std::endl;
	}
	stream_ << "# Types of prescribed flows(t = threeD domain interface)" << std::endl;
	stream_ << "t" << std::endl;
	for (auto componentInfo : prescribedFlowComponentsMetadata) {
		stream_ << componentInfo.getPrescribedFlowTypeCharCode() << std::endl;
	}
}

void crc_builder::writeNumberOfNodes(const int numberOfNodes)
{
  stream_ << "# Number of pressure nodes (including everything- 3D interface, zero-pressure points, internal nodes, etc.):" << std::endl;
  stream_ << numberOfNodes << std::endl;

  // Writing the next line really belongs to writeInitialPressures, but it is easier to do here, and is not at all confusing.
  stream_ << "# Initial pressures at the pressure nodes:" << std::endl;
}

void crc_builder::writeInitialPressures(const int nodeIndex, const double prescribedPressure)
{
  stream_ << nodeIndex << " " << prescribedPressure << std::endl;
}

void crc_builder::writeNumberOfControlledComponents(const int numberOfComponentsWithControl)
{
	stream_ << "# Number of components with control:" << std::endl;
	stream_ << numberOfComponentsWithControl << std::endl;
	stream_ << "# List of components with control, with the type of control" << std::endl;
}

void crc_builder::writeSingleControlledComponentOrNodeInfo(const int componentOrNodeIndex, const std::string controlType, const std::string additionalDataFileName)
{
	// Note that additionalDataFileName will be an empty std::string if this is not a python-controlled or time-varying component,
	// i.e. if controlType is not customPython or periodicPrescribedPressure / Flow.
	stream_ << componentOrNodeIndex << " " << controlType << " " << additionalDataFileName << std::endl;
}

void crc_builder::writeNumberOfControlledNodes(const int numberOfNodesWithControl)
{
	stream_ << "# number of nodes with control" << std::endl;
	stream_ << numberOfNodesWithControl << std::endl;
	stream_ << "# List of nodes with control, with the type of control(l = LV elastance):" << std::endl;
}

void crc_builder::add_item(
    std::string name,
    std::vector<double> parameterValues,
    std::vector<unsigned int> nodes
  )
{
	stream_ << "# Component number " << indexOfNextComponentToWrite_ << ": " << name << ". Type: " << std::endl;
	char componentType = (char)tolower(name.at(0));
	char volumeTrackingPressureChamberType = 't';
	char inductorType = 'l';
	// This is a despicable hack, because the character flag
	// "v" was already taken in qsapecng, and the flowsolver
	// expects "v" for volume-tracking pressure chambers,
	// not the "t" used internally by the new CRIMSON boundary
	// condition toolbox (built from qsapecng).
	//
	//
	// \todo fix this by getting rid of the vestigial "v" usage in
	// CRIMSON boundary condition toolbox that we don't actually need,
	// and replace the internal use of "t" with "v" (or better yet,
	// an enumerated type, or even classes!
	//
	// Similarly with inductors (which are represented by 'l' here, and by 'i' in the flowsolver).
	if (componentType == volumeTrackingPressureChamberType)
	{
		stream_ << "v" << std::endl;
	}
	else if (componentType == inductorType)
	{
		stream_ << "i" << std::endl;
	}
	else
	{
		stream_ << componentType << std::endl;
	}
	stream_ << "# Component " << indexOfNextComponentToWrite_ << " details (start-node index, end-node index, associated parameter (resistance for resistors, capacitance for capacitors):" << std::endl;
  for(std::vector<unsigned int>::size_type i = 0; i < nodes.size(); ++i)
    stream_ << nodes[i] << std::endl;
  if (parameterValues.size() > 0) {
    for (double parameter : parameterValues) {
      stream_ << parameter << " ";
    }
    stream_ << std::endl;
  }
  

  indexOfNextComponentToWrite_++;
}


}
