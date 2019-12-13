#ifndef BSPLINEGRID_H_
#define BSPLINEGRID_H_

// -------------------------------------------------------------------------
/**
*     \class                  BsplineGrid
*     \brief                  This is a class of a container of a n-D grid
*
*
*     \author                 Alberto G&oacute;mez <alberto.gomez@kcl.ac.uk>
*     \date             October 2013
*     \note             Copyright (c) King's College London, The Rayne Institute.
*                             Division of Imaging Sciences, 4th floow Lambeth Wing, St Thomas' Hospital.
*                             All rights reserved.
*/
// -------------------------------------------------------------------------

#include <vector>
#include <iostream>
#include <cmath>
#include <string>
#include <functional>

/// Convenience tools
#include "Image_tools.hxx"
#include "Point_tools.hxx"
#include "Matrix_tools.hxx"
#include "Mesh_tools.hxx"
#include "String_tools.hxx"
#include "BSplines.hxx"
#include "myTimer.hxx"

/// Matrix
#include <Eigen/Sparse>
#include <Eigen/Dense>

/// Smart Pointers from boost library
#include <boost/shared_ptr.hpp>
/// Multithreading from boost library
#include <boost/thread.hpp>

/// I/O
#include <itkImage.h>
#include <itkVector.h>
#include <itkImageFileWriter.h>
#include <itkImageRegionIterator.h>
#include <itkImageRegionConstIterator.h>
#include <itkVectorLinearInterpolateImageFunction.h>
#include <itkLinearInterpolateImageFunction.h>

#include <vtkStructuredGrid.h>
#include <vtkDataSetWriter.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPassArrays.h>

const double EPSILON = 1E-06;
//const unsigned int OUT_OF_PATCH = std::numeric_limits<unsigned int>::max(); /// Highly unlikely that we will have this node index

struct BSGThreadConfig {
    unsigned int cyclic_dimension;
    int derivative_order;
};


/**
 * Bspline grid class
 */

template<class VALUETYPE, unsigned int InputDimension = 3, unsigned int OutputDimension = 3>
class BsplineGrid
{
public:

    typedef double MatrixElementType;
    static const int IDims = InputDimension;
    static const int ODims = OutputDimension;
	typedef typename boost::shared_ptr<BsplineGrid<VALUETYPE, InputDimension, OutputDimension> > Pointer;
    typedef echo::Point<MatrixElementType, OutputDimension> CoefficientType;
    typedef echo::Point<MatrixElementType, InputDimension> PointType;
    typedef echo::Point<MatrixElementType, InputDimension> ContinuousIndexType;
    typedef echo::Point<int, InputDimension> IndexType;
    /// Matrix types
    typedef Eigen::Matrix<MatrixElementType, Eigen::Dynamic,Eigen::Dynamic> DenseMatrixType;
    typedef Eigen::SparseMatrix<MatrixElementType> SparseMatrixType; /// Sparse and Col major.
    typedef Eigen::SparseVector<MatrixElementType> SparseVectorType;
    typedef Eigen::Matrix<MatrixElementType, Eigen::Dynamic,1> DenseVectorType;
    typedef Eigen::Triplet<MatrixElementType> TripletType;
    /// ITK stuff
    typedef itk::Image<VALUETYPE, InputDimension> ITKImageType;
    typedef itk::Image<itk::Vector<VALUETYPE,OutputDimension>, InputDimension> ITKImageVectorType;

    /// Public member attributes
    unsigned int m_ngridpoints; /// total number of gridpoints.  This is the number of coefficients including the bspline border AND the user defined border

    /// Constructors
    BsplineGrid();
    BsplineGrid(const double *bounds, unsigned int bsd, const PointType &spacing, unsigned int border=0);
    BsplineGrid(typename ITKImageType::Pointer refimage, unsigned int bsd); /// For scalar
    BsplineGrid(typename ITKImageVectorType::Pointer refimage, unsigned int bsd); /// For vector

    ~BsplineGrid() {};

    /// Member functions

    void SetCyclicDimensions(const IndexType & cyclic_dimensions){
        this->m_cyclic_dimensions = cyclic_dimensions;
    }

    void SetCyclicBounds(unsigned int idx, double xmin, double xmax ){
        m_cyclicBounds[idx][0] = xmin;
        m_cyclicBounds[idx][1] = xmax;
    }

    void setMaxMemory(double m){
        this->m_max_memory = m;
    }



    void GetCyclicBounds(unsigned int idx, double * bounds){
        bounds[0] = m_cyclicBounds[idx][0];
        bounds[1] = m_cyclicBounds[idx][1];
    }

    IndexType GetCyclicDimensions(){
        return this->m_cyclic_dimensions;
    }
    void UpdateCyclicBehaviour(); /// This function recalculates the number of points and the spacing

    void SetDebugOn()
    {
        m_debug=true;
    };
    void SetDebugOff()
    {
        m_debug=false;
    };

    void SetDebug(bool f)
    {
        m_debug=f;
    };

    void SetParallelOn()
    {
        m_is_parallel=true;
    };
    bool GetParallel()
    {
        return m_is_parallel;
    };
    void SetParallelOn(int n)
    {
        m_is_parallel=true;
        this->m_threads_to_use = n;
    };
    void SetParallelOff()
    {
        m_is_parallel=false;
    };

    void SetParallel(bool f)
    {
        m_is_parallel=f;
    };
    void SetParallel(bool f, int n)
    {
        m_is_parallel=f;
        this->m_threads_to_use = n;
    };


    PointType GetOrigin();
    PointType GetSpacing();
    unsigned int  GetInputDimensions();
    unsigned int  GetOutputDimensions();


    IndexType GetSize();
    IndexType GetSizeParent();
    IndexType GetSizeWithBorder();
    unsigned int GetBorder();

    CoefficientType Get(const IndexType &index);
    CoefficientType Get(const unsigned int *index);
    void Set(const IndexType &index, const  CoefficientType &value);
    /**
     * @brief SetBlock sets the coefficients in the patch (ignoring the border region) into the grid.
     * @param block
     */
    void SetBlock( const typename BsplineGrid::Pointer &block);

    /**
     * It is assumed that ArrayType has iterators as in stl vector
     * Set all coefficients within the patch to the values. All coefficients in the border zone are assigned, but they will be ignored later.
     */
    template<class ArrayType>  void setAll(const ArrayType &x, const std::vector<unsigned int> &kept_nodes);

    /**
     * \brief This function returns the 1-D index of a 3D node.
     * nd_to_oned_index:
     * The n-D index is with respect to the full grid, Therefore if this is a patch it will still start at [0,0,...,0]
     * However, the 1D input index is only with respect to the patch (INCLUDING borders).
     */
    unsigned int nd_to_oned_index(const IndexType &index);
    /**
     * \brief This function returns the n-D index of a node.
     * oned_to_nd_index: The n-D index is with respect to the full grid, Therefore if this is a patch it will still start at [0,0,...,0]
     * However, the 1D input index is only with respect to the patch, INCLUDING borders.
     */
    IndexType oned_to_nd_index(unsigned int index1D);

    /**
     * \brief This function returns the n-D index of a node.
     * oned_to_nd_index: The n-D index is with respect to the full grid, Therefore if this is a patch it will still start at [0,0,...,0]
     * However, the 1D input index is only with respect to the patch, without including borders.
     */
    IndexType oned_to_nd_index_noBorders(const unsigned int index1D);

    ContinuousIndexType GetContinuousIndex(const PointType &p);

    /**
    * \brief Splits the grid into smaller patches.
    *
    * find_patches Splits the ND grid into number_of_patches patches in each dimension.
    *
    * @param number_of_patches vector with the number of patches in each dimension.
    * @param patches the output vector of patches.
    * @return 0 if ok.
    */
	int find_patches(unsigned int number_of_patches, std::vector< typename BsplineGrid::Pointer  > &patches, unsigned int border);
    /**
     * \brief This function will extract a subset of the grid
     *
     * It can use a border or not.
     * \param index coordinates of the first control point of the grid. The index does not accout for the border. The border plus index will be stored in another variable of the patch
     * \param size number of grid nodes to take
     * \param border number of extra grid points extracted
     * \return a smaller patch of the same class
     */
	//template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
	typename Pointer extractPatch(const IndexType &index, const IndexType &size, unsigned int border = 0);

    /**
     * This returns by argument an array of pointers towards the neighbourhood for evaluating the B-splines that
     * Affect each node. For example, in the case of b-splines of degree 1 and 3D input domain, the indices returned would be:
     *
     * index 0: [-1 -1 -1]
     * index 1: [0 -1 -1]
     * index 2: [-1 0 -1]
     * index 3: [0 0 -1]
     * index 4: [-1 -1 0]
     * index 5: [0 -1 0]
     * index 6: [-1 0 0]
     * index 7: [0 0 0]
     *
     * This is done by calculating the permutations with repetition
     */
    void createNeighbourhoodPointers(std::vector<IndexType > &gridpos);

    /**
     * \brief This function returns the Diagonal matrix.
     * This method is designed for use with the matrix class from the Eigen library.
     */
    template <class PointType>    SparseMatrixType createDMatrix(const std::vector<PointType> &directions);
    /**
    * \brief This function returns the continuous Bspline derivative (divergence) sampling matrix.
    * This uses the convolution operator as described in Arigovindan: TMI 2007
    */
    SparseMatrixType createContinuousDivergenceSamplingMatrix(const std::vector<unsigned int> &kept_nodes=std::vector<unsigned int>(),  const std::vector<unsigned int> &corresponding_nodes=std::vector<unsigned int>());
    /**
    * \brief This function returns the continuous Bspline derivative (gradient) sampling matrix.
    * This uses the convolution operator as described in Arigovindan: TMI 2007
    */
    SparseMatrixType createContinuousGradientSamplingMatrix(const double remove_nodes_th, const std::vector<unsigned int> &kept_nodes);


    /**
     * \brief This function returns the Bspline sampling matrix.
     * This method is designed for use with the matrix class from the Eigen library.
     */
    SparseMatrixType createSamplingMatrix(const std::vector<PointType> &positions, const double remove_nodes_th, std::vector<unsigned int> &kept_nodes, std::vector<unsigned int> &kept_nodescorresponding_columns, int derivative_order=0);

    /**
     * \brief Set the points where the value of the function is known
     * This method is designed for use with the matrix class from the Eigen library.
     */
    void SetInputPoints(const std::vector<PointType> &positions);

    /**
     * \brief converts a vector of data into a vector compatible with eigen
     * This method is designed for use with the matrix class from the Eigen library.
     */
    template<class VectorType>
    DenseVectorType toVector(const VectorType &values);

    /**
     * \brief This function returns the A matrix and the b matrix, which can otherwise be calculated
     * as A = (D*B)'(D*B), b = (D*B)*m, in the velocity reconstruction problem.
     * This method is designed for use with the matrix class from the Eigen library.
     */
    template <class DirectionType, class DopplerValueType> void createAbMatrices(const std::vector<PointType> &positions, const std::vector<DirectionType> &directions, const std::vector<DopplerValueType> &values, const double remove_nodes_th, std::vector<unsigned int> &kept_nodes, std::vector<unsigned int> &kept_nodescorresponding_columns, SparseMatrixType & A, DenseVectorType & b);

    /**
     * \brief This function returns the Weights matrix.
     * This method is designed for use with the matrix class from the Eigen library.
     */
    SparseMatrixType createWeightsMatrix(const std::vector<PointType> &positions, const DenseVectorType & weights);

    /**
     * \brief This function calculates the non-zero coefficients
     * nonzero_coefficients has indices of all non-zero coefficients
     * corresponding_nonzero_coefficients has the correspondance (the index)
     */
    void nonZeroCoefficients(std::vector<unsigned int> &nonzero_coefficients, std::vector<unsigned int> & corresponding_nonzero_coefficients);

    /**
     * \brief This function returns the bounds of the points that should be included or are affected by the splines in this patch
     *
     */
    void GetBoundsOfInfluence(double *bounds);

    /**
     * \brief This function returns the bounds of the points that should be included or are affected by the splines in this patch (including border)
     *
     */
    void GetBorderBoundsOfInfluence(double *bounds);

    /**
     * \brief This function returns the bounds after applying the border
     *
     */
    void GetBorderBounds(double *bounds);
    /**
     * \brief This function returns the bounds before applying the border
     *
     */
    void GetBounds(double *bounds);
    /**
     * \brief Write as a 3D image.
     *  Write only the actual values within the region defined by index and size, but not the region where there is border
     */

    void writeAsVectorImage(std::string &filename);

    void writeAs3DImage(std::string &filename);
    void writeAsVTKDataSet(std::string &filename);

    /**
     * Calculate the underlying function using bspline interpolation at the given points
     */

    DenseVectorType GetCoefficientVector(std::vector<unsigned int> & nonzero_coefficients, std::vector<unsigned int> & corresponding_nonzero_coefficients, bool getall=false);
    std::vector<CoefficientType> evaluate(const std::vector<PointType> &pointList);
    CoefficientType evaluate(const PointType &p);
    CoefficientType evaluateDerivative(const PointType &p, unsigned int dim);  /// Calculate derivative of the BSpline at one point, along the direction 'dim';

    /**
     * \brief This function warps an image using the Bdpline grid, which must matcht the appropriate dimensions in the input and output.
     */
    template <class ImageType>
    typename ImageType::Pointer warp(typename ImageType::Pointer inputImage);


    std::string toString();

	friend std::ostream & operator << (std::ostream &os, const typename BsplineGrid::Pointer &bsg)
    {
        os << bsg->toString();
        return os;
    };


private:

    /// Private member attributes
    PointType m_spacing;
    PointType m_origin;
    IndexType m_size; /// This is the number of coefficients including the bspline border but NOT the user defined border
    IndexType m_size_parent; /// This is the number of coefficients including the bspline border but NOT the user defined border in the parent grid, for which this is a patch. If this is the full grid, this is equal to m_size
    IndexType m_index;
    IndexType m_borderIndex1; /// The index of the first control node of the patch accounting for the border
    IndexType m_borderIndex2; /// The index of the last control node of the patch accounting for the border
    std::vector< CoefficientType > m_data; /// This DOES store any values for the borders
    IndexType m_cyclic_dimensions; /// Any dimension in this vector will be considered cyclic (this is actually a boolean)
    double m_cyclicBounds[InputDimension][2];

    unsigned int m_border;
    unsigned int m_bsd; /// Bspline degree
    bool m_debug;
    double m_max_memory;

    /// For parallel processing
    boost::mutex m_coutmutex, m_tl_mutex, m_cs_mutex;
    bool m_is_parallel;
    int m_threads_to_use;

    /// Private member functions
	void patchRegion(const IndexType &maxPatchSize, std::vector< typename BsplineGrid::Pointer > &patches, unsigned int border = 0);

