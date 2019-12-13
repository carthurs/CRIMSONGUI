// This will be an interface for solid model service and blueberry extension point etc.

#pragma once

#include <vector>
#include <functional>
#include <mitkBaseData.h>
#include <mitkPlanarFigure.h>

#include "PCMRIKernelExports.h"

#include <PCMRIData.h>
#include <AsyncTaskWithResult.h>

#include <ImmutableRanges.h>

/// Internal includes
#include "BsplineGrid.hxx"
#include "LinearSolvers.hxx"

/// Internal includes from namespace echo
#include "Image_tools.hxx"
#include "Mesh_tools.hxx"
#include "Point_tools.hxx"
#include "String_tools.hxx"


typedef itk::Image<float, 4>    WholeImageTypeFloat;

namespace crimson
{


class PCMRIKernel_EXPORT IPCMRIKernel
{
public:

	// Define types

	typedef vnl_vector_fixed< float, 3 >  PhaseEncodingVelocityType;

	typedef unsigned short BModePixelType;
	typedef itk::Image<BModePixelType, 2> BModeImageType;
	typedef float DopplerPixelType;
	typedef itk::Image<DopplerPixelType, 2> DopplerImageType;

	typedef itk::Image<itk::Vector<DopplerPixelType, 1>, 2> VectorImageType;

	typedef BsplineGrid<float, 2, 1> BsplineGridType;
	typedef BsplineGrid<float, 1, 1> BsplineGrid1DType;

	typedef BsplineGridType::PointType PointType;
	typedef BsplineGridType::CoefficientType VectorType;
	std::vector <PointType > beam_sources;

	typedef echo::Point<BsplineGridType::MatrixElementType, 1> ValueType;

	typedef std::vector<mitk::PlanarFigure::Pointer> ContourSet;

	struct ToBSGridType : public std::unary_function<mitk::Point2D, IPCMRIKernel::PointType>
	{
		IPCMRIKernel::PointType operator()(mitk::Point2D original) const
		{
			IPCMRIKernel::PointType pointBS = IPCMRIKernel::BsplineGridType::PointType(original.GetDataPointer());
			return pointBS;
		}
	};

	struct ToMitkType : public std::unary_function<IPCMRIKernel::BsplineGrid1DType::PointType, double>
	{
		double operator()(IPCMRIKernel::BsplineGrid1DType::PointType original) const
		{
			double pointBS = double(original.at(0));
			return pointBS;
		}
	};
	/*!
	* \brief   Create an asynchronous task that computes a mapped PCMRI-based velocity profile.
	*
	* \param   contoursPCMRI              Contours that user marked in the PCMRI image.
	* \param   contourModel				  Contour of the selected face of the model.
	* \param   coordinatesOut			  Coordinates of the mesh elements on the selected model face.

	*/
	/*static std::shared_ptr<async::TaskWithResult<mitk::BaseData::Pointer>>
		createMapTask(const ContourSet& contoursPCMRI, const mitk::PlanarFigure::Pointer& contourModel, 
		const std::vector<mitk::Point2D>& coordinatesOut, const std::vector<mitk::Point3D>& coordinatesMesh,
		const mitk::Surface::Pointer meshSurfaceRepresentation,	mitk::Point2D pcmriLandmark, 
		mitk::Point2D mraLandmark, mitk::Image::Pointer pcmriImage);*/
	static std::shared_ptr<async::TaskWithResult<mitk::BaseData::Pointer>>
		createMapTask(const ContourSet& contoursPCMRI, const mitk::PlanarFigure::Pointer& contourModel,
		const MeshData::Pointer mesh, FaceIdentifier face, mitk::Point2D pcmriLandmark,
		mitk::Point2D mraLandmark, WholeImageTypeFloat::Pointer pcmriImage,
		bool imageFlipped, std::string meshNodeUID, int cardiacFrequency, int startIndex);

