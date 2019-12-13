
#pragma once

#include <mitkBaseData.h>
#include <mitkImage.h>
#include <mitkSurface.h>

#include <MeshData.h>

#include <boost/multi_array.hpp>
#include <boost/serialization/vector.hpp>

#include "IPCMRIKernel.h"
#include "IMeshingKernel.h"

#include "PCMRIKernelExports.h"

#include <boost/serialization/version.hpp>

namespace boost {
namespace serialization  {
class access;
}
}

namespace crimson {

	//pass parameters from MappingTask to enable time interpolation
	struct TimeInterpolationParameters
	{
		int _cardiacFrequency;
		mitk::Vector3D _normal;
		int _nOriginal; //number of time samples in the original PCMRI image
		int _nControlPoints;
	};

	//pass parameters from MappingTask to enable time visualization of the frame with maximum flow
	struct VisualizationParameters
	{
		int _maxIndex; //index of slice with maximum flow - use for visualization
		double _scaleFactor; //used for visualization
	};

/*!
 * \brief   PCMRIData represents a patient-specific velocity inflow data class.
 */
class PCMRIKernel_EXPORT PCMRIData : public mitk::BaseData
{
public:
    mitkClassMacro(PCMRIData, BaseData);
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);
    mitkCloneMacro(Self);

public:
	
    ////////////////////////////////////////////////////////////
    // mitk::BaseData interface implementation
    ////////////////////////////////////////////////////////////
    void UpdateOutputInformation() override;
    void SetRequestedRegionToLargestPossibleRegion() override {}
    bool RequestedRegionIsOutsideOfTheBufferedRegion() override { return false; }
    bool VerifyRequestedRegion() override { return true; }
    void SetRequestedRegion(const itk::DataObject *) override {}
    void PrintSelf(std::ostream& os, itk::Indent indent) const override;

	///@{ 
	/*!
	* \brief   Sets the MeshData object. 
	*/
	void setMesh(MeshData::Pointer mesh){ _mesh = mesh.GetPointer(); }

	/*!
	* \brief   Gets the MeshData object.
	*/
	MeshData::Pointer getMesh(){ return _mesh.GetPointer(); }

	/*!
	* \brief   Sets the UID of the node containing the MeshData object.
	*/
	void setMeshNodeUID(std::string meshUID){ _meshNodeUID = meshUID; }

	/*!
	* \brief   Gets the UID of the node containing the MeshData object.
	*/
	std::string getMeshNodeUID(){ return _meshNodeUID; }

	/*!
	* \brief   Sets the mapped PCMRI values for all face mesh points and timepoints.
	*/
	void setMappedPCMRIvalues(boost::multi_array<double, 2>  mappedPCMRIvalues);

	/*!
	* \brief   Gets the mapped PCMRI vectors for all face mesh points and timepoints.
	*/
	boost::multi_array<double, 2> getMappedPCMRIvalues() const { return _mappedPCMRIvalues; }

	/*!
	* \brief   Sets the FaceIdentifierIndex of the solid model face unto which the velocity profile is mapped.
	*/
	void setFaceIdentifierIndex(int index) const { _faceIdentifierIndex = index; };

	/*!
	* \brief   Sets the fixed parameters needed for time interpolation
	*/
	void setTimeInterpolationParameters(int cardiacFrequency, mitk::Vector3D normal, int nOriginal);

	/*!
	* \brief   Gets the fixed parameters needed for time interpolation
	*/
	TimeInterpolationParameters getTimeInterpolationParameters() { return _parameters; };

	/*!
	* \brief   Sets the fixed parameters needed for visualization
	*/
	void setVisualizationParameters(int maxIndex, double scaleFactor);

	/*!
	* \brief   Gets the fixed parameters needed for visualization
	*/
	VisualizationParameters getVisualizationParameters() { return _parametersVis; };




	/*!
	* \brief   Gets the mesh coordinates for the selected mesh point and timepoint.
	*/
	mitk::Vector3D getSingleMappedPCMRIvector(int pointIndex, int timepointIndex) const { return _mappedPCMRIvectorsInterpolated[pointIndex][timepointIndex]; }

	/*!
	* \brief   Gets the timepoints after time interpolation.
	*/
	std::vector<double>  getTimepoints() const { return _timepointsInterp; }

	/*!
	* \brief   Gets the flow waveform.
	*/
	std::vector<double>  getFlowWaveform() const { return _flowWaveform; }




	/*!
	* \brief   Gets the FaceIdentifier of the solid model face unto which the velocity profile is mapped.
	*/
	FaceIdentifier getFace() const;

	///*!
	//* \brief   Sets the warped inflow face representation as an mitk::Surface (primarily for rendering)
	//*/
	//const mitk::Surface::Pointer getMeshSurfaceRepresentation() const {return _mesh->getSurfaceRepresentationForFace(getFace()); };

	/*!
	* \brief   Gets the warped inflow face representation as an mitk::Surface (primarily for rendering)
	*/
	mitk::Surface::Pointer getSurfaceRepresentation(bool regenerate = false) const;

	

	void setTimeStepSize(double timeStepSize){ _timeStepSize = timeStepSize; };

	void setNControlPoints(double nControlPoints){ _parameters._nControlPoints = nControlPoints; };

	int getNControlPoints(){return _parameters._nControlPoints; };

	void timeInterpolate();

	void flipFlow();

	///@} 

protected:
    PCMRIData();
    virtual ~PCMRIData();

    PCMRIData(const Self& other);

	mutable mitk::Surface::Pointer _surfaceRepresentation = nullptr;

private:
	friend class IPCMRIKernel;
	friend class PCMRIDataIO;

	std::vector<mitk::Point3D> getMeshFaceCoordinates() const;
	void generateMappedVectors();

	//results of MappingTask
	mutable boost::multi_array<double, 2> _mappedPCMRIvalues; //just mapped, not interpolated - needed for time interpolation

	//set through IPCMRIKernel
	std::string _meshNodeUID;
	mutable int _faceIdentifierIndex;
	//set at the end of mapping task
	TimeInterpolationParameters _parameters; //time interpolation settings 
	VisualizationParameters _parametersVis; //visualization settings 

	 template<class Archive>
     friend void serialize(Archive & ar, crimson::PCMRIData& data, const unsigned int version);
     friend class boost::serialization::access;

	MeshData::Pointer _mesh;

	//generated based on mapped PCMRI values
	mutable boost::multi_array<mitk::Vector3D, 2> _mappedPCMRIvectors; //just mapped, not interpolated

	//set each time a solver setup is written
	double _timeStepSize; 

	//set through timeInterpolate - called each time a solver setup is written
	boost::multi_array<mitk::Vector3D, 2> _mappedPCMRIvectorsInterpolated; //results of the mapping (mitk::3Dpoint defining the vector of flow);
	//first std::vector contains mesh nodes, second std::vector contains timepoints per mesh node
	boost::multi_array<double, 2> _mappedPCMRIvaluesInterpolated;
	std::vector<double> _timepointsInterp; //timepoints after interpolation - needed for bct.dat - changes based on num selected in solver setup
	std::vector<double> _flowWaveform;
	
};

} // namespace crimson

BOOST_CLASS_VERSION(crimson::PCMRIData, 0)