    void m_fill_BMatrix_triplets_cyclic(const std::vector<PointType> &positions,
                                        const std::vector<IndexType>  &gridpos, int initial_row,
                                        const std::vector<unsigned int> &kept_nodes, const std::vector<unsigned int> &corresponding_columns,
                                        std::vector<TripletType> &triplet_list,DenseVectorType &column_summation,
                                        BSGThreadConfig &config);

    void m_fill_BMatrix_triplets(const std::vector<PointType> &positions,
                                 const std::vector<IndexType>  &gridpos,const int initial_row,
                                 const std::vector<unsigned int> &kept_nodes, const std::vector<unsigned int> &corresponding_columns,
                                 std::vector<TripletType> &triplet_list,DenseVectorType &column_summation,
                                 int derivative_order);




    void writeAs3DVectorImage(std::string &filename);
    void writeAs2DVectorImage(std::string &filename);
    void writeAs4DVectorImage(std::string &filename);

};

///---------------------------------------------IMPLEMENTATION ---------------------------------///

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::BsplineGrid()
{
    //std::cout << "default constructor"<<std::endl;
    m_bsd=3;
    m_border=0;
    m_ngridpoints=0;
    m_debug=false;
    m_is_parallel = false;
    this->m_threads_to_use = 1;
};

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::BsplineGrid(const double *bounds, unsigned int bsd, const PointType &spacing, unsigned int border)
{
    //std::cout << "Usual constructor"<<std::endl;
    /// The border will only be used when extracting patches
    int leftBorder=std::ceil(((double) bsd-1.0)/2.0), rightBorder=std::ceil(( (double) bsd-1.0)/2.0);
    this->m_ngridpoints=1;
    this->m_max_memory=-1;
    for (int i=0; i<InputDimension; i++)
    {
        this->m_size[i]=ceil((bounds[2*i+1]-bounds[2*i]+EPSILON)/spacing[i]) + 1 +leftBorder + rightBorder;
        this->m_size_parent[i] = this->m_size[i];
        m_origin[i]=(bounds[2*i+1]+bounds[2*i]-EPSILON)/2.0-((double) m_size[i]-1.0)/2.0*spacing[i];
        m_spacing[i]=spacing[i];
        m_index[i]=0;
        m_borderIndex1[i]=0;
        m_borderIndex2[i]=m_size[i]-1;

        this->m_cyclicBounds[i][0]=0;
        this->m_cyclicBounds[i][1]=1;
        this->m_cyclic_dimensions[i]=0;
    }
    this->m_ngridpoints *=(this->m_borderIndex2-this->m_borderIndex1+1).prod(); /// m_ngridpoints DOES account for the border

    m_bsd = bsd;
    m_border = border;
    m_debug=false;
    m_is_parallel = false;
    this->m_threads_to_use=1;
    for (int j=0; j<m_ngridpoints; j++)
    {
        CoefficientType c;
        m_data.push_back(c);
    }




    //std::cout << "Exit usual constructor"<<std::endl;
}

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::BsplineGrid(typename ITKImageType::Pointer refimage, unsigned int bsd)
{

    typename ITKImageType::SpacingType itkSpacing = refimage->GetSpacing();
    typename ITKImageType::PointType itkOrigin = refimage->GetOrigin();
    typename ITKImageType::SizeType itkSize = refimage->GetLargestPossibleRegion().GetSize();


    for (int i=0; i<InputDimension; i++)
    {
        this->m_size[i] = itkSize[i];
        this->m_size_parent[i] = this->m_size[i];
        this->m_spacing[i] = itkSpacing[i];
        this->m_origin[i] = itkOrigin[i];
        this->m_index[i]=0;
        this->m_borderIndex1[i]=0;
        this->m_borderIndex2[i]=this->m_size[i]-1;
        this->m_cyclicBounds[i][0]=0;
        this->m_cyclicBounds[i][1]=1;
        this->m_cyclic_dimensions[i]=0;
    }
    this->m_border = 0;
    this->m_bsd = bsd;
    this->m_max_memory=-1;
    m_debug=false;
    m_is_parallel = false;
    this->m_threads_to_use = 1;
    this->m_ngridpoints =this->m_size.prod();

    /// Copy coefficients
    this->m_data.resize(this->m_ngridpoints);

    typedef itk::ImageRegionConstIterator< ITKImageType > IteratorType;
    IteratorType in( refimage, refimage->GetRequestedRegion() );
    typename std::vector< CoefficientType >::iterator iter;
    CoefficientType coef;
    for ( in.GoToBegin(),  iter = this->m_data.begin(); iter != this->m_data.end(); ++iter,++in)
    {
        coef[0]=(VALUETYPE) in.Get();
        *iter = coef;
    }


} /// For scalar

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::BsplineGrid(typename ITKImageVectorType::Pointer refimage, unsigned int bsd)
{

    typename ITKImageVectorType::SpacingType itkSpacing = refimage->GetSpacing();
    typename ITKImageVectorType::PointType itkOrigin = refimage->GetOrigin();
    typename ITKImageVectorType::SizeType itkSize = refimage->GetLargestPossibleRegion().GetSize();


    for (int i=0; i<InputDimension; i++)
    {
        this->m_size[i] = itkSize[i];
        this->m_size_parent[i] = this->m_size[i];
        this->m_spacing[i] = itkSpacing[i];
        this->m_origin[i] = itkOrigin[i];
        this->m_index[i]=0;
        this->m_borderIndex1[i]=0;
        this->m_borderIndex2[i]=this->m_size[i]-1;
        this->m_cyclicBounds[i][0]=0;
        this->m_cyclicBounds[i][1]=1;
        this->m_cyclic_dimensions[i]=0;
    }
    this->m_border = 0;
    this->m_max_memory=-1;
    this->m_bsd = bsd;
    m_debug=false;
    m_is_parallel = false;
    this->m_threads_to_use =1;
    this->m_ngridpoints =this->m_size.prod();

    /// Copy coefficients
    this->m_data.resize(this->m_ngridpoints);

    typedef itk::ImageRegionConstIterator< ITKImageVectorType > IteratorType;
    IteratorType in( refimage, refimage->GetRequestedRegion() );
    typename std::vector< CoefficientType >::iterator iter;
    CoefficientType coef;
    for ( in.GoToBegin(),  iter = this->m_data.begin(); iter != this->m_data.end(); ++iter,++in)
    {
        for (int k=0; k<OutputDimension; k++)
        {
            coef[k]=(VALUETYPE) in.Get()[k];
            (*iter)[k] = coef[k];
        }
    }


} /// For vector

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
int BsplineGrid<VALUETYPE, InputDimension, OutputDimension>::find_patches(unsigned int number_of_patches, std::vector< typename BsplineGrid::Pointer  > &patches, unsigned int border)
{
    unsigned int maxPatchSize[InputDimension];
    for (int i=0; i<InputDimension; i++)
    {
        maxPatchSize[i]=std::ceil((double) this->m_size[i]/ (double) number_of_patches);
    }
    //    if (this->m_debug) {
    //        std::cout << "\t\tmax patch size ";
    //        for (int i=0; i<InputDimension; i++)
    //            std::cout << maxPatchSize[i] << ", ";
    //        std::cout << std::endl;
    //    }
    this->patchRegion(maxPatchSize,patches, border);

    return 0;
}

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
typename BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::PointType BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::GetOrigin()
{
    return this->m_origin;
}

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
typename BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::PointType BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::GetSpacing()
{
    return this->m_spacing;
}

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
unsigned int  BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::GetInputDimensions(){
    return InputDimension;
}

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
unsigned int  BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::GetOutputDimensions(){
    return OutputDimension;
}


/**
 * This size is with the bspline border, but without the user defined border so it is smaller than the actual patch for instance
 */
template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
typename BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::IndexType BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::GetSize()
{
    return this->m_size;
}

/**
 * This size is with the bspline border, but without the user defined border so it is smaller than the actual patch for instance
 */
template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
typename BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::IndexType BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::GetSizeParent()
{
    return this->m_size_parent;
}


template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
typename BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::IndexType BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::GetSizeWithBorder()
{
    return (this->m_borderIndex2-this->m_borderIndex1)+1;
}


template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
unsigned int  BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::GetBorder()
{
    return this->m_border;
}

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
void BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::Set(const IndexType &index, const CoefficientType &value)
{
    unsigned int index1D = this->nd_to_oned_index(index);
    if (index1D!=OUT_OF_PATCH)
        m_data[index1D]=value;
    else
    {
        std::cout<< "BsplineGrid::Set - ERROR::The requested index "<<index<<" is outside the patch"<<std::endl;
    }

}




template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension> template <class ArrayType>
void BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::setAll(const ArrayType &x,  const std::vector<unsigned int> &kept_nodes)
{

    /// Kept_nodes contains less or equal that OutputDimension x n_mgridpoints elements. The same number as x.

    unsigned int n_kept_nodes = kept_nodes.size();

    CoefficientType c;
    //unsigned int node_index1D;
    for (int j=0; j<n_kept_nodes; j++)
    {
        for (int k=0; k<OutputDimension; k++){
            c[k]=x[j*OutputDimension+k];
        }
        //node_index1D=(kept_nodes[j*OutputDimension+0])/OutputDimension;
        this->m_data[kept_nodes[j]]=c;

    }
}

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
typename BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::CoefficientType
BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::Get(const IndexType &index)
{
    unsigned int index1D = this->nd_to_oned_index(index);
    if (index1D!=OUT_OF_PATCH){
        return m_data[index1D];
        //std::cout<< "BsplineGrid::Get - GOOD::The requested index "<<index<<" is inside the patch"<<std::endl;
    }else
    {
        std::cout<< "BsplineGrid::Get - ERROR::The requested index "<<index<<" is outside the patch"<<std::endl;
        return m_data[0];
    }
}

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
typename BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::CoefficientType
BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::Get(const  unsigned int *index)
{

    IndexType indexNd(index);
    unsigned int index1D = this->nd_to_oned_index(indexNd);
    if (index1D!=OUT_OF_PATCH)
        return m_data[index1D];
    else
    {
        std::cout<< "BsplineGrid::Get - ERROR::The requested index "<<index[0]<<" is outside the patch"<<std::endl;
        return 0;
    }
}

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
unsigned int BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::nd_to_oned_index(const IndexType &index)
{
    typename  IndexType::const_iterator const_iter1, const_iter2, const_iter3;


    for (const_iter1=m_borderIndex1.begin(),const_iter2=m_borderIndex2.begin(),const_iter3=index.begin();
         const_iter1!=m_borderIndex1.end();
         ++const_iter1, ++const_iter2, ++const_iter3 )
    {
        if (*const_iter3 < *const_iter1 || *const_iter3 > *const_iter2)
            return OUT_OF_PATCH;
    }

    IndexType index_local=index-this->m_borderIndex1;
    unsigned int index1D=0;

    for (int i=InputDimension-1; i>=0; i--)
    {
        unsigned int dimensions_tmp=1;
        for (int j=0; j<i; j++)
        {
            dimensions_tmp*=(this->m_borderIndex2[j]-this->m_borderIndex1[j]+1);
        }
        index1D+=index_local[i]*dimensions_tmp;
    }
    return index1D;
}


template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
typename BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::IndexType
BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::oned_to_nd_index(unsigned int index1D)
{
    IndexType index;
    unsigned int remainder=index1D;
    for (int i=InputDimension-1; i>=0; i--)
    {
        unsigned int dimensions_tmp=1;
        for (int j=0; j<i; j++)
        {
            dimensions_tmp*=(this->m_borderIndex2[j]-this->m_borderIndex1[j]+1);/// Account for all the nodes
        }
        index[i] = std::floor((double) remainder/dimensions_tmp);
        remainder -= dimensions_tmp*index[i];
    }


    index+=this->m_borderIndex1;
    return index;

}

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
typename BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::IndexType
BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::oned_to_nd_index_noBorders(const unsigned int index1D)
{
    IndexType index;
    unsigned int remainder=index1D;
    for (int i=InputDimension-1; i>=0; i--)
    {
        unsigned int dimensions_tmp=1;
        for (int j=0; j<i; j++)
        {
            dimensions_tmp*=(this->m_size[j]);/// Account for all the nodes but not for borders
        }
        index[i] = std::floor((double) remainder/dimensions_tmp);
        remainder -= dimensions_tmp*index[i];
    }

    index+=this->m_index;
    return index;

}



template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
typename BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::ContinuousIndexType
BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::GetContinuousIndex(const PointType &p)
{
    ContinuousIndexType output = (p-this->m_origin)/this->m_spacing;
    return output;
}


template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
void BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::nonZeroCoefficients(std::vector<unsigned int> &nonzero_coefficients, std::vector<unsigned int> & corresponding_nonzero_coefficients){

    int ncolumns = this->GetSizeWithBorder().prod();
    //int ncolumns = this->GetSizeWithBorder().prod()*OutputDimension;
    nonzero_coefficients.clear();
    try{
        nonzero_coefficients.reserve(ncolumns);
    }
    catch (std::exception& ba)
    {
        std::cerr << "BsplineGrid::nonZeroCoefficients-- exception caught while reserving "<< ncolumns <<" values into nonzero_coefficients: " << ba.what() << '\n';
        exit(-1);
    }
    corresponding_nonzero_coefficients.resize(ncolumns,OUT_OF_PATCH);

    CoefficientType ZERO;

    //if (this->m_debug) std::cout << " BsplineGrid::nonZeroCoefficients -- This patch is  "<< std::endl<< this->toString()<<std::endl;

    int lastInserted=-1;
    for (int i_=0; i_< this->m_ngridpoints; i_++){
        if (this->m_data[i_] != ZERO){
            IndexType index = oned_to_nd_index_noBorders(i_);
            unsigned int i = nd_to_oned_index(index);
            corresponding_nonzero_coefficients[i]=++lastInserted;
            nonzero_coefficients.push_back(i);
            //            for (int j=0; j<OutputDimension; j++){
            //                corresponding_nonzero_coefficients[i*OutputDimension+j]=++lastInserted;
            //                nonzero_coefficients.push_back(i*OutputDimension+j);
            //            }
        }
    }


}


