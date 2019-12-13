/*
    QSapecNG - Qt based SapecNG GUI front-end
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

#include "utility/qsapecngUtility.h"
#include "utility/strings.h"

#include "gui/editor/schematicsceneparser.h"
#include "gui/editor/component.h"
#include "gui/editor/label.h"
#include "gui/editor/wire.h"
#include "gui/editor/item.h"

#include "QtBoolPropertyManager"
#include "QtStringPropertyManager"
#include "QtProperty"

#include <QtCore/QPointF>
#include <QtCore/QPointer>
#include <QGraphicsItem>
#include <QMessageBox>
#include <QFileInfo>

#include <boost/assign.hpp>
#include <memory>
#include <cctype>

#include "parser/crc_circuit.h"

#include "PrescribedFlowComponentMetadata.h"


namespace qsapecng
{

SchematicSceneParser::ControlInfoMap SchematicSceneParser::componentControlInfos{
    {predefinedStrings::customPythonScriptString, 
        {predefinedStrings::controlScriptTypes::ControlScript_StandardPython, "customPython"}
    },
    {predefinedStrings::periodicPrescribedFlowString, 
        {predefinedStrings::controlScriptTypes::ControlScript_DatFileData, predefinedStrings::prescribedPeriodicFlow}
    },
    {predefinedStrings::lvElastanceControllerString, 
        {predefinedStrings::controlScriptTypes::ControlScript_None, "l"}
    }
};

SchematicSceneParser::ControlInfoMap SchematicSceneParser::nodeControlInfos{
    {predefinedStrings::customPythonScriptString,
        {predefinedStrings::controlScriptTypes::ControlScript_StandardPython, predefinedStrings::pythonControlNamestringForFlowsolver}
    },
    {predefinedStrings::periodicPrescribedPressureString,
        {predefinedStrings::controlScriptTypes::ControlScript_DatFileData, predefinedStrings::prescribedPeriodicPressure}
    }
};

SchematicSceneParser::SchematicSceneParser(
    const SchematicScene& scene
  ): discard_(0)
{
  setupMap();
  items_ = scene.activeItems();
  init(scene);
}


SchematicSceneParser::SchematicSceneParser(
    const SchematicScene& scene,
    const QList<QGraphicsItem*>& items
  ): discard_(0)
{
  setupMap();
  foreach(QGraphicsItem* gItem, items) {
    Item* item = qgraphicsitem_cast<Item*>(gItem);
    if(item && scene.activeItems().contains(item))
      items_.push_back(item);
  }
  init(scene);
}


SchematicSceneParser::SchematicSceneParser(
    const SchematicScene& scene,
    QGraphicsItem* gItem
  ): discard_(0)
{
  setupMap();
  Item* item = qgraphicsitem_cast<Item*>(gItem);
  if(item && scene.activeItems().contains(item))
    items_.push_back(item);
  init(scene);
}

void SchematicSceneParser::init(const SchematicScene& scene)
{
  components_ = scene.getStandardList();
  prescribedPressureNodes_ = scene.getAllPrescribedPressureNodes();
  nodalPressurePrescriptions_ = scene.getNodePropertyContainer()->subProperties();
  numberOfNodes_ = scene.getNumberOfNodes();
}


void SchematicSceneParser::parse(sapecng::abstract_builder& builder)
{
  foreach(Item* item, items_)
    parse_item(builder, item);

  foreach(Item* prescribedPressureNode, prescribedPressureNodes_)
  {
	  parse_item(builder, prescribedPressureNode);
  }

  builder.flush();
}

std::vector<std::pair<predefinedStrings::controlScriptTypes::controlScriptTypes, std::string>> SchematicSceneParser::getNamesAndTypesOfUserDefinedControlScripts()
{
	std::vector<std::pair<predefinedStrings::controlScriptTypes::controlScriptTypes, std::string>> componentControlInfo = getControlScriptsAndTypesForComponents();
	std::vector<std::pair<predefinedStrings::controlScriptTypes::controlScriptTypes, std::string>> nodeControlInfo = getControlScriptsAndTypesForNodes();
  
	std::vector<std::pair<predefinedStrings::controlScriptTypes::controlScriptTypes, std::string>> returnValue;
  returnValue = componentControlInfo;
  // append the node control info
  for (auto oneControlInfo = nodeControlInfo.begin(); oneControlInfo != nodeControlInfo.end(); oneControlInfo++)
  {
    returnValue.push_back(*oneControlInfo);
  }

  return returnValue;
}

QString SchematicSceneParser::removeExtensionIfPresent(const QString& fileName, const char* extension)
{
    auto trimmedFileName = fileName.trimmed();
    auto fileInfo = QFileInfo{trimmedFileName};
    if (fileInfo.suffix().compare(extension, Qt::CaseInsensitive) == 0) {
        return fileInfo.completeBaseName();
    }
    return trimmedFileName;
}

const char* SchematicSceneParser::getExtensionForControlType(predefinedStrings::controlScriptTypes::controlScriptTypes controlType)
{
    switch (controlType) {
    case predefinedStrings::controlScriptTypes::ControlScript_StandardPython:
        return "py";
    case predefinedStrings::controlScriptTypes::ControlScript_DatFileData:
        return "dat";
    default:
        return "";
    }
}

SchematicSceneParser::ControlScriptAndType SchematicSceneParser::getControlScriptAndType(Item* component, const ControlInfoMap& infoMap)
{
    QHash<QString, QString> subproperties = extractSubproperties(component->data(1));

    using namespace predefinedStrings;

    auto controlType = subproperties.value(componentControlTypeFieldName);
	if (controlType.isEmpty()) {
		controlType = subproperties.value(nodeControlTypeFieldName);
	}

    auto fileName = subproperties.value(additionalDataFileFieldName);

    const auto& controlInfo = infoMap[controlType];
    return {controlInfo.controlType, removeExtensionIfPresent(fileName, getExtensionForControlType(controlInfo.controlType)).toStdString()};
}

std::vector<SchematicSceneParser::ControlScriptAndType> SchematicSceneParser::getControlScriptsAndTypesForComponents()
{
    std::vector<ControlScriptAndType> componentControlScriptsAndTypes;

    // Loop the components, getting the control types and script/data file names:
    for (Item* component: components_) {
        if (isComponentWithControl(component)) {
            componentControlScriptsAndTypes.push_back(getControlScriptAndType(component, componentControlInfos));
        }
    }

    return componentControlScriptsAndTypes;
}

std::vector<SchematicSceneParser::ControlScriptAndType> SchematicSceneParser::getControlScriptsAndTypesForNodes()
{
    std::vector<ControlScriptAndType> nodeControlScriptsAndTypes;

    // Loop the prescribed pressure nodes again, getting the control datafile/script info
    for (Component* node: prescribedPressureNodes_) {
        if (isNodeWithControl(node)) {
            nodeControlScriptsAndTypes.push_back(getControlScriptAndType(node, nodeControlInfos));
        }
    }
    return nodeControlScriptsAndTypes;
}

QHash<QString, QString> SchematicSceneParser::extractSubproperties(QVariant props)
{
    QtProperty* properties = props.value<QtProperty*>();
    
    QHash<QString, QString> subproperties;
    if (properties) {
        subproperties.insert("__NAME", properties->valueText());
        foreach(QtProperty* prop, properties->subProperties())
            subproperties.insert(prop->propertyName(), prop->valueText());
    }

    return subproperties;
}

void SchematicSceneParser::writeSingleItem(sapecng::crc_builder* downcast_crc, Item* component, int oneIndexedComponent, const ControlInfoMap& infoMap)
{
    QHash<QString, QString> subproperties = extractSubproperties(component->data(1));

    auto controlType = subproperties.value(predefinedStrings::componentControlTypeFieldName);
	if (controlType.isEmpty()) {
		controlType = subproperties.value(predefinedStrings::nodeControlTypeFieldName);
	}
    auto fileName = subproperties.value(predefinedStrings::additionalDataFileFieldName);

    const auto& controlInfo = infoMap[controlType];

    std::string additionalDataFileName;
    auto ext = getExtensionForControlType(controlInfo.controlType);
    if (ext[0] != '\0') {
        additionalDataFileName =
            removeExtensionIfPresent(subproperties.value(predefinedStrings::additionalDataFileFieldName), ext).toStdString();
    }

    downcast_crc->writeSingleControlledComponentOrNodeInfo(oneIndexedComponent, controlInfo.controlTypeToWrite.toStdString(),
                                                           additionalDataFileName);
}

bool SchematicSceneParser::hasPeriodicPrescribedFlow(Item* component) const
{
	bool foundPrescribedFlow = false;
	QHash<QString, QString> subproperties = extractSubproperties(component->data(1));
	QString controlledComponentType = subproperties.value(predefinedStrings::componentControlTypeFieldName);
	std::string cctstd(controlledComponentType.toStdString());
	std::cout << cctstd << std::endl;
	if (controlledComponentType.compare(QString(predefinedStrings::periodicPrescribedFlowString)) == 0)
	{
		foundPrescribedFlow = true;
	}

	return foundPrescribedFlow;
}

void SchematicSceneParser::writeCrimsonFlowsolverCircuitDescriptionFile(
    sapecng::abstract_builder& builder
  )
{
    builder.writeFileHeaderComments();
    builder.writeBoundaryConditionIndexComment();

    int numberOfComponents = 0;
    foreach (Item* item, items_)
        if (isComponent(item)) {
            numberOfComponents++;
        }
    builder.writeNumberOfComponents(numberOfComponents);

	// Write the component details:
    foreach (Item* item, items_)
        parse_item(builder, item);

    // Do the prescribed pressure nodes. This only works with the crc_builder at present;
    // the assert will trigger if we try to do it with any other builders.
    //
    // First, we extract the prescribed pressure node data into std::vectors, for passing:
    std::vector<int> prescribedPressureNodeIndices;
    std::vector<double> prescriptionValues;
    foreach (Component* prescribedPressureNode, prescribedPressureNodes_) {
        prescribedPressureNodeIndices.push_back(prescribedPressureNode->getIndexIfThisIsAPrescribedPressureNode());

        auto subproperties = extractSubproperties(prescribedPressureNode->data(1));
        prescriptionValues.push_back(subproperties.value(predefinedStrings::prescribedPressureParameterValueName).toDouble());
    }
    // Now call for the writing of the prescribed pressure nodes data:
    sapecng::crc_builder* downcast_crc = dynamic_cast<sapecng::crc_builder*>(&builder);
    //  assert(downcast_crc); //\removed assert
    // int size = prescribedPressureNodes_.size();
    downcast_crc->writePrescribedPressureNodesToFile(prescribedPressureNodeIndices, prescriptionValues);

	std::vector<PrescribedFlowComponentMetadata> prescribedFlowComponentMetadata;
	int currentComponentIndex = 0; // I hate this... need to re-write so that the components just know their indices...
	foreach(Item* maybeComponent, items_) {
		if (isComponent(maybeComponent)) {
			currentComponentIndex++;
			if (hasPeriodicPrescribedFlow(maybeComponent)) {
				double thisFieldDoesNothingYet = 0.0; // needs implementing so that a fixed (i.e. non-Python-adjusted) component flow can be set in the GUI and passed as this value.
				prescribedFlowComponentMetadata.push_back(PrescribedFlowComponentMetadata(currentComponentIndex, PrescribedFlowComponentType::PrescribedFlow_Fixed, thisFieldDoesNothingYet));
			}
		}
	}
    int indexOfComponentAt3DInterface = findIndexOfComponentAt3DInterface();
	downcast_crc->writePrescribedComponentFlowsToFile(indexOfComponentAt3DInterface, prescribedFlowComponentMetadata);

    // write the list of nodes:
    downcast_crc->writeNumberOfNodes(numberOfNodes_);

    // Write the initial pressures for all the nodes:
    int indexOfNextNodeToWriteToFile = 1;
    foreach (QtProperty* subproperty, nodalPressurePrescriptions_) {
        downcast_crc->writeInitialPressures(indexOfNextNodeToWriteToFile, subproperty->valueText().toDouble());
        indexOfNextNodeToWriteToFile++;
    }

    downcast_crc->writeIndexOfNodeAt3DInterface();

    {
        downcast_crc->writeNumberOfControlledComponents(
            std::count_if(items_.begin(), items_.end(), [this](Item* item) { return isComponentWithControl(item); }));

        // Now loop the items again, writing the actual control information to file:
        for (int index = 0; index < components_.size(); index++) {
            Item* component = components_.at(index);
            if (isComponentWithControl(component)) {
                writeSingleItem(downcast_crc, component, toOneIndexing(index), componentControlInfos);
            }
        }
    }

    {
        downcast_crc->writeNumberOfControlledNodes(
            std::count_if(prescribedPressureNodes_.begin(), prescribedPressureNodes_.end(), [this](Component* node) { return isNodeWithControl(node); }));

        // Now loop the prescribed pressure nodes again, writing the actual control information to file:
        for (int index = 0; index < prescribedPressureNodes_.size(); index++) {
            Component* node = prescribedPressureNodes_.at(index);
            if (isNodeWithControl(node)) {
                writeSingleItem(downcast_crc, node, node->getIndexIfThisIsAPrescribedPressureNode(), nodeControlInfos);
            }
        }
    }

  builder.flush();
}

int SchematicSceneParser::findIndexOfComponentAt3DInterface() const
{
  int indexToReturn = -1; // nonsense initial value
  int numberOfComponentsFoundAt3Dinterface = 0; // This should never not be 1; we'll count it and check!
	for (int index = 0; index < components_.size(); index++)
	{
		if (static_cast<Component*>( components_.at(index) )->isAt3DInterface())
		{
			numberOfComponentsFoundAt3Dinterface++;
			// The components should not be zero-indexed, but the
			// list is:
			indexToReturn = toOneIndexing(index);
		}
	}
  
	// We shouldn't reach this point (it means we didn't find a component at the 3D interface):
  if (numberOfComponentsFoundAt3Dinterface==0)
  {
  	QMessageBox messageBox;
  	messageBox.critical(0, "Error", "No 3D interface component was found!");
  	messageBox.setFixedSize(500, 200);
  }
  else if (numberOfComponentsFoundAt3Dinterface > 1)
  {
    QMessageBox messageBox;
    messageBox.critical(0, "Error", "Only one component may connect to the 3D interface directly!");
    messageBox.setFixedSize(500, 200);
  }

	return indexToReturn;
}

void SchematicSceneParser::storeItemData(
    std::map<std::string, std::string>& props,
    SchematicScene::SupportedItemType type,
    Item* item
  )
{
  QPointF position = item->pos();
  std::string x_pos = QString::number(position.x()).toStdString();
  std::string y_pos = QString::number(position.y()).toStdString();

  std::string mirrored = (item->mirrored() ? "1" : "0");
  std::string rot_factor =
    QString::number(item->angle() / 90).toStdString();

  props["type"] = QString::number(type).toStdString();
  props["x"] = x_pos;
  props["y"] = y_pos;
  props["mirrored"] = mirrored;
  props["angle"] = rot_factor;

  // Get other properties from the item:
  QHash<QString, QString> userConfiguredProperties = item->getProperties();
  if (userConfiguredProperties.contains(predefinedStrings::componentControlTypeFieldName))
  {
    props[predefinedStrings::componentControlTypeFieldName] = userConfiguredProperties.value(predefinedStrings::componentControlTypeFieldName).toStdString();
  }

  if (userConfiguredProperties.contains(predefinedStrings::nodeControlTypeFieldName))
  {
    props[predefinedStrings::nodeControlTypeFieldName] = userConfiguredProperties.value(predefinedStrings::nodeControlTypeFieldName).toStdString();
  }

  if (userConfiguredProperties.contains(predefinedStrings::additionalDataFileFieldName))
  {
    props[predefinedStrings::additionalDataFileFieldName] = userConfiguredProperties.value(predefinedStrings::additionalDataFileFieldName).toStdString();
  }

  // if (userConfiguredProperties.contains(predefinedStrings::prescribedPressureParameterValueName))
  // {
  //   props[predefinedStrings::prescribedPressureParameterValueName] = userConfiguredProperties.value(predefinedStrings::prescribedPressureParameterValueName).toStdString();
  // }

  if (userConfiguredProperties.contains(predefinedStrings::genericComponentParameterName))
  {
    props[predefinedStrings::genericComponentParameterName] = userConfiguredProperties.value(predefinedStrings::genericComponentParameterName).toStdString();
  }

}

void SchematicSceneParser::storeLabel(
    std::map<std::string, std::string>& props,
    Component* component
  )
{
  if(!component->label()->toPlainText().trimmed().isEmpty()) {
    QPointF pos = component->label()->pos();
    props["x_label"] = QString::number(pos.x()).toStdString();
    props["y_label"] = QString::number(pos.y()).toStdString();
  }
}


void SchematicSceneParser::setupMap()
{
	dualMap_[SchematicScene::CapacitorItemType] = sapecng::abstract_builder::Component_Capacitor;
	dualMap_[SchematicScene::DiodeItemType] = sapecng::abstract_builder::Component_Diode;
  dualMap_[SchematicScene::VolumeTrackingChamberItemType] = sapecng::abstract_builder::Component_VolumeTrackingPressureChamber;
  dualMap_[SchematicScene::ConductanceItemType] = sapecng::abstract_builder::G;
  dualMap_[SchematicScene::CurrentSourceItemType] =
    sapecng::abstract_builder::I;
  dualMap_[SchematicScene::ResistorItemType] = sapecng::abstract_builder::Component_Resistor;
  //dualMap_[SchematicScene::threeDInterfaceNodeType] = sapecng::abstract_builder::T; //CA2
  dualMap_[SchematicScene::InductorItemType] = sapecng::abstract_builder::Component_Inductor;
  dualMap_[SchematicScene::VoltageSourceItemType] =
    sapecng::abstract_builder::V;

  // dualMap_[SchematicScene::VoltmeterItemType] = sapecng::abstract_builder::VM;
  // dualMap_[SchematicScene::AmmeterItemType] = sapecng::abstract_builder::AM;

  quadMap_[SchematicScene::CCCSItemType] = sapecng::abstract_builder::CCCS;
  quadMap_[SchematicScene::CCVSItemType] = sapecng::abstract_builder::CCVS;
  quadMap_[SchematicScene::VCCSItemType] = sapecng::abstract_builder::VCCS;
  quadMap_[SchematicScene::VCVSItemType] = sapecng::abstract_builder::VCVS;
  quadMap_[SchematicScene::OpAmplItemType] = sapecng::abstract_builder::AO;
  quadMap_[SchematicScene::TransformerItemType] = sapecng::abstract_builder::n;
  quadMap_[SchematicScene::MutualInductanceItemType] =
    sapecng::abstract_builder::K;
}

bool SchematicSceneParser::isComponent(Item* item)
{
	SchematicScene::SupportedItemType type = SchematicScene::itemType(item);

	switch (type)
	{
		case SchematicScene::CapacitorItemType:
		case SchematicScene::DiodeItemType:
		case SchematicScene::VolumeTrackingChamberItemType:
		//case SchematicScene::ConductanceItemType:
		case SchematicScene::InductorItemType:
		case SchematicScene::ResistorItemType:
		//case SchematicScene::CurrentSourceItemType:
		//case SchematicScene::VoltageSourceItemType:
		{
			return true;
		}
	}

	return false;
}

bool SchematicSceneParser::isComponentWithControl(Item* item)
{
	bool itemIsAComponent = isComponent(item);
	bool itemHasControl = false; // we will change this to true in a moment if necessary.

	if (itemIsAComponent)
	{
		QHash<QString, QString> subproperties = getSubproperties(static_cast<Component*> (item));
        std::string controlType = subproperties.value(predefinedStrings::componentControlTypeFieldName).toStdString();

		itemHasControl = (controlType.compare(predefinedStrings::noControlString) != 0);
	}

	return (itemIsAComponent && itemHasControl);
}

bool SchematicSceneParser::isNodeWithControl(Component* component)
{
    bool itemIsAPrescribedPressureNode = component->isReallyJustAPrescribedPressureNode();

    bool itemHasControl = false;
    if (itemIsAPrescribedPressureNode) {
        QHash<QString, QString> subproperties = getSubproperties(component);
        std::string controlType = subproperties.value(predefinedStrings::nodeControlTypeFieldName).toStdString();

        itemHasControl = (controlType.compare(predefinedStrings::noControlString) != 0);
    }

    return (itemIsAPrescribedPressureNode && itemHasControl);
}

QHash<QString, QString> SchematicSceneParser::getSubproperties(Component* component)
{
	std::string controlType;

	QtProperty* properties = SchematicScene::itemProperties(component);
	QHash<QString, QString> subproperties;
	if (properties) {
		subproperties.insert("__NAME", properties->valueText());
		foreach(QtProperty* prop, properties->subProperties())
			subproperties.insert(prop->propertyName(), prop->valueText());
	}
	return subproperties;
}


void SchematicSceneParser::parse_item(
    sapecng::abstract_builder& builder,
    Item* item
  )
{
  SchematicScene::SupportedItemType type = SchematicScene::itemType(item);
  std::map<std::string, std::string> props;
  storeItemData(props, type, item);

  QtProperty* properties = SchematicScene::itemProperties(item);

  QHash<QString, QString> subproperties;
  if(properties) {
    subproperties.insert("__NAME", properties->valueText());
	foreach(QtProperty* prop, properties->subProperties())
	{
		subproperties.insert(prop->propertyName(), prop->valueText());
		//std::string nothing = prop->propertyName().toStdString();
	}
  }

  switch(type)
  {
  case SchematicScene::GroundItemType:
  // case SchematicScene::PortItemType:
  //   {
  //     // add as unknow 
  //     Component* component = static_cast<Component*>(item);
  //     props["node"] = QString::number(component->nodes().front()).toStdString();

  //     builder.add_unknow_component(props);

  //     break;
  //   }
  case SchematicScene::LabelItemType:
    {
      // add as unknow 
      Label* label = static_cast<Label*>(item);
      props["text"] = label->text().toStdString();

      builder.add_unknow_component(props);

      break;
    }
  case SchematicScene::WireItemType:
    {
      Wire* wire = static_cast<Wire*>(item);
      props["orientation"] =
        QString::number(wire->orientation()).toStdString();
      props["to_x"] =
        QString::number(wire->toPoint().x()).toStdString();
      props["to_y"] =
        QString::number(wire->toPoint().y()).toStdString();
      props["conn"] =
        QString::number(wire->isJunctionsConnected()).toStdString();

      builder.add_wire_component(props);

      break;
    }
  case SchematicScene::threeDInterfaceNodeType:
	  {
		  if (!discard_) {
			  Component* threeDInterfaceNode = static_cast<Component*>(item);

			  builder.add_threeD_interface_node(threeDInterfaceNode->nodes().front(), props);
		  }

		  break;
	  }
  case SchematicScene::prescribedPressureNodeType:
  {
	  if (!discard_)
	  {
		  Component* prescribedPressureNode = static_cast<Component*>(item);
		  builder.add_prescribed_pressure_node(subproperties.value("__NAME").toStdString(), subproperties.value(predefinedStrings::prescribedPressureParameterValueName).toDouble(), prescribedPressureNode->nodes().front(), props);
	  }
	  break;
  }
  // case SchematicScene::OutItemType:
  //   {
  //     if(!discard_) {
  //       Component* out = static_cast<Component*>(item);

  //       builder.add_out_component(out->nodes().front(), props);
  //     }

  //     break;
  //   }
  // case SchematicScene::VoltmeterItemType:
  // case SchematicScene::AmmeterItemType:
  //   {
  //     Component* component = static_cast<Component*>(item);
  //     QVector<int> nodes = component->nodes();

  //     builder.add_dual_component(
  //       dualMap_[type], "", 1.,
  //       nodes.at(1), nodes.at(0),
  //       props
  //     );

  //     break;
  //   }
  case SchematicScene::VolumeTrackingChamberItemType:
  {
	  Component* component = static_cast<Component*>(item);
	  QVector<int> nodes = component->nodes();
	  storeLabel(props, component);

	  std::vector<double> componentParameters{ subproperties.value(predefinedStrings::genericComponentParameterName).toDouble(), subproperties.value(predefinedStrings::initialVolumeParameterName).toDouble() };
	  builder.add_dual_component(
		  dualMap_[type], subproperties.value("__NAME").toStdString(),
		  componentParameters,
		  nodes.at(1), nodes.at(0),
		  props
		  );

	  break;
  }
  case SchematicScene::CapacitorItemType:
  case SchematicScene::DiodeItemType:
  case SchematicScene::ConductanceItemType:
  case SchematicScene::InductorItemType:
  case SchematicScene::ResistorItemType:
  case SchematicScene::CurrentSourceItemType:
  case SchematicScene::VoltageSourceItemType:
    {
      Component* component = static_cast<Component*>(item);
      QVector<int> nodes = component->nodes();
      storeLabel(props, component);

      builder.add_dual_component(
        dualMap_[type], subproperties.value("__NAME").toStdString(),
		subproperties.value(predefinedStrings::genericComponentParameterName).toDouble(),
        nodes.at(1), nodes.at(0),
        props
      );

      break;
    }
  case SchematicScene::CCCSItemType:
  case SchematicScene::CCVSItemType:
  case SchematicScene::VCCSItemType:
  case SchematicScene::VCVSItemType:
    {
      Component* component = static_cast<Component*>(item);
      QVector<int> nodes = component->nodes();
      storeLabel(props, component);

      builder.add_quad_component(
        quadMap_[type], subproperties.value("__NAME").toStdString(),
        subproperties.value("Value").toDouble(),
        QVariant(subproperties.value("Controlled")).toBool(),
        nodes.at(1), nodes.at(0), nodes.at(3), nodes.at(2),
        props
      );

      break;
    }
  case SchematicScene::OpAmplItemType:
    {
      Component* component = static_cast<Component*>(item);
      QVector<int> nodes = component->nodes();
      storeLabel(props, component);

      builder.add_quad_component(
        quadMap_[type], subproperties.value("__NAME").toStdString(), 1., false,
        SchematicScene::Ground, nodes.at(2), nodes.at(1), nodes.at(0), props
      );

      break;
    }
  case SchematicScene::TransformerItemType:
    {
      Component* component = static_cast<Component*>(item);
      QVector<int> nodes = component->nodes();
      storeLabel(props, component);

      builder.add_quad_component(
        quadMap_[type], subproperties.value("__NAME").toStdString(),
        subproperties.value("Value").toDouble(),
        QVariant(subproperties.value("Controlled")).toBool(),
        nodes.at(3), nodes.at(2), nodes.at(1), nodes.at(0),
        props
      );

      break;
    }
  case SchematicScene::MutualInductanceItemType:
    {
      Component* component = static_cast<Component*>(item);
      QVector<int> nodes = component->nodes();
      storeLabel(props, component);

      props["lp:name"] = subproperties.value("lp:name").toStdString();
      props["lp:value"] = subproperties.value("lp:value").toStdString();
      props["ls:name"] = subproperties.value("ls:name").toStdString();
      props["ls:value"] = subproperties.value("ls:value").toStdString();

      builder.add_quad_component(
        quadMap_[type], subproperties.value("__NAME").toStdString(),
        subproperties.value("Value").toDouble(),
        QVariant(subproperties.value("Controlled")).toBool(),
        nodes.at(3), nodes.at(2), nodes.at(1), nodes.at(0),
        props
      );

      break;
    }
  // case SchematicScene::UserDefItemType:
  //   {
  //     Component* component = static_cast<Component*>(item);
  //     storeLabel(props, component);

  //     builder.begin_userdef_component(
  //         subproperties.value("__NAME").toStdString(),
  //         props
  //       );

  //     ++discard_;

  //     QPointer<qsapecng::SchematicScene> scene =
  //       item->data(101).value< QPointer<qsapecng::SchematicScene> >();

  //     foreach(Item* item, scene->activeItems())
  //       parse_item(builder, item);

  //     --discard_;

  //     builder.end_userdef_component(
  //         subproperties.value("__NAME").toStdString(),
  //         props
  //       );

  //     // outer-to-inner port buffer resistors
  //     QVector<int> nodes = component->nodes();
  //     QVector<int> ports = scene->ports();

  //     if(nodes.size() == ports.size()) {
  //       for(int i = 0; i < nodes.size(); ++i)
  //         builder.add_dual_component(
  //           dualMap_[SchematicScene::ResistorItemType],
  //           "", .0, nodes.at(i), ports.at(i),
  //           boost::assign::map_list_of("discard", "1")
  //         );
  //     } else {
  //       // something strange happens - port-to-ground short circuits
  //       foreach(int node, nodes)
  //         builder.add_dual_component(
  //           dualMap_[SchematicScene::ResistorItemType],
  //           "", .0, node, SchematicScene::Ground,
  //           boost::assign::map_list_of("discard", "1")
  //         );
  //     }

  //     break;
  //   }
  default:
    break;
  }
}


void SchematicSceneBuilder::add_circuit_properties(
  std::map<std::string, std::string> map)
{
}


void SchematicSceneBuilder::add_circuit_property(
  std::string name, std::string value)
{
}


void SchematicSceneBuilder::add_wire_component(
    std::map<std::string, std::string> props
  )
{
  if(!discard(props)) {
    Wire* item = static_cast<Wire*>(
      SchematicScene::itemByType(SchematicScene::WireItemType));

    bool ok;

    double to_x = QString::fromStdString(props["to_x"]).toDouble(&ok);
    if(!ok)
      to_x = SchematicScene::GridStep;

    double to_y = QString::fromStdString(props["to_y"]).toDouble(&ok);
    if(!ok)
      to_y = SchematicScene::GridStep;

    bool conn = QString::fromStdString(props["conn"]).toInt(&ok);
    if(!ok)
      conn = false;

    item->setWire(QLineF(QPointF(0, 0), QPointF(to_x, to_y)));
    item->setConnectedJunctions(conn);

    scene_->addSupportedItem(item, false);
    grid_coordinate(item, props);
    items_.push_back(item);
  }
}


// void SchematicSceneBuilder::add_out_component(
//     unsigned int v,
//     std::map<std::string, std::string> props
//   )
// {
//   if(!discard(props)) {
//     Item* item = static_cast<Item*>(
//       SchematicScene::itemByType(SchematicScene::OutItemType));

//     scene_->addSupportedItem(item, false);
//     grid_coordinate(item, props);
//     mirror_and_rotate(item, props);
//     items_.push_back(item);
//   }
// }

void SchematicSceneBuilder::add_threeD_interface_node(
    unsigned int v,
    std::map<std::string, std::string> props
  )
{
  if(!discard(props)) {
    Item* item = static_cast<Item*>(
      SchematicScene::itemByType(SchematicScene::threeDInterfaceNodeType));

    scene_->addSupportedItem(item, false);
    grid_coordinate(item, props);
    mirror_and_rotate(item, props);
    items_.push_back(item);
  }
}

void SchematicSceneBuilder::add_prescribed_pressure_node(
	std::string name,
	double value,
	unsigned int v,
	std::map<std::string, std::string> props
	)
{
	if (!discard(props))
	{
		Component* prescribedPressureNode = static_cast<Component*>(
			SchematicScene::itemByType(SchematicScene::prescribedPressureNodeType));

		scene_->addSupportedItem(prescribedPressureNode, false);
		grid_coordinate(prescribedPressureNode, props);
		mirror_and_rotate(prescribedPressureNode, props);
		setup_properties(prescribedPressureNode, name, std::vector<double> {value}, props);
		items_.push_back(prescribedPressureNode);
	}
}

void SchematicSceneBuilder::add_dual_component(
    sapecng::abstract_builder::dual_component_type c_type,
    std::string name,
    std::vector<double> parameterValues,
    unsigned int va,
    unsigned int vb,
    std::map<std::string, std::string> props
  )
{
  if(!discard(props)) {
    Component* component = 0;

    switch(c_type)
    {
	case sapecng::abstract_builder::Component_Resistor:
      {
        component = static_cast<Component*>(
          SchematicScene::itemByType(SchematicScene::ResistorItemType));
        break;
      }
    case sapecng::abstract_builder::G:
      {
        component = static_cast<Component*>(
          SchematicScene::itemByType(SchematicScene::ConductanceItemType));
        break;
      }
	case sapecng::abstract_builder::Component_Inductor:
      {
        component = static_cast<Component*>(
          SchematicScene::itemByType(SchematicScene::InductorItemType));
        break;
      }
	case sapecng::abstract_builder::Component_Capacitor:
      {
        component = static_cast<Component*>(
          SchematicScene::itemByType(SchematicScene::CapacitorItemType));
        break;
      }
	case sapecng::abstract_builder::Component_Diode:
	{
		component = static_cast<Component*>(
			SchematicScene::itemByType(SchematicScene::DiodeItemType));
		break;
	}
	case sapecng::abstract_builder::Component_VolumeTrackingPressureChamber:
	{
		component = static_cast<Component*>(
			SchematicScene::itemByType(SchematicScene::VolumeTrackingChamberItemType));
		break;
	}
    case sapecng::abstract_builder::V:
      {
        component = static_cast<Component*>(
          SchematicScene::itemByType(SchematicScene::VoltageSourceItemType));
        break;
      }
    case sapecng::abstract_builder::I:
      {
        component = static_cast<Component*>(
          SchematicScene::itemByType(SchematicScene::CurrentSourceItemType));
        break;
      }
    // case sapecng::abstract_builder::VM:
    //   {
    //     component = static_cast<Component*>(
    //       SchematicScene::itemByType(SchematicScene::VoltmeterItemType));
    //     break;
    //   }
    // case sapecng::abstract_builder::AM:
    //   {
    //     component = static_cast<Component*>(
    //       SchematicScene::itemByType(SchematicScene::AmmeterItemType));
    //     break;
    //   }
    default:
      break;
    }

    if(component) {
      scene_->addSupportedItem(component, false);
      grid_coordinate(component, props);
	  setup_properties(component, name, parameterValues, props);
      mirror_and_rotate(component, props);
      adjust_label(component, props);
      items_.push_back(component);
    }
  }
}


void SchematicSceneBuilder::add_quad_component(
    sapecng::abstract_builder::quad_component_type c_type,
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
  if(!discard(props)) {
    Component* component = 0;

    switch(c_type)
    {
    case sapecng::abstract_builder::VCCS:
      {
        component = static_cast<Component*>(
          SchematicScene::itemByType(SchematicScene::VCCSItemType));
        break;
      }
    case sapecng::abstract_builder::VCVS:
      {
        component = static_cast<Component*>(
          SchematicScene::itemByType(SchematicScene::VCVSItemType));
        break;
      }
    case sapecng::abstract_builder::CCCS:
      {
        component = static_cast<Component*>(
          SchematicScene::itemByType(SchematicScene::CCCSItemType));
        break;
      }
    case sapecng::abstract_builder::CCVS:
      {
        component = static_cast<Component*>(
          SchematicScene::itemByType(SchematicScene::CCVSItemType));
        break;
      }
    case sapecng::abstract_builder::AO:
      {
        component = static_cast<Component*>(
          SchematicScene::itemByType(SchematicScene::OpAmplItemType));
        break;
      }
    case sapecng::abstract_builder::n:
      {
        component = static_cast<Component*>(
          SchematicScene::itemByType(SchematicScene::TransformerItemType));
        break;
      }
    case sapecng::abstract_builder::K:
      {
        component = static_cast<Component*>(
          SchematicScene::itemByType(
            SchematicScene::MutualInductanceItemType));
        break;
      }
    default:
      break;
    }

    if(component) {
      scene_->addSupportedItem(component, false);
      grid_coordinate(component, props);
	  setup_properties(component, name, parameterValues, props);
      mirror_and_rotate(component, props);
      adjust_label(component, props);
      items_.push_back(component);
    }
  }
}


void SchematicSceneBuilder::add_unknow_component(
    std::map<std::string, std::string> props
  )
{
  if(!discard(props)) {
    bool ok;

    SchematicScene::SupportedItemType type =
      (SchematicScene::SupportedItemType)
        QString::fromStdString(props["type"]).toInt(&ok);

    if(!ok)
      type = SchematicScene::NilItemType;

    switch(type)
    {
    case SchematicScene::GroundItemType:
    // case SchematicScene::PortItemType:
    //   {
    //     Item* item = static_cast<Item*>(
    //       SchematicScene::itemByType(type));

    //     scene_->addSupportedItem(item, false);
    //     grid_coordinate(item, props);
    //     mirror_and_rotate(item, props);
    //     items_.push_back(item);

    //     break;
    //   }
    case SchematicScene::LabelItemType:
      {
        Label* item = static_cast<Label*>(
          SchematicScene::itemByType(SchematicScene::LabelItemType));

        scene_->addSupportedItem(item, false);
        grid_coordinate(item, props);
        mirror_and_rotate(item, props);
        item->setText(QString::fromStdString(props["text"]));
        items_.push_back(item);

        break;
      }
    default:
      break;
    }
  }
}


// void SchematicSceneBuilder::begin_userdef_component(
//     std::string name,
//     std::map<std::string,std::string> props
//   )
// {
//   QPair< SchematicScene*, QList<QGraphicsItem*> > pair(scene_, items_);
  
//   stack_.push(pair);
//   scene_ = new SchematicScene;
//   items_.clear();
// }


// void SchematicSceneBuilder::end_userdef_component(
//     std::string name,
//     std::map<std::string,std::string> props
//   )
// {
//   int size = scene_->size();
//   QByteArray md5 = stack_.back().first->registerUserDef(*scene_);

//   Component* component = static_cast<Component*>(
//     SchematicScene::itemByType(SchematicScene::UserDefItemType));
  
//   component->setPath(SchematicScene::userDefPath(size));
//   component->addNodes(SchematicScene::userDefNodes(size));
//   component->setData(99, md5);
  
//   QPointer<qsapecng::SchematicScene> smart(scene_);
//   component->setData(101, qVariantFromValue(smart));
  
//   QPair< SchematicScene*, QList<QGraphicsItem*> > pair = stack_.pop();
  
//   items_.clear();
//   items_.append(pair.second);
//   scene_ = pair.first;
  
//   scene_->addSupportedItem(component, false);
//   grid_coordinate(component, props);
//   setup_properties(component, name, 0, props);
//   mirror_and_rotate(component, props);
//   adjust_label(component, props);
//   items_.push_back(component);
// }


void SchematicSceneBuilder::flush()
{
  QPointF offset = QPointF(0, 0);
  if(offset_ != QPointF(0, 0)) {
    foreach(QGraphicsItem* item, items_)
      offset += item->pos();

    offset /= items_.size();
  }

  QPointF realOffset = offset_ - offset;
  foreach(QGraphicsItem* item, items_)
    item->setPos(scene_->closestGridPoint(item->pos() + realOffset));
  
  scene_->addItems(items_);
}


bool SchematicSceneBuilder::discard(std::map<std::string,std::string> props) {
  bool ok;

  int discard = QString::fromStdString(props["discard"]).toInt(&ok);
  if(!ok)
    discard = 0;
  
  return discard;
}


void SchematicSceneBuilder::grid_coordinate(
    Item* item, std::map<std::string, std::string> props
  )
{
  bool ok;

  double x = QString::fromStdString(props["x"]).toDouble(&ok);
  if(!ok)
    x = 0;

  double y = QString::fromStdString(props["y"]).toDouble(&ok);
  if(!ok)
    y = 0;

  item->setPos(scene_->closestGridPoint(QPointF(x, y)));
}


void SchematicSceneBuilder::mirror_and_rotate(
  Item* item, std::map<std::string, std::string> props)
{
  bool ok;

  bool mirrored = QString::fromStdString(props["mirrored"]).toInt(&ok);
  if(!ok)
    mirrored = false;

  int rot_factor = QString::fromStdString(props["angle"]).toInt(&ok);
  if(!ok)
    rot_factor = 0;

  if(mirrored)
    item->mirror();

  for(int i = 0; i < rot_factor; ++i)
    item->rotate();
}


void SchematicSceneBuilder::adjust_label(
  Component* cmp, std::map<std::string, std::string> props)
{
  bool xOk, yOk;

  double xLab = QString::fromStdString(props["x_label"]).toDouble(&xOk);
  double yLab = QString::fromStdString(props["y_label"]).toDouble(&yOk);

  if(xOk && yOk)
    cmp->label()->setPos(QPointF(xLab, yLab));
}


void SchematicSceneBuilder::setup_properties(
    Item* item,
    std::string name,
    std::vector<double> passedItemParameterValues,
    std::map<std::string, std::string> props
  )
{
  QtProperty* properties = SchematicScene::itemProperties(item);

  QHash<QString, QtProperty*> subproperties;
  if(properties) {
    subproperties.insert("__NAME", properties);
    foreach(QtProperty* prop, properties->subProperties())
      subproperties.insert(prop->propertyName(), prop);
  }

  if(subproperties.contains("__NAME")) {
    QtStringPropertyManager* spm =
      qobject_cast<QtStringPropertyManager*>(
        subproperties.value("__NAME")->propertyManager());
      if(spm)
        spm->setValue(
            subproperties.value("__NAME"),
            QString::fromStdString(name)
          );
  }

  if (subproperties.contains(predefinedStrings::initialVolumeParameterName))
  {
	  QtStringPropertyManager* spm = qobject_cast<QtStringPropertyManager*>(subproperties.value(predefinedStrings::initialVolumeParameterName)->propertyManager());
	  if (spm)
		  spm->setValue(subproperties.value(predefinedStrings::initialVolumeParameterName), QString::number(passedItemParameterValues.at(1)));
  }

  if (subproperties.contains(predefinedStrings::prescribedPressureParameterValueName))
  {
    QtDoublePropertyManager* doublePropertyManager =
		qobject_cast<QtDoublePropertyManager*>(subproperties.value(predefinedStrings::prescribedPressureParameterValueName)->propertyManager());
  	if (doublePropertyManager)
    { 
		  doublePropertyManager->setValue(subproperties.value(predefinedStrings::prescribedPressureParameterValueName), passedItemParameterValues.at(0));
    }
  }

  if (subproperties.contains(predefinedStrings::genericComponentParameterName)) {
    QtStringPropertyManager* spm =
      qobject_cast<QtStringPropertyManager*>(
	  subproperties.value(predefinedStrings::genericComponentParameterName)->propertyManager());
      if(spm)
        spm->setValue(
		subproperties.value(predefinedStrings::genericComponentParameterName),
		QString::number(passedItemParameterValues.at(0))
          );
  }

  if (subproperties.contains(predefinedStrings::additionalDataFileFieldName))
  {
      QtStringPropertyManager* spm = qobject_cast<QtStringPropertyManager*>(subproperties.value(predefinedStrings::additionalDataFileFieldName)->propertyManager());

      if(spm)
      {
        spm->setValue( subproperties.value(predefinedStrings::additionalDataFileFieldName), QString::fromStdString(props.at(predefinedStrings::additionalDataFileFieldName)) );        
      }
  }

  if (subproperties.contains(predefinedStrings::componentControlTypeFieldName))
  {
	  QtVariantPropertyManager* variantManager = qobject_cast<QtVariantPropertyManager*>(subproperties.value(predefinedStrings::componentControlTypeFieldName)->propertyManager());

      if(variantManager)
      {
        int controlCode;
        std::string controlTypeStringFromFile = props.at(predefinedStrings::componentControlTypeFieldName);
        int loopIndex = 0;
        // Loop the possible control types, looking for the index ("controlCode") of the control type string that was read from the xml:
        while(predefinedStrings::componentControlTypes[loopIndex] != NULL) // while we've not reached the end of our null-terminated char* array:
        {
          if (strcmp(controlTypeStringFromFile.c_str(), predefinedStrings::componentControlTypes[loopIndex]) == 0)
          {
            controlCode = loopIndex;
            break;
          }
		      loopIndex++;
        }

        // This assert will trigger if the control type from the file didn't match any known type
        assert(loopIndex < predefinedStrings::componentControlTypeCodes::ControlNullLast);

		    variantManager->setValue(subproperties.value(predefinedStrings::componentControlTypeFieldName), controlCode);
      }
  }

  if (subproperties.contains(predefinedStrings::nodeControlTypeFieldName))
  {
    QtVariantPropertyManager* variantManager = qobject_cast<QtVariantPropertyManager*>(subproperties.value(predefinedStrings::nodeControlTypeFieldName)->propertyManager());

      if(variantManager)
      {
        int controlCode;
        std::string controlTypeStringFromFile = props.at(predefinedStrings::nodeControlTypeFieldName);
        int loopIndex = 0;
        // Loop the possible control types, looking for the index ("controlCode") of the control type string that was read from the xml:
		while (predefinedStrings::nodeControlTypes[loopIndex] != NULL) // while we've not reached the end of our null-terminated char* array:
        {
			if (strcmp(controlTypeStringFromFile.c_str(), predefinedStrings::nodeControlTypes[loopIndex]) == 0)
          {
            controlCode = loopIndex;
            break;
          }
          loopIndex++;
        }

        // This assert will trigger if the control type from the file didn't match any known type
        assert(loopIndex < predefinedStrings::nodeControlTypeCodes::ControlNullLast);

        variantManager->setValue(subproperties.value(predefinedStrings::nodeControlTypeFieldName), controlCode);
      }
  }

  if(subproperties.contains("lp:name")) {
    QtStringPropertyManager* spm =
      qobject_cast<QtStringPropertyManager*>(
        subproperties.value("lp:name")->propertyManager());
      if(spm)
        spm->setValue(
            subproperties.value("lp:name"),
            QString::fromStdString(props["lp:name"])
          );
  }

  if(subproperties.contains("lp:value")) {
    QtStringPropertyManager* spm =
      qobject_cast<QtStringPropertyManager*>(
        subproperties.value("lp:value")->propertyManager());
      if(spm)
        spm->setValue(
            subproperties.value("lp:value"),
            QString::number(
              QString::fromStdString(props["lp:value"]).toDouble())
          );
  }

  if(subproperties.contains("ls:name")) {
    QtStringPropertyManager* spm =
      qobject_cast<QtStringPropertyManager*>(
        subproperties.value("ls:name")->propertyManager());
      if(spm)
        spm->setValue(
            subproperties.value("ls:name"),
            QString::fromStdString(props["ls:name"])
          );
  }

  if(subproperties.contains("ls:value")) {
    QtStringPropertyManager* spm =
      qobject_cast<QtStringPropertyManager*>(
        subproperties.value("ls:value")->propertyManager());
      if(spm)
        spm->setValue(
            subproperties.value("ls:value"),
            QString::number(
              QString::fromStdString(props["ls:value"]).toDouble())
          );
  }
}


}
