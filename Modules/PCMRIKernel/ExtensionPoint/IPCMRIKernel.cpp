// This will be an interface for PCMRI service and blueberry extension point etc.

/**
* This is similar to the 3D reconstruction but also over time. Time is treated as just another dimension,
* however it is imposed periodic. This is achieved by extending input data up to 2 b-spline grid points on
* each side along the time direction (or is it just 1?). Also, temporal resolution is also increased over
* the multiple scales is not divided by 2, instead it goes from 4 control points to 10 (maybe change this,
* we will see). Another difference is that temporal variation can be penalised. For starters, we don't
* really do this.
*
* (c) Alberto Gomez 2014 - King's College London
* Marija Marcan 2017 - King's College London
*/

#include <gsl.h>

#include <queue>
#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/multi_array.hpp>

#include "IPCMRIKernel.h"
#include "ISolidModelKernel.h"
#include "IMeshingKernel.h"

#include <mitkPlanarCircle.h>
#include <mitkPlanarEllipse.h>
#include <mitkPlaneGeometry.h>
#include <mitkPoint.h>
#include <mitkImage.h>
#include <mitkRotationOperation.h>
#include <mitkInteractionConst.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkImageVtkReadAccessor.h>

#include <mitkContourModelToSurfaceFilter.h>
#include <mitkImageStatisticsCalculator.h>
#include <mitkExtractSliceFilter.h>
#include <mitkVtkImageOverwrite.h>

#include "itkNearestNeighborInterpolateImageFunction.h"
#include <itkNeighborhoodBinaryThresholdImageFunction.h>
#include <itkMetaDataObject.h>

#include <vtkDistancePolyDataFilter.h>
#include <vtkPointData.h>
#include <vtkNew.h>
#include <vtkProbeFilter.h>
#include <vtkIdList.h>
#include <vtkPolyData.h>
#include <vtkDoubleArray.h>
#include <vtkShortArray.h>
#include <vtkCellData.h>
#include <vtkMassProperties.h>
#include <vtkTriangleFilter.h>

#include <mitkContourModelSet.h>
#include <mitkContourModel.h>
#include "mitkImageAccessByItk.h"

#include <Wm5IntrRay3Plane3.h>

//#define _DEBUG
//#ifdef _DEBUG
//#endif // _DEBUG

#include <chrono>
#include <numeric>
#include <math.h>

#define _USE_MATH_DEFINES

// Variables that are accessed by many methods

/// misc
const bool KEEP_SMALL_control_pointsES = true; /// If this is on, control_pointses are adaptively subdivided
/// For mac
//const int MAX_COLS_PER_control_points = 25000; /// For the A matrix
//const int MAX_POINTS_PER_control_points = 1E+07; /// For the B matrix

/// For linux
int MAX_COLS_PER_control_points = 10000; /// For the A matrix
int MAX_POINTS_PER_control_points = 2.5E+05; /// For the B matrix
float MAX_MEMORY = -1;

const double CMS_TO_MS = 1E-02;
const double MS_TO_CMS = 100.0;

const unsigned int NDIMS = 2;
const unsigned int OUTPUT_DIMENSIONS = 1;

const double MAX_ERROR = 1E-05;
const double B_TO_MB = 1 / 1024.0 / 1024.0;
const double GB_TO_MB = 1024.0;
unsigned int number_of_control_pointses;
const double ZERO = 1E-10;
double spacing_reduction_factor_space[4] = { 2.0, 2.0, 2.0, 1.2 };
double minimum_grid_size[4] = { -1, -1, -1, -1 };
bool isCartesian = false;
bool fitData = false;
bool writeErrors = false;
/// mandatory arguments
int n_images;
std::string reference_image_filename = "";
std::string input_image_flename;
std::string extra = "";
std::string input_motion_mesh_config_file;
std::string output_prefix;
/// default arguments
bool debug = false;
bool parallel_on = false;
int threads_to_use = 2;
bool verbose = false;
unsigned int border = 0;
unsigned int bsd = 3;
unsigned int max_levels = 3;
unsigned int points_between_two_nodes = 2;
double user_bounds[2 * 2];

double image_downsampling[4] = { -1, -1, -1, -1 };

bool cyclictime = false;
double cycle_time[2];

bool allLevels = false;
bool use_user_bounds = false;
double th_velocity = -1;
double remove_nodes_th = 0.0;
bool use_extra_components = false;
bool v_in_mps = false;
double wall_velocity_factor = 1.0;
std::string wall_velocity = "wall_velocity";
int initialLevel = 0;
std::vector <std::string> initial_coeffs_filename;


// parallel computing
/// Macros for the maximun value out of three scalars or the three components of one vector
#define MAX2(x,y,z) std::max( std::max(x,y),z  )
#define MAX2_V(x) std::max( std::max(x[0],x[1]),x[2]  )
#define MAX2_V_abs(x) std::max( std::max( std::abs(x[0]), std::abs(x[1])), std::abs(x[2])  )


//template <typename TBsplineGridType>
//void fit_cyclicBspline1D(const std::vector<typename TBsplineGridType::PointType > &t, const std::vector< typename TBsplineGridType::CoefficientType > &v,
//	double lambda, typename TBsplineGridType::Pointer control_points);
//
//
//
//template <typename TBsplineGrid1DType, typename TBsplineGrid2DType>
//void interpolateClosedContour(const std::vector< typename TBsplineGrid2DType::PointType> &coordinates, const std::vector< typename TBsplineGrid1DType::PointType> &interpolated_contour_angle,
//	std::vector< typename TBsplineGrid2DType::PointType> &interpolated_contour);


namespace crimson
{

	static void setupCatchSystemSignals()
	{
		static bool catchSignalsWasSetup = false;
		if (!catchSignalsWasSetup) {
			catchSignalsWasSetup = true;
		}
	}

	static double calculateParRecScaling(mitk::Image::Pointer pcmriImage)
	{
		int numPhases = 0;
		int phaseEncodingVelocityValue = 0;
		std::vector<short> phaseValues;
		double max_ph = 0;

		pcmriImage->GetPropertyList()->GetIntProperty("mapping.venc", phaseEncodingVelocityValue);
		pcmriImage->GetPropertyList()->GetIntProperty("PAR.MaxNumberOfCardiacPhases", numPhases);

		for (int i = 2*numPhases; i < pcmriImage->GetDimensions()[3]; i++)
		{
			for (int j = 0; j < pcmriImage->GetDimensions()[0]; j++)
			{
				for (int z = 0; z < pcmriImage->GetDimensions()[1]; z++)
				{
					itk::Index<3> idx;
					idx[0] = j;
					idx[1] = z;
					idx[2] = 0;
					phaseValues.push_back(pcmriImage->GetPixelValueByIndex(idx, i));
					auto pixValue = pcmriImage->GetPixelValueByIndex(idx, i);
					if (pcmriImage->GetPixelValueByIndex(idx, i)>max_ph)
						max_ph = pcmriImage->GetPixelValueByIndex(idx, i);
				}
			}
		}

		int power = ceil(log2(max_ph));
		double scale = (pow(2, power)) / 2;

		return scale;
	}