template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
void BsplineGrid<VALUETYPE, InputDimension, OutputDimension>::SetBlock(const typename BsplineGrid::Pointer &block)
{

    IndexType indexND;
    unsigned int npoints_in_block = block->m_size.prod(); /// size without borders

    // std::cout << "Block: "<< block<<std::endl;

    for (unsigned int i=0; i<npoints_in_block; i++)
    {
        //std::cout << "   point: "<< i+1 <<" of "<< npoints_in_block<<std::endl;
        indexND = block->oned_to_nd_index_noBorders(i);
        //std::cout << "       index1D: "<< indexND <<std::endl;
        CoefficientType coef = block->Get(indexND);
        this->Set(indexND,coef);
    }
    //std::cout << "Block done"<<std::endl;


}

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
typename BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::Pointer
BsplineGrid<VALUETYPE, InputDimension, OutputDimension>::extractPatch(const IndexType &index, const IndexType &size, unsigned int border)
{

    double dummy_bounds[2*InputDimension];
    for (int i=0; i<InputDimension; i++)
    {
        dummy_bounds[2*i]=-1;
        dummy_bounds[2*i+1]=1;
    }

	BsplineGrid::Pointer patch(new BsplineGrid(dummy_bounds, this->m_bsd, this->m_spacing, border));
    patch->m_ngridpoints = 1;
    patch->m_debug = this->m_debug;
    patch->m_is_parallel = this->m_is_parallel;
    patch->m_threads_to_use = this->m_threads_to_use;


    patch->m_size=size; /// size does ot account for the border
    patch->m_size_parent=this->m_size_parent;
    IndexType  preborder, postborder;

    for (int i=0; i<InputDimension; i++)
    {
        preborder[i] = ((int) index[i]-(int) border>=this->m_borderIndex1[i]) ? border :  index[i]-this->m_borderIndex1[i];
        postborder[i] = index[i]+size[i]-1+border<=this->m_borderIndex2[i] ? border : (int) this->m_borderIndex2[i] - (int) (size[i]-1) - (int) index[i];
    }
    patch->m_borderIndex1=index-preborder;
    patch->m_borderIndex2=index+(patch->m_size-1)+postborder;
    patch->m_index=index;
    patch->m_origin=this->m_origin; /// origin is always associated to the index [0 0 0], not to the first index of the patch
    patch->m_ngridpoints =(patch->m_size+preborder+postborder).prod(); /// m_ngridpoints DOES account for the border
    patch->m_max_memory = this->m_max_memory;
    patch->m_data.resize(patch->m_ngridpoints);

    /// Copy the data only in this patch

    unsigned int oned_index_patch;
    IndexType nd_index;
    for (oned_index_patch=0; oned_index_patch<patch->m_ngridpoints; oned_index_patch++)
    {
        nd_index = patch->oned_to_nd_index(oned_index_patch);
        patch->Set(nd_index,this->Get(nd_index));
    }


    if ( this->m_cyclic_dimensions.isnz().sum() ){
        patch->SetCyclicDimensions(this->m_cyclic_dimensions);
        for (int i=0; i<InputDimension; i++){
            patch->SetCyclicBounds(i,this->m_cyclicBounds[i][0], this->m_cyclicBounds[i][1]);
        }
        patch->UpdateCyclicBehaviour();
    }


    return patch;

}

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
void BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::patchRegion(const IndexType &maxPatchSize,
	std::vector< typename BsplineGrid::Pointer > &patches,
                                                                        unsigned int border)
{

    IndexType npatches = (this->m_size/ maxPatchSize).floor();/// Patches of size (maxPatchSize)
    IndexType remaining_nodes=this->GetSize()-npatches*maxPatchSize;
    IndexType total_npatches = npatches + remaining_nodes.isnz();/// Patches of size (maxPatchSize) and smaller

    unsigned int total_new_patches = total_npatches.prod();
    //    if (this->m_debug) std::cout <<"\t\t Subdivide into a total of "<< total_new_patches << " patches."<<std::endl;

    try{
        patches.reserve(total_new_patches);
    } catch (std::exception &ba ){
        std::cerr << "BsplineGrid::patchRegion-- exception caught while reserving "<< total_new_patches <<" values into patches: " << ba.what() << '\n';
        exit(-1);
    }

    for (unsigned int i=0; i<total_new_patches; i++)
    {
        //        if (this->m_debug) std::cout <<"\t\t\tPatch "<< i<< " of "<< total_new_patches <<std::endl;
        IndexType index;
        echo::oned_to_nd_index(i,index, total_npatches, InputDimension);
        IndexType patch_index= index*maxPatchSize+this->m_index;
        //        if (this->m_debug) std::cout << "\t\t\t\t1D index "<< i << " is ND index "<< index << '\n';
        IndexType current_size;

        for (int j=0; j<InputDimension; j++)
        {
            if (index[j]<npatches[j])
            {
                current_size[j] = maxPatchSize[j];
            }
            else
            {
                current_size[j] = remaining_nodes[j];
            }
        }
        /// Calculate the bounds of the patch based on the coordinates
        //        if (this->m_debug) std::cout << "\t\t\t\tBsplineGrid::patchRegion-- extract patch at index "<< patch_index<< " with size "<< current_size<< " and border " <<border<< '\n';
        BsplineGrid::Pointer current_patch = this->extractPatch(patch_index, current_size, border);
        patches.push_back(current_patch);

    } /// End further subdivision
}

/**
 * This function might be used on a data that already has coefficients assigned, thus must not change the size in that case.
 */
template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
void BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::UpdateCyclicBehaviour(){

    /// Verify if the grid is already cyclic

    bool already_cyclic=true;

    for (int i=0; i<InputDimension; i++)
    {
        if (this->m_cyclic_dimensions[i]){
            //if (std::abs((this->m_origin[i]-this->m_spacing[i]/2.0)-this->m_cyclicBounds[i][0])>EPSILON ||
            if (std::abs(this->m_origin[i]-this->m_cyclicBounds[i][0])>EPSILON ||
                    std::abs(this->m_origin[i]+this->m_size_parent[i]*this->m_spacing[i]-this->m_cyclicBounds[i][1])>EPSILON){
                already_cyclic = false;
                break;
            }
        }
    }

    if (already_cyclic){
        /// Nothing to be done
        return;
    }

    /// Adjust the grid.
    m_ngridpoints=1;

    bool isparent = this->m_size == this->m_size_parent; /// This means that we are not in the patchwise approach, or that there is just one patch

    for (int i=0; i<InputDimension; i++)
    {
        if (this->m_cyclic_dimensions[i]){
            this->m_size[i]=ceil( (this->m_cyclicBounds[i][1]-this->m_cyclicBounds[i][0])/this->m_spacing[i]);
            if (isparent){
                this->m_size_parent[i]=this->m_size[i];
            } else {
                this->m_size_parent[i]=this->m_size_parent[i];
            }
            /// TODO think carefully about the borderindices when the grid is cyclic!
            //            this->m_borderIndex1[i]=0-this->m_border;
            //            this->m_borderIndex2[i]=this->m_size[i]-1+this->m_border;
            this->m_borderIndex1[i]=std::max(0,this->m_borderIndex1[i]); /// it wraps back to the end of the sequence
            this->m_borderIndex2[i]=std::min((int) this->m_size[i]-1+ (int) this->m_border, (int) this->m_size_parent[i]);
            this->m_spacing[i]= ((this->m_cyclicBounds[i][1]-this->m_cyclicBounds[i][0])/ this->m_size[i]);
            this->m_origin[i]=this->m_cyclicBounds[i][0];
        }
    }
    this->m_ngridpoints *=(this->m_borderIndex2-this->m_borderIndex1+1).prod(); /// m_ngridpoints DOES account for the border

    m_data.resize(this->m_ngridpoints);

}

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
void BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::createNeighbourhoodPointers(std::vector<IndexType > &gridpos)
{
    int m= 0-std::ceil((double) this->m_bsd / 2);
    int M=std::floor( (double) this->m_bsd / 2);

    int n_options = M-m+1;
    int n_indices = std::pow(n_options,InputDimension);
    //std::cout << "m "<<m<<", M "<< M<<", n_indices "<<n_indices<<std::endl;

    /// Calculate the indices starting from 0 and then subtract the first.
    unsigned int index[InputDimension];
    unsigned int nvalues[InputDimension];
    for (int i=0; i<InputDimension; i++)
    {
        nvalues[i]=n_options;
    }

    for (int i=0; i<n_indices; i++)
    {
        echo::oned_to_nd_index(i,index, nvalues, InputDimension);
        IndexType idx(index);
        idx+=m;
        gridpos.push_back(idx);
        //  std::cout << idx<<std::endl; /// Only for debug
    }


}


template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension> template <class PointType2>
typename BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::SparseMatrixType
BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::createDMatrix(const  std::vector<PointType2> &directions)
{
    int npoints = directions.size();

    SparseMatrixType D(npoints, OutputDimension*npoints);

    /// Create the 'triplets' provided by Eigen. A triplet
    std::vector<TripletType> triplet_list;
    unsigned int nnz_max = OutputDimension*npoints;
    try
    {
        triplet_list.reserve(nnz_max);
    }
    catch (std::exception& ba)
    {
        std::cerr << "BsplineGrid::createDMatrix-- exception caught while reserving "<< nnz_max <<" values into triplet_list: " << ba.what() << '\n';
        exit(-1);
    }


    int current_row=0, current_column=0;
    MatrixElementType current_value=0.0;

    current_row=-1;
    for ( typename std::vector< PointType2 >::const_iterator iter = directions.begin(); iter != directions.end(); ++iter)
    {
        ++current_row; /// Row corresponds to point
        for (int k=0; k<OutputDimension; k++)
        {
            current_column = current_row*OutputDimension+k;
            current_value = (*iter)[k];
            triplet_list.push_back(TripletType(current_row,current_column,current_value));
        }
    }

    /// Deal with removing points
    D.setFromTriplets(triplet_list.begin(), triplet_list.end());
    D.makeCompressed();
    return D;
}

/**
 * This has just been optimised for performance and memory consumption
 */
template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
typename BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::SparseMatrixType
BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::createContinuousGradientSamplingMatrix(const double remove_nodes_th, const std::vector<unsigned int> &kept_nodes)
{

    std::vector<TripletType> triplet_list;
    //unsigned int nnz_max = std::pow(2*(this->m_bsd+1)+1, InputDimension)*OutputDimension*kept_nodes.size();
    unsigned int nnz_max = kept_nodes.size()*kept_nodes.size()*InputDimension;

    try
    {
        triplet_list.reserve(nnz_max);
    }
    catch (std::exception& ba)
    {
        std::cerr << "BsplineGrid::createContinuousGradientSamplingMatrix-- exception caught while reserving "<< nnz_max <<" values into triplet_list: " << ba.what() << '\n';
        exit(-1);
    }

    /// derivative along each input dimension
    int newrow, newcolumn;
    MatrixElementType value;

    //for (int i=0; i<this->m_ngridpoints; i++)
    for (int i_=0; i_<kept_nodes.size(); i_++)
    {
        int i = kept_nodes[i_];
        newrow=  i*OutputDimension;
        //for (int j=0; j<this->m_ngridpoints; j++)
        for (int j_=0; j_<kept_nodes.size(); j_++)
        {
            int j = kept_nodes[j_];
            IndexType idx = this->oned_to_nd_index(i)-this->oned_to_nd_index(j);
            value=0.0;
            newcolumn =  j*OutputDimension;
            for (int k=0; k<InputDimension; k++)
            {
                MatrixElementType tmp_value=1;
                for (int kk=0; kk<InputDimension; kk++)
                {
                    if (kk==k)
                        continue;
                    tmp_value*=echo::BsplineConv(this->m_bsd,idx[kk]);
                }
                value+= echo::dBsplineConv(this->m_bsd,idx[k])*this->GetSpacing().prod()*tmp_value;
            }
            if (value!=0.0)
            {
                triplet_list.push_back(TripletType(i,j,value));
            }
        }
    }/// Validated

    if (triplet_list.size() > nnz_max){
        std::cerr << "BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::createContinuousGradientSamplingMatrix -- WARNING: ";
        std::cerr<< "the number of estimated non-zero elements ("<< nnz_max<<") ";
        std::cerr << "was lower than the actual number of non-zero elements ("<< triplet_list.size()<<"). ";
        std::cerr << "This will bring in significant performance drop."<<std::endl;
    }

    SparseMatrixType A(OutputDimension*this->m_ngridpoints, OutputDimension*this->m_ngridpoints);
    //A.reserve(triplet_list.size());
    A.setFromTriplets(triplet_list.begin(), triplet_list.end());


    //std::string filename2 = std::string("/media/AGOMEZ_KCL_2/data/patients/patient-CAS_17_07Jan2013_DICOM/tmp/Adiv2.txt");
    //echo::Eigen::writeMatrixToFile<SparseMatrixType>(A, filename2);


    /// Remove nodes
    if (remove_nodes_th>=0)
    {
        /// Keep from B all the columns whose indices are in kept_nodes
        echo::Eigen::keepColumnsAndRows<SparseMatrixType>(A, kept_nodes);
    }
    //std::string filename3 = std::string("/media/AGOMEZ_KCL_2/data/patients/patient-CAS_17_07Jan2013_DICOM/tmp/Adiv3.txt");
    //echo::Eigen::writeMatrixToFile<SparseMatrixType>(A, filename3);
    return A;
}

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
typename BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::SparseMatrixType
BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::createContinuousDivergenceSamplingMatrix(const std::vector<unsigned int> &kept_nodes,
                                                                                                const std::vector<unsigned int> &corresponding_columns)
{
    /// Validated
    /// TODO if kept nodes is active, why not do the lower loops using keptnodes instead of all nodes (as in createContinuousGradientSamplingMatrix)
    myTimer t;
    t.start();
    std::vector<TripletType> triplet_list;
    /// Estimate the number of non-zeros
    //unsigned int nnz_max = std::pow(2*(this->m_bsd+1)-1, InputDimension)*kept_nodes.size();
    //unsigned int nnz_max = OutputDimension*this->m_ngridpoints * OutputDimension*this->m_ngridpoints; // this would be as a full matrix
    unsigned int nnz_max = std::pow(2*(this->m_bsd+1)-1, InputDimension)*this->m_ngridpoints*OutputDimension*2;

    try
    {
        triplet_list.reserve(nnz_max);
    }
    catch (std::exception& ba)
    {
        std::cerr << "BsplineGrid::createContinuousDivergenceSamplingMatrix -- exception caught while reserving "<< nnz_max <<" values into triplet_list: " << ba.what() << '\n';
        exit(-1);
    }
    IndexType idx ;
    MatrixElementType Pxx_value, Pxy_value,  tmp_value_xx, tmp_value_xy;
    int newrow, newcolumn;

    /// derivative along each dimension
    for (int i=0; i<this->m_ngridpoints; i++)
    {
        newrow=  i;

        if (kept_nodes.size()){
            /// See if the current row does not correspond to a node I wanted to remove
            bool ignore_row=true;
            for( std::vector<unsigned int>::const_iterator it= kept_nodes.begin(); it != kept_nodes.end(); ++it) {
                if (newrow==*it ){
                    //                        std::cout << "\t\tcurrent_column "<<current_column<<" is not to be ignored"<<std::endl;
                    ignore_row= false;
                    break;
                }
            }
            if (ignore_row){
                //                     std::cout << "\tcurrent_column "<<current_column<<" is to be ignored"<<std::endl;
                continue;
            }
            newrow = corresponding_columns[newrow];
        }


        for (int j=0; j<this->m_ngridpoints; j++)
        {
            newcolumn=  j;

            if (kept_nodes.size()){
                /// See if the current column does not correspond to a node I wanted to remove
                bool ignore_column=true;
                for( std::vector<unsigned int>::const_iterator it= kept_nodes.begin(); it != kept_nodes.end(); ++it) {
                    if (newcolumn==*it ){
                        //                        std::cout << "\t\tcurrent_column "<<current_column<<" is not to be ignored"<<std::endl;
                        ignore_column = false;
                        break;
                    }
                }
                if (ignore_column){
                    //                     std::cout << "\tcurrent_column "<<current_column<<" is to be ignored"<<std::endl;
                    continue;
                }
                newcolumn = corresponding_columns[newcolumn];
            }


            idx = this->oned_to_nd_index(i)-this->oned_to_nd_index(j);

            /// This loop is only for Pxx ----------------------------
            Pxx_value=0;
            //for (int k=0; k<InputDimension; k++) ///I think the mistake is here. Maybe it should be OutputDimension
            for (int k=0; k<OutputDimension; k++)
            {
                tmp_value_xx=1;
                for (int kk=0; kk<InputDimension; kk++)
                {
                    if (kk==k)
                        continue;
                    tmp_value_xx*=echo::BsplineConv(this->m_bsd,idx[kk]);
                }

                Pxx_value= echo::dBsplineConv(this->m_bsd,idx[k])*this->GetSpacing().prod()*tmp_value_xx;
                if (Pxx_value!=0)
                {
                    triplet_list.push_back(TripletType(newrow*OutputDimension+k,newcolumn*OutputDimension+k,Pxx_value));
                }
            }

            /// This loop is only for Pxy ----------------------------
            for (int k=0; k<OutputDimension; k++)
            {
                for (int l=0; l<OutputDimension; l++)
                {
                    if (k==l)
                        continue;

                    tmp_value_xy=1;
                    for (int kk=0; kk<InputDimension; kk++)
                    {
                        if (kk==k || kk==l)
                            continue;
                        tmp_value_xy*=echo::BsplineConv(this->m_bsd,idx[kk]);
                    }
                    Pxy_value = echo::BsplineCrossedConv(this->m_bsd,idx[k],-1)*echo::BsplineCrossedConv(this->m_bsd,idx[l],1)*this->GetSpacing().prod()*tmp_value_xy;
                    if (Pxy_value!=0.0)
                    {
                        triplet_list.push_back(TripletType(newrow*OutputDimension+k,newcolumn*OutputDimension+l,Pxy_value));
                    }
                }
            }
        }
    }/// Validated
    /// Until this point, everything would be exactly the same for calculating other quantities such as the gradient.

    std::vector<TripletType>(triplet_list).swap(triplet_list);

    if (triplet_list.size() > nnz_max){
        std::cerr << "BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::createContinuousDivergenceSamplingMatrix -- WARNING: ";
        std::cerr<< "the number of estimated non-zero elements ("<< nnz_max<<") ";
        std::cerr << "was lower than the actual number of non-zero elements ("<< triplet_list.size()<<"). ";
        std::cerr << "This will bring in significant performance drop."<<std::endl;
    }

    int ncolumns = OutputDimension*this->m_ngridpoints;
    if (kept_nodes.size()){
        ncolumns = OutputDimension*kept_nodes.size();
    }

    SparseMatrixType A(ncolumns, ncolumns);
    //A.reserve(triplet_list.size());
    A.setFromTriplets(triplet_list.begin(), triplet_list.end());
    A.makeCompressed();
    //std::string filenameAreordered("/media/AGOMEZ_KCL_2/data/patients/patient-CAS_17_07Jan2013_DICOM/tmp/Areordered.txt");
    //echo::Eigen::writeMatrixToFile<SparseMatrixType>(A, filenameAreordered);
    if (this->m_debug) std::cout << "\t\t\tBsplineGrid::createContinuousDivergenceSamplingMatrix takes "<< t.GetSeconds()<<" s"<<std::endl;

    //    t.start();
    //    /// Remove nodes
    //    if (remove_nodes_th>=0)
    //    {
    //        /// Keep from B all the columns whose indices are in kept_nodes
    //        echo::Eigen::keepColumnsAndRows<SparseMatrixType>(A, kept_nodes);
    //    }
    //    if (this->m_debug) std::cout << "\t\t\tcreateContinuousDivergenceSamplingMatrix::Remove nodes takes "<< t.GetSeconds()<<" s"<<std::endl;
    //    //std::string filenameAremoved("/media/AGOMEZ_KCL_2/data/patients/patient-CAS_17_07Jan2013_DICOM/tmp/Aremoved.txt");
    //    //echo::Eigen::writeMatrixToFile<SparseMatrixType>(A, filenameAremoved);

    return A;
}


