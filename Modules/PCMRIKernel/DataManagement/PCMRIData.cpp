#include "PCMRIData.h"

#include <vtkIdList.h>
#include <vtkPolyData.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkNew.h>
#include <vtkShortArray.h>
#include <vtkCellData.h>
#include <vtkMassProperties.h>
#include <vtkTriangleFilter.h>

namespace crimson
{

PCMRIData::PCMRIData()
{
    PCMRIData::InitializeTimeGeometry(1);
	_timeStepSize = 0;
	_parameters._nControlPoints = 15;
}

PCMRIData::PCMRIData(const Self& other)
	: mitk::BaseData(other)
	, _parameters(other._parameters)
	, _timeStepSize(other._timeStepSize)
	, _timepointsInterp(other._timepointsInterp)
	, _flowWaveform(other._flowWaveform)
{
	PCMRIData::InitializeTimeGeometry(1);

	if (!other._mappedPCMRIvalues.empty())
		this->setMappedPCMRIvalues(other._mappedPCMRIvalues);
}

PCMRIData::~PCMRIData() {}

void PCMRIData::UpdateOutputInformation()
{
    if (this->GetSource()) {
        this->GetSource()->UpdateOutputInformation();
    }
}

void PCMRIData::PrintSelf(std::ostream& os, itk::Indent indent) const
{
}

void PCMRIData::setMappedPCMRIvalues(boost::multi_array<double, 2>  mappedPCMRIvalues)
{
	auto nPoints = mappedPCMRIvalues.shape()[0];
	auto nTimepoints = mappedPCMRIvalues.shape()[1];
	_mappedPCMRIvalues.resize(boost::extents[nPoints][nTimepoints]);
	_mappedPCMRIvalues = mappedPCMRIvalues;

	//produce mapped vectors
	generateMappedVectors();
}

std::vector<mitk::Point3D> PCMRIData::getMeshFaceCoordinates() const
{
	std::vector<int> faceNodes = _mesh->getNodeIdsForFace(getFace());

	std::vector<mitk::Point3D> coordinatesMesh;
	for (std::vector<int>::iterator it = faceNodes.begin(); it < faceNodes.end(); it++) {
		coordinatesMesh.push_back(_mesh->getNodeCoordinates(*it));
	}

	return coordinatesMesh;
};

FaceIdentifier PCMRIData::getFace() const
{
	FaceIdentifier faceId = _mesh->getFaceIdentifierMap().getFaceIdentifier(_faceIdentifierIndex);
	return faceId;
}

void PCMRIData::setTimeInterpolationParameters(int cardiacFrequency, mitk::Vector3D normal, int nOriginal)
{
	_parameters._cardiacFrequency = cardiacFrequency;
	_parameters._normal = normal;
	_parameters._nOriginal = nOriginal;

}

void PCMRIData::setVisualizationParameters(int maxIndex, double scaleFactor)
{
	_parametersVis._maxIndex = maxIndex;
	_parametersVis._scaleFactor = scaleFactor;

}

void PCMRIData::timeInterpolate()
{

	bool debug = false;
	bool parallel_on = false;
	int threads_to_use = 2;

	//PCMRI: temporal smoothing


	///****************************************************************************
	///*                 INITIALISE GRID                                           *
	///****************************************************************************


	/// Create the grid
	/// The original grid spacing is such that there will be 4 grid points within the domain.
	/// This, for the experiments, seemend like a reasonable starting point. Along the time direction,
	/// we will start by 4 points and, for the time being stay there



	/// Algorithmic parameters
	double cycleDuration = 60 / (double)_parameters._cardiacFrequency;
	double timeStepSize = _timeStepSize; //in seconds, taken from the solver settings (aka model resolution)
	int NstepsSolver = 600; //number of time steps in total taken from the solver settings
	//int N2 = round(((cycleDuration + (timeStepSize*0.5)) / timeStepSize)); /// Number of samples after interpolation. 
	//MAY NOT BE CORRECT BECAUSE WE ACTUALLY NEED TO CREATE A LINSPACE AND THERE MIGHT BE AN EXTRA SAMPLE!!!
	unsigned int bsd = 3; // b-spline degree, by default is cubic
	const unsigned ndmis = 1;
	double nControlPoints = 15; //default value in Alberto's matlab code
	double frameRate = _parameters._nOriginal / cycleDuration; //taken from Alberto's code

	/****************************************************************************
	*                               Create input and output data                           *
	****************************************************************************/

	//original time resolution of the PCMRI image - calculate based on frame rate and trigger delay
	std::vector<double> t(_parameters._nOriginal);
	for (int i = 0; i < _parameters._nOriginal; i++)
	{
		t[i] = i / frameRate;
	}

	// copy input into the appropriate vectors
	//original time resolution of the PCMRI image
	std::vector< IPCMRIKernel::BsplineGrid1DType::PointType > coordinates; // in this case, 1D
	for (int i = 0; i < _parameters._nOriginal; i++){
		coordinates.push_back(t[i]);
	}

	//timepoints after interpolation
	std::vector< IPCMRIKernel::BsplineGrid1DType::PointType > interpolated_t;
	for (double j = 0; j <= (cycleDuration + timeStepSize / 2); j = j + timeStepSize){
		interpolated_t.push_back(j);
	}

	std::vector<mitk::Point3D> coordinatesMesh = getMeshFaceCoordinates();

	//output arrays
	_mappedPCMRIvaluesInterpolated.resize(boost::extents[coordinatesMesh.size()][interpolated_t.size()]);
	_mappedPCMRIvectorsInterpolated.resize(boost::extents[coordinatesMesh.size()][interpolated_t.size()]);

	//do for all points
	for (int i = 0; i < coordinatesMesh.size(); i++)
	{

		std::vector<IPCMRIKernel::BsplineGrid1DType::CoefficientType> values; // in this case, velocity
		for (int j = 0; j < _parameters._nOriginal; j++){
			values.push_back(_mappedPCMRIvalues[i][j]);
		}

		///****************************************************************************
		///                    Initialise the B-spline grid
		///****************************************************************************
		double bounds[2 * ndmis]; // bounds of the domain, time in this case
		bounds[0] = t[0];
		bounds[1] = t[_parameters._nOriginal - 1];
		IPCMRIKernel::BsplineGrid1DType::PointType grid_spacing;
		double bspline_spacing_1D = (bounds[1] - bounds[0]) / (nControlPoints - 1);

		grid_spacing[0] = bspline_spacing_1D; // for example!

		IPCMRIKernel::BsplineGrid1DType::Pointer control_points(new IPCMRIKernel::BsplineGrid1DType(bounds, bsd, grid_spacing, 0)); /// the control point grid does not need border. This grid covers the whole ROI and there is no control_points division.
		IPCMRIKernel::BsplineGrid1DType::IndexType cyclicDimensions;
		cyclicDimensions[0] = 1; // first dimension is cyclic
		control_points->SetCyclicDimensions(cyclicDimensions);
		control_points->SetCyclicBounds(0, t[0], cycleDuration);
		control_points->SetDebug(debug);
		if (debug) std::cout << "\t\tUsing " << threads_to_use << " threads." << std::endl;
		control_points->SetParallel(parallel_on, threads_to_use);
		control_points->UpdateCyclicBehaviour();

		double lambda = 0; // no regularization



		fit_cyclicBspline1D<IPCMRIKernel::BsplineGrid1DType>(coordinates, values, lambda, control_points);

		/// interpolate ---------------------------------------

		std::vector<IPCMRIKernel::BsplineGrid1DType::CoefficientType>  interpolated_values;
		interpolated_values = control_points->evaluate(interpolated_t);

		//transform mapped PCMRI values relative to the plane of the model face
		std::vector<mitk::Point3D> mappedPcmriValues3D;
		std::vector<mitk::Vector3D> mappedPcmriVectors3D;
		//std::vector<mitk::Point3D> mappedPcmriValues3D2;


		for (int j = 0; j < interpolated_t.size(); j++){
			_mappedPCMRIvaluesInterpolated[i][j] = interpolated_values[j][0];
			mitk::Vector3D mappedPcmriVector;
			mappedPcmriVector[0] = _parameters._normal[0] * _mappedPCMRIvaluesInterpolated[i][j];
			mappedPcmriVector[1] = _parameters._normal[1] * _mappedPCMRIvaluesInterpolated[i][j];
			mappedPcmriVector[2] = _parameters._normal[2] * _mappedPCMRIvaluesInterpolated[i][j];
			_mappedPCMRIvectorsInterpolated[i][j] = mappedPcmriVector;

		}
		
		// ----------------------- print to file ---------------------

		///---------------------------------------------------------------------------//
		///                       PRINT RESULTS TO FILE                               //
		///---------------------------------------------------------------------------//

	}


	//{
	//	std::ofstream file;
	//	std::string filename = "./output/original.txt";
	//	file.open(filename, ofstream::out);
	//	for (int i = 0; i < coordinatesMesh.size(); i++){
	//		file << coordinatesMesh[i][0] << " " << coordinatesMesh[i][1] << std::endl;
	//		for (int j = 0; j < _parameters._nOriginal; j++){

	//			file << " " << _mappedPCMRIvalues[i][j] << "  " << coordinates[j] << std::endl;
	//		}
	//	}
	//	file.close();
	//	std::cout << "output in " << filename << std::endl;
	//}

	//{
	//	std::ofstream file;
	//	std::string filename = "./output/interpolated.txt";
	//	file.open(filename, ofstream::out);
	//	for (int i = 0; i < coordinatesMesh.size(); i++){
	//		file << coordinatesMesh[i][0] << " " << coordinatesMesh[i][1] << std::endl;
	//		for (int j = 0; j < interpolated_t.size(); j++){

	//			file << " " << _mappedPCMRIvaluesInterpolated[i][j] << "  " << interpolated_t[j] << std::endl;
	//		}
	//	}
	//	file.close();
	//	std::cout << "output in " << filename << std::endl;
	//}

	std::vector<double> timepointsInterp;
	std::transform(interpolated_t.begin(), interpolated_t.end(),
		std::back_inserter(timepointsInterp),
		IPCMRIKernel::ToMitkType());

	_timepointsInterp = timepointsInterp;

	//set flowWaveform
	_flowWaveform = IPCMRIKernel::calculateFlowWaveform(_mappedPCMRIvaluesInterpolated, _mesh, getFace());

}

void PCMRIData::flipFlow()
{
#ifdef _DEBUG
	MITK_INFO << _mappedPCMRIvalues.shape()[0] << "  " << _mappedPCMRIvalues.shape()[1];
#endif

	//generate new mapped values, vectors and surface representation
	for (int i = 0; i < _mappedPCMRIvalues.shape()[0]; i++)
	{
		for (int j = 0; j < _mappedPCMRIvalues.shape()[1]; j++)
		{
			_mappedPCMRIvalues[i][j] = -_mappedPCMRIvalues[i][j];
		}
	}
	generateMappedVectors();

	getSurfaceRepresentation(true);
}

mitk::Surface::Pointer PCMRIData::getSurfaceRepresentation(bool regenerate) const
{
	if ((!_surfaceRepresentation && !_mappedPCMRIvectors.empty()) || regenerate) {

		// scale the values of the flow velocity! 
		std::vector<mitk::Vector3D> timePointPCMRIVectors;

		for (int j = 0; j < _mappedPCMRIvalues.shape()[0]; j++)
		{
			mitk::Vector3D mappedPcmriVector;
			mappedPcmriVector[0] = _parameters._normal[0] * _mappedPCMRIvalues[j][_parametersVis._maxIndex] / _parametersVis._scaleFactor;
			mappedPcmriVector[1] = _parameters._normal[1] * _mappedPCMRIvalues[j][_parametersVis._maxIndex] / _parametersVis._scaleFactor;
			mappedPcmriVector[2] = _parameters._normal[2] * _mappedPCMRIvalues[j][_parametersVis._maxIndex] / _parametersVis._scaleFactor;
			timePointPCMRIVectors.push_back(mappedPcmriVector);
		}
		
		_surfaceRepresentation = IPCMRIKernel::generateSurfaceRepresentation(timePointPCMRIVectors, _mesh->getSurfaceRepresentationForFace(getFace()));

	}

	return _surfaceRepresentation;
	

}

void PCMRIData::generateMappedVectors()
{
	_mappedPCMRIvectors.resize(boost::extents[_mappedPCMRIvalues.shape()[0]][_mappedPCMRIvalues.shape()[1]]);
	for (int i = 0; i < _mappedPCMRIvalues.shape()[0]; i++)
	{
		for (int j = 0; j < _mappedPCMRIvalues.shape()[1]; j++)
		{
			mitk::Vector3D pcmriPoint;
			mitk::Vector3D mappedPcmriVector;
			pcmriPoint[0] = 0;
			pcmriPoint[1] = 0;
			pcmriPoint[2] = _mappedPCMRIvalues[i][j];

			mappedPcmriVector[0] = _parameters._normal[0] * _mappedPCMRIvalues[i][j];
			mappedPcmriVector[1] = _parameters._normal[1] * _mappedPCMRIvalues[i][j];
			mappedPcmriVector[2] = _parameters._normal[2] * _mappedPCMRIvalues[i][j];
			_mappedPCMRIvectors[i][j] = mappedPcmriVector;
		}
	}
}


} // namespace crimson