	static double calculateDicomScaling(mitk::Image::Pointer magnitudeImage, mitk::Image::Pointer phaseImage)
	{
		auto propList = phaseImage->GetPropertyList();
		auto propList2 = magnitudeImage->GetPropertyList();
		double venscale = 0;
		int phaseEncodingVelocityValue = 0;
		double scale = 0;

		phaseImage->GetPropertyList()->GetIntProperty("mapping.venc", phaseEncodingVelocityValue);
		//phaseImage->GetPropertyList()->GetDoubleProperty("Private_0019_10e2", venscale); //Private_0019_10e2
		phaseImage->GetPropertyList()->GetDoubleProperty("mapping.venscale", venscale); //Private_0019_10e2


		double vscale = (venscale*M_PI) / (double)phaseEncodingVelocityValue;

#ifdef _DEBUG
		MITK_INFO << vscale;
#endif

		return vscale;
	}

	
	//////////////////////////////////////////////////////////////////////////
	// Mapping of the PCMRI onto a model mesh face.
	//////////////////////////////////////////////////////////////////////////

	class MappingTask : public crimson::async::TaskWithResult<mitk::BaseData::Pointer>
	{
	public:
		MappingTask(const IPCMRIKernel::ContourSet& contoursPCMRI, const mitk::PlanarFigure::Pointer contourModel,
			const MeshData::Pointer mesh, const FaceIdentifier face,
			mitk::Point2D pcmriLandmark, mitk::Point2D mraLandmark, WholeImageTypeFloat::Pointer pcmriImage,
			bool imageFlipped, std::string meshNodeUID, int cardiacFrequency, int startIndex)
			: contoursPCMRI(contoursPCMRI)
			, contourModel(contourModel)
			, mesh(mesh)
			, face(face)
			, pcmriLandmark(pcmriLandmark)
			, mraLandmark(mraLandmark)
			, pcmriImage(pcmriImage)
			, imageFlipped(imageFlipped)
			, meshNodeUID(meshNodeUID)
			, cardiacFrequency(cardiacFrequency)
			, startIndex(startIndex)

		{
		}