/**
 * Function for weighting the contribution of each point in a linear system. The output is the diagonal matrix W.
 * There are several methods for weighting, controled by the argument 'method'.
 * Any weight passed as -1 in the argument weights matrix will be set to 1.
 */

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
typename BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::SparseMatrixType
BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::createWeightsMatrix(const std::vector<PointType> &positions,  const DenseVectorType & weights){
    int npoints = positions.size();
    SparseMatrixType W(npoints,npoints);

    std::vector<TripletType> triplet_list;

    try
    {
        triplet_list.reserve(npoints);
    }
    catch (std::exception& ba)
    {
        std::cerr << "BsplineGrid::createWeightsMatrix-- exception caught while reserving "<< npoints <<" values into triplet_list: " << ba.what() << '\n';
        exit(-1);
    }

    unsigned int row=0;

    for ( typename std::vector< PointType >::const_iterator iter = positions.begin(); iter != positions.end(); ++iter)
    {
        row = iter -positions.begin();

        if (weights[row]<0){
            triplet_list.push_back(TripletType(row,row,1));
        }else{
            triplet_list.push_back(TripletType(row,row,weights[row]));
        }
    }
    W.setFromTriplets(triplet_list.begin(), triplet_list.end());

    W.makeCompressed();
    return W;

}


/**
 * This function warps an image using the Bdpline grid, which must matcht the appropriate dimensions in the input and output.
 * By default uses linear interpolation
 */
template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
template <class ImageType>
typename ImageType::Pointer
BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::warp(typename ImageType::Pointer inputImage){

    typename ImageType::Pointer output = echo::initialiseImage<ImageType, ImageType>(inputImage);

    /// Convert all the image points to a point list
    std::vector<PointType> points;
    echo::ImageToPointList<ImageType, PointType>(output, points);
    /// Calculate deformation vectors at points
    std::vector<CoefficientType> displacements = this->evaluate(points);
    /// Apply the displacement to the current positions
    std::vector<CoefficientType> displaced_points = points + displacements;
    /// Retrieve the image values at those points
    typedef itk::LinearInterpolateImageFunction<ImageType,double> InterpolatorType;
    typename InterpolatorType::Pointer interpolator = InterpolatorType::New();
    interpolator->SetInputImage(inputImage);

    typename std::vector<CoefficientType  >::const_iterator cit ;


    // Make the lines
    typedef itk::ImageRegionIterator< ImageType > IteratorType;
    IteratorType out( output, output->GetRequestedRegion());
    typename ImageType::PixelType value;
    typename ImageType::PointType imagePoint;

    for (cit = displaced_points.begin(), out.GoToBegin(); cit != displaced_points.end(); ++cit,++out){

        for (unsigned int j=0; j<InputDimension; j++){
            imagePoint[j] = (*cit)[j];
        }

        value = interpolator->Evaluate(imagePoint);

        out.Set(value);

    }

    return output;
}


/**
 * This is actually much much slower than doing the matrix product!!!!
 */

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension> template <class DirectionType, class DopplerValueType>
void
BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::createAbMatrices(const std::vector<PointType> &positions, const std::vector<DirectionType> &directions,
                                                                        const std::vector<DopplerValueType> &values, const double remove_nodes_th,
                                                                        std::vector<unsigned int> &kept_nodes,
                                                                        std::vector<unsigned int> &kept_nodescorresponding_columns,
                                                                        SparseMatrixType & A, DenseVectorType & b){

    int ngridpoints = this->GetSizeWithBorder().prod(); // This takes into account border too. Is the same as this->m_ngridpoints
    int ncolumns = OutputDimension*ngridpoints;

    if (kept_nodes.size()){
        /// If the vector kept_nodes has been initialized, then it contains the indices of the columns that must be kept. These columns should
        /// Be in form c1 c2 c3 ... where each column actually accounts for 3 columns so we do multiply its size times the output dimension
        ncolumns = kept_nodes.size()*OutputDimension;
    }

    A.resize(ncolumns,ncolumns);
    b.resize(ncolumns);

    /// For each input data point, there would be up to (bsd+1)^InputDims * OutputDIms non zero coefficients
    unsigned int nnz_max = std::pow(this->m_bsd+1,InputDimension)*OutputDimension*OutputDimension;

    int cyclic_dimension = InputDimension-1; /// TODO this always assumes cyclic!

    std::vector<IndexType > gridpos;
    this->createNeighbourhoodPointers(gridpos); /// This has all the permutations  of the bspline neighborhood
    DenseVectorType column_summation = DenseVectorType::Zero(ngridpoints); /// This array contains the sum of each column, which we will use to remove gridpoints which have few input data points around
    //// ------------------------------------------

    /// What I do is I basically create a full A matrix for each input data point
    typename std::vector< PointType >::const_iterator p_iter_row, p_iter_column; /// iterator over positions
    typename std::vector< DirectionType >::const_iterator d_iter_row, d_iter_column; /// iterator over beam directions
    typename std::vector< DopplerValueType >::const_iterator v_iter; /// iterator over Doppler values
    int current_row1, current_row2;
    for (  p_iter_row = positions.begin(), d_iter_row=directions.begin(), v_iter = values.begin(); p_iter_row != positions.end(); ++p_iter_row,++d_iter_row, ++ v_iter)
    {

        current_row1 = p_iter_row-positions.begin();
        ContinuousIndexType p1 = this->GetContinuousIndex(*p_iter_row);
        p1[cyclic_dimension] = std::fmod(std::fmod(p1[cyclic_dimension],this->m_size_parent[cyclic_dimension])+this->m_size_parent[cyclic_dimension],this->m_size_parent[cyclic_dimension]);

        for (int node1=0; node1<gridpos.size(); node1++)
        {
            IndexType associated_grid_node1 = p1.floor()-gridpos[node1];
            IndexType associated_grid_node_modulus1(associated_grid_node1);
            associated_grid_node_modulus1[cyclic_dimension] = ((associated_grid_node1[cyclic_dimension]%this->m_size_parent[cyclic_dimension])+this->m_size_parent[cyclic_dimension]) % this->m_size_parent[cyclic_dimension];

            unsigned int current_column1 = this->nd_to_oned_index(associated_grid_node_modulus1);
            // here goes the code to deal with kept_nodes

            /// Pointer to the first of the OutputDimension columns
            MatrixElementType bspline_tensor1 = (echo::Bspline<PointType>(this->m_bsd,(p1 - associated_grid_node1))).prod();
            if (bspline_tensor1==0)
                continue;
            std::cout << "Point "<< current_row1  <<" of "<< positions.size()<< " node "<< node1<<std::endl;
            for (  p_iter_column = positions.begin(), d_iter_column=directions.begin(); p_iter_column != positions.end(); ++p_iter_column,++d_iter_column)
            {
                /// Create the 'triplets' (row, column,m value) provided by Eigen.
                std::vector<TripletType> triplet_list;

                try
                {
                    triplet_list.reserve(nnz_max);
                }
                catch (std::exception& ba)
                {
                    std::cerr << "BsplineGrid::createSamplingMatrix-- exception caught while reserving "<< nnz_max <<" values into triplet_list: " << ba.what() << '\n';
                    exit(-1);
                }

                current_row2 = p_iter_column-positions.begin();
                ContinuousIndexType p2 = this->GetContinuousIndex(*p_iter_column);
                p2[cyclic_dimension] = std::fmod(std::fmod(p2[cyclic_dimension],this->m_size_parent[cyclic_dimension])+this->m_size_parent[cyclic_dimension],this->m_size_parent[cyclic_dimension]);


                for (int node2=0; node2<gridpos.size(); node2++)
                {
                    IndexType associated_grid_node2 = p2.floor()-gridpos[node2];
                    IndexType associated_grid_node_modulus2(associated_grid_node2);
                    associated_grid_node_modulus2[cyclic_dimension] = ((associated_grid_node2[cyclic_dimension]%this->m_size_parent[cyclic_dimension])+this->m_size_parent[cyclic_dimension]) % this->m_size_parent[cyclic_dimension];

                    unsigned int current_column2 = this->nd_to_oned_index(associated_grid_node_modulus2);
                    // here goes the code to deal with kept_nodes

                    /// Pointer to the first of the OutputDimension columns
                    MatrixElementType bspline_tensor2 = (echo::Bspline<PointType>(this->m_bsd,(p2 - associated_grid_node2))).prod();
                    if (bspline_tensor2==0)
                        continue;
                    /// I add as many triplets as output dimensions!)
                    for (int k1=0; k1<OutputDimension; k1++)
                    {
                        for (int k2=0; k2<OutputDimension; k2++)
                        {
                            MatrixElementType current_value = bspline_tensor1* (*d_iter_row)[k1] * bspline_tensor2* (*d_iter_column)[k2];

                            triplet_list.push_back(TripletType(current_column1*OutputDimension+k1,current_column2*OutputDimension+k2,current_value));
                        }
                        //column_summation[current_column]+=current_value;
                    }
                }
                /// Build a one point A matrix
                std::vector<TripletType>(triplet_list).swap(triplet_list);
                SparseMatrixType A_tmp(ncolumns,ncolumns);
                A_tmp.setFromTriplets(triplet_list.begin(), triplet_list.end());
                //A_tmp.makeCompressed();
                A = A+A_tmp;
            }
            /// Now calculate vector b
        }
    }

    //    if (remove_nodes_th>=0)
    //    {
    //        /// Remove nodes which have few input data points around.
    //        /// If kept_nodes is empty, then we calculate how many points we should keep
    //        if (!kept_nodes.size())
    //        {
    //            double maximum_sum = column_summation.maxCoeff();
    //            DenseVectorType normalised_aggregated_node_value =echo::Eigen::powerArray<DenseVectorType>(column_summation/maximum_sum,1.0/ (double) OutputDimension);
    //            /// Find the values that are less than the actual threshold.
    //            echo::Eigen::addGtThan<DenseVectorType>(normalised_aggregated_node_value, remove_nodes_th, kept_nodes,corresponding_columns);
    //            /// Removing the nodes is the same as creating a new matrix with the kept nodes filled
    //            return this->createSamplingMatrix(positions, -1, kept_nodes,corresponding_columns);

    //        }
    //    }

}


/**
 * This uses "setFromTriplets
 * If remove_nodes_th is -1, there is no threshold.
 * If remove_nodes_th is -2, only non-zero coefficients are used
 * TODO make changes for cyclic behaviour
 */
