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


#ifndef CRC_CIRCUIT_H
#define CRC_CIRCUIT_H


#include "parser/parser.h"
#include "exception/sapecngexception.h"
#include <qlist.h>
#include "gui/editor/component.h"

#include <iostream>
#include "gui/editor/PrescribedFlowComponentMetadata.h"


namespace sapecng
{


class crc_builder: public abstract_builder
{

public:
  crc_builder(
      std::ostream& stream
	  ) : stream_(stream), out_(0), indexOfNextComponentToWrite_(1){}

  void add_circuit_properties(std::map<std::string,std::string> map) { }
  void add_circuit_property(std::string name, std::string value) { }

  void add_wire_component(
      std::map<std::string,std::string> props =
        std::map<std::string,std::string>()
    ) { }

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
    std::map<std::string,std::string> props =
      std::map<std::string,std::string>()
    )
  {
    assert(false);
  }

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
  void writeFileHeaderComments();
  void writeBoundaryConditionIndexComment();
  void writeNumberOfComponents(const int numberOfComponents);
  void writePrescribedPressureNodesToFile(const std::vector<int> prescribedPressureNodeIndices, const std::vector<double> prescribedPressures);
  void writePrescribedComponentFlowsToFile(const int indexOfComponentAt3DInterface, const std::vector<qsapecng::PrescribedFlowComponentMetadata> prescribedFlowComponentsMetadata);
  void writeNumberOfNodes(const int numberOfnodes);
  void writeInitialPressures(const int nodeIndex, const double prescribedPressure);
  void writeNumberOfControlledComponents(const int numberOfComponentsWithControl);
  void writeSingleControlledComponentOrNodeInfo(const int componentOrNodeIndex, const std::string controlType, const std::string additionalDataFileName);
  void writeNumberOfControlledNodes(const int numberOfNodesWithControl);
  void writeIndexOfNodeAt3DInterface();

private:
  void add_item(
      std::string name,
      std::vector<double> parameterValues,
      std::vector<unsigned int> nodes
    );

private:
  std::ostream& stream_;
  unsigned int out_;
  unsigned int indexOfNextComponentToWrite_;

};



}

#endif // CRC_CIRCUIT_H