		std::tuple<State, std::string> runTask() override
		{
			setupCatchSystemSignals();

			//setup progress tracking

			int progressConvert = contoursPCMRI.size();
			int progressLeft = progressConvert;

			stepsAddedSignal(progressLeft);

			//setup stuff

			Eigen::setNbThreads(threads_to_use);
			Eigen::initParallel();

			/// We are assuming that the first point is the reference point for the rotation.
			/// If this is determined by other means it does not matter, in the end what we need
			/// is the value 'a' of the angle between the two (computed below)

			int Ninterp = 75; /// Number of corresponding points
			double lambda = 0.00001;
			int ncontrol_pts = 4;



			//extract face nodes
			std::vector<int> faceNodes = mesh->getNodeIdsForFace(face);

			std::vector<mitk::Point3D> coordinatesMesh;
			for (std::vector<int>::iterator it = faceNodes.begin(); it < faceNodes.end(); it++) {
				coordinatesMesh.push_back(mesh->getNodeCoordinates(*it));
			}

			#ifdef _DEBUG
				{
					std::ofstream file;
					std::string filename = "./output/mesh3D.txt";
					file.open(filename, ofstream::out);
					for (int i = 0; i < coordinatesMesh.size(); i++){
						file << coordinatesMesh[i][0] << " " << coordinatesMesh[i][1] << " " << coordinatesMesh[i][2] << std::endl;
					}
					file.close();
					std::cout << "output in " << filename << std::endl;
				}

			#endif
			//transform model points to 2D

			std::vector<mitk::Point2D> coordinatesOut;

			//TODO: check if reference image spacing here messes up mapping geometries
			mitk::PlaneGeometry::Pointer plane = contourModel->GetPlaneGeometry()->Clone();

			for (std::vector<mitk::Point3D>::iterator it = coordinatesMesh.begin(); it < coordinatesMesh.end(); it++) {
				mitk::Point2D projectedPoint;
				plane->Map(*it, projectedPoint);
				//TODO: mm and index-to-world coordinates?
				coordinatesOut.push_back(projectedPoint);
			}
#ifdef _DEBUG
					{
						std::ofstream file;
						std::string filename = "./output/mesh2D.txt";
						file.open(filename, ofstream::out);
						for (int i = 0; i < coordinatesOut.size(); i++){
							file << coordinatesOut[i][0] << " " << coordinatesOut[i][1] << std::endl;
						}
						file.close();
						std::cout << "output in " << filename << std::endl;
					}

#endif
	
			//transfer model contour points into std::vector< PointType > format

			std::vector< mitk::Point2D > polyLineModel(contourModel->GetNumberOfControlPoints());
			for (int i = 0; i < contourModel->GetNumberOfControlPoints(); i++)
				polyLineModel[i] = (contourModel->GetControlPoint(i));

			std::vector<IPCMRIKernel::BsplineGridType::PointType> contourModel2D;
			contourModel2D.reserve(polyLineModel.size());    //  avoids unnecessary reallocations
			std::transform(polyLineModel.begin(), polyLineModel.end(),
				std::back_inserter(contourModel2D),
				IPCMRIKernel::ToBSGridType());


			//find centroid of model 2D face contour and center the contour

			IPCMRIKernel::BsplineGridType::PointType centroidModel;
			centroidModel[0] = 0; centroidModel[1] = 0;
			for (int i = 0; i < contourModel2D.size(); i++){
				centroidModel += contourModel2D[i];
			}
			centroidModel /= contourModel2D.size();


			//check if the PC-MRI image is flipped and setup things accordingly
			std::vector< IPCMRIKernel::BsplineGridType::PointType > centred_coordinatesModel(contourModel2D.size());
			for (int i = 0; i < contourModel2D.size(); i++){
				centred_coordinatesModel[i] = contourModel2D[i] - centroidModel;
				if (imageFlipped)
					centred_coordinatesModel[i][0] = -centred_coordinatesModel[i][0];
			}

			//the resulting vectors
			boost::multi_array<double, 2> _mappedPCMRIvalues(boost::extents[coordinatesOut.size()][contoursPCMRI.size()]);
			boost::multi_array<mitk::Vector3D, 2> _mappedPCMRIvectors(boost::extents[coordinatesOut.size()][contoursPCMRI.size()]);

			std::vector<double> _originalFlow(contoursPCMRI.size());
			std::vector<double> _originalFlow2;

			if (isCancelling()) {
				return std::make_tuple(State_Cancelled, std::string("Operation cancelled."));
			}

			try {

				//do stuff for one contour at a time
				for (int slice = 0; slice < contoursPCMRI.size(); ++slice) {

					//extract PCMRI boundary points into an array
					const mitk::PlanarFigure::Pointer figure = contoursPCMRI[slice];

					//transfer model contour points into std::vector< PointType > format
					std::vector< mitk::Point2D > polyLineImage(figure->GetNumberOfControlPoints());
					for (int i = 0; i < figure->GetNumberOfControlPoints(); i++)
						polyLineImage[i] = (figure->GetControlPoint(i));

					auto planePCMRI = figure->GetPlaneGeometry();

					std::vector< mitk::Point2D > polyLineImage2;
					for (const mitk::Point2D& polyLineElement : figure->GetPolyLine(0)) {
						polyLineImage2.push_back(polyLineElement);
					}

					//transfer model contour points into std::vector< PointType > format
					std::vector< mitk::Point3D > polyLineImage3(figure->GetNumberOfControlPoints());
					for (int i = 0; i < figure->GetNumberOfControlPoints(); i++)
						polyLineImage3[i] = figure->GetWorldControlPoint(i);

					std::vector< mitk::Point2D > polyLineImage2D;

					for (std::vector<mitk::Point3D>::iterator it = polyLineImage3.begin(); it < polyLineImage3.end(); it++) {
						mitk::Point2D projectedPoint;
						planePCMRI->Map(*it, projectedPoint);
						//TODO: mm and index-to-world coordinates?
						polyLineImage2D.push_back(projectedPoint);
					}

					//TODO: polylineimage2 or polylineimage2D?
					std::vector<IPCMRIKernel::PointType> coordinatesImage;
					coordinatesImage.reserve(polyLineImage2.size());    //  avoids unnecessary reallocations
					std::transform(polyLineImage2.begin(), polyLineImage2.end(),
						std::back_inserter(coordinatesImage),
						IPCMRIKernel::ToBSGridType());



					///****************************************************************************
					///*                            Find centroids                             *
					///****************************************************************************

					//find centroid of PCMRI contour
					IPCMRIKernel::BsplineGridType::PointType centroidImage;

					centroidImage[0] = 0; centroidImage[1] = 0;
					for (int i = 0; i < coordinatesImage.size(); i++){
						centroidImage += coordinatesImage[i];
					}
					centroidImage /= coordinatesImage.size();

					//centre image spline points (store transformation as displacementPCMRI) displacementPCMRI=centroidImage here

					std::vector< IPCMRIKernel::BsplineGridType::PointType > centred_coordinatesImage(coordinatesImage.size());

					for (int i = 0; i < coordinatesImage.size(); i++){
						centred_coordinatesImage[i] = coordinatesImage[i] - centroidImage;
					}


					///****************************************************************************
					///*                            Find the rotation                             *
					///****************************************************************************
					IPCMRIKernel::BsplineGridType::PointType vA, vB; //vA model landmark, vB image landmark
					//landmark needs to be centred as well! 
					vA[0] = mraLandmark[0] - centroidModel[0];
					vA[1] = mraLandmark[1] - centroidModel[1];
					vB[0] = pcmriLandmark[0] - centroidImage[0];
					vB[1] = pcmriLandmark[1] - centroidImage[1];

					if (imageFlipped)
						vA[0] = -vA[0];
					
					double angleA = std::atan2(vA[1], vA[0]);
					double angleB = std::atan2(vB[1], vB[0]);
					double a = angleB - angleA;
					
					IPCMRIKernel::BsplineGridType::DenseMatrixType Rot(2, 2);
					Rot << std::cos(a), -std::sin(a),
						std::sin(a), std::cos(a);

					std::vector< IPCMRIKernel::BsplineGridType::PointType> rotated_centred_coordinatesModel(centred_coordinatesModel.size());

					for (int i = 0; i < centred_coordinatesModel.size(); i++){
						/// not very elegant, sorry !
						rotated_centred_coordinatesModel[i][0] = Rot(0, 0) * centred_coordinatesModel[i][0] + Rot(0, 1) * centred_coordinatesModel[i][1];
						rotated_centred_coordinatesModel[i][1] = Rot(1, 0) * centred_coordinatesModel[i][0] + Rot(1, 1) * centred_coordinatesModel[i][1];
					}

					//warp the rest of the model face - DOUBLE CHECK IF WE NEED ROTATION HERE???!!!

					std::vector<IPCMRIKernel::PointType> coordinatesOutBS;
					coordinatesOutBS.reserve(coordinatesOut.size());    //  avoids unnecessary reallocations
					std::transform(coordinatesOut.begin(), coordinatesOut.end(),
						std::back_inserter(coordinatesOutBS),
						IPCMRIKernel::ToBSGridType());

					std::vector<IPCMRIKernel::BsplineGridType::PointType> coordinatesOut_centred(coordinatesOutBS);
					for (int i = 0; i < coordinatesOut.size(); i++){
						coordinatesOut_centred[i] = coordinatesOutBS[i] - centroidModel;
						if (imageFlipped)
							coordinatesOut_centred[i][0] = -coordinatesOut_centred[i][0];
					}

					std::vector< IPCMRIKernel::BsplineGridType::PointType> rotated_centred_coordinatesOut(coordinatesOut_centred.size());

					for (int i = 0; i < coordinatesOut_centred.size(); i++){
						/// not very elegant, sorry !
						rotated_centred_coordinatesOut[i][0] = Rot(0, 0) * coordinatesOut_centred[i][0] + Rot(0, 1) * coordinatesOut_centred[i][1];
						rotated_centred_coordinatesOut[i][1] = Rot(1, 0) * coordinatesOut_centred[i][0] + Rot(1, 1) * coordinatesOut_centred[i][1];
					}

					///****************************************************************************
					///*                            FIT A CYCLIC B-spline to each contour         *
					///****************************************************************************

					//fit image spline points

					std::vector< IPCMRIKernel::BsplineGrid1DType::PointType > interpolated_contour_angle(Ninterp);
					for (int i = 0; i < Ninterp; i++){
						interpolated_contour_angle[i] = i * 2 * M_PI / Ninterp + angleB;
					}

					//fit n-points spline to rotated 2D model (b)

					std::vector< IPCMRIKernel::BsplineGridType::PointType> interpolated_contourA, interpolated_contourB;

					interpolateClosedContour<IPCMRIKernel::BsplineGrid1DType, IPCMRIKernel::BsplineGridType>(rotated_centred_coordinatesModel, interpolated_contour_angle, interpolated_contourA);
					interpolateClosedContour<IPCMRIKernel::BsplineGrid1DType, IPCMRIKernel::BsplineGridType>(centred_coordinatesImage, interpolated_contour_angle, interpolated_contourB);



					///****************************************************************************
					///                Calculate the vectors from one contour to another          *
					///****************************************************************************
					std::vector< IPCMRIKernel::BsplineGridType::CoefficientType> contour_deformation_vectorsx(Ninterp), contour_deformation_vectorsy(Ninterp);
					for (int i = 0; i < Ninterp; i++){

						auto vec = interpolated_contourB[i] - interpolated_contourA[i];
						contour_deformation_vectorsx[i][0] = vec[0];
						contour_deformation_vectorsy[i][0] = vec[1];
					}

					///****************************************************************************
					///             Interpolate the deformation field densely                     *
					///****************************************************************************

					//interpolated_contourA; // coordinates
					//contour_deformation_vectors; // values


					// compute bounds
					double bounds[IPCMRIKernel::BsplineGridType::IDims * 2];
					for (int i = 0; i < IPCMRIKernel::BsplineGridType::IDims; i++){
						bounds[2 * i] = std::numeric_limits<double>::max();
						bounds[2 * i + 1] = -std::numeric_limits<double>::max();
					}
					for (int i = 0; i < Ninterp; i++){
						for (int j = 0; j < IPCMRIKernel::BsplineGridType::IDims; j++){
							  if (interpolated_contourA[i][j]<bounds[2*j]){
							      bounds[2*j]  =interpolated_contourA[i][j];
							  }
							if (interpolated_contourB[i][j] < bounds[2 * j]){
								bounds[2 * j] = interpolated_contourB[i][j];
							}
							  if (interpolated_contourA[i][j]>bounds[2*j+1]){
							      bounds[2*j+1]  =interpolated_contourA[i][j];
							  }
							if (interpolated_contourB[i][j] > bounds[2 * j + 1]){
								bounds[2 * j + 1] = interpolated_contourB[i][j];
							}
						}
					}

					IPCMRIKernel::BsplineGridType::PointType grid_spacing;
					for (int j = 0; j < IPCMRIKernel::BsplineGridType::IDims; j++){
						grid_spacing[j] = (bounds[2 * j + 1] - bounds[2 * j]) / ncontrol_pts;
					}

					IPCMRIKernel::BsplineGridType::Pointer control_points2D(new IPCMRIKernel::BsplineGridType(bounds, bsd, grid_spacing, 0));
					/// the control point grid does not need border. This grid covers the whole ROI and there is no control_points division.
					control_points2D->SetParallel(parallel_on, threads_to_use);


					IPCMRIKernel::BsplineGridType::DenseVectorType values_vectorx(IPCMRIKernel::BsplineGridType::ODims*contour_deformation_vectorsx.size(), 1);
					IPCMRIKernel::BsplineGridType::DenseVectorType values_vectory(IPCMRIKernel::BsplineGridType::ODims*contour_deformation_vectorsy.size(), 1);
					std::vector< IPCMRIKernel::BsplineGridType::CoefficientType>::const_iterator const_iter;
					IPCMRIKernel::BsplineGridType::MatrixElementType *eigen_iter;


					for (const_iter = contour_deformation_vectorsx.begin(), eigen_iter = values_vectorx.data(); const_iter != contour_deformation_vectorsx.end(); ++const_iter)
					{
						for (int nd = 0; nd < IPCMRIKernel::BsplineGridType::ODims; nd++){
							(*eigen_iter) = (*const_iter)[nd];
							++eigen_iter;
						}
					}


					for (const_iter = contour_deformation_vectorsy.begin(), eigen_iter = values_vectory.data(); const_iter != contour_deformation_vectorsy.end(); ++const_iter)
					{
						for (int nd = 0; nd < IPCMRIKernel::BsplineGridType::ODims; nd++){
							(*eigen_iter) = (*const_iter)[nd];
							++eigen_iter;
						}
					}

					IPCMRIKernel::BsplineGridType::SparseMatrixType A;
					IPCMRIKernel::BsplineGridType::DenseVectorType b;
					std::vector<unsigned int> kept_nodes;
					std::vector<unsigned int> corresponding_nodes;

					std::vector<IPCMRIKernel::BsplineGridType::CoefficientType> interpolated_vectorsx, interpolated_vectorsy, interpolated_vectorsxBorder, interpolated_vectorsyBorder;
					IPCMRIKernel::BsplineGridType::SparseMatrixType B = control_points2D->createSamplingMatrix(interpolated_contourA, remove_nodes_th, kept_nodes, corresponding_nodes);
					IPCMRIKernel::BsplineGridType::SparseMatrixType Agrad;
					if (lambda)
					{
						//BsplineGridType::SparseMatrixType Adiv = control_points2D->createContinuousDivergenceSamplingMatrix(kept_nodes,corresponding_nodes);
						Agrad = control_points2D->createContinuousGradientSamplingMatrix(remove_nodes_th, kept_nodes);
					}


					///---------------------------------------
					///     Do For X
					///---------------------------------------
					{

						{ /// Keep scope to save memory
							Eigen::setNbThreads(8);
							A = B.transpose() *B;
							b = B.transpose()*values_vectorx;
						}

						// Regularisation
						if (lambda){
							A = A + lambda / (1 - lambda)*Agrad;/// See notes on p118 of my thesis for an explanation
						}


						if (debug) std::cout << "\tCompute coefficients in this control_points" << std::endl;
						/// Available additional solvers
						/// BiCGSTAB + IncompletLUT preconditioner
						/// SparseQRf
						/// SPQR
						/// PastixLU

						IPCMRIKernel::BsplineGridType::DenseVectorType x;//,x2,x3,x4;
						echo::solvers::ConfigurationParameter config;
						config.max_iterations = 10000; /// This number might look huge but we need to get to the desired tolerance
						config.tol = 1E-06;
						config.verbose = true;
						std::string str_solver;

						/// Add some condition like if the matrix is larger than e.g. 5000 x 5000 then use an iterative method such as BiCGSTAB
						if (debug) std::cout << "\tSolve linear system where A is " << A.rows() << "x" << A.cols() << std::endl;
						//echo::solvers::solveWithSPQR<BsplineGridType::SparseMatrixType, BsplineGridType::DenseVectorType>(A,b,x,config); str_solver="_SPQR";
						/// This can happen if there is a region with very densely populated points
						echo::solvers::solveWithGMRES<IPCMRIKernel::BsplineGridType::SparseMatrixType, IPCMRIKernel::BsplineGridType::DenseVectorType>(A, b, x, config); str_solver = "_GMRES";
						//echo::solvers::solveWithBiCGSTAB<BsplineGridType::SparseMatrixType, BsplineGridType::DenseVectorType>(A,b,x,config); str_solver="_BiCGSTAB";

						if (debug) std::cout << "\tCoefficients computed, replace control_points values with the calculated coefficients" << std::endl;
						/// Reorganize x into an array of coefficients, taking into account the reordering
						control_points2D->setAll<IPCMRIKernel::BsplineGridType::DenseVectorType>(x, kept_nodes);


						interpolated_vectorsx = control_points2D->evaluate(rotated_centred_coordinatesOut); //TODO or coordinatesOut_centred?
						interpolated_vectorsxBorder = control_points2D->evaluate(interpolated_contourA); 
					}

					///---------------------------------------
					///     Do For Y
					///---------------------------------------
					{



						{ /// Keep scope to save memory
							Eigen::setNbThreads(8);
							A = B.transpose() *B;
							b = B.transpose()*values_vectory;
						}

						// Regularisation
						if (lambda){
							A = A + lambda / (1 - lambda)*Agrad;/// See notes on p118 of my thesis for an explanation
						}


						if (debug) std::cout << "\tCompute coefficients in this control_points" << std::endl;
						/// Available additional solvers
						/// BiCGSTAB + IncompletLUT preconditioner
						/// SparseQR
						/// SPQR
						/// PastixLU

						IPCMRIKernel::BsplineGridType::DenseVectorType x;//,x2,x3,x4;
						echo::solvers::ConfigurationParameter config;
						config.max_iterations = 10000; /// This number might look huge but we need to get to the desired tolerance
						config.tol = 1E-06;
						config.verbose = true;
						std::string str_solver;

						/// Add some condition like if the matrix is larger than e.g. 5000 x 5000 then use an iterative method such as BiCGSTAB
						if (debug) std::cout << "\tSolve linear system where A is " << A.rows() << "x" << A.cols() << std::endl;
						//echo::solvers::solveWithSPQR<BsplineGridType::SparseMatrixType, BsplineGridType::DenseVectorType>(A,b,x,config); str_solver="_SPQR";
						/// This can happen if there is a region with very densely populated points
						echo::solvers::solveWithGMRES<IPCMRIKernel::BsplineGridType::SparseMatrixType, IPCMRIKernel::BsplineGridType::DenseVectorType>(A, b, x, config); str_solver = "_GMRES";
						//echo::solvers::solveWithBiCGSTAB<BsplineGridType::SparseMatrixType, BsplineGridType::DenseVectorType>(A,b,x,config); str_solver="_BiCGSTAB";

						if (debug) std::cout << "\tCoefficients computed, replace control_points values with the calculated coefficients" << std::endl;
						/// Reorganize x into an array of coefficients, taking into account the reordering
						control_points2D->setAll<IPCMRIKernel::BsplineGridType::DenseVectorType>(x, kept_nodes);


						interpolated_vectorsy = control_points2D->evaluate(rotated_centred_coordinatesOut);
						interpolated_vectorsyBorder = control_points2D->evaluate(interpolated_contourA);
					}

					//apply displacement on rotated model 2D
					//warp the rest of the model face
					//displace the result above for displacementPCMRI - we are now in the same frame of reference as the PCMRI image


					std::vector<mitk::Point2D> coordinatesDisplaced;
					std::vector<mitk::Point2D> coordinatesDisplacedBorder;
					std::vector<mitk::Point2D> coordinatesDisplacedIndex;
					std::vector<mitk::Point2D> coordinatesDisplacedIndexMapped;

					for (int i = 0; i < interpolated_contourA.size(); i++)
					{
						mitk::Point2D pointDisplacedBorder;
						pointDisplacedBorder[0] = interpolated_contourA[i][0] + interpolated_vectorsxBorder[i][0] + centroidImage[0];
						pointDisplacedBorder[1] = interpolated_contourA[i][1] + interpolated_vectorsyBorder[i][0] + centroidImage[1];
						coordinatesDisplacedBorder.push_back(pointDisplacedBorder);
					}
					for (int i = 0; i < rotated_centred_coordinatesOut.size(); i++){
						mitk::Point2D pointDisplaced;
						pointDisplaced[0] = rotated_centred_coordinatesOut[i][0] + interpolated_vectorsx[i][0] + centroidImage[0];
						pointDisplaced[1] = rotated_centred_coordinatesOut[i][1] + interpolated_vectorsy[i][0] + centroidImage[1];
						coordinatesDisplaced.push_back(pointDisplaced);
						
						itk::Index<4> idx, idx2;
						itk::Point<float, 4> phyPoint, phyPoint2;
						mitk::Point2D pointDisplacedIndex, pointDisplacedIndexMapped;
						mitk::Point3D pointMapped;
						phyPoint[0] = pointDisplaced[0];
						phyPoint[1] = pointDisplaced[1];
						phyPoint[2] = 0;
						phyPoint[3] = slice;
						pcmriImage->TransformPhysicalPointToIndex(phyPoint, idx);
						pointDisplacedIndex[0] = idx[0];
						pointDisplacedIndex[1] = idx[1];
						coordinatesDisplacedIndex.push_back(pointDisplacedIndex);

						plane->Map(pointDisplaced, pointMapped);
						phyPoint2[0] = pointMapped[0];
						phyPoint2[1] = pointMapped[1];
						phyPoint2[2] = pointMapped[2];
						phyPoint2[3] = slice;
						pcmriImage->TransformPhysicalPointToIndex(phyPoint2, idx2);
						pointDisplacedIndexMapped[0] = idx2[0];
						pointDisplacedIndexMapped[1] = idx2[1];
						coordinatesDisplacedIndexMapped.push_back(pointDisplacedIndexMapped);
					}


					//calculate the scaling factor for velocities in case of different contour vs model areas - to preserve flow volume
					auto areaContour = ISolidModelKernel::contourArea(figure);
					double area = mesh->calculateArea(face);
					auto radius = sqrt(area / 3.14);
					auto scalingFactor = areaContour / area;

					MITK_INFO << "Planar figure area " << areaContour;
					MITK_INFO << "Mesh face area " << area;
					MITK_INFO << "Area scaling factor " << scalingFactor;
#ifdef _DEBUG
					MITK_INFO << "Planar figure area " << areaContour;
					MITK_INFO << "Mesh face area " << area;
					MITK_INFO << "Area scaling factor " << scalingFactor;
#endif
					//interpolate pre-calculated profile points to mesh warped points
					typedef itk::LinearInterpolateImageFunction<WholeImageTypeFloat, float>	LinearInterpolatorType;
					auto interpolator = LinearInterpolatorType::New();
					interpolator->SetInputImage(pcmriImage);

					for (int i = 0; i < coordinatesDisplaced.size(); i++){
						itk::ContinuousIndex<float, 4> idx;
						itk::Point<float, 4> phyPoint;
						double value;
						mitk::Point2D point2D;
						mitk::Point3D point3D;
						point2D[0] = coordinatesDisplaced[i][0];
						point2D[1] = coordinatesDisplaced[i][1];
						planePCMRI->Map(point2D, point3D);
						
						phyPoint[0] = point3D[0];
						phyPoint[1] = point3D[1];
						phyPoint[2] = point3D[2];
						phyPoint[3] = slice + startIndex;

						pcmriImage->TransformPhysicalPointToContinuousIndex(phyPoint, idx);

						value = interpolator->EvaluateAtContinuousIndex(idx);
						
						if (imageFlipped)
							value = -value;
						_mappedPCMRIvalues[i][slice] = value * scalingFactor;
					}


					///---------------------------------------------------------------------------//
					///                       PRINT RESULTS TO FILE                               //
					///---------------------------------------------------------------------------//

					#ifdef _DEBUG

					{
						std::ofstream file;
						std::string filename = "./output/centredModel.txt";
						file.open(filename, ofstream::out);
						for (int i = 0; i < centred_coordinatesModel.size(); i++){
							file << centred_coordinatesModel[i][0] << " " << centred_coordinatesModel[i][1] << std::endl;
						}
						file.close();
						std::cout << "output in " << filename << std::endl;
					}

					{
						std::ofstream file;
						std::string filename = "./output/rotated_centredModel.txt";
						file.open(filename, ofstream::out);
						for (int i = 0; i < rotated_centred_coordinatesModel.size(); i++){
							file << rotated_centred_coordinatesModel[i][0] << " " << rotated_centred_coordinatesModel[i][1] << std::endl;
						}
						file.close();
						std::cout << "output in " << filename << std::endl;
					}

					{
						std::ofstream file;
						std::string filename = "./output/rotated_centredMesh.txt";
						file.open(filename, ofstream::out);
						for (int i = 0; i < rotated_centred_coordinatesOut.size(); i++){
							file << rotated_centred_coordinatesOut[i][0] << " " << rotated_centred_coordinatesOut[i][1] << std::endl;
						}
						file.close();
						std::cout << "output in " << filename << std::endl;
					}

					{
						std::ofstream file;
						std::string filename = "./output/coordinatesMesh2D.txt";
						file.open(filename, ofstream::out);
						for (int i = 0; i < coordinatesOut.size(); i++){
							file << coordinatesOut[i][0] << " " << coordinatesOut[i][1] << std::endl;
						}
						file.close();
						std::cout << "output in " << filename << std::endl;
					}

					{
						std::ofstream file;
						std::string filename = "./output/centred_coordinatesMesh.txt";
						file.open(filename, ofstream::out);
						for (int i = 0; i < coordinatesOut_centred.size(); i++){
							file << coordinatesOut_centred[i][0] << " " << coordinatesOut_centred[i][1] << std::endl;
						}
						file.close();
						std::cout << "output in " << filename << std::endl;
					}

					{
						std::ofstream file;
						std::string filename = "./output/interpolatedModelContour.txt";
						file.open(filename, ofstream::out);
						for (int i = 0; i < interpolated_contourA.size(); i++){
							file << interpolated_contourA[i][0] << " " << interpolated_contourA[i][1] << std::endl;
						}
						file.close();
						std::cout << "output in " << filename << std::endl;
					}

					{
						std::ofstream file;
						std::string filename = "./output/originalImageContour.txt";
						file.open(filename, ofstream::out);
						for (int i = 0; i < coordinatesImage.size(); i++){
							file << coordinatesImage[i][0] << " " << coordinatesImage[i][1] << std::endl;
						}
						file.close();
						std::cout << "output in " << filename << std::endl;
					}

					{
						std::ofstream file;
						std::string filename = "./output/centredImageContour.txt";
						file.open(filename, ofstream::out);
						for (int i = 0; i < centred_coordinatesImage.size(); i++){
							file << centred_coordinatesImage[i][0] << " " << centred_coordinatesImage[i][1] << std::endl;
						}
						file.close();
						std::cout << "output in " << filename << std::endl;
					}

					{
						std::ofstream file;
						std::string filename = "./output/interpolatedImageContour.txt";
						file.open(filename, ofstream::out);
						for (int i = 0; i < interpolated_contourB.size(); i++){
							file << interpolated_contourB[i][0] << " " << interpolated_contourB[i][1] << std::endl;
						}
						file.close();
						std::cout << "output in " << filename << std::endl;
					}

					{
						std::ofstream file;
						std::string filename = "./output/contourVectors.txt";
						file.open(filename, ofstream::out);
						for (int i = 0; i < interpolated_contourA.size(); i++){
							file <<  contour_deformation_vectorsx[i][0] << " " << contour_deformation_vectorsy[i][0] << std::endl;
						}
						file.close();
						std::cout << "output in " << filename << std::endl;
					}

					{
						std::ofstream file;
						std::string filename = "./output/denseDeformation.txt";
						file.open(filename, ofstream::out);
						for (int i = 0; i < coordinatesOut_centred.size(); i++){
							file << interpolated_vectorsx[i][0] << " " << interpolated_vectorsy[i][0] << std::endl;
						}
						file.close();
						std::cout << "output in " << filename << std::endl;
					}

					{
						std::ofstream file;
						std::string filename = "./output/denseDeformationBorder.txt";
						file.open(filename, ofstream::out);
						for (int i = 0; i < interpolated_vectorsxBorder.size(); i++){
							file << interpolated_vectorsxBorder[i][0] << " " << interpolated_vectorsyBorder[i][0] << std::endl;
						}
						file.close();
						std::cout << "output in " << filename << std::endl;
					}

					{
						std::ofstream file;
						std::string filename = "./output/coordinatesDisplaced.txt";
						file.open(filename, ofstream::out);
						for (int i = 0; i < coordinatesDisplaced.size(); i++){
							file <<  coordinatesDisplaced[i][0] << " " << coordinatesDisplaced[i][1] << std::endl;
						}
						file.close();
						std::cout << "output in " << filename << std::endl;
					}

					{
						std::ofstream file;
						std::string filename = "./output/coordinatesDisplacedBorder.txt";
						file.open(filename, ofstream::out);
						for (int i = 0; i < coordinatesDisplacedBorder.size(); i++){
							file << coordinatesDisplacedBorder[i][0] << " " << coordinatesDisplacedBorder[i][1] << std::endl;
						}
						file.close();
						std::cout << "output in " << filename << std::endl;
					}

					{
						std::ofstream file;
						std::string filename = "./output/coordinatesDisplacedIndex.txt";
						file.open(filename, ofstream::out);
						for (int i = 0; i < coordinatesDisplaced.size(); i++){
							file <<  coordinatesDisplacedIndex[i][0] << " " << coordinatesDisplacedIndex[i][1] << std::endl;
						}
						file.close();
						std::cout << "output in " << filename << std::endl;
					}

					{
						std::ofstream file;
						std::string filename = "./output/coordinatesDisplacedIndexMapped.txt";
						file.open(filename, ofstream::out);
						for (int i = 0; i < coordinatesDisplaced.size(); i++){
							file << coordinatesDisplacedIndexMapped[i][0] << " " << coordinatesDisplacedIndexMapped[i][1] << std::endl;
						}
						file.close();
						std::cout << "output in " << filename << std::endl;
					}

					{
						std::ofstream file;
						std::string filename = "./output/mappedPcmriValues" + std::to_string(slice) + ".txt";
						file.open(filename, ofstream::out);
						for (int i = 0; i < coordinatesMesh.size(); i++){
							file << _mappedPCMRIvalues[i][slice] << " " << coordinatesDisplaced[i][0] << " "
								<< " " << coordinatesDisplaced[i][1] << std::endl;
						}
						file.close();
						std::cout << "output in " << filename << std::endl;
					}

					/*{
						std::ofstream file;
						std::string filename = "./output/mappedPcmriValues3D.txt";
						file.open(filename, ofstream::out);
						for (int i = 0; i < coordinatesMesh.size(); i++){
							file << mappedPcmriValues3D[i][0] << " " << mappedPcmriValues3D[i][1] << " " << mappedPcmriValues3D[i][2] << std::endl;
						}
						file.close();
						std::cout << "output in " << filename << std::endl;
					}*/

					/*{
						std::ofstream file;
						std::string filename = "./output/mappedPcmriVectors3D.txt";
						file.open(filename, ofstream::out);
						for (int i = 0; i < coordinatesMesh.size(); i++){
							file << _mappedPCMRIvectors[i][slice][0] << " " << _mappedPCMRIvectors[i][slice][1] << " " << _mappedPCMRIvectors[i][slice][2] << std::endl;
						}
						file.close();
						std::cout << "output in " << filename << std::endl;
					}*/

					//{
					//	std::ofstream file;
					//	std::string filename = "./output/originalImagePointsIndex.txt";
					//	file.open(filename, ofstream::out);
					//	for (int i = 0; i < _originalPCMRIcoordinates.size(); i++){
					//		file << _originalPCMRIcoordinates[i][0] << " " << _originalPCMRIcoordinates[i][1] << " " << _originalPCMRIcoordinates[i][2] << " " << _originalPCMRIvalues[i] << std::endl;
					//	}
					//	file.close();
					//	std::cout << "output in " << filename << std::endl;
					//}

					#endif
					
					progressMadeSignal(1);

				}

				if (isCancelling()) {
					return std::make_tuple(State_Cancelled, std::string("Operation cancelled."));
				}

				//calculate flow waveform
				auto flowWaveform = IPCMRIKernel::calculateFlowWaveform(_mappedPCMRIvalues, mesh, face);
				std::vector<double> flowWaveformAbs;
				for (std::vector<double>::iterator itflow = flowWaveform.begin(); itflow < flowWaveform.end(); itflow++)
				{
					flowWaveformAbs.push_back(abs(*itflow));
				}

				#ifdef _DEBUG
				{
					std::ofstream file;
					std::string filename = "./output/flowWaveform.txt";
					file.open(filename, ofstream::out);
					for (int i = 0; i < flowWaveform.size(); i++){
						file << flowWaveform[i] << std::endl;
					}
					file.close();
					std::cout << "output in " << filename << std::endl;
				}
				#endif

				double area = mesh->calculateArea(face);
				auto radius = sqrt(area / 3.14);
	
				//setting the surface representation for the PCMRIData to a scaled profile timepoint with maximum flow
				double max = *std::max_element(flowWaveformAbs.begin(), flowWaveformAbs.end());
				int maxIndex = std::distance(flowWaveformAbs.begin(), std::max_element(flowWaveformAbs.begin(), flowWaveformAbs.end()));
								
				// scale the values of the flow velocity! 
				std::vector<double> timePointPCMRIValues;
				for (int j = 0; j < coordinatesMesh.size(); j++)
				{
					timePointPCMRIValues.push_back(abs(_mappedPCMRIvalues[j][maxIndex]));
				}

				double maxValue = *std::max_element(timePointPCMRIValues.begin(), timePointPCMRIValues.end());

				//scaling factor should be such that maximum profile value is equivalent to double the size of the face diameter
				double scaleFactor = maxValue / (radius * 4);

				// Finally, create the output MITK object
				mitk::BaseData::Pointer output;

				//set the output to point to results of the mapping
				PCMRIData::Pointer data = PCMRIData::New();

				data->setMappedPCMRIvalues(_mappedPCMRIvalues);				
				data->setMesh(mesh.GetPointer());
				data->setMeshNodeUID(meshNodeUID);
				data->setFaceIdentifierIndex(mesh->getFaceIdentifierMap().faceIdentifierIndex(face));

				//set fixed properties needed for time interpolation
				auto normal = contourModel->GetPlaneGeometry()->GetNormal();
				data->setTimeInterpolationParameters(cardiacFrequency, normal, contoursPCMRI.size());
				data->setVisualizationParameters(maxIndex, scaleFactor);

				output = data.GetPointer();


				if (isCancelling()) {
					return std::make_tuple(State_Cancelled, std::string("Operation cancelled."));
				}

				setResult(output.GetPointer());
				return std::make_tuple(State_Finished, std::string("Mapping finished successfully."));
			}
			catch (...) {
				progressMadeSignal(progressLeft);

				std::string errorMessage = "Exception caught during mapping\n";
			
				return std::make_tuple(State_Failed, std::string(errorMessage));
			}
		}