template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
typename BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::SparseMatrixType
BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::createSamplingMatrix(const std::vector<PointType> &positions,
                                                                            const double remove_nodes_th,
                                                                            std::vector<unsigned int> &kept_nodes,
                                                                            std::vector<unsigned int> &corresponding_columns,
                                                                            int derivative_order)
{
    int npoints = positions.size();
    int ngridpoints = this->GetSizeWithBorder().prod(); // This takes into account border too. Is the same as this->m_ngridpoints
    int ncolumns = OutputDimension*ngridpoints;
    if (kept_nodes.size()){
        /// If the vector kept_nodes has been initialized, then it contains the indices of the columns that must be kept. These columns should
        /// Be in form c1 c2 c3 ... where each column actually accounts for 3 columns so we do multiply its size times the output dimension
        ncolumns = kept_nodes.size()*OutputDimension;
    }

    SparseMatrixType B(OutputDimension*npoints, ncolumns);

    /// Create the 'triplets' (row, column,m value) provided by Eigen.
    std::vector<TripletType> triplet_list;
    /// For each input data point, there would be up to (bsd+1)^InputDims * OutputDIms non zero coefficients
    unsigned int nnz_max = (std::pow((this->m_bsd+1), InputDimension))*OutputDimension*npoints;
    try
    {
        triplet_list.reserve(nnz_max);
    }
    catch (std::exception& ba)
    {
        std::cerr << "BsplineGrid::createSamplingMatrix-- exception caught while reserving "<< nnz_max <<" values into triplet_list: " << ba.what() << '\n';
        exit(-1);
    }
    std::vector<IndexType > gridpos;
    this->createNeighbourhoodPointers(gridpos); /// This has all the permutations  of the bspline neighborhood
    DenseVectorType column_summation = DenseVectorType::Zero(ngridpoints); /// This array contains the sum of each column, which we will use to remove gridpoints which have few input data points around

    myTimer t;
    t.start();
    /// Parallelize the following loop

    if (m_is_parallel && this->m_threads_to_use>1){

        boost::thread_group threads;

        std::vector< std::vector<TripletType> > list_of_triplet_lists(this->m_threads_to_use);
        std::vector<DenseVectorType> list_of_column_summations(this->m_threads_to_use);

        int npoints_per_thread = std::floor((double) positions.size()/ (double) this->m_threads_to_use);

        for (int i=0; i< this->m_threads_to_use; i++){
            list_of_column_summations[i] = DenseVectorType::Zero(ngridpoints);
            int first_index = i*npoints_per_thread, last_index = std::min((i+1)*npoints_per_thread-1, (int) positions.size()-1);
            if (i==this->m_threads_to_use-1){
                /// It is the last thread so we adjust
                last_index = positions.size()-1;
            }
            unsigned int nnz_max_block = (std::pow((this->m_bsd+1), InputDimension))*OutputDimension*(last_index-first_index+1);
            try
            {
                list_of_triplet_lists[i].reserve(nnz_max_block);
            }
            catch (std::exception& ba)
            {
                std::cerr << "BsplineGrid::createSamplingMatrix-- exception caught while reserving "<< nnz_max_block <<" in parallel values into triplet_list: " << ba.what() << '\n';
                exit(-1);
            }
        }

        for (int i=0; i< this->m_threads_to_use; i++){

            int first_index = i*npoints_per_thread, last_index = std::min((i+1)*npoints_per_thread-1, (int) positions.size()-1);
            if (i==this->m_threads_to_use-1){
                /// It is the last thread so we adjust
                last_index = positions.size()-1;
            }
            std::vector<PointType> point_block;
            try{
                point_block.reserve(last_index-first_index+1);
            }
            catch (std::exception& ba)
            {
                std::cerr << "BsplineGrid::createSamplingMatrix-- exception caught while reserving "<< last_index-first_index+1 <<" values into point_block: " << ba.what() << '\n';
                exit(-1);
            }

            point_block.insert(point_block.end(), positions.begin()+first_index, positions.begin()+last_index+1);


            if (this->m_cyclic_dimensions[InputDimension-1])
            {
                BSGThreadConfig config;
                config.cyclic_dimension = InputDimension-1;
                config.derivative_order = derivative_order;


                boost::thread *current_thread =
                        new boost::thread(&BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::m_fill_BMatrix_triplets_cyclic,this,
                                          point_block, gridpos, npoints_per_thread*i, kept_nodes, corresponding_columns,
                                          boost::ref(list_of_triplet_lists[i]), boost::ref(list_of_column_summations[i]),config);


                threads.add_thread(current_thread);

            } else {
                boost::thread *current_thread =
                        new boost::thread(&BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::m_fill_BMatrix_triplets,this,
                                          point_block, gridpos, npoints_per_thread*i, kept_nodes, corresponding_columns,
                                          boost::ref(list_of_triplet_lists[i]), boost::ref(list_of_column_summations[i]),derivative_order);

                threads.add_thread(current_thread);
            }

        }

        threads.join_all();

        for (int i=0; i< this->m_threads_to_use; i++){
            triplet_list.insert(triplet_list.end(),list_of_triplet_lists[i].begin(),list_of_triplet_lists[i].end());
            column_summation +=list_of_column_summations[i];
        }


    } else {
        const int INITIAL_ROW = 0;
        if (this->m_cyclic_dimensions[InputDimension-1])
        {
            BSGThreadConfig config;
            config.cyclic_dimension = InputDimension-1;
            config.derivative_order = derivative_order;

            this->m_fill_BMatrix_triplets_cyclic(positions, gridpos, INITIAL_ROW, kept_nodes, corresponding_columns, triplet_list, column_summation,config);
        } else {
            this->m_fill_BMatrix_triplets(positions, gridpos, INITIAL_ROW, kept_nodes, corresponding_columns, triplet_list, column_summation,derivative_order);
        }
        if (triplet_list.size() > nnz_max){
            std::cout << "BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::createSamplingMatrix -- WARNING: ";
            std::cout<< "the number of estimated non-zero elements ("<< nnz_max<<") ";
            std::cout << "was lower than the actual number of non-zero elements ("<< triplet_list.size()<<"). ";
            std::cout << "This will bring in significant performance drop."<<std::endl;
        }
    }

    std::vector<TripletType>(triplet_list).swap(triplet_list);

    if (this->m_debug) std::cout << "\t\t\t\t\tcreateSamplingMatrix::Loop  takes "<< t.GetSeconds()<< " s, to fill "<< triplet_list.size()<< " triplets"<<std::endl;

    /// Deal with removing points
    B.setFromTriplets(triplet_list.begin(), triplet_list.end());
    B.makeCompressed();


    /// Remove nodes which have few input data points around.
    /// If kept_nodes is empty, then we calculate how many points we should keep.
    /// We always need this for when we later do setAll!
    if (!kept_nodes.size())
    {
        MatrixElementType maximum_sum = column_summation.maxCoeff();
        DenseVectorType normalised_aggregated_node_value =echo::Eigen::powerArray<DenseVectorType>(column_summation/maximum_sum,1.0/ (double) OutputDimension);
        /// Find the values that are less than the actual threshold.
        echo::Eigen::addGtThan<DenseVectorType>(normalised_aggregated_node_value, remove_nodes_th, kept_nodes,corresponding_columns);
        /// Removing the nodes is the same as creating a new matrix with the kept nodes filled
        if (remove_nodes_th>=0)
        {
            return this->createSamplingMatrix(positions, -1, kept_nodes,corresponding_columns);

        }
    }
    return B;
}



template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
void BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::m_fill_BMatrix_triplets(const std::vector<PointType> &positions,
                                                                                    const std::vector<IndexType> &gridpos, const int initial_row,
                                                                                    const std::vector<unsigned int> &kept_nodes, const std::vector<unsigned int> &corresponding_columns,
                                                                                    std::vector<TripletType> &triplet_list, DenseVectorType &column_summation,
                                                                                    int derivative_order) {


    //    if (this->m_debug && this->m_is_parallel && this->m_threads_to_use>1){
    //        m_coutmutex.lock();
    //        std::cout << "\t\t\tStart thread: TID " << boost::this_thread::get_id() << " with " << positions.size() <<" elements, initial row is  "<< initial_row<< std::endl;
    //        m_coutmutex.unlock();
    //    }
    int current_row;
    for (int node=0; node<gridpos.size(); node++)
    {
        current_row=initial_row-1;
        //        if (this->m_debug && this->m_is_parallel && this->m_threads_to_use>1 && node==176){
        //            m_coutmutex.lock();
        //            std::cout << "\t\t\t\tthread: TID " << boost::this_thread::get_id() << " node " << node << " of " << gridpos.size()<<", row "<< current_row << " grid "<< this->toString()<< std::endl;
        //            m_coutmutex.unlock();
        //        }

        for ( typename std::vector< PointType >::const_iterator iter = positions.begin(); iter != positions.end(); ++iter)
        {
            ++current_row; /// Row corresponds to point

            ContinuousIndexType p = this->GetContinuousIndex(*iter);
            //            if (this->m_debug && this->m_is_parallel && this->m_threads_to_use>1 && node==176){
            //                m_coutmutex.lock();
            //                std::cout << "\t\t\t\t\tthread: TID " << boost::this_thread::get_id() << "continuous index "<< p << std::endl;
            //                m_coutmutex.unlock();
            //            }
            //std::cout << *iter<<std::endl;
            IndexType associated_grid_node = p.floor()-gridpos[node];
            //IndexType associated_grid_node = p.floor()-gridpos;
            /// Since the input data points might be in the border of the patch, some points will not be affected by all -2,
            /// -1 0 and 1 nodes. As a result, here we have to check of the point is within bounds for the particular node

            int current_column = this->nd_to_oned_index(associated_grid_node);
            if (current_column==OUT_OF_PATCH)
            {
                continue;
            }
            //            if (this->m_debug && this->m_is_parallel && this->m_threads_to_use>1 && node==176){
            //                m_coutmutex.lock();
            //                std::cout << "\t\t\t\t\tthread: TID " << boost::this_thread::get_id() << "Current column "<< current_column << std::endl;
            //                m_coutmutex.unlock();
            //            }
            if (kept_nodes.size()){
                /// If the current column is in the kept_nodes array I continue, otherwise I ignore it.
                bool ignore_column=true;
                //for( int idx=0; idx<kept_nodes.size(); idx+=OutputDimension) {
                for( int idx=0; idx<kept_nodes.size(); idx++) {
                    if (current_column==kept_nodes[idx] ){
                        ignore_column = false;
                        current_column = corresponding_columns[current_column];
                        if (current_column == OUT_OF_PATCH){
                            m_coutmutex.lock();
                            std::cout << "BsplineGrid::m_fill_BMatrix_triplets - Something went wrong. There were removed nodes, and when trying to calculate the corresponding node, the result was -1."<<std::endl;
                            m_coutmutex.unlock();
                            exit(-1);
                        }
                        break;
                    }
                }
                if (ignore_column){
                    continue;
                }
            }
            //MatrixElementType current_value = (echo::Bspline<PointType>(this->m_bsd,(p - associated_grid_node))).prod();

            MatrixElementType current_value;
            switch (derivative_order){
            case 0:
                current_value = (echo::Bspline<PointType>(this->m_bsd,(p - associated_grid_node))).prod();
                break;
            case 1:
                current_value = (echo::dBspline<PointType>(this->m_bsd,(p - associated_grid_node))/this->m_spacing).prod();
                break;
            default:
                current_value = 0;
                std::cerr<< "BsplineGrid::m_fill_BMatrix_triplets::Only derivatives of order 0 and 1 are implemented"<<std::endl;
                break;
            }

            if ((current_value<=0 && derivative_order==0) || current_value==0) // B-splines are always positives but derivatives not
                continue;
            /// I add as many triplets as output dimensions!)
            for (int k=0; k<OutputDimension; k++)
            {
                triplet_list.push_back(TripletType(current_row*OutputDimension+k,current_column*OutputDimension+k,current_value));
            }
            column_summation[current_column]+=current_value;
        }
    }

    //    if (this->m_debug && this->m_is_parallel && this->m_threads_to_use>1){
    //        m_coutmutex.lock();
    //        std::cout << "\t\t\tFinish thread: TID " << boost::this_thread::get_id() << " put into triplet list "<< triplet_list.size()<< " elements."<< std::endl;
    //        m_coutmutex.unlock();
    //    }
}

/**
 *  This function considers that the  dimension 'cyclic_dimension' is cyclic
 *  kept_nodes is the index of nonzero coefficients!
 */