	static std::vector<double> calculateFlowWaveform(boost::multi_array<double, 2> mappedPCMRIvalues,
		const MeshData::Pointer mesh, const FaceIdentifier face);

	static mitk::Surface::Pointer generateSurfaceRepresentation(std::vector<mitk::Vector3D> timePointPCMRIVectorsInterpolated,
		mitk::Surface::Pointer meshSurfaceRepresentation);

};

template <typename TBsplineGridType>
void fit_cyclicBspline1D(const std::vector<typename TBsplineGridType::PointType > &t, const std::vector< typename TBsplineGridType::CoefficientType > &v,
	double lambda, typename TBsplineGridType::Pointer control_points){

	bool debug = false;
	bool parallel_on = false;
	int threads_to_use = 2;

	/****************************************************************************
	*                               CALCULATE EXTENT                            *
	****************************************************************************/

	Eigen::setNbThreads(threads_to_use);
	Eigen::initParallel();



	///---------------------------------------------------------------------------//
	///                       FIND BSPLINE COEFFICIENTS                              //
	///---------------------------------------------------------------------------//

	echo::Point<double, 2 * TBsplineGridType::IDims> control_points_bounds;
	control_points->GetBorderBoundsOfInfluence(&control_points_bounds[0]);

	typename TBsplineGridType::DenseVectorType values_vector(v.size(), 1);
	std::vector<IPCMRIKernel::ValueType>::const_iterator const_iter;
	typename TBsplineGridType::MatrixElementType *eigen_iter;

	for (const_iter = v.begin(), eigen_iter = values_vector.data(); const_iter != v.end(); ++const_iter)
	{
		for (int nd = 0; nd < TBsplineGridType::IDims; nd++){
			(*eigen_iter) = (*const_iter)[nd];
			++eigen_iter;
		}
	}

	typename TBsplineGridType::SparseMatrixType A;
	typename TBsplineGridType::DenseVectorType b;
	std::vector<unsigned int> kept_nodes;
	std::vector<unsigned int> corresponding_nodes;

	/// Keep environment to save memory

	typename TBsplineGridType::SparseMatrixType B = control_points->createSamplingMatrix(t, 0.0, kept_nodes, corresponding_nodes);

	{ /// Keep scope to save memory
		Eigen::setNbThreads(8);
		A = B.transpose() *B;
		b = B.transpose()*values_vector;
	}


	// Regularisation
	if (lambda)
	{
		if (debug) std::cout << "\tUse regularisation with lambda =" << lambda << std::endl;
		IPCMRIKernel::BsplineGridType::SparseMatrixType Adiv = control_points->createContinuousDivergenceSamplingMatrix(kept_nodes, corresponding_nodes);
		A = A + lambda / (1 - lambda)*Adiv;/// See notes on p118 of my thesis for an explanation
	}


	/// Available additional solvers
	/// BiCGSTAB + IncompletLUT preconditioner
	/// SparseQR
	/// SPQR
	/// PastixLU

	typename TBsplineGridType::DenseVectorType x;//,x2,x3,x4;
	echo::solvers::ConfigurationParameter config;
	config.max_iterations = 1000; /// This number might look huge but we need to get to the desired tolerance
	config.tol = 1E-06;
	config.verbose = true;
	std::string str_solver;

	/// Add some condition like if the matrix is larger than e.g. 5000 x 5000 then use an iterative method such as BiCGSTAB
	if (debug) std::cout << "\tSolve linear system where A is " << A.rows() << "x" << A.cols() << std::endl;
	//echo::solvers::solveWithSPQR<BsplineGridType::SparseMatrixType, BsplineGridType::DenseVectorType>(A,b,x,config); str_solver="_SPQR";
	/// This can happen if there is a region with very densely populated points
	echo::solvers::solveWithGMRES<IPCMRIKernel::BsplineGridType::SparseMatrixType, IPCMRIKernel::BsplineGridType::DenseVectorType>(A, b, x, config); str_solver = "_GMRES";
	//echo::solvers::solveWithBiCGSTAB<BsplineGridType::SparseMatrixType, BsplineGridType::DenseVectorType>(A,b,x,config); str_solver="_BiCGSTAB";

	/// Reorganize x into an array of coefficients, taking into account the reordering
	control_points->setAll(x, kept_nodes);
}


template <typename TBsplineGrid1DType, typename TBsplineGrid2DType>
void interpolateClosedContour(const std::vector< typename TBsplineGrid2DType::PointType> &coordinates, const std::vector< typename TBsplineGrid1DType::PointType> &interpolated_contour_angle,
	std::vector< typename TBsplineGrid2DType::PointType> &interpolated_contour){

	/// parameters: these normally do not need changing
	double grid_spacing1D = M_PI / 2;
	double lambda = 0;

	/// Points should be passed centred but in case they are not, we centre them

	typename TBsplineGrid2DType::PointType centroid;
	centroid[0] = 0; centroid[1] = 0;
	for (int i = 0; i < coordinates.size(); i++){
		centroid += coordinates[i];
	}
	centroid /= coordinates.size();
	/// -------
	std::vector< typename TBsplineGrid2DType::PointType > coordinatesModels(coordinates.size());
	// compute centroid


	// compute polar coords
	for (int i = 0; i < coordinates.size(); i++){
		double angle = std::atan2(coordinates[i][1] - centroid[1], coordinates[i][0] - centroid[0]);
		double magnitude = sqrt((coordinates[i][0] - centroid[0])*(coordinates[i][0] - centroid[0]) +
			(coordinates[i][1] - centroid[1])*(coordinates[i][1] - centroid[1]));
		coordinatesModels[i][0] = magnitude;
		coordinatesModels[i][1] = angle;
	}
	// do cyclic interpolation on the polar coords
	std::vector<typename TBsplineGrid1DType::PointType> coordinates1D(coordinates.size());
	typedef echo::Point<typename TBsplineGrid1DType::MatrixElementType, 1> Value1DType;
	std::vector<Value1DType> values(coordinates.size());
	double mint = std::numeric_limits<double>::max();
	double maxt = -std::numeric_limits<double>::max();
	for (int i = 0; i < coordinates.size(); i++){
		coordinates1D[i] = coordinatesModels[i][1];// angle
		if (coordinates1D[i][0] < mint){
			mint = coordinates1D[i][0];
		}
		if (coordinates1D[i][0] > maxt){
			maxt = coordinates1D[i][0];
		}
		values[i] = coordinatesModels[i][0];
	}
	double bounds[] = { mint, maxt };

	typename TBsplineGrid1DType::Pointer control_points(new TBsplineGrid1DType(bounds, bsd, grid_spacing1D, 0)); /// the control point grid does not need border. This grid covers the whole ROI and there is no control_points division.
	typename TBsplineGrid1DType::IndexType cyclicDimensions;
	cyclicDimensions[0] = 1; // first dimension is cyclic
	control_points->SetCyclicDimensions(cyclicDimensions);
	control_points->SetCyclicBounds(0, -M_PI, M_PI);

	control_points->SetParallel(parallel_on, threads_to_use);
	control_points->UpdateCyclicBehaviour();

	fit_cyclicBspline1D<TBsplineGrid1DType>(coordinates1D, values, lambda, control_points);
	auto interpolated_contourAs = control_points->evaluate(interpolated_contour_angle);
	interpolated_contour.resize(interpolated_contourAs.size());
	// re-convert into cartesian
	for (int i = 0; i < interpolated_contourAs.size(); i++){
		interpolated_contour[i][0] = interpolated_contourAs[i][0] * std::cos(interpolated_contour_angle[i][0]) + centroid[0];
		interpolated_contour[i][1] = interpolated_contourAs[i][0] * std::sin(interpolated_contour_angle[i][0]) + centroid[1];
	}
}

} // namespace crimson
