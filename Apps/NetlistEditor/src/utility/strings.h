#ifndef STRINGS_H
#define STRINGS_H

#include <map>
#include <utility>

namespace predefinedStrings
{
	// Predefined strings
	static const char* const genericComponentParameterName = "Parameter Value";
	static const char* const initialVolumeParameterName = "Initial Volume";
	static const char* const initialVolumeXmlFieldName = "initialVolume";
	static const char* const prescribedPressureParameterValueName = "Prescribed Pressure";
	// const char* const prescribedPressureNodesIndividualName = "prescribedPressureNode";
	static const char* const prescribedPressureNodesCollectiveName = "prescribedPressureNodes";
	static const char* const additionalDataFileFieldName = "Additional Input Data Filename";
	static const char* const pythonScriptnamePlaceholder = "yourPythonScriptNameHere.py";
	
	static const char* const timeFlowDataFilePlaceholder = "yourPeriodicTime-FlowFileHere.dat";
	static const char* const componentControlTypeFieldName = "Component Control Type";
	static const char* const noControlString = "No Dynamic Control";
	static const char* const customPythonScriptString = "Custom Python Script";
	static const char* const lvElastanceControllerString = "Built-in LV Elastance Controller";	
	static const char* const periodicPrescribedFlowString = "Periodic Prescribed Flow Function";
	static const char* componentControlTypes[] = {noControlString, customPythonScriptString, periodicPrescribedFlowString, lvElastanceControllerString, NULL};
	namespace componentControlTypeCodes
	{
		enum componentControlTypeCodes{ ControlNone = 0, ControlCustomPython, ControlPeriodicPrescribedFlow, ControlLVElastance, ControlNullLast };
	}
	static const char* const pythonControlNamestringForFlowsolver = "customPython";
	static const char* const prescribedPeriodicFlow = "prescribedPeriodicFlow";

	static const char* const timePressureDataFilePlaceholder = "yourPeriodicTime-PressureFileHere.dat";
	static const char* const nodeControlTypeFieldName = "Node Control Type";
	static const char* const periodicPrescribedPressureString = "Periodic Prescribed Pressure Function";
	static const char* nodeControlTypes[] = {noControlString, customPythonScriptString, periodicPrescribedPressureString, NULL};
	namespace nodeControlTypeCodes
	{
		enum nodeControlTypeCodes{ ControlNone = 0, ControlCustomPython, ControlPeriodicPrescribedPressure, ControlNullLast };
	}
	static const char* const prescribedPeriodicPressure = "prescribedPeriodicPressure";

	// const char* const controlTypeXmlName = "controlType";
	// const char* const pythonScriptXmlName = "pythonScriptName";

	namespace controlScriptTypes
	{
		enum controlScriptTypes{ ControlScript_None = 0, ControlScript_StandardPython, ControlScript_DatFileData, ControlScript_NullLast};
	}
}

#endif