template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
void BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::m_fill_BMatrix_triplets_cyclic(const std::vector<PointType> &positions,
                                                                                           const std::vector<IndexType> &gridpos, int initial_row,
                                                                                           const std::vector<unsigned int> &kept_nodes, const std::vector<unsigned int> &corresponding_columns,
                                                                                           std::vector<TripletType> &triplet_list, DenseVectorType &column_summation,
                                                                                           BSGThreadConfig &config) {
    unsigned int cyclic_dimension = config.cyclic_dimension;
    int derivative_order = config.derivative_order;


    //unsigned int cyclic_dimension=3;

    //    if (this->m_debug && this->m_is_parallel && this->m_threads_to_use>1){
    //        m_coutmutex.lock();
    //        std::cout << "\t\t\tStart thread: TID " << boost::this_thread::get_id() << " with " << positions.size() <<" elements, initial row is  "<< initial_row<< std::endl;
    //        m_coutmutex.unlock();
    //    }
    int current_row;
    for (int node=0; node<gridpos.size(); node++)
    {
        current_row=initial_row-1;

        for ( typename std::vector< PointType >::const_iterator iter = positions.begin(); iter != positions.end(); ++iter)
        {
            ++current_row; /// Row corresponds to point

            ContinuousIndexType p = this->GetContinuousIndex(*iter);
            /// I have to get the modulus here!
            p[cyclic_dimension] = std::fmod(std::fmod(p[cyclic_dimension],this->m_size_parent[cyclic_dimension])+this->m_size_parent[cyclic_dimension],this->m_size_parent[cyclic_dimension]);
            IndexType associated_grid_node = p.floor()-gridpos[node];
            IndexType associated_grid_node_modulus(associated_grid_node);
            associated_grid_node_modulus[cyclic_dimension] = ((associated_grid_node[cyclic_dimension]%this->m_size_parent[cyclic_dimension])+this->m_size_parent[cyclic_dimension]) % this->m_size_parent[cyclic_dimension];
            /// The modulus has to be with respect to the full grid, so we need a member variable which stores the actual size (no borders) of the full grid
            /// In the following I call % twice because I need the true modulus, not the remainder (i.e. has to work for negative values too!)
            // associated_grid_node[cyclic_dimension] = std::floor(((associated_grid_node[cyclic_dimension]%this->m_size_parent[cyclic_dimension])+this->m_size_parent[cyclic_dimension])%this->m_size_parent[cyclic_dimension]);
            //            if (this->m_debug){
            //                std::cout << " Associated grid node is "<< associated_grid_node[cyclic_dimension] << " for a grid size of "<< this->m_size[cyclic_dimension] << " and a continuous index of "<< p<<std::endl;
            //            }
            /// matlab code :   floor(mod(gr_positions(:,i) - ones(size(gr_positions,1),1)*grpos(node,i),gr.size(i)))+1; % plus one because nodes start at 1 here


            /// Since the input data points might be in the border of the patch, some points will not be affected by all -2,
            /// -1 0 and 1 nodes. As a result, here we have to check of the point is within bounds for the particular node

            unsigned int current_column = this->nd_to_oned_index(associated_grid_node_modulus);
            if (current_column==OUT_OF_PATCH)
            {
                continue;
            }

            if (kept_nodes.size()){
                /// If the current column is in the kept_nodes array I continue, otherwise I ignore it.
                bool ignore_column=true;
                for( int idx=0; idx<kept_nodes.size(); idx++) {
                    if (current_column==kept_nodes[idx] ){
                        ignore_column = false;
                        current_column = corresponding_columns[current_column];
                        if (current_column == OUT_OF_PATCH){
                            m_coutmutex.lock();
                            std::cout << "BsplineGrid::m_fill_BMatrix_triplets_cycliT - Something went wrong. There were removed nodes, and when trying to calculate the corresponding node, the result was -1."<<std::endl;
                            m_coutmutex.unlock();
                            exit(-1);
                        }
                        break;
                    }
                }
                if (ignore_column){
                    continue;
                }
            }
            /// Pointer to the first of the OutputDimension columns
            MatrixElementType current_value;
            switch (derivative_order){
            case 0:
                current_value = (echo::Bspline<PointType>(this->m_bsd,(p - associated_grid_node))).prod();
                break;
            case 1:
                current_value = (echo::dBspline<PointType>(this->m_bsd,(p - associated_grid_node))/this->m_spacing).prod();
                break;
            default:
                current_value = 0;
                std::cerr<< "BsplineGrid::m_fill_BMatrix_triplets::Only derivatives of order 0 and 1 are implemented"<<std::endl;
                break;
            }

            if ((current_value<=0 && derivative_order==0) || current_value==0) // B-splines are always positives but derivatives not
                continue;
            /// I add as many triplets as output dimensions!)
            for (int k=0; k<OutputDimension; k++)
            {
                triplet_list.push_back(TripletType(current_row*OutputDimension+k,current_column*OutputDimension+k,current_value));
            }
            column_summation[current_column]+=current_value;
        }
    }

    //    if (this->m_debug && this->m_is_parallel && this->m_threads_to_use>1){
    //        m_coutmutex.lock();
    //        std::cout << "\t\t\tFinish thread: TID " << boost::this_thread::get_id() << " put into triplet list "<< triplet_list.size()<< " elements."<< std::endl;
    //        m_coutmutex.unlock();
    //    }
}







template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
void  BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::GetBorderBounds(double *bounds)
{
    for (int i=0; i<InputDimension; i++)
    {
        bounds[2*i]=this->m_origin[i]+this->m_borderIndex1[i]*this->m_spacing[i];
        bounds[2*i+1]=this->m_origin[i]+this->m_borderIndex2[i]*this->m_spacing[i];
    }
}

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
void  BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::GetBorderBoundsOfInfluence(double *bounds)
{
    for (int i=0; i<InputDimension; i++)
    {
        //bounds[2*i]=this->m_origin[i]+this->m_borderIndex1[i]*this->m_spacing[i]-this->m_spacing[i]*std::ceil((double) this->m_bsd/2.0);
        //bounds[2*i+1]=this->m_origin[i]+this->m_borderIndex2[i]*this->m_spacing[i]+this->m_spacing[i]*std::ceil((double) this->m_bsd/2.0);

        /// Only the points within the border must be added. See the paper.
        bounds[2*i]=this->m_origin[i]+this->m_borderIndex1[i]*this->m_spacing[i];
        bounds[2*i+1]=this->m_origin[i]+this->m_borderIndex2[i]*this->m_spacing[i];
    }
}


template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
void  BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::GetBounds(double *bounds)
{
    for (int i=0; i<InputDimension; i++)
    {
        bounds[2*i]=this->m_origin[i]+this->m_index[i]*this->m_spacing[i];
        bounds[2*i+1]=this->m_origin[i]+(this->index[i]+this->m_size[i]-1)*this->m_spacing[i];
    }
}

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
void  BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::GetBoundsOfInfluence(double *bounds)
{
    for (int i=0; i<InputDimension; i++)
    {
        bounds[2*i]=this->m_origin[i]+this->m_index1[i]*this->m_spacing[i]-this->m_spacing[i]*std::ceil((double) this->m_bsd/2.0);
        bounds[2*i+1]=this->m_origin[i]+this->m_index2[i]*this->m_spacing[i]+this->m_spacing[i]*std::ceil((double) this->m_bsd/2.0);
    }
}


template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
void  BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::writeAsVTKDataSet(std::string &filename)
{

    vtkSmartPointer<vtkStructuredGrid> dataSet= vtkSmartPointer<vtkStructuredGrid>::New();
    dataSet->SetDimensions(&(this->m_size[0]));

    /// Calculate the points
    vtkSmartPointer<vtkPoints> points = vtkPoints::New();
    points->SetNumberOfPoints(this->m_size[0]*this->m_size[1]*this->m_size[2]);

    vtkSmartPointer<vtkFloatArray> coefficients = vtkSmartPointer<vtkFloatArray>::New();
    coefficients->SetNumberOfComponents(3);
    coefficients->SetNumberOfTuples(this->m_size[0]*this->m_size[1]*this->m_size[2]);

    int idx=0;
    float x[3];
    for ( int k=0; k<this->m_size[2]; k++)
    {
        x[2] = k*this->m_spacing[2]+this->m_origin[2]+m_index[2]*m_spacing[2];
        for (int j=0; j<this->m_size[1]; j++)
        {
            x[1] = j*this->m_spacing[1]+this->m_origin[1]+m_index[1]*m_spacing[1];
            for (int i=0; i<this->m_size[0]; i++)
            {
                x[0] = i*this->m_spacing[0]+this->m_origin[0]+m_index[0]*m_spacing[0];
                points->InsertPoint(idx, x);
                coefficients->InsertTuple(idx,&(m_data[idx][0]));
                idx++;
            }
        }
    }

    dataSet->SetPoints(points);
    dataSet->GetPointData()->SetVectors(coefficients);

    vtkSmartPointer<vtkDataSetWriter> writer  =vtkSmartPointer<vtkDataSetWriter>::New();
    writer->SetInputData(dataSet);
    writer->SetFileName(filename.c_str());
    writer->Write();

}

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
std::string  BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::toString(){

    std::string output("\n");
    output +=  "Grid index:\t\t";
    for (int i=0; i<InputDimension; i++)
        output+=echo::str::toString(this->m_index[i])+"\t";
    output+= "\n";
    output+=  "Grid size:\t\t";
    for (int i=0; i<InputDimension; i++)
        output+=echo::str::toString(this->m_size[i])+"\t";
    output+= "\n";
    output+=  "Grid parent size:\t\t";
    for (int i=0; i<InputDimension; i++)
        output+=echo::str::toString(this->m_size_parent[i])+"\t";
    output+= "\n";
    output+=  "Grid border:\t\t"+echo::str::toString(this->m_border)+ "\n";
    output+=  "Grid border index:\t\t";
    for (int i=0; i<InputDimension; i++)
    {
        output+="("+echo::str::toString(this->m_borderIndex1[i])+","+echo::str::toString(this->m_borderIndex2[i])+")\t";
    }
    output+= "\n";
    output+=  "Bspline degree:\t"+echo::str::toString(this->m_bsd) + "\n";
    output+=  "Grid spacing:\t";
    for (int i=0; i<InputDimension; i++)
        output+=echo::str::toString(this->m_spacing[i])+"\t";
    output+= "\n";
    output+=  "Grid origin:\t";
    for (int i=0; i<InputDimension; i++)
        output+=echo::str::toString(this->m_origin[i])+"\t";
    output+= "\n";

    return output;
}



template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
void  BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::writeAsVectorImage(std::string &filename)
{

    if (InputDimension==2){
        this->writeAs2DVectorImage(filename);
    }else if (InputDimension==3){
        this->writeAs3DVectorImage(filename);
    } if (InputDimension==4){
        this->writeAs4DVectorImage(filename);
    }

}

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
void  BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::writeAs4DVectorImage(std::string &filename)
{

    const unsigned int NDIMSIN =4;
    const unsigned int NDIMSOUT =3;
    typedef itk::Image<itk::Vector<MatrixElementType,NDIMSOUT>,NDIMSIN> ImageType;
    typedef itk::ImageFileWriter<ImageType> WriterType;

    /// Create image
    ImageType::Pointer image = ImageType::New();
    ImageType::RegionType region;
    ImageType::SpacingType spacing;
    ImageType::PointType origin;

    ImageType::IndexType start;
    ImageType::SizeType size;
    for (int i=0; i<NDIMSIN; i++)
    {
        start[i] = 0;
        size[i] = this->m_size[i];
        spacing[i]=this->m_spacing[i];
        origin[i]=this->m_origin[i]+m_index[i]*m_spacing[i];
    }

    region.SetSize(size);
    region.SetIndex(start);
    image->SetRegions(region);
    image->Allocate();
    image->SetSpacing(spacing);
    image->SetOrigin(origin);

    typedef itk::ImageRegionIterator< ImageType > IteratorType;
    IteratorType out( image, image->GetRequestedRegion() );

    ImageType::IndexType currentOutputIndex;
    for ( out.GoToBegin(); !out.IsAtEnd(); ++out)
    {
        currentOutputIndex=out.GetIndex();
        IndexType myindex((long unsigned int*) &currentOutputIndex[0]);
        myindex+=this->m_index;
        CoefficientType coef = this->Get(myindex);
        ImageType::PixelType pixel;
        for (int idx=0; idx<NDIMSOUT; idx++)
        {
            pixel[idx]= coef[idx];
        }
        out.Set(pixel);
    }

    /// Write to file
    WriterType::Pointer writer = WriterType::New();
    writer->SetInput(image);
    writer->SetFileName(filename);
    writer->Write();


}


template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
void  BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::writeAs3DVectorImage(std::string &filename)
{

    const unsigned int NDIMSIN =3;
    const unsigned int NDIMSOUT =3;
    typedef itk::Image<itk::Vector<MatrixElementType,NDIMSOUT>,NDIMSIN> ImageType;
    typedef itk::ImageFileWriter<ImageType> WriterType;

    /// Create image
    ImageType::Pointer image = ImageType::New();
    ImageType::RegionType region;
    ImageType::SpacingType spacing;
    ImageType::PointType origin;

    ImageType::IndexType start;
    ImageType::SizeType size;
    for (int i=0; i<NDIMSIN; i++)
    {
        start[i] = 0;
        size[i] = this->m_size[i];
        spacing[i]=this->m_spacing[i];
        origin[i]=this->m_origin[i]+m_index[i]*m_spacing[i];
    }

    region.SetSize(size);
    region.SetIndex(start);
    image->SetRegions(region);
    image->Allocate();
    image->SetSpacing(spacing);
    image->SetOrigin(origin);

    typedef itk::ImageRegionIterator< ImageType > IteratorType;
    IteratorType out( image, image->GetRequestedRegion() );

    ImageType::IndexType currentOutputIndex;
    for ( out.GoToBegin(); !out.IsAtEnd(); ++out)
    {
        currentOutputIndex=out.GetIndex();
        IndexType myindex((long unsigned int*) &currentOutputIndex[0]);
        myindex+=this->m_index;
        CoefficientType coef = this->Get(myindex);
        ImageType::PixelType pixel;
        for (int idx=0; idx<NDIMSOUT; idx++)
        {
            pixel[idx]= coef[idx];
        }
        out.Set(pixel);
    }

    /// Write to file
    WriterType::Pointer writer = WriterType::New();
    writer->SetInput(image);
    writer->SetFileName(filename);
    writer->Write();


}

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
void  BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::writeAs2DVectorImage(std::string &filename)
{

    const unsigned int NDIMSIN =2;
    const unsigned int NDIMSOUT =2;
    typedef itk::Image<itk::Vector<MatrixElementType,NDIMSOUT>,NDIMSIN> ImageType;
    typedef itk::ImageFileWriter<ImageType> WriterType;

    /// Create image
    ImageType::Pointer image = ImageType::New();
    ImageType::RegionType region;
    ImageType::SpacingType spacing;
    ImageType::PointType origin;

    ImageType::IndexType start;
    ImageType::SizeType size;
    for (int i=0; i<NDIMSIN; i++)
    {
        start[i] = 0;
        size[i] = this->m_size[i];
        spacing[i]=this->m_spacing[i];
        origin[i]=this->m_origin[i]+m_index[i]*m_spacing[i];
    }

    region.SetSize(size);
    region.SetIndex(start);
    image->SetRegions(region);
    image->Allocate();
    image->SetSpacing(spacing);
    image->SetOrigin(origin);

    typedef itk::ImageRegionIterator< ImageType > IteratorType;
    IteratorType out( image, image->GetRequestedRegion() );

    ImageType::IndexType currentOutputIndex;
    for ( out.GoToBegin(); !out.IsAtEnd(); ++out)
    {
        currentOutputIndex=out.GetIndex();
        IndexType myindex((long unsigned int*) &currentOutputIndex[0]);
        myindex+=this->m_index;
        CoefficientType coef = this->Get(myindex);
        ImageType::PixelType pixel;
        for (int idx=0; idx<NDIMSOUT; idx++)
        {
            pixel[idx]= coef[idx];
        }
        out.Set(pixel);
    }

    /// Write to file
    WriterType::Pointer writer = WriterType::New();
    writer->SetInput(image);
    writer->SetFileName(filename);
    writer->Write();


}

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
void  BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::writeAs3DImage(std::string &filename)
{

    const unsigned int NDIMSIN =3;
    typedef itk::Image<MatrixElementType,NDIMSIN> ImageType;
    typedef itk::ImageFileWriter<ImageType> WriterType;

    /// Create image
    ImageType::Pointer image = ImageType::New();
    ImageType::RegionType region;
    ImageType::SpacingType spacing;
    ImageType::PointType origin;

    ImageType::IndexType start;
    ImageType::SizeType size;
    for (int i=0; i<NDIMSIN; i++)
    {
        start[i] = 0;
        size[i] = this->m_size[i];
        spacing[i]=this->m_spacing[i];
        origin[i]=this->m_origin[i]+m_index[i]*m_spacing[i];
    }

    region.SetSize(size);
    region.SetIndex(start);
    image->SetRegions(region);
    image->Allocate();
    image->SetSpacing(spacing);
    image->SetOrigin(origin);

    typedef itk::ImageRegionIterator< ImageType > IteratorType;
    IteratorType out( image, image->GetRequestedRegion() );

    ImageType::IndexType currentOutputIndex;
    for ( out.GoToBegin(); !out.IsAtEnd(); ++out)
    {
        currentOutputIndex=out.GetIndex();
        IndexType myindex((long unsigned int*) &currentOutputIndex[0]);
        myindex+=this->m_index;
        CoefficientType coef = this->Get(myindex);
        ImageType::PixelType pixel;
        pixel= coef[0];
        out.Set(pixel);
    }


    /// Write to file
    WriterType::Pointer writer = WriterType::New();
    writer->SetInput(image);
    writer->SetFileName(filename);
    writer->Write();


}

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
typename BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::DenseVectorType
BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::GetCoefficientVector(std::vector<unsigned int> & nonzero_coefficients, std::vector<unsigned int> & corresponding_nonzero_coefficients, bool getall){

    DenseVectorType coefficients;//(0,0);

    if (getall){
        coefficients.resize(this->m_ngridpoints*OutputDimension);
        typename std::vector<CoefficientType>::const_iterator const_iter;
        MatrixElementType *eigen_iter;
        for (const_iter = this->m_data.begin(), eigen_iter= coefficients.data();const_iter!=this->m_data.end();  ++const_iter)
        {
            int current_idx = (const_iter-this->m_data.begin());
            for (int k=0; k<OutputDimension; k++)
            {
                (*eigen_iter)=(*const_iter)[k];
                ++eigen_iter;
            }
        }

        corresponding_nonzero_coefficients.resize(this->m_ngridpoints);
        typename std::vector<unsigned int>::iterator iter;
        for (iter = corresponding_nonzero_coefficients.begin();iter!=corresponding_nonzero_coefficients.end();  ++iter)
        {
            *iter = iter-corresponding_nonzero_coefficients.begin();
        }

    } else {
        this->nonZeroCoefficients(nonzero_coefficients, corresponding_nonzero_coefficients);
		int a = OutputDimension;
        coefficients.resize(nonzero_coefficients.size()*OutputDimension);

        if (!coefficients.size()){
            /// All coefficients are zero. We need do nothing.
            return coefficients;
        }

        typename std::vector<CoefficientType>::const_iterator const_iter;
        MatrixElementType *eigen_iter;

        for (const_iter = this->m_data.begin(), eigen_iter= coefficients.data();const_iter!=this->m_data.end();  ++const_iter)
        {
            //int current_idx = (const_iter-this->m_data.begin())*OutputDimension;
            int current_idx = (const_iter-this->m_data.begin());
            if (corresponding_nonzero_coefficients[current_idx]!=OUT_OF_PATCH){
                for (int k=0; k<OutputDimension; k++)
                {
                    (*eigen_iter)=(*const_iter)[k];
                    ++eigen_iter;
                }
            }
        }
    }
    return coefficients;

}


