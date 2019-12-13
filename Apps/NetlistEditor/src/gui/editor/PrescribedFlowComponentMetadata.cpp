#include "PrescribedFlowComponentMetadata.h"
namespace qsapecng {
	const char* getPrescribedFlowCodeFromType(const PrescribedFlowComponentType prescribedFlowType)
	{
		const char* prescribedFlowCharTypeCode;
		switch (prescribedFlowType)
		{
		case PrescribedFlow_None:
		{
			prescribedFlowCharTypeCode = "0";
			break;
		}
		case PrescribedFlow_ThreeDInterface:
		{
			prescribedFlowCharTypeCode = "t";
			break;
		}
		case PrescribedFlow_Fixed:
		{
			prescribedFlowCharTypeCode = "f";
			break;
		}
		}
		return prescribedFlowCharTypeCode;
	}
}