	private:
		const IPCMRIKernel::ContourSet contoursPCMRI;
		const mitk::PlanarFigure::Pointer contourModel;
		const MeshData::Pointer mesh;
		const FaceIdentifier face;
		const mitk::Point2D pcmriLandmark;
		const mitk::Point2D mraLandmark;
		const WholeImageTypeFloat::Pointer pcmriImage;
		const bool imageFlipped;
		const std::string meshNodeUID;
		const int cardiacFrequency;
		const int startIndex;
	};


	std::shared_ptr<async::TaskWithResult<mitk::BaseData::Pointer>>
		IPCMRIKernel::createMapTask(const ContourSet& contoursPCMRI, const mitk::PlanarFigure::Pointer& contourModel,
		MeshData::Pointer mesh, FaceIdentifier face, mitk::Point2D pcmriLandmark,
		mitk::Point2D mraLandmark, WholeImageTypeFloat::Pointer pcmriImage, 
		bool imageFlipped, std::string meshNodeUID, int cardiacFrequency, int startIndex)
	{
		return std::static_pointer_cast<crimson::async::TaskWithResult<mitk::BaseData::Pointer>>(std::make_shared<MappingTask>(
			contoursPCMRI, contourModel, mesh, face, pcmriLandmark, mraLandmark, pcmriImage, imageFlipped, meshNodeUID, cardiacFrequency, startIndex));
	}