template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
std::vector<typename BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::CoefficientType> BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::evaluate(const std::vector<PointType> &pointList)
{
    /// Build the sampling  matrix
    double NZ_COEFFICIENTS =-1; /// Use only the coefficients given by kept_nodes or all coefficients if kept_nodes is empty

    /// Build the coefficient vector
    myTimer t;

    std::vector<CoefficientType> data_list(pointList.size());

    //std::string filename("/media/AGOMEZ_KCL_2/data/patients/patient-CAS_17_07Jan2013_DICOM/tmp/B_calculate.txt");
    //echo::Eigen::writeMatrixToFile<SparseMatrixType>(B, filename);

    /// Calculate the coefficient vector

    std::vector<unsigned int> nonzero_coefficients(0);
    std::vector<unsigned int> corresponding_nonzero_coefficients(0);

    DenseVectorType coefficients = GetCoefficientVector(nonzero_coefficients,corresponding_nonzero_coefficients);
    if (!coefficients.size()){
        /// All coefficients are zero. We need do nothing.
        return data_list;
    }

    /// Split into blocks to preserve memory

    double B_TO_MB = 1.0/1024.0/1024.0;
    double GB_TO_MB = 1024.0;
    double navailable_bytes;
    if (this->m_max_memory<0){
        unsigned long int navailable_bytes_ = echo::getAvailableSystemMemory();
        navailable_bytes = navailable_bytes_*B_TO_MB;
    } else{
        navailable_bytes = this->m_max_memory*GB_TO_MB;
    }

    /// Calculate approximately how much memory will our problem require.
    double bytes_required_B = 1.0*sizeof(MatrixElementType)* pointList.size()*std::pow(this->m_bsd+1,InputDimension)*OutputDimension*B_TO_MB;
    if (this->m_debug) std::cout << "\t\t\tAvailable memory: "<< navailable_bytes<<" MB, required memory: "<< bytes_required_B<< " MB"<<std::endl;std::cout.flush ();
    unsigned int  number_of_blocks = std::ceil((double) bytes_required_B/ (double) navailable_bytes)*this->m_threads_to_use;/// +1 to leave some memory just in case
    unsigned int block_length = std::ceil( (double) pointList.size() / (double) number_of_blocks);

    if (m_debug) std::cout << "\t\t\t\tCreate the sampling matrix in "<< number_of_blocks<< " blocks"<<std::endl;
    if (m_debug) std::cout.flush ();
    if (m_debug) t.start();


    /// These are the iterators for the whole list
    typename std::vector<CoefficientType>::iterator iter2;
    iter2 = data_list.begin();
    typename std::vector<PointType>::const_iterator const_iter2;
    const_iter2 = pointList.begin();
    for (int bl=0; bl<number_of_blocks; bl++)
    {
        unsigned int first = bl*block_length;
        unsigned int last = std::min((bl+1)*block_length-1, (unsigned int) pointList.size()-1);

        if (last < first){
            number_of_blocks = bl;
            break;
        }

        if (m_debug) std::cout << "\t\t\t\t\tBlock "<< bl+1 <<" of "<< number_of_blocks<< ", the first point is "<<first  <<" and the second is "<<last <<std::endl;
        if (m_debug) std::cout.flush ();

        std::vector<PointType> current_pointList(last-first+1);
        typename std::vector<PointType>::iterator iter;

        if (m_debug) std::cout << "\t\t\t\t\t\tThe point list has then "<< current_pointList.size() <<" points"<<std::endl;
        if (m_debug) std::cout.flush ();

        for (iter = current_pointList.begin(); iter!=current_pointList.end();  ++iter, ++const_iter2){
            *iter = *const_iter2;
        }

        SparseMatrixType B = this->createSamplingMatrix(current_pointList, NZ_COEFFICIENTS, nonzero_coefficients,corresponding_nonzero_coefficients);/// Reorder is assumed!

        if (m_debug) std::cout << "\t\t\t\t\t\tThe B matrix has been created of size "<< B.rows()<<" x "<< B.cols()<<std::endl;
        if (m_debug) std::cout.flush ();

        /// Calculate the values
        DenseVectorType values = B*coefficients;

        /// Put them into the result vector

        MatrixElementType *eigen_iter2;
        for (eigen_iter2= values.data();  eigen_iter2<values.data()+values.size();  ++iter2, ++eigen_iter2)
        {
            for (int k=0; k<OutputDimension; ++k, ++eigen_iter2)
            {
                (*(iter2))[k] = *eigen_iter2;
            }
            --eigen_iter2;
        }

        if (m_debug) std::cout << "\t\t\t\t\t\tBlock "<< bl <<" of "<< number_of_blocks<< ", with npoints="<<values.size() <<" done"<<std::endl;
        if (m_debug) std::cout.flush ();
    }
    if (m_debug) std::cout << "\t\t\t\tCreate the sampling matrix in "<< number_of_blocks<< " blocks took " << t.GetSeconds()<<" seconds"<<std::endl;
    return data_list;

}

template <class VALUETYPE, unsigned int InputDimension, unsigned int OutputDimension>
typename BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::CoefficientType BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::evaluate(const PointType &p)
{

    std::vector<PointType> pointList(1);
    pointList[0]=p;
    std::vector<typename BsplineGrid<VALUETYPE,InputDimension,OutputDimension>::CoefficientType> data_list = this->evaluate(pointList);
    return data_list[0];


}


