#ifndef MATRIXTOOLS_H_
#define MATRIXTOOLS_H_

#include <iostream>

const unsigned int OUT_OF_PATCH = std::numeric_limits<unsigned int>::max(); /// Highly unlikely that we will have this node index

namespace echo
{



/// Misc functions

namespace Eigen /// Matrix functions adapted to the interface of the Eigen library
{

template<class ArrayClass>
ArrayClass powerArray(const ArrayClass &input, double exponent);

template <class ArrayType>       void addLessThan(const ArrayType &array, double th, std::vector<unsigned int > &indices);
template <class ArrayType>       void addLessOrEqualThan(const ArrayType &array, double th,  std::vector<unsigned int > &indices);
template <class ArrayType>       void addGtThan(const ArrayType &array, double th, std::vector<unsigned int > &indices, std::vector<int> &cc);
template <class ArrayType>       void addGtOrEqualThan(const ArrayType &array, double th,  std::vector<unsigned int > &indices);

template <class MatrixType>
void keepColumns(MatrixType &B, const std::vector<unsigned int> &kept_columns);
template <class MatrixType>
void keepColumnsAndRows(MatrixType &B, const std::vector<unsigned int> &kept_columns);

template <class MatrixType>
int writeMatrixToFile(const MatrixType &matrix, std::string &filename);

template <class TripletType>
int writeTripletListToFile(const std::vector<TripletType> &tl, std::string &filename);
/****************************************************************
*                          IMPLEMENTATION                       *
****************************************************************/


template <class TripletType>
int writeTripletListToFile(const std::vector<TripletType> &tl, std::string &filename){

    std::ofstream myfileW(filename.c_str());
    if (myfileW.is_open()) {
        for (int i = 0; i < tl.size(); i++) {
            myfileW << tl[i].row() << "\t"<< tl[i].col() << "\t"<<tl[i].value() <<std::endl;
        }
        myfileW.close();
    } else {
        std::cerr << "Could not open file " << filename << " for writing."<< std::endl;
        return -1;
    }
    return 0;
}


/**
 * This will write the matrix always as full to be able to open it easily with other softwares
 */
template <class MatrixType>
int writeMatrixToFile(const MatrixType &matrix, std::string &filename){

    std::ofstream myfileW(filename.c_str());
    if (myfileW.is_open()) {
        for (int i = 0; i < matrix.rows(); i++) {
            for (int j = 0; j < matrix.cols(); j++) {
                myfileW << matrix.coeff(i,j);
                if (j<matrix.cols()-1){
                    myfileW<< " ";
                }
            }
            myfileW << std::endl;
        }
        myfileW.close();
    } else {
        std::cerr << "Could not open file " << filename << " for writing."<< std::endl;
        return -1;
    }
    return 0;

}

/**
 * This will write the matrix always as full to be able to open it easily with other softwares
 */
template <class VectorType>
int writeVectorToFile(const VectorType &vector, std::string &filename){

    std::ofstream myfileW(filename.c_str());
    if (myfileW.is_open()) {
        for (int i = 0; i < vector.size(); i++) {
            myfileW << vector[i]<<std::endl;
        }
        myfileW.close();
    } else {
        std::cerr << "Could not open file " << filename << " for writing."<< std::endl;
        return -1;
    }
    return 0;

}

/**
 * Removes from the matrix all the columns that are not in kept nodes
 */
template <class MatrixType>
void keepColumns(MatrixType &B, const std::vector<unsigned int> &kept_columns){
    /// The best is probably to permute then extract the first n columns. Build the permutation matrix

    /// If using Column major order
    MatrixType S_(B.rows(),kept_columns.size());
    for (int j=0; j<kept_columns.size(); j++){
        S_.col(j)=B.col(kept_columns[j]);
    }

    B=S_;

}

/**
 * Removes from the matrix all the columns that are not in kept nodes
 */
template <class MatrixType>
void keepColumnsAndRows(MatrixType &B, const std::vector<unsigned int> &kept_columns){

    keepColumns(B, kept_columns);
    B=B.transpose();
    keepColumns(B, kept_columns);
    B=B.transpose();

}

/**
 * Calculates the power of an array or matrix (as long as they implement the function rows and cols)
 */
template<class ArrayClass>
ArrayClass powerArray(const ArrayClass &input, double exponent)
{
    ArrayClass output(input);
    int nelements = input.rows()*input.cols();
    for (int i=0; i<nelements; i++)
    {
        output[i]=std::pow(input[i],exponent);
    }
    return output;
}

/**
 * Similar behaviour to the function find in matlab
 * it does indices = find(array<th)
 */

template <class ArrayType>    void addLessThan(const ArrayType &array, double th, std::vector<unsigned int > &indices)
{
    int nelements = array.rows()*array.cols();
    for (int i=0; i<nelements; i++)
    {
        if (array[i]<th)
        {
            indices.push_back(i);
        }
    }
}

/**
* Similar behaviour to the function find in matlab
* it does indices = find(array<th)
*/

template <class ArrayType>    void addLessOrEqualThan(const ArrayType &array, double th, std::vector<unsigned int > &indices)
{
    int nelements = array.rows()*array.cols();
    for (int i=0; i<nelements; i++)
    {
        if (array[i]<=th)
        {
            indices.push_back(i);
        }
    }
}
/**
 * Similar behaviour to the function find in matlab
 * it does indices = find(array<th)
 * cc contains an array which translates from columns of the original matrix to columns of the reduced matrix
 */

template <class ArrayType>  void addGtThan(const ArrayType &array, double th, std::vector<unsigned int > &indices, std::vector<unsigned int > &cc)
{

    int nelements = array.rows()*array.cols();
    indices.reserve(nelements);
    cc.reserve(nelements);
    for (int i=0; i<nelements; i++)
    {
        if (array[i]>th)
        {
            cc.push_back(indices.size());
            indices.push_back(i);
        } else {
            cc.push_back(OUT_OF_PATCH);
        }
    }
}

/**
* Similar behaviour to the function find in matlab
* it does indices = find(array<th)
*/

template <class ArrayType>    void addGtOrEqualThan(const ArrayType &array, double th, std::vector<unsigned int > &indices)
{
    int nelements = array.rows()*array.cols();
    for (int i=0; i<nelements; i++)
    {
        if (array[i]>=th)
        {
            indices.push_back(i);
        }
    }
}
}





}

#endif /// MATRIXTOOLS_H_