	mitk::Surface::Pointer IPCMRIKernel::generateSurfaceRepresentation(std::vector<mitk::Vector3D> timePointPCMRIVectorsInterpolated, mitk::Surface::Pointer meshSurfaceRepresentation)
	{

		//TODO: pass _meshSurfaceRepresentation for the whole mesh
		vtkPolyData* meshSurfaceOriginal = meshSurfaceRepresentation->GetVtkPolyData();
		vtkNew<vtkPolyData> polyData;
		vtkNew<vtkPoints> points;
		polyData->SetPoints(points.GetPointer());

		polyData->DeepCopy(meshSurfaceOriginal);

		auto nPoints = meshSurfaceOriginal->GetNumberOfPoints();
		for (vtkIdType a = 0; a < meshSurfaceOriginal->GetNumberOfPoints(); a++)
		{
			double templatepoint[3];
			meshSurfaceOriginal->GetPoint(a, templatepoint);

			double newPoint[3];
			if (a < timePointPCMRIVectorsInterpolated.size())
			{
				newPoint[0] = templatepoint[0] + timePointPCMRIVectorsInterpolated[a][0];
				newPoint[1] = templatepoint[1] + timePointPCMRIVectorsInterpolated[a][1];
				newPoint[2] = templatepoint[2] + timePointPCMRIVectorsInterpolated[a][2];

			}
			else
			{
				newPoint[0] = templatepoint[0];
				newPoint[1] = templatepoint[1];
				newPoint[2] = templatepoint[2];
			}

			points->InsertNextPoint(newPoint);
			points->Modified();
		}

		polyData->SetPoints(points.GetPointer());
		auto _surfaceRepresentation = mitk::Surface::New();
		_surfaceRepresentation->SetVtkPolyData(polyData.GetPointer());

		return _surfaceRepresentation;
		
	}

