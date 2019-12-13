#ifndef IMAGETOOLS_H_
#define IMAGETOOLS_H_

#include <itkImage.h>
#include <itkImageFileWriter.h>
#include <itkExtractImageFilter.h>
#include <itkMatrix.h>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkMatrix4x4.h>
#include <itkImageRegionIterator.h>
#include <itkImageDuplicator.h>

/// For resampling
#include <itkResampleImageFilter.h>
#include <itkVectorResampleImageFilter.h>
#include <itkIdentityTransform.h>
#include <itkBSplineInterpolateImageFunction.h>
#include <itkLinearInterpolateImageFunction.h>
#include <itkNearestNeighborInterpolateImageFunction.h>
#include <itkVectorLinearInterpolateImageFunction.h>
#include <itkVectorNearestNeighborInterpolateImageFunction.h>
#include <itkDiscreteGaussianImageFilter.h>
#include <itkRecursiveGaussianImageFilter.h>

/// For transform
#include <itkCastImageFilter.h>
#include <itkCenteredAffineTransform.h>



/// For mamory stuff
#ifdef  __linux
#include <unistd.h>
#elif __APPLE__
#include <sys/types.h>
#include <sys/sysctl.h>
//#include <sys/stat.h>
//#include <kstat.h>
#endif

namespace echo
{

/// Some useful constants
enum Interpolator {NN, linear, cuadratic, cubic};

template <typename OutputImageType, typename RefImageType>
typename OutputImageType::Pointer initialiseImage( const typename RefImageType::Pointer model);

template <typename OutputImageType>
typename OutputImageType::Pointer copyImage( const typename OutputImageType::Pointer model);

template <typename OutputImageType>
typename OutputImageType::Pointer initialiseImage(double *size, double *origin, double *spacing, unsigned int ndims);

template <typename OutputImageType>
typename OutputImageType::Pointer initialiseGridImage(unsigned int *size, double *origin, double *spacing, unsigned int ndims, unsigned int *linestep, typename OutputImageType::PixelType value=1);

template <typename InputImageType>
typename InputImageType::Pointer copy(typename InputImageType::Pointer image);

template <typename InputImageType>
typename InputImageType::Pointer downsample(typename InputImageType::Pointer image, unsigned int *downsampling_factor_vector, Interpolator interp);

template <typename InputImageType>
typename InputImageType::Pointer downsampleVector(typename InputImageType::Pointer image, double *downsampling_factor_vector, Interpolator interp);

template <typename InputImageType>
void writeImage(typename InputImageType::Pointer image, std::string &filename);

template <typename InputImageType, typename OutputImageType>
typename OutputImageType::Pointer extractFrame( typename InputImageType::Pointer inputImage, unsigned int nframe, bool debug=false);

template <typename InputImageType>
void replaceFrame( typename InputImageType::Pointer inputImage, unsigned int frame, typename InputImageType::PixelType *values, unsigned int nvalues);

template <typename InputImageType>
void replaceValues( typename InputImageType::Pointer inputImage, typename InputImageType::PixelType *values, unsigned int nvalues);

typedef itk::Matrix<double,4,4> itkMatrix4x4Type;

inline itkMatrix4x4Type readITKMatrix( std::string matrix_filename, vtkSmartPointer<vtkMatrix4x4> vtkMatrix=0, bool debug=false);

inline size_t getAvailableSystemMemory();

template <typename ImageType>
typename ImageType::Pointer transformImage3D(const typename ImageType::Pointer input, const typename ImageType::Pointer ref, const itkMatrix4x4Type matrix, bool inverse=false);

template <typename ImageType>
typename ImageType::Pointer orientateImage3D(const typename ImageType::Pointer input,  const itkMatrix4x4Type matrix, bool inverse=false);

template <typename ImageType, typename ArrayType>
typename ImageType::Pointer imageFromArray(const ArrayType &array, const typename ImageType::Pointer reference);

template <typename ImageType, typename ArrayType>
typename ImageType::Pointer imageFromArray(const ArrayType &array, double *size, double *origin, double *spacing);

template <typename ImageType,  typename PointType, typename ValueType >
void ImageToPointDataset(const typename ImageType::Pointer image,
                         const  vtkSmartPointer<vtkMatrix4x4> matrix_, std::vector<PointType> &points, std::vector<ValueType> &values,
                         bool invert=false);

template <typename ImageType,  typename PointType >
void ImageToPointList(const typename ImageType::Pointer image,
                      std::vector<PointType> &points, const  vtkSmartPointer<vtkMatrix4x4> matrix_=NULL,
                      bool invert=false, double th = -1);

template <typename ImageType >
void calculateBounds(const typename ImageType::Pointer image, std::vector<double> &bounds);


/// Similarity measures

/**
 * @brief ssd
 * @param target
 * @param source
 * @param debug
 * @param sourceMask Must be of the same size as source
 * @param targetMask Must be of same size as target
 * @return
 */
template <typename FixedImageType,  typename MovingImageType >
double ssd(typename FixedImageType::Pointer target, typename  MovingImageType::Pointer source, bool debug=false, typename MovingImageType::Pointer sourceMask=0, typename FixedImageType::Pointer targetMask=0);

/****************************************************************
*                          IMPLEMENTATION                       *
****************************************************************/

template <typename OutputImageType>
typename OutputImageType::Pointer copyImage( const typename OutputImageType::Pointer model){

    typename OutputImageType::Pointer image = OutputImageType::New();
    typename OutputImageType::RegionType region;
    typename OutputImageType::IndexType start;
    start = model->GetLargestPossibleRegion().GetIndex();
    typename OutputImageType::SizeType size;

    size = model->GetLargestPossibleRegion().GetSize();
    region.SetSize(size);
    region.SetIndex(start);

    image->SetRegions(region);
    image->Allocate();

    typename OutputImageType::PixelType p;

    typedef itk::ImageRegionIterator< OutputImageType > IteratorType;
    IteratorType out( image, image->GetRequestedRegion());

    typedef itk::ImageRegionConstIterator< OutputImageType > ConstIteratorType;
    ConstIteratorType in( model, model->GetRequestedRegion());

    for ( in.GoToBegin(), out.GoToBegin(); !in.IsAtEnd(); ++in, ++out)
    {
        p = in.Get();
        out.Set(p);
    }

    image->SetSpacing(model->GetSpacing());
    image->SetOrigin(model->GetOrigin());
    return image;
}

/**
 * Note: this function does not initialise to zero!
 */
template <typename OutputImageType, typename RefImageType>
typename OutputImageType::Pointer initialiseImage( const typename RefImageType::Pointer model)
{

    typename OutputImageType::Pointer image = OutputImageType::New();

    typename OutputImageType::RegionType region;

    typename OutputImageType::IndexType start;

    start = model->GetLargestPossibleRegion().GetIndex();
    typename OutputImageType::SizeType size;

    size = model->GetLargestPossibleRegion().GetSize();
    region.SetSize(size);
    region.SetIndex(start);

    image->SetRegions(region);
    image->Allocate();
    ///Initialise to 0
    typename OutputImageType::PixelType p;
    int nc = image->GetNumberOfComponentsPerPixel();
    if (nc>1){
    #define NDIMENSIONS 2
    } else {
        p=0;
    }

    #ifdef NDIMENSIONS
            p.Fill(0);
    #endif

    image->FillBuffer(p);// initialise to (0,0,0)
    image->SetSpacing(model->GetSpacing());
    image->SetOrigin(model->GetOrigin());
    return image;

}


template <typename InputImageType>
void writeImage(const typename InputImageType::Pointer image, std::string &filename)
{

    typedef itk::ImageFileWriter<InputImageType> T_Writer;
    // Write the result
    typename   T_Writer::Pointer pWriter = T_Writer::New();
    pWriter->SetFileName(filename);
    pWriter->SetInput(image);
    try
    {
        pWriter->Update();
    }
    catch( itk::ExceptionObject & err )
    {
        std::cerr << "ExceptionObject caught when writting to file "<< filename << std::endl;
        std::cerr << err << std::endl;
    }

}


template <typename InputImageType>
typename InputImageType::Pointer copy(typename InputImageType::Pointer image)
{
    typedef itk::ImageDuplicator< InputImageType > DuplicatorType;
    typename DuplicatorType::Pointer duplicator = DuplicatorType::New();
    duplicator->SetInputImage(image);
    duplicator->Update();
    return duplicator->GetModifiableOutput();
}

/**
 * \brief downsamples an image using interpolation.
 * The downsampling also does some blurring prior to downsampling to minimise information loss.
 * \param
 */
template <typename InputImageType>
typename InputImageType::Pointer downsample(typename InputImageType::Pointer image, double *downsampling_factor_vector, Interpolator interp)
{


    // std::string test_filename1= std::string("/media/ag09_local/AGOMEZ_KCL_2/data/patients/HLHS_19Sep2013/output/reconstruction4D_02_03_06_07_08_10_11_12/") + "_DopplerSpherical_view.gipl";
    // echo::writeImage<InputImageType>(image,test_filename1);

    const unsigned int  NDIMS = InputImageType::ImageDimension;

    typedef itk::RecursiveGaussianImageFilter<InputImageType,InputImageType > GaussianFilterType;
    typename GaussianFilterType::Pointer smootherX = GaussianFilterType::New();
    typename GaussianFilterType::Pointer smootherY = GaussianFilterType::New();
    typename GaussianFilterType::Pointer smootherZ = GaussianFilterType::New();
    typename GaussianFilterType::Pointer smootherT = GaussianFilterType::New();

    smootherX->SetInput( image );
    smootherY->SetInput( smootherX->GetOutput() );
    smootherZ->SetInput( smootherY->GetOutput() );
    smootherT->SetInput( smootherZ->GetOutput() );

    const typename InputImageType::SpacingType& inputSpacing = image->GetSpacing();
    const double sigmaX = inputSpacing[0] * (double) downsampling_factor_vector[0]/3.0;
    const double sigmaY = inputSpacing[1] * (double) downsampling_factor_vector[1]/3.0;
    const double sigmaZ = inputSpacing[2] * (double) downsampling_factor_vector[2]/3.0;
    const double sigmaT = inputSpacing[3] * (double) downsampling_factor_vector[3]/3.0;
    smootherX->SetSigma( sigmaX );
    smootherY->SetSigma( sigmaY );
    smootherZ->SetSigma( sigmaZ );
    smootherT->SetSigma( sigmaT );

    smootherX->SetDirection( 0 );
    smootherY->SetDirection( 1 );
    smootherZ->SetDirection( 2 );
    smootherT->SetDirection( 3 );
    smootherX->SetNormalizeAcrossScale( false );
    smootherY->SetNormalizeAcrossScale( false );
    smootherZ->SetNormalizeAcrossScale( false );
    smootherT->SetNormalizeAcrossScale( false );

    typedef itk::ResampleImageFilter<InputImageType, InputImageType > ResampleFilterType;
    typename ResampleFilterType::Pointer resampler = ResampleFilterType::New();

    typedef itk::IdentityTransform< double, NDIMS > TransformType;
    typename TransformType::Pointer transform = TransformType::New();
    transform->SetIdentity();
    resampler->SetTransform( transform );

    if (interp==NN)
    {
        typedef itk::NearestNeighborInterpolateImageFunction<InputImageType, double>    InterpolatorType;
        typename InterpolatorType::Pointer _pInterpolator = InterpolatorType::New();
        resampler->SetInterpolator(_pInterpolator);

    }
    else if (interp==linear)
    {
        typedef itk::LinearInterpolateImageFunction<InputImageType, double>    InterpolatorType;
        typename InterpolatorType::Pointer _pInterpolator = InterpolatorType::New();
        resampler->SetInterpolator(_pInterpolator);

    }
    else if (interp==cubic)
    {
        typedef itk::BSplineInterpolateImageFunction<InputImageType, double, double>    InterpolatorType;
        typename InterpolatorType::Pointer _pInterpolator = InterpolatorType::New();
        _pInterpolator->SetSplineOrder(3);
        resampler->SetInterpolator(_pInterpolator);
    }


    typename InputImageType::SpacingType spacing;
    spacing[0] = inputSpacing[0] * (double) downsampling_factor_vector[0];
    spacing[1] = inputSpacing[1] * (double) downsampling_factor_vector[1];
    spacing[2] = inputSpacing[2] * (double) downsampling_factor_vector[2];
    spacing[3] = inputSpacing[3] * (double) downsampling_factor_vector[3];
    resampler->SetOutputSpacing( spacing );

    resampler->SetOutputOrigin( image->GetOrigin() );
    resampler->SetOutputDirection( image->GetDirection() );

    typename InputImageType::SizeType inputSize =image->GetLargestPossibleRegion().GetSize();
    typedef typename InputImageType::SizeType::SizeValueType SizeValueType;
    typename InputImageType::SizeType size;
    size[0] = static_cast< SizeValueType >( inputSize[0] / (double) downsampling_factor_vector[0] );
    size[1] = static_cast< SizeValueType >( inputSize[1] / (double) downsampling_factor_vector[1] );
    size[2] = static_cast< SizeValueType >( inputSize[2] / (double) downsampling_factor_vector[2] );
    size[3] = static_cast< SizeValueType >( inputSize[3] / (double) downsampling_factor_vector[3] );
    resampler->SetSize( size );

    resampler->SetInput( smootherT->GetOutput() );

    try{
        resampler->Update();
    } catch (itk::ExceptionObject &e){
        std::cout << "Exception caught in echo::downsample, "<< e<< std::endl;
        std::cout << std::endl;
        exit(-1);
    }

    // std::string test_filename2= std::string("/media/ag09_local/AGOMEZ_KCL_2/data/patients/HLHS_19Sep2013/output/reconstruction4D_02_03_06_07_08_10_11_12/") + "_DopplerSpherical_resampled_view.gipl";
    // echo::writeImage<InputImageType>(resampler->GetOutput(),test_filename2);

    return resampler->GetOutput();

}



/**
 * \brief downsamples an image using interpolation.
 * The downsampling also does some blurring prior to downsampling to minimise information loss.
 * \param
 */
template <typename InputImageType>
typename InputImageType::Pointer downsampleVector(typename InputImageType::Pointer image, double *downsampling_factor_vector, Interpolator interp)
{

    typedef itk::VectorResampleImageFilter<InputImageType,InputImageType> ResampleFilterType;
    const unsigned int  NDIMS = InputImageType::ImageDimension;
    typedef itk::IdentityTransform<double , NDIMS> TransformType;
    typedef itk::DiscreteGaussianImageFilter< InputImageType, InputImageType >  BlurringFilterType;

    //std::string test_filename1= output_prefix + "_DopplerSpherical_view" +echo::str::toString<int>(j)+".mhd";
    //echo::writeImage<Doppler3ImageType>(dopplerframe,test_filename1);

    typename TransformType::Pointer _pTransform = TransformType::New();
    _pTransform->SetIdentity();

    typename ResampleFilterType::Pointer _pResizeFilter = ResampleFilterType::New();
    _pResizeFilter->SetTransform(_pTransform);

    if (interp==NN)
    {
        typedef itk::VectorNearestNeighborInterpolateImageFunction<InputImageType, double>    InterpolatorType;
        typename InterpolatorType::Pointer _pInterpolator = InterpolatorType::New();
        _pResizeFilter->SetInterpolator(_pInterpolator);

    }
    else if (interp==linear)
    {
        typedef itk::VectorLinearInterpolateImageFunction<InputImageType, double>    InterpolatorType;
        typename InterpolatorType::Pointer _pInterpolator = InterpolatorType::New();
        _pResizeFilter->SetInterpolator(_pInterpolator);

    }
    else if (interp==cubic)
    {
        //        typedef itk::VectorBSplineInterpolateImageFunction<InputImageType, double, double>    InterpolatorType;
        //        typename InterpolatorType::Pointer _pInterpolator = InterpolatorType::New();
        //        _pInterpolator->SetSplineOrder(3);
        //        _pResizeFilter->SetInterpolator(_pInterpolator);
    }

    _pResizeFilter->SetOutputOrigin(image->GetOrigin());
    typename InputImageType::PointType outputOrigin = image->GetOrigin();

    typename InputImageType::SpacingType inputSpacing =    image->GetSpacing(), outputSpacing;
    itk::Size<NDIMS> inputSize = image->GetLargestPossibleRegion().GetSize(), outputSize;
    double variance[InputImageType::ImageDimension];
    double max_error[InputImageType::ImageDimension];
    double max_downsampling_factor=0;
    for (int i=0; i<NDIMS; i++)
    {
        if (max_downsampling_factor<downsampling_factor_vector[i])
        {
            max_downsampling_factor = downsampling_factor_vector[i];
        }
        outputSpacing[i] = inputSpacing[i] * (double) downsampling_factor_vector[i];
        outputSize[i] = std::ceil((double) inputSize[i]*inputSpacing[i]/outputSpacing[i]);
        variance[i]=outputSpacing[i]/2.0;
        max_error[i]=std::min(downsampling_factor_vector[i]/inputSize[i],0.9999999);
    }

    _pResizeFilter->SetOutputSpacing(outputSpacing);
    _pResizeFilter->SetSize(outputSize);
    _pResizeFilter->SetOutputOrigin(outputOrigin);

    // Specify the input.

    typename BlurringFilterType::Pointer blurringFilter = BlurringFilterType::New();
    blurringFilter->SetInput( image);
    blurringFilter->SetVariance(variance);
    blurringFilter->SetUseImageSpacingOn();
    blurringFilter->SetMaximumKernelWidth(std::max((int) std::ceil(max_downsampling_factor),32));

    blurringFilter->SetMaximumError(max_error);
    _pResizeFilter->SetInput(blurringFilter->GetOutput());
    try{
        _pResizeFilter->Update();
    } catch (itk::ExceptionObject &e){
        std::cout << "Exception caught in echo::downsample, "<< e<< std::endl;

        std::cout << "Variance was ";
        for (int i=0; i<NDIMS; i++)
        {
            std::cout << variance[i]<<"  ";
        }
        std::cout << "Max error was ";
        for (int i=0; i<NDIMS; i++)
        {
            std::cout << max_error[i]<<"  ";
        }
        std::cout << std::endl;
        exit(-1);
    }

    return _pResizeFilter->GetOutput();

}


/**
 * Orientate an image using a matrix.
 */
template <typename ImageType>
typename ImageType::Pointer orientateImage3D(const typename ImageType::Pointer input, const itkMatrix4x4Type matrix, bool inverse)
{

    typedef itk::CastImageFilter<ImageType, ImageType> DummyFilerType;
    typename DummyFilerType::Pointer dummyFilter = DummyFilerType::New();
    dummyFilter->SetInput(input);
    dummyFilter->Update();
    typename ImageType::Pointer output  = dummyFilter->GetOutput();

    typename ImageType::DirectionType orientation;

    /// Read matrix
    typedef itk::Matrix<double,4,4> itkMatrix4x4Type;

    itkMatrix4x4Type imageMatrix;
    imageMatrix.SetIdentity();
    typename ImageType::DirectionType imageorientation = output->GetDirection();

    for (int i=0; i<3; i++)
        for (int j=0; j<3; j++)
            for (int k=0; k<3; k++)
                imageMatrix[i][j]=imageorientation[i][j];

    imageMatrix[0][3]=output->GetOrigin()[0];
    imageMatrix[1][3]=output->GetOrigin()[1];
    imageMatrix[2][3]=output->GetOrigin()[2];

    itkMatrix4x4Type matrix_use;
    if (inverse)
        matrix_use=  matrix;
    else
        matrix_use=  itkMatrix4x4Type(matrix.GetInverse());

    itkMatrix4x4Type orientationMatrix;
    orientationMatrix = matrix_use * imageMatrix;

    orientation.SetIdentity();
    for (int i=0; i<3; i++)
        for (int j=0; j<3; j++)
            for (int k=0; k<3; k++)
                orientation[i][j] = orientationMatrix[i][j];

    typename ImageType::PointType origin_old = output->GetOrigin();
    typename ImageType::PointType origin_new;

    origin_new[0] =  matrix_use[0][0]*origin_old[0]+ matrix_use[0][1]*origin_old[1]+ matrix_use[0][2]*origin_old[2]+1*matrix_use[0][3];
    origin_new[1] =  matrix_use[1][0]*origin_old[0]+ matrix_use[1][1]*origin_old[1]+ matrix_use[1][2]*origin_old[2]+1*matrix_use[1][3];
    origin_new[2] =  matrix_use[2][0]*origin_old[0]+ matrix_use[2][1]*origin_old[1]+ matrix_use[2][2]*origin_old[2]+1*matrix_use[2][3];

    output->SetDirection(orientation);
    output->SetOrigin(origin_new);

    return output;
}



/**
 * Transform an image using a matrix and a reference image,
 * This method is hardcoded to perform linear interpolation
 */
template <typename ImageType>
typename ImageType::Pointer transformImage3D(const typename ImageType::Pointer input, const typename ImageType::Pointer ref, const itkMatrix4x4Type matrix, bool inverse)
{


    typedef itk::CenteredAffineTransform<double, 3> CenteredTransformType;
    CenteredTransformType::Pointer transform = CenteredTransformType::New();
    typedef itk::Matrix<double,3,3> itkMatrix3x3Type;
    itkMatrix3x3Type matrix2;
    itkMatrix3x3Type matrix2inv;

    for (int i=0; i<3; i++)
        for (int j=0; j<3; j++)
        {
            matrix2[i][j]=matrix[i][j];
            matrix2inv[i][j] = matrix.GetInverse()[i][j];
        }

    CenteredTransformType::OffsetType offset2;
    CenteredTransformType::OffsetType offset2inv;
    for (int i = 0; i < 3; i++)
    {
        offset2inv[i] = matrix.GetInverse()[i][3];
        offset2[i] = matrix[i][3];
    }

    if(inverse)
    {
        transform->SetMatrix( matrix2inv);
        transform->SetOffset(offset2inv);
    }
    else
    {
        // note that this matrix maps target to source!
        transform->SetMatrix( matrix2);
        transform->SetOffset(offset2);
    }

    typedef itk::ResampleImageFilter<ImageType, ImageType>		ResampleFilterType;
    typename ResampleFilterType::Pointer transformer =	ResampleFilterType::New();
    transformer->SetTransform(transform);
    transformer->SetInput(input);
    typedef itk::LinearInterpolateImageFunction<ImageType, double> InterpolatorType;
    typename InterpolatorType::Pointer interpolator = InterpolatorType::New();
    transformer->SetInterpolator(interpolator);

    transformer->SetReferenceImage(ref);
    transformer->UseReferenceImageOn ();

    try
    {
        transformer->Update();
    }
    catch (itk::ExceptionObject & err)
    {
        std::cerr << "ExceptionObject caught !" << std::endl;
        std::cerr << err << std::endl;
    }

    return  transformer->GetOutput();


}

template <typename OutputImageType>
typename OutputImageType::Pointer initialiseImage(unsigned int *size, double *origin, double *spacing, unsigned int ndims){

    typename OutputImageType::Pointer image = OutputImageType::New();
    typename OutputImageType::RegionType region;
    typename OutputImageType::IndexType start;
    typename OutputImageType::SizeType size_;
    for (int i=0; i<ndims; i++){
        start[i] =0 ;
        size_[i]=size[i];
    }

    region.SetSize(size_);
    region.SetIndex(start);

    image->SetRegions(region);
    image->Allocate();
    image->SetSpacing(spacing);
    image->SetOrigin(origin);

    return image;
}

template <typename OutputImageType>
typename OutputImageType::Pointer initialiseGridImage(unsigned int *size, double *origin, double *spacing, unsigned int ndims, unsigned int *linestep, typename OutputImageType::PixelType value){

    typename OutputImageType::Pointer image = initialiseImage<OutputImageType>(size, origin,spacing, ndims);

    // Make the lines
    typedef itk::ImageRegionIterator< OutputImageType > IteratorType;
    IteratorType in( image, image->GetRequestedRegion());

    typename OutputImageType::PixelType Zero(0);
    typename OutputImageType::IndexType currentIndex;


    for ( in.GoToBegin(); !in.IsAtEnd(); ++in)
    {
        currentIndex = in.GetIndex();
        bool iszero=true;
        double rem;
        for (int i=0; i<OutputImageType::GetImageDimension(); i++){
            rem  = currentIndex[i] % linestep[i];
            if (!rem){
                iszero=false;
                break;
            }
        }

        if (iszero){
            in.Set(Zero);
        } else {
            in.Set(value);
        }


    }

    return image;
}



template <typename ImageType, typename ArrayType>
typename ImageType::Pointer imageFromArray(const ArrayType &array, const typename ImageType::Pointer reference){


    typedef itk::CastImageFilter<ImageType, ImageType> DummyFilerType;
    typename DummyFilerType::Pointer dummyFilter = DummyFilerType::New();
    dummyFilter->SetInput(reference);
    dummyFilter->Update();
    typename ImageType::Pointer output = dummyFilter->GetOutput();

    typedef itk::ImageRegionIterator< ImageType > IteratorType;
    IteratorType in( output, output->GetRequestedRegion());

    int idx=0;
    for ( in.GoToBegin(); !in.IsAtEnd(); ++in)
    {
        in.Set(array[idx++]) ;
    }

    return output;

}

template <typename ImageType, typename ArrayType>
typename ImageType::Pointer imageFromArray(const ArrayType &array, double *size_, double *origin_, double *spacing_){

    typename ImageType::Pointer image = ImageType::New();
    typename ImageType::RegionType region;
    typename ImageType::IndexType start;

    int ndims = ImageType::GetImageDimension();
    typename ImageType::SizeType size;
    for (int i=0; i<ndims; i++){
        start[i] =0 ;
        size[i]=size_[i];
    }

    region.SetSize(size);
    region.SetIndex(start);

    image->SetRegions(region);
    image->Allocate();
    image->SetSpacing(spacing_);
    image->SetOrigin(origin_);

    typename ImageType::Pointer outputImage = imageFromArray<ImageType, ArrayType>(array, image);

    return outputImage;

}


template <typename InputImageType, typename OutputImageType>
typename OutputImageType::Pointer extractFrame( typename InputImageType::Pointer inputImage, unsigned int nframe, bool debug)
{

    /// extract frame
    typedef itk::ExtractImageFilter<InputImageType,OutputImageType> ExtractFilterType;
    typename ExtractFilterType::Pointer extractFilter = ExtractFilterType::New();

    typename InputImageType::RegionType roi;
    typename InputImageType::SizeType size=inputImage->GetLargestPossibleRegion().GetSize();
    size[3]=0;
    typename InputImageType::IndexType start= {0,0,0,nframe};

    roi.SetIndex(start);
    roi.SetSize(size);

    if (debug)
    {
        std::cout << "\tIndex: "<<start<<std::endl;
        std::cout << "\tSize: "<<size<<std::endl;
        std::cout << "\tRoi: "<<roi<<std::endl;
    }

    extractFilter->SetExtractionRegion(roi);
    extractFilter->SetInput(inputImage);
    extractFilter->SetDirectionCollapseToSubmatrix();

    try
    {
        extractFilter->Update();
    }
    catch( itk::ExceptionObject & err )
    {
        std::cerr << "ExceptionObject caught in extracter !" << std::endl;
        std::cerr << err << std::endl;
        return 0;
    }

    return extractFilter->GetOutput();


}

/**
 * readITKMatrix
 * read a matrix file into ITK matrix (and optionally vtk matrix file too)
 */

itkMatrix4x4Type readITKMatrix( std::string matrix_filename, vtkSmartPointer<vtkMatrix4x4> vtkMatrix, bool debug)
{
    itkMatrix4x4Type matrix;
    std::fstream filereader;
    filereader.open(matrix_filename.c_str(), std::fstream::in);
    std::string dummy;
    /// First line should be # itkMatrix 4 x 4
    filereader >> dummy >> dummy >> dummy >> dummy >> dummy;
    filereader >> matrix[0][0] >> matrix[0][1]>> matrix[0][2] >> matrix[0][3];
    filereader >> matrix[1][0] >> matrix[1][1]>> matrix[1][2] >> matrix[1][3];
    filereader >> matrix[2][0] >> matrix[2][1]>> matrix[2][2] >> matrix[2][3];
    filereader >> matrix[3][0] >> matrix[3][1]>> matrix[3][2] >> matrix[3][3];
    filereader.close();

    if (vtkMatrix)
    {
        if (debug)
        {
            std::cout << "calculate also vtkMatrix"<<std::endl;
        }
        for (int i=0; i<4; i++)
            for (int j=0; j<4; j++)
            {
                vtkMatrix->SetElement(i,j,matrix[i][j]);
            }
    }

    return matrix;
}

/**
 * readITKMatrix
 * read a matrix file into ITK matrix (and optionally vtk matrix file too)
 */


inline void writeITKMatrix( std::string matrix_filename, itkMatrix4x4Type &matrix){
    std::fstream filewriter;
    filewriter.open(matrix_filename.c_str(), std::fstream::out);

    /// First line should be # itkMatrix 4 x 4
    filewriter <<"# itkMatrix 4 x 4" <<std::endl;
    for (int i=0; i<4; i++){
        for (int j=0; j<4; j++){
            filewriter << matrix[i][j] << "\t";
        }
        filewriter << std::endl;
    }
    filewriter.close();
}

inline void writeITKMatrix( std::string matrix_filename, vtkSmartPointer<vtkMatrix4x4> matrix)
{
    std::fstream filewriter;
    filewriter.open(matrix_filename.c_str(), std::fstream::out);

    /// First line should be # itkMatrix 4 x 4
    filewriter <<"# itkMatrix 4 x 4" <<std::endl;
    for (int i=0; i<4; i++){
        for (int j=0; j<4; j++){
            filewriter << matrix->GetElement(i,j);
            if (i<3)
                filewriter << "\t";
        }
        filewriter << std::endl;
    }
    filewriter.close();
}

/**
 * getTotalSystemMemory()
 * Returns the amout of available memory, in Bytes
 */

size_t getAvailableSystemMemory()
{

#ifdef __linux
    long pages = sysconf(_SC_AVPHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
#elif __APPLE__
    //    /// From https://github.com/i-scream/libstatgrab/blob/master/src/libstatgrab/memory_stats.c
    //    kstat_ctl_t *kc;
    //    kstat_t *ksp;
    //    kstat_named_t *kn;
    //    if( (kc = kstat_open()) == NULL ) {
    //            RETURN_WITH_SET_ERROR("mem", SG_ERROR_KSTAT_OPEN, NULL);
    //        }

    //        if((ksp=kstat_lookup(kc, "unix", 0, "system_pages")) == NULL) {
    //            RETURN_WITH_SET_ERROR("mem", SG_ERROR_KSTAT_LOOKUP, "unix,0,system_pages");
    //        }

    //        if (kstat_read(kc, ksp, 0) == -1) {
    //            RETURN_WITH_SET_ERROR("mem", SG_ERROR_KSTAT_READ, NULL);
    //        }

    //        if((kn=kstat_data_lookup(ksp, "physmem")) == NULL) {
    //            RETURN_WITH_SET_ERROR("mem", SG_ERROR_KSTAT_DATA_LOOKUP, "physmem");
    //        }

    //        mem_stats_buf->total = ((unsigned long long)kn->value.ul) * ((unsigned long long)sys_page_size);

    //    if((kn=kstat_data_lookup(ksp, "freemem")) == NULL) {
    //            RETURN_WITH_SET_ERROR("mem", SG_ERROR_KSTAT_DATA_LOOKUP, "freemem");
    //        }
    //    mem_stats_buf->free = ((unsigned long long)kn->value.ul) * ((unsigned long long)sys_page_size);
    //        kstat_close(kc);

    ///
    int mib[2] = { CTL_HW, HW_MEMSIZE };
    u_int namelen = sizeof(mib) / sizeof(mib[0]);
    uint64_t size;
    size_t len = sizeof(size);

    if (sysctl(mib, namelen, &size, &len, NULL, 0) < 0)
    {
        perror("sysctl");
        return 0;
    }
    else
    {
        // printf("HW.HW_MEMSIZE = %llu bytes\n", size);


        // std::cerr << "Image_tools.hxx::getAvailableSystemMemory() -- Memory calculation should be implementd for MAC OS"<<std::endl;
        return  (size_t) ( (double) size*0.8); /// Hardcoded, return half the total physical memory
    }


#endif

	return 1024*1024*1024;
}

/**
 * convinience functions sub2ind ind2sub
 */

template<class T>
unsigned int nd_to_oned_index(const T *index, const T *size, unsigned int InputDimension)
{
    unsigned int index1D=0;
    for (int i=0; i<InputDimension; i++)
    {
        unsigned int dimensions_tmp=1;
        for (int j=0; j<i; j++)
        {
            dimensions_tmp*=size[j];
        }
        index1D+=index[i]*dimensions_tmp;
    }
    return index1D;
}

/**
 * convinience functions sub2ind ind2sub
 */

template<class IndexType>
unsigned int nd_to_oned_index(const IndexType &index, const IndexType &size, unsigned int InputDimension)
{
    unsigned int index1D=0;
    for (int i=0; i<InputDimension; i++)
    {
        unsigned int dimensions_tmp=1;
        for (int j=0; j<i; j++)
        {
            dimensions_tmp*=size[j];
        }
        index1D+=index[i]*dimensions_tmp;
    }
    return index1D;
}

template <class IndexType>
void oned_to_nd_index(unsigned int index1D, IndexType &index, const IndexType &size, unsigned int InputDimension)
{
    unsigned int remainder=index1D;
    for (int i=InputDimension-1; i>=0; i--)
    {
        unsigned int dimensions_tmp=1;
        for (int j=0; j<i; j++)
        {
            dimensions_tmp*=size[j];
        }
        index[i] = std::floor((double) remainder/dimensions_tmp);
        remainder -= dimensions_tmp*index[i];
    }
}

template <typename InputImageType>
void replaceFrame( typename InputImageType::Pointer inputImage, unsigned int frame, std::vector<typename InputImageType::PixelType> &values, unsigned int nvalues)
{

    unsigned int ndims = InputImageType::ImageDimension;

    typedef typename InputImageType::IndexType IndexType;
    IndexType regionIndex, size2;
    typename InputImageType::SizeType regionSize = inputImage->GetLargestPossibleRegion().GetSize();
    for (int i=0; i<ndims; i++)
        size2[i] = regionSize[i];

    regionSize[ndims-1]=1;

    unsigned int idx=0;
    for (idx=0; idx<nvalues; idx++)
    {
        oned_to_nd_index<IndexType>(idx,regionIndex,size2,ndims);
        regionIndex[ndims-1]=frame;
        inputImage->SetPixel(regionIndex, values[idx]);

    }
}

template <typename InputImageType>
void replaceValues( typename InputImageType::Pointer inputImage, std::vector<typename InputImageType::PixelType> &values)
{

    unsigned int ndims = InputImageType::ImageDimension;
    typedef typename InputImageType::IndexType IndexType;
    IndexType regionIndex, size;
    typename InputImageType::SizeType regionSize = inputImage->GetLargestPossibleRegion().GetSize();

    for (int i=0; i<ndims; i++)
        size[i] = regionSize[i];

    unsigned int idx=0;
    for (idx=0; idx<values.size(); idx++)
    {
        oned_to_nd_index<IndexType>(idx,regionIndex,size,ndims);
        //regionIndex[ndims-1]=frame;
        inputImage->SetPixel(regionIndex, values[idx]);

    }
}




template <class T>
bool pair_compare(std::pair<T,T> i,std::pair<T,T> j)
{
    return (i.first<j.first);
}


template <typename ImageType,  typename PointType, typename ValueType >
void VectorImageToPointDataset(const typename ImageType::Pointer image,
                               const  vtkSmartPointer<vtkMatrix4x4> matrix_, std::vector<PointType> &points, std::vector<ValueType> &values,
                               bool invert=0){

    typename ImageType::SizeType size = image->GetLargestPossibleRegion().GetSize();

    int npoints =1;
    for (int i =0; i<ImageType::ImageDimension; i++){
        npoints*=size[i];
    }
    points.reserve(npoints);
    values.reserve(npoints);

    vtkSmartPointer<vtkMatrix4x4> matrix = vtkSmartPointer<vtkMatrix4x4>::New();
    if (invert){
        matrix->Invert(matrix_,matrix);
    }else{
        matrix->DeepCopy(matrix_);
    }

    typedef itk::ImageRegionConstIterator< ImageType > IteratorType;
    IteratorType iterator( image	, image->GetRequestedRegion() );
    typename ImageType::IndexType imageIndex;
    typename ImageType::PointType imageCoordinates;
    typename ImageType::PixelType pixel;

    for ( iterator.GoToBegin(); !iterator.IsAtEnd(); ++iterator )
    {
        imageIndex = iterator.GetIndex();
        image->TransformIndexToPhysicalPoint(imageIndex,imageCoordinates);
        /// Transform to cartesian coordinates

        const double input_p[]={imageCoordinates[0], imageCoordinates[1], imageCoordinates[2], 1.0}; /// Homogeneous coordinates
        double output_p[4];

        matrix->MultiplyPoint(input_p,output_p); /// Todo check if this is the correct orientation or if I have to invert the thing

        PointType point(&(output_p[0]));

        point[ImageType::ImageDimension-1]=imageCoordinates[ImageType::ImageDimension-1]; /// Copy the temporal coordinate

        points.push_back(point);
        pixel = iterator.Get();
        ValueType val;
        for (int i=0; i<val.GetDimensions() ; i++)
            val[i]=pixel[i];
        //((typename PointType::ValueType *) &(pixel[0]));
        values.push_back(val);
    }

}

template <typename ImageType,  typename PointType, typename ValueType >
void ImageToPointDataset(const typename ImageType::Pointer image,
                         const  vtkSmartPointer<vtkMatrix4x4> matrix_, std::vector<PointType> &points, std::vector<ValueType> &values,
                         bool invert){

    typename ImageType::SizeType size = image->GetLargestPossibleRegion().GetSize();

    int npoints =1;
    for (int i =0; i<ImageType::ImageDimension; i++){
        npoints*=size[i];
    }
    points.reserve(npoints);
    values.reserve(npoints);

    vtkSmartPointer<vtkMatrix4x4> matrix = vtkSmartPointer<vtkMatrix4x4>::New();
    if (invert){
        matrix->Invert(matrix_,matrix);
    }else{
        matrix->DeepCopy(matrix_);
    }

    typedef itk::ImageRegionConstIterator< ImageType > IteratorType;
    IteratorType iterator( image	, image->GetRequestedRegion() );
    typename ImageType::IndexType imageIndex;
    typename ImageType::PointType imageCoordinates;
    typename ImageType::PixelType pixel;

    for ( iterator.GoToBegin(); !iterator.IsAtEnd(); ++iterator )
    {
        imageIndex = iterator.GetIndex();
        image->TransformIndexToPhysicalPoint(imageIndex,imageCoordinates);
        /// Transform to cartesian coordinates

        const double input_p[]={imageCoordinates[0], imageCoordinates[1], imageCoordinates[2], 1.0}; /// Homogeneous coordinates
        double output_p[4];

        matrix->MultiplyPoint(input_p,output_p); /// Todo check if this is the correct orientation or if I have to invert the thing

        PointType point(&(output_p[0]));

        point[ImageType::ImageDimension-1]=imageCoordinates[ImageType::ImageDimension-1]; /// Copy the temporal coordinate

        points.push_back(point);

        pixel = iterator.Get();
        //ValueType val( (typename PointType::ValueType *) &(pixel[0]));
        ValueType val = iterator.Get();
        values.push_back(val);
    }

}

template <typename ImageType,  typename PointType >
void ImageToPointList(const typename ImageType::Pointer image,
                      std::vector<PointType> &points, const  vtkSmartPointer<vtkMatrix4x4> matrix_, bool invert, double th){

    typename ImageType::SizeType size = image->GetLargestPossibleRegion().GetSize();

    int npoints =1;
    for (int i =0; i<ImageType::GetImageDimension(); i++){
        npoints*=size[i];
    }
    points.reserve(npoints);


    typedef itk::ImageRegionConstIterator< ImageType > IteratorType;
    IteratorType iterator( image	, image->GetRequestedRegion() );
    typename ImageType::IndexType imageIndex;
    typename ImageType::PointType imageCoordinates;
    typename ImageType::PixelType pixel;

    vtkSmartPointer<vtkMatrix4x4> matrix;
    if (matrix_){
        matrix = vtkSmartPointer<vtkMatrix4x4>::New();
        if (invert){
            matrix->Invert(matrix_,matrix);
        }else{
            matrix->DeepCopy(matrix_);
        }
    }

    for ( iterator.GoToBegin(); !iterator.IsAtEnd(); ++iterator )
    {
        if (iterator.Get()>th){
            imageIndex = iterator.GetIndex();
            image->TransformIndexToPhysicalPoint(imageIndex,imageCoordinates);
            /// Transform to cartesian coordinates

            const double input_p[]={imageCoordinates[0], imageCoordinates[1], imageCoordinates[2], 1.0}; /// Homogeneous coordinates
            double output_p[ImageType::GetImageDimension()];

            PointType point(&(output_p[0]));

            if (matrix_){
                matrix->MultiplyPoint(input_p,output_p); /// Todo check if this is the correct orientation or if I have to invert the thing
                point = PointType(&(output_p[0]));
            }else{
                point = PointType(&(input_p[0]));
            }

            point[ImageType::GetImageDimension()-1]=imageCoordinates[ImageType::GetImageDimension()-1]; /// Copy the temporal coordinate
            points.push_back(point);
        }

    }

}


template <typename FixedImageType,  typename MovingImageType >
double ssd(typename FixedImageType::Pointer target, typename MovingImageType::Pointer source, bool debug, typename MovingImageType::Pointer sourceMask, typename FixedImageType::Pointer targetMask){

    /// TODO add masks and divide by the number of voxels

    typedef itk::ImageRegionConstIterator< FixedImageType > FixedIteratorType;
    typedef itk::ImageRegionConstIterator< MovingImageType > MovingIteratorType;
    FixedIteratorType fixedIterator( target	, target->GetRequestedRegion() );
    MovingIteratorType movingIterator( source	, source->GetRequestedRegion() );

    double ssd=0;

    double v1, v2, d;

    if (targetMask && !sourceMask){
        FixedIteratorType fixedMaskIterator( targetMask	, targetMask->GetRequestedRegion() );
        for ( movingIterator.GoToBegin(), fixedIterator.GoToBegin(), fixedMaskIterator.GoToBegin();
              !fixedMaskIterator.IsAtEnd(); ++fixedIterator, ++movingIterator, ++fixedMaskIterator )
        {
            if (fixedMaskIterator.Get()){
                v1 = fixedIterator.Get();
                v2 = movingIterator.Get();
                d = v1-v2;
                ssd += d*d;
            }
        }
    } else if (sourceMask && !targetMask){
        MovingIteratorType movingMaskIterator( sourceMask	, sourceMask->GetRequestedRegion() );
        for ( movingIterator.GoToBegin(), fixedIterator.GoToBegin(), movingMaskIterator.GoToBegin();
              !movingMaskIterator.IsAtEnd(); ++fixedIterator, ++movingIterator, ++movingMaskIterator )
        {
            if (movingMaskIterator.Get()){
                v1 = fixedIterator.Get();
                v2 = movingIterator.Get();
                d = v1-v2;
                ssd += d*d;
            }
        }
    } else if (sourceMask && targetMask){
        FixedIteratorType fixedMaskIterator( targetMask	, targetMask->GetRequestedRegion() );
        MovingIteratorType movingMaskIterator( sourceMask	, sourceMask->GetRequestedRegion() );
        for ( movingIterator.GoToBegin(), fixedIterator.GoToBegin(), movingMaskIterator.GoToBegin(), fixedMaskIterator.GoToBegin();
              !movingMaskIterator.IsAtEnd(); ++fixedIterator, ++movingIterator, ++movingMaskIterator, ++fixedMaskIterator )
        {
            if (movingMaskIterator.Get() && fixedMaskIterator.Get()){
                v1 = fixedIterator.Get();
                v2 = movingIterator.Get();
                d = v1-v2;
                ssd += d*d;
            }
        }
    } else {
        for ( movingIterator.GoToBegin(), fixedIterator.GoToBegin(); !movingIterator.IsAtEnd(); ++fixedIterator, ++movingIterator )
        {
            v1 = fixedIterator.Get();
            v2 = movingIterator.Get();
            d = v1-v2;
            ssd += d*d;
        }
    }

    return ssd;
}

template <typename ImageType >
void calculateBounds(const typename ImageType::Pointer image, std::vector<double> &bounds){

    /// Get the corner points and convert to physical coordinates
    typename ImageType::SizeType size = image->GetLargestPossibleRegion().GetSize();
    unsigned int ndims = ImageType::GetImageDimension();

    int n_indices = std::pow(2,ndims);
    std::vector< std::vector<double> > positions(ndims);


    /// Calculate the indices starting from 0 and then subtract the first.
    std::vector<unsigned int> index(ndims), nvalues(ndims);


    for (int i=0; i<ndims; i++){
        nvalues[i]=2;
    }

    for (unsigned int i=0; i<n_indices; i++)
    {
        echo::oned_to_nd_index<std::vector<unsigned int> >(i,index, nvalues, ndims);
        typename ImageType::IndexType idx;
        for (int j=0; j<ndims; j++){
            idx[j]=index[j]*(size[j]-1);
        }
        /// Convert to physical coordinates
        typename ImageType::PointType pt;
        image->TransformIndexToPhysicalPoint(idx,pt);

        for (int k=0; k<ndims; k++){
            positions[k].push_back(pt[k]);
        }
    }

    bounds.resize(2*ndims);
    /// Find min/max
    for (int i=0; i<ndims; i++){
        bounds[i*ndims]=*std::min_element(positions[i].begin(), positions[i].end()); // min of positions[i]
        bounds[i*ndims+1]=*std::max_element(positions[i].begin(), positions[i].end()); // max of positions[i]

    }



}

}

#endif /// IMAGETOOLS_H_
