#ifndef BOILERPLATESCRIPTS_H
#define BOILERPLATESCRIPTS_H

#include <string>

const char* genericPythonScript_char = 
"from CRIMSONPython import *\n"
"from math import *\n"
"\n"
"class parameterController(abstractParameterController):\n"
"\n"
	"\tdef __init__(self, baseNameOfThisScriptAndOfRelatedFlowOrPressureDatFile, MPIRank):\n"
		"\t\tabstractParameterController.__init__(self,baseNameOfThisScriptAndOfRelatedFlowOrPressureDatFile, MPIRank)\n"
		"\t\tself.periodicTime = 0.0; \n"
		"\t\tself.heartPeriod = 0.86;\n"
		"\t\tself.time = 0.0"
"\n"
"\n"
	"\tdef updateControl(self, currentParameterValue, delt, dictionaryOfPressuresByComponentIndex, dictionaryOfFlowsByComponentIndex, dictionaryOfVolumesByComponentIndex):\n"
"\n"
		"\t\tself.updatePeriodicTime(delt)\n"
		"\t\tself.time = self.time + delt\n"
		"\t\tnewValueOfTargetParameter = abs(0.001 * cos(pi*100*self.periodicTime))\n"
"\n"
		"\t\t# for key in dictionaryOfPressuresByComponentIndex:\n"
		"\t\t# 	print \"Pressure \", key, \" was \", dictionaryOfPressuresByComponentIndex[key]\n"
		"\t\t# for key in dictionaryOfFlowsByComponentIndex:\n"
		"\t\t# 	print \"Flow \", key, \" was \", dictionaryOfFlowsByComponentIndex[key]\n"
"\n"
		"\t\treturn newValueOfTargetParameter\n"
"\n"
	"\tdef updatePeriodicTime(self, delt):\n"
	"\n"
		"\t\tself.periodicTime = self.periodicTime + delt\n"
		"\t\t# Keep periodicTime in the range [0,heartPeriod):\n"
		"\t\tif self.periodicTime >= self.heartPeriod:\n"
			"\t\t\tself.periodicTime = self.periodicTime - self.heartPeriod\n";

const std::string genericPythonScript(genericPythonScript_char);

const char* pythonDatFileFlowPrescriber_char = 
"from CRIMSONPython import *\n"
"from math import pi, cos\n"
"import numpy\n"
"import scipy.interpolate\n"
"\n"
"class parameterController(abstractParameterController):\n"
"\n"
	"\tdef __init__(self, baseNameOfThisScriptAndOfRelatedFlowOrPressureDatFile, MPIRank):\n"
		"\t\tabstractParameterController.__init__(self,baseNameOfThisScriptAndOfRelatedFlowOrPressureDatFile, MPIRank)\n"
		"\t\tself.periodicTime = 0.0;\n"
		"\t\tself.nameOfThisScript = baseNameOfThisScriptAndOfRelatedFlowOrPressureDatFile\n"
		"\t\tself.getPeriodicFlowPrescriberData()\n"
		"\t\tself.finishSetup()\n"
"\n"
	"\tdef updateControl(self, currentParameterValue, delt, dictionaryOfPressuresByComponentIndex, dictionaryOfFlowsByComponentIndex, dictionaryOfVolumesByComponentIndex):\n"
"\n"
		"\t\tself.updatePeriodicTime(delt)	\n"
		"\t\tprescribedFlow = self.flowFunction(self.periodicTime)\n"
"\n"
		"\t\treturn prescribedFlow.astype(float)\n"
"\n"
	"\tdef updatePeriodicTime(self, delt):\n"
"\n"
		"\t\tself.periodicTime = self.periodicTime + delt\n"
		"\t\t# Keep periodicTime in the range [0,self.endTime):\n"
		"\t\tif self.periodicTime >= self.endTime:\n"
			"\t\t\tself.periodicTime = self.periodicTime - self.endTime\n"
"\n"
	"\tdef getPeriodicFlowPrescriberData(self):\n"
		"\t\t# Load a flow file which has the same name as this controller script, but with extension \".dat\".\n"
		"\t\t# It should contain two columns in plain text: the first gives the time,\n"
		"\t\t# and the second gives the flow to prescribe at that time.\n"
		"\t\t#\n"
		"\t\t# The time should start at zero.\n"
		"\t\t#\n"
		"\t\t# This script will loop the flow data when it reaches the end of the time.\n"
		"\t\tself.flowFileData = numpy.loadtxt(self.nameOfThisScript+'.dat')\n"
		"\t\tself.flowFunction = scipy.interpolate.interp1d(self.flowFileData[:,0],self.flowFileData[:,1])\n"
		"\t\tself.endTime = self.flowFileData[-1,0].astype(float)\n";

const std::string pythonDatFileFlowPrescriber(pythonDatFileFlowPrescriber_char);

const char* exampleFlowOrPressureFile_char =
"0.0 0.0\n"
"0.1 1000.0\n"
"0.2 1100.0\n"
"0.3 1200.0\n"
"0.4 1400.0\n"
"0.5 1600.0\n"
"0.6 1200.0\n"
"0.7 700.0\n"
"0.8 300.0\n";

const std::string exampleFlowOrPressureFile(exampleFlowOrPressureFile_char);

#endif