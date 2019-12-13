#ifndef PRESCRIBEDFLOWCOMPONENTMETADATA_H_
#define PRESCRIBEDFLOWCOMPONENTMETADATA_H_
enum PrescribedFlowComponentType {PrescribedFlow_None = 0, PrescribedFlow_ThreeDInterface, PrescribedFlow_Fixed, PrescribedFlow_NullLast};
namespace qsapecng {
	const char* getPrescribedFlowCodeFromType(const PrescribedFlowComponentType prescribedFlowType);

	class PrescribedFlowComponentMetadata {
	public:
		PrescribedFlowComponentMetadata(const int componentIndex, const PrescribedFlowComponentType flowType, const double prescribedFlowValue)
		: componentIndex_(componentIndex),
		prescribedFlowType_(flowType),
		prescribedFlowValue_(prescribedFlowValue)
		{}

		PrescribedFlowComponentMetadata(){}

		int getComponentIndex() const
		{
			return componentIndex_;
		}

		const char* getPrescribedFlowTypeCharCode() const
		{
			return getPrescribedFlowCodeFromType(prescribedFlowType_);
		}

		double getPrescribedFlowValue() const
		{
			return prescribedFlowValue_;
		}
	
	private:
		int componentIndex_;
		PrescribedFlowComponentType prescribedFlowType_;
		double prescribedFlowValue_;
	};
}
#endif