namespace echo
{

/**
 * This function creates only a scalar image.
 */
template <class InputImage, class BsplineGridType>
typename InputImage::Pointer calculateImage(const typename InputImage::Pointer referenceImage,
                                            const std::vector<typename BsplineGridType::Pointer> &control_points,
                                            unsigned int nlevels,  double th)
{
    bool m_debug = true;
    myTimer t0;
    if (m_debug) t0.start();
    myTimer t;
    if (m_debug) std::cout << "\tInitialise image takes ";
    if (m_debug) t.start();
    typename InputImage::Pointer outputImage = echo::initialiseImage<InputImage,InputImage>(referenceImage);
    if (m_debug) std::cout << t.GetSeconds()<< " seconds" << std::endl;
    /// Generate an array with all the positions
    if (m_debug) std::cout << "\tA few operations take ";
    if (m_debug) t.start();
    std::vector<typename BsplineGridType::PointType > pointList;

    typedef itk::ImageRegionConstIterator< InputImage > ConstIteratorType;
    typedef itk::ImageRegionIterator< InputImage > IteratorType;
    ConstIteratorType in1( referenceImage, referenceImage->GetRequestedRegion());
    typename InputImage::IndexType currentIndex;
    typename InputImage::PointType currentInputPos;
    typename BsplineGridType::PointType point;

    typename InputImage::SizeType itkSize =  outputImage->GetLargestPossibleRegion().GetSize();
    typename BsplineGridType::IndexType tmp_index((long unsigned int *)&itkSize[0]);
    if (m_debug) std::cout << t.GetSeconds()<< " seconds" << std::endl;
    if (m_debug) std::cout << "\tAllocate for the point list take ";
    t.start();
    try{
        pointList.reserve(tmp_index.prod());/// allocate in excess
    }
    catch (std::exception& ba)
    {
        std::cerr << "BsplineGrid::calculateImage-- exception caught while reserving "<< tmp_index.prod() <<" values into pointist: " << ba.what() << '\n';
        exit(-1);
    }
    if (m_debug) std::cout << t.GetSeconds()<< " seconds" << std::endl;
    /// Find the positions of the pixels that are not zero

    if (m_debug) std::cout << "\tLoop to find nonzero positions takes ";
    if (m_debug) t.start();
    for ( in1.GoToBegin(); !in1.IsAtEnd(); ++in1)
    {
        if (in1.Get()>th)
        {
            referenceImage->TransformIndexToPhysicalPoint(in1.GetIndex(),currentInputPos);
            for (int i=0; i<InputImage::ImageDimension; i++)
                point[i]=currentInputPos[i];

            pointList.push_back(point);
        }

    }
    if (m_debug) std::cout << t.GetSeconds()<< " seconds" << std::endl;


    /// Evaluate and add data at each scale

    typename InputImage::PixelType pixel;

    typename std::vector<typename BsplineGridType::CoefficientType>::const_iterator const_iter_coef;

    if (m_debug) std::cout << "\tCalculate the values "<<std::endl;
    for (int i=0; i<nlevels; i++)
    {
        if (m_debug)
        {
            std::cout << "\t\tLevel "<< i <<std::endl;
            std::cout << "\t\t\tevaluate"<<std::endl;
        }
        std::vector<typename BsplineGridType::CoefficientType> data_list =control_points[i]->evaluate(pointList);
        /// Copy into a DenseVector to print

        /// add the contents of data list to each pixel in the image
        if (m_debug) std::cout << "\t\t\tAdd the values to the actual image takes ";
        if (m_debug) t.start();
        IteratorType out( outputImage, outputImage->GetRequestedRegion() );
        ConstIteratorType in2( referenceImage, referenceImage->GetRequestedRegion());
        for ( in2.GoToBegin(), out.GoToBegin(), const_iter_coef = data_list.begin(); !out.IsAtEnd(); ++out, ++in2)
        {
            pixel = out.Get();
            if (in2.Get()>th)
            {
                pixel+=(typename InputImage::PixelType) (*const_iter_coef)[0]; /// Only works if it is a output=1D image
                ++const_iter_coef;
            }


            out.Set(  pixel ) ;
        }

        if (m_debug) std::cout << t.GetSeconds()<< " seconds" << std::endl;

    }

    if (m_debug) std::cout << "\tThe whole calculateImage method takes " << t0.GetSeconds()<< " seconds" << std::endl;
    return outputImage;

};

/**
 * This function creates only a scalar image.
 */
template <class InputImage, class BsplineGridType>
std::vector<typename InputImage::Pointer> calculateImageAllLevels(const typename InputImage::Pointer referenceImage,
                                                                  const std::vector<typename BsplineGridType::Pointer> &control_points,
                                                                  unsigned int nlevels,  double th)
{
    bool m_debug = true;
    myTimer t0;
    if (m_debug) t0.start();
    myTimer t;
    if (m_debug) std::cout << "\tInitialise image takes ";
    if (m_debug) t.start();



    typename InputImage::Pointer outputImage = echo::initialiseImage<InputImage,InputImage>(referenceImage);
    if (m_debug) std::cout << t.GetSeconds()<< " seconds" << std::endl;
    /// Generate an array with all the positions
    if (m_debug) std::cout << "\tA few operations take ";
    if (m_debug) t.start();
    std::vector<typename BsplineGridType::PointType > pointList;

    typedef itk::ImageRegionConstIterator< InputImage > ConstIteratorType;
    typedef itk::ImageRegionIterator< InputImage > IteratorType;
    ConstIteratorType in1( referenceImage, referenceImage->GetRequestedRegion());
    typename InputImage::IndexType currentIndex;
    typename InputImage::PointType currentInputPos;
    typename BsplineGridType::PointType point;

    typename InputImage::SizeType itkSize =  outputImage->GetLargestPossibleRegion().GetSize();
    typename BsplineGridType::IndexType tmp_index((long unsigned int *)&itkSize[0]);
    if (m_debug) std::cout << t.GetSeconds()<< " seconds" << std::endl;
    if (m_debug) std::cout << "\tAllocate for the point list take ";
    t.start();
    try{
        pointList.reserve(tmp_index.prod());/// allocate in excess
    }
    catch (std::exception& ba)
    {
        std::cerr << "BsplineGrid::calculateImageAllLevels-- exception caught while reserving "<< tmp_index.prod() <<" values into point_block: " << ba.what() << '\n';
        exit(-1);
    }
    if (m_debug) std::cout << t.GetSeconds()<< " seconds" << std::endl;
    /// Find the positions of the pixels that are not zero

    if (m_debug) std::cout << "\tLoop to find nonzero positions takes ";
    if (m_debug) t.start();
    for ( in1.GoToBegin(); !in1.IsAtEnd(); ++in1)
    {
        if (in1.Get()>th)
        {
            referenceImage->TransformIndexToPhysicalPoint(in1.GetIndex(),currentInputPos);
            for (int i=0; i<InputImage::ImageDimension; i++)
                point[i]=currentInputPos[i];

            pointList.push_back(point);
        }

    }
    if (m_debug) std::cout << t.GetSeconds()<< " seconds" << std::endl;


    /// Evaluate and add data at each scale

    typename InputImage::PixelType pixel;

    typename std::vector<typename BsplineGridType::CoefficientType>::const_iterator const_iter_coef;

    if (m_debug) std::cout << "\tCalculate the values "<<std::endl;
    std::vector<typename InputImage::Pointer> output_images(nlevels);

    for (int i=0; i<nlevels; i++)
    {
        if (m_debug)
        {
            std::cout << "\t\tLevel "<< i <<std::endl;
            std::cout << "\t\t\tevaluate"<<std::endl;
        }
        std::vector<typename BsplineGridType::CoefficientType> data_list =control_points[i]->evaluate(pointList);
        /// Copy into a DenseVector to print

        /// add the contents of data list to each pixel in the image
        if (m_debug) std::cout << "\t\t\tAdd the values to the actual image takes ";
        if (m_debug) t.start();
        IteratorType out( outputImage, outputImage->GetRequestedRegion() );
        ConstIteratorType in2( referenceImage, referenceImage->GetRequestedRegion());
        for ( in2.GoToBegin(), out.GoToBegin(), const_iter_coef = data_list.begin(); !out.IsAtEnd(); ++out, ++in2)
        {
            pixel = out.Get();
            if (in2.Get()>th)
            {
                pixel+=(typename InputImage::PixelType) (*const_iter_coef)[0]; /// Only works if it is a output=1D image
                ++const_iter_coef;
            }


            out.Set(  pixel ) ;
        }

        if (m_debug) std::cout << t.GetSeconds()<< " seconds" << std::endl;

        typedef itk::CastImageFilter< InputImage, InputImage> DummyFilerType;
        typename DummyFilerType::Pointer dummyFilter = DummyFilerType::New();
        dummyFilter->SetInput(outputImage);
        dummyFilter->Update();

        typename InputImage::Pointer output  = dummyFilter->GetOutput();

        output_images[i]=output;

    }

    if (m_debug) std::cout << "\tThe whole calculateImage method takes " << t0.GetSeconds()<< " seconds" << std::endl;
    return output_images;

};

/**
 * This function creates only a vector image.
 */
template <class InputImage, class RefImage, class BsplineGridType>
typename InputImage::Pointer calculateVectorImage(const typename RefImage::Pointer referenceImage,
                                                  const std::vector<typename BsplineGridType::Pointer> &control_points,
                                                  unsigned int nlevels,  unsigned int ncom, double th)
{
    bool m_debug = true;
    myTimer t0;
    if (m_debug) t0.start();
    myTimer t;
    if (m_debug) std::cout << "\tInitialise image takes ";
    if (m_debug) t.start();
    typename InputImage::Pointer outputImage = echo::initialiseImage<InputImage,RefImage>(referenceImage);
    if (m_debug) std::cout << t.GetSeconds()<< " seconds" << std::endl;
    /// Generate an array with all the positions
    if (m_debug) std::cout << "\tA few operations take ";
    if (m_debug) t.start();
    std::vector<typename BsplineGridType::PointType > pointList;

    typedef itk::ImageRegionConstIterator< RefImage > ConstIteratorType;
    typedef itk::ImageRegionIterator< InputImage > IteratorType;
    ConstIteratorType in1( referenceImage, referenceImage->GetRequestedRegion());
    typename InputImage::IndexType currentIndex;
    typename InputImage::PointType currentInputPos;
    typename BsplineGridType::PointType point;

    typename InputImage::SizeType itkSize =  outputImage->GetLargestPossibleRegion().GetSize();
    typename BsplineGridType::IndexType tmp_index((long unsigned int *)&itkSize[0]);
    if (m_debug) std::cout << t.GetSeconds()<< " seconds" << std::endl;
    if (m_debug) std::cout << "\tAllocate for the point list take ";
    t.start();
    try{
        pointList.reserve(tmp_index.prod());/// allocate in excess
    }
    catch (std::exception& ba)
    {
        std::cerr << "BsplineGrid::calculateVectorImage-- exception caught while reserving "<< tmp_index.prod() <<" values into point_block: " << ba.what() << '\n';
        exit(-1);
    }
    if (m_debug) std::cout << t.GetSeconds()<< " seconds" << std::endl;
    /// Find the positions of the pixels that are not zero

    if (m_debug) std::cout << "\tLoop to find nonzero positions takes ";
    if (m_debug) t.start();
    for ( in1.GoToBegin(); !in1.IsAtEnd(); ++in1)
    {
        if (in1.Get()>th)
        {
            referenceImage->TransformIndexToPhysicalPoint(in1.GetIndex(),currentInputPos);
            for (int i=0; i<InputImage::ImageDimension; i++)
                point[i]=currentInputPos[i];

            pointList.push_back(point);
        }

    }
    if (m_debug) std::cout << t.GetSeconds()<< " seconds" << std::endl;


    /// Evaluate and add data at each scale

    typename InputImage::PixelType pixel, previous_pixel;

    typename std::vector<typename BsplineGridType::CoefficientType>::const_iterator const_iter_coef;

    if (m_debug) std::cout << "\tCalculate the values "<<std::endl;
    for (int i=0; i<nlevels; i++)
    {
        if (m_debug)
        {
            std::cout << "\t\tLevel "<< i <<std::endl;
            std::cout << "\t\t\tevaluate"<<std::endl;
        }
        std::vector<typename BsplineGridType::CoefficientType> data_list = control_points[i]->evaluate(pointList);
        /// Copy into a DenseVector to print

        /// add the contents of data list to each pixel in the image
        if (m_debug) std::cout << "\t\t\tAdd the values to the actual image takes ";
        if (m_debug) t.start();
        IteratorType out( outputImage, outputImage->GetRequestedRegion() );
        ConstIteratorType in2( referenceImage, referenceImage->GetRequestedRegion());
        for ( in2.GoToBegin(), out.GoToBegin(), const_iter_coef = data_list.begin(); !out.IsAtEnd(); ++out, ++in2)
        {
            previous_pixel = out.Get();
            pixel = out.Get();
            if (in2.Get()>th)
            {
                /// This is a vector image
                for (int k=0; k<ncom; k++)
                {
                    pixel.SetNthComponent(k,(*const_iter_coef)[k]+previous_pixel[k]);
                }
                ++const_iter_coef;
            }


            out.Set(  pixel ) ;
        }

        if (m_debug) std::cout << t.GetSeconds()<< " seconds" << std::endl;



    }

    if (m_debug) std::cout << "\tThe whole calculateImage method takes " << t0.GetSeconds()<< " seconds" << std::endl;
    return outputImage;

}

/**
 * Output mesh and reference mesh should be of class vtkSmartPointer<vtkPolyData>
 */
template <class BsplineGridType, class MeshType>
std::vector< vtkSmartPointer<MeshType> > calculateMeshAllLevels(const vtkSmartPointer<MeshType> referenceMesh,
                                                                const std::vector<typename BsplineGridType::Pointer> &control_points,
                                                                unsigned int nlevels,  unsigned int ncom, double th, double t_position,
                                                                double coordinates_factor=1.0 )
{
    bool m_debug = true;
    myTimer t0;
    if (m_debug) t0.start();
    myTimer t;
    /// Initialise output mesh. Copy the input mesh but none of its arrays
    //vtkSmartPointer<vtkPolyData> output = vtkSmartPointer<vtkPolyData>::New();
    //output->GetPointData()->DeepCopy(referenceMesh->GetPointData());

    vtkSmartPointer<vtkPassArrays> passFilter = vtkSmartPointer<vtkPassArrays>::New();
    passFilter->SetInputData(referenceMesh);
    passFilter->Update(); /// Since no array is specified, none will be passed
    vtkSmartPointer<MeshType> ref = static_cast<MeshType*>(passFilter->GetOutput());


    /// Generate an array with all the positions of the output mesh
    std::vector<typename BsplineGridType::PointType > pointList;
    echo::polyDataToPointList<typename BsplineGridType::PointType>(ref, pointList, t_position,coordinates_factor );

    /// Evaluate and add data at each scale

    std::vector<vtkSmartPointer<MeshType> > output_meshes(nlevels);
    int npoints = ref->GetNumberOfPoints();

    std::vector<typename BsplineGridType::CoefficientType> data_cummulated(npoints); // This is by default init to all zeros



    if (m_debug) std::cout << "\tCalculate the values "<<std::endl;
    for (int i=0; i<nlevels; i++)
    {
        if (m_debug)
        {
            std::cout << "\t\tLevel "<< i <<std::endl;
            std::cout << "\t\t\tevaluate"<<std::endl;
        }
        std::vector<typename BsplineGridType::CoefficientType> data_list = control_points[i]->evaluate(pointList);
        /// add the contents of data list to each pixel in the image
        vtkSmartPointer<vtkDataArray> data = vtkSmartPointer<vtkFloatArray>::New();
        data->SetNumberOfComponents(control_points[i]->GetOutputDimensions());
        data->SetNumberOfTuples(npoints);

        data->SetName("Data");
        for (int p=0; p<npoints; p++){
            //std::cout << "\tDo point  "<< p <<std::endl;
            data_cummulated[p]+=data_list[p];
            data->SetTuple(p, &(data_cummulated[p][0])) ;
        }
        output_meshes[i]=vtkSmartPointer<MeshType>::New();
        output_meshes[i]->DeepCopy(ref);
        output_meshes[i]->GetPointData()->AddArray(data);

        // std::cout << *output_meshes[i] <<std::endl;
    }



    if (m_debug) std::cout << "\tThe whole calculateImage method takes " << t0.GetSeconds()<< " seconds" << std::endl;
    return output_meshes;

}



/**
 *
 */
template <class InputImage, class RefImage, class BsplineGridType>
std::vector<typename InputImage::Pointer> calculateVectorImageAllLevels(const typename RefImage::Pointer referenceImage,
                                                                        const std::vector<typename BsplineGridType::Pointer> &control_points,
                                                                        unsigned int nlevels,  unsigned int ncom, double th)
{
    bool m_debug = true; /// This is not a member function!
    myTimer t0;
    if (m_debug) t0.start();
    myTimer t;
    if (m_debug) std::cout << "\tInitialise image takes ";
    if (m_debug) t.start();
    typename InputImage::Pointer outputImage = echo::initialiseImage<InputImage,RefImage>(referenceImage);
    if (m_debug) std::cout << t.GetSeconds()<< " seconds" << std::endl;
    /// Generate an array with all the positions
    if (m_debug) std::cout << "\tA few operations take ";
    if (m_debug) t.start();
    std::vector<typename BsplineGridType::PointType > pointList;

    typedef itk::ImageRegionConstIterator< RefImage > ConstIteratorType;
    typedef itk::ImageRegionIterator< InputImage > IteratorType;
    ConstIteratorType in1( referenceImage, referenceImage->GetRequestedRegion());
    typename InputImage::PointType currentInputPos;
    typename BsplineGridType::PointType point;

    typename InputImage::SizeType itkSize =  outputImage->GetLargestPossibleRegion().GetSize();
    typename BsplineGridType::IndexType tmp_index((long unsigned int *)&itkSize[0]);
    if (m_debug) std::cout << t.GetSeconds()<< " seconds" << std::endl;
    if (m_debug) std::cout << "\tAllocate for the point list take ";
    t.start();
    try{
        pointList.reserve(tmp_index.prod());/// allocate in excess
    }
    catch (std::exception& ba)
    {
        std::cerr << "BsplineGrid::calculateVectorImageAllLevels-- exception caught while reserving "<< tmp_index.prod() <<" values into point_block: " << ba.what() << '\n';
        exit(-1);
    }
    if (m_debug) std::cout << t.GetSeconds()<< " seconds" << std::endl;
    /// Find the positions of the pixels that are not zero

    if (m_debug) std::cout << "\tLoop to find nonzero positions takes ";
    if (m_debug) t.start();
    for ( in1.GoToBegin(); !in1.IsAtEnd(); ++in1)
    {
        if (in1.Get()>th)
        {
            referenceImage->TransformIndexToPhysicalPoint(in1.GetIndex(),currentInputPos);
            for (int i=0; i<InputImage::ImageDimension; i++){
                point[i]=currentInputPos[i];
            }
            //if (m_debug) std::cout << "echo::calculateVectorImageAllLevels -- current point "<<point<<std::endl;
                pointList.push_back(point);
        }

    }
    if (m_debug) std::cout << t.GetSeconds()<< " seconds" << std::endl;


    /// Evaluate and add data at each scale

    typename InputImage::PixelType pixel, previous_pixel;

    typename std::vector<typename BsplineGridType::CoefficientType>::const_iterator const_iter_coef;
    std::vector<typename InputImage::Pointer> output_images(nlevels);
    if (m_debug) std::cout << "\tCalculate the values "<<std::endl;
    for (int i=0; i<nlevels; i++)
    {
        if (m_debug)
        {
            std::cout << "\t\tLevel "<< i <<std::endl;
            std::cout << "\t\t\tevaluate"<<std::endl;
        }
        std::vector<typename BsplineGridType::CoefficientType> data_list = control_points[i]->evaluate(pointList);
        /// Copy into a DenseVector to print

        /// add the contents of data list to each pixel in the image
        if (m_debug) std::cout << "\t\t\tAdd the values to the actual image takes ";
        if (m_debug) t.start();
        IteratorType out( outputImage, outputImage->GetRequestedRegion() );
        ConstIteratorType in2( referenceImage, referenceImage->GetRequestedRegion());
        for ( in2.GoToBegin(), out.GoToBegin(), const_iter_coef = data_list.begin(); !out.IsAtEnd(); ++out, ++in2)
        {
            previous_pixel = out.Get();
            pixel = out.Get();
            if (in2.Get()>th)
            {
                /// This is a vector image
                for (int k=0; k<ncom; k++)
                {
                    pixel.SetNthComponent(k,(*const_iter_coef)[k]+previous_pixel[k]);
                }
                ++const_iter_coef;
            }


            out.Set(  pixel ) ;
        }

        if (m_debug) std::cout << t.GetSeconds()<< " seconds" << std::endl;

        typedef itk::CastImageFilter< InputImage, InputImage> DummyFilerType;
        typename DummyFilerType::Pointer dummyFilter = DummyFilerType::New();
        dummyFilter->SetInput(outputImage);
        dummyFilter->Update();

        typename InputImage::Pointer output  = dummyFilter->GetOutput();

        output_images[i]=output;
    }

    if (m_debug) std::cout << "\tThe whole calculateImage method takes " << t0.GetSeconds()<< " seconds" << std::endl;
    return output_images;

}


}


#endif /* BSPLINEGRID_H_ */