	double computeSubvolume(std::vector<double> velocities, mitk::Point3D point1, mitk::Point3D point2, mitk::Point3D point3, double& areaTotal)
	{
		itk::Vector<double, 3> vec1 = (point2 - point1);
		itk::Vector<double, 3> vec2 = (point3 - point1);
		auto cross = itk::CrossProduct(vec1, vec2);
		auto area = (itk::CrossProduct(vec1, vec2)).GetNorm() / 2;
		areaTotal += area;
		double volume = (velocities[0] + velocities[1] + velocities[2]) * area / 3;
		return volume;
	}

	std::vector<double> IPCMRIKernel::calculateFlowWaveform(boost::multi_array<double, 2> mappedPCMRIvalues, const MeshData::Pointer mesh,
		const FaceIdentifier face)
	{
		std::vector<double> flowWaveform;
		double areaTotal = 0;

		int size = mappedPCMRIvalues.shape()[1];
		int nPoints = mappedPCMRIvalues.shape()[0];
		std::vector<crimson::MeshData::MeshFaceInfo> faceInfo = mesh->crimson::MeshData::getMeshFaceInfoForFace(face);

		//do for every timestep
		for (int i = 0; i < size; i++)
		{
			double volume = 0;

			std::map<int, double> velocitiesMap;
			auto meshNodesFace = mesh->getNodeIdsForFace(face);
			int j = 0;
			for (std::vector<int>::iterator it = meshNodesFace.begin(); it < meshNodesFace.end(); it++)
			{
				velocitiesMap.insert(std::pair<int, double>(*it, mappedPCMRIvalues[j][i]));
				j++;
			}

			//calculate subvolume for every face node
			for (std::vector<crimson::MeshData::MeshFaceInfo>::iterator it = faceInfo.begin(); it < faceInfo.end(); it++) {
				int nodeIdx[3];
				std::vector<double> velocities;
				nodeIdx[0] = (*it).nodeIds[0];
				nodeIdx[1] = (*it).nodeIds[1];
				nodeIdx[2] = (*it).nodeIds[2];
				mitk::Point3D point1, point2, point3;
				point1 = mesh->getNodeCoordinates(nodeIdx[0]);
				point2 = mesh->getNodeCoordinates(nodeIdx[1]);
				point3 = mesh->getNodeCoordinates(nodeIdx[2]);
				//TODO: check what happens when the node has no velocity in the map
				double vel1 = 0, vel2 = 0, vel3 = 0;
				if (velocitiesMap.find(nodeIdx[0]) != velocitiesMap.end())
					vel1 = velocitiesMap.find(nodeIdx[0])->second;
				if (velocitiesMap.find(nodeIdx[1]) != velocitiesMap.end())
					vel2 = velocitiesMap.find(nodeIdx[1])->second;
				if (velocitiesMap.find(nodeIdx[2]) != velocitiesMap.end())
					vel3 = velocitiesMap.find(nodeIdx[2])->second;
				velocities = { vel1, vel2, vel3 };
				volume += computeSubvolume(velocities, point1, point2, point3, areaTotal);
			}

			flowWaveform.push_back(volume);
		}

		return flowWaveform;
	}

} // namespace crimson
