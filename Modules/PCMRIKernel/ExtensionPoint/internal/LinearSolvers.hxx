#ifndef LINEARSOLVERS_H_
#define LINEARSOLVERS_H_

#include <Eigen/Sparse>
//#include <SuiteSparse_config.h>
#include <Eigen/IterativeLinearSolvers>
#include <Eigen/OrderingMethods>
//#include <Eigen/SPQRSupport>
#include <unsupported/Eigen/IterativeSolvers>
//#include <Eigen/PaStiXSupport>

using namespace Eigen;

namespace echo
{

namespace solvers
{


class ConfigurationParameter{

public:
    ConfigurationParameter(){
        max_iterations=100;
        tol=1E-06;
        verbose=true;
    }
    ~ConfigurationParameter(){}
    unsigned int max_iterations;
    double tol;
    bool verbose;
};

/**
* Solve a linear system using conjugate gradient
* The 'config' argument is an object of the class ConfigurationParameter
* which has the following members:
*   unsigned int max_iterations [100]
*   double tol [1E-06]
*   bool [true]
*/
template <class SparseMatrixType, class DenseVectorType>
void solveWithConjugateGradient(const SparseMatrixType &A, const DenseVectorType &b, DenseVectorType &x, ConfigurationParameter &config);

/**
* Solve a linear system using sparseQR
* The 'config' argument is an object of the class ConfigurationParameter
* which has the following members:
*   unsigned int max_iterations [100]
*   double tol [1E-06]
*   bool [true]
*/
//template <class SparseMatrixType, class DenseVectorType>
//void solveWithSparseQR(const SparseMatrixType &A, const DenseVectorType &b, DenseVectorType &x, ConfigurationParameter &config);

/**
* Solve a linear system using SPQR from SuiteSparse. Requires SuiteSparse to run
* The 'config' argument is an object of the class ConfigurationParameter
* which has the following members:
*   unsigned int max_iterations [100]
*   double tol [1E-06]
*   bool [true]
*/
//template <class SparseMatrixType, class DenseVectorType>
//void solveWithSPQR(const SparseMatrixType &A, const DenseVectorType &b, DenseVectorType &x, ConfigurationParameter &config);

/**
* Solve a linear system using biconjugate gradient stabilized
* The 'config' argument is an object of the class ConfigurationParameter
* which has the following members:
*   unsigned int max_iterations [100]
*   double tol [1E-06]
*   bool [true]
*/
template <class SparseMatrixType, class DenseVectorType>
void solveWithBiCGSTAB(const SparseMatrixType &A, const DenseVectorType &b, DenseVectorType &x, ConfigurationParameter &config);

/**
 * This implementation of GMRES is provided *as is* by Eigen, with no support planned.
 * Documentation here http://eigen.tuxfamily.org/dox/unsupported/classEigen_1_1GMRES.html
 */
template <class SparseMatrixType, class DenseVectorType>
void solveWithGMRES(const SparseMatrixType &A, const DenseVectorType &b, DenseVectorType &x, ConfigurationParameter &config);

}




}

/// --------------------------------------------------------------------///
/// 				         IMPLEMENTATION          				    ///
/// --------------------------------------------------------------------///

template <class SparseMatrixType, class DenseVectorType>
void echo::solvers::solveWithConjugateGradient(const SparseMatrixType &A, const DenseVectorType &b, DenseVectorType &x, ConfigurationParameter &config)
{

    ConjugateGradient<SparseMatrixType > solver;
    solver.setMaxIterations(config.max_iterations);
    solver.setTolerance(config.tol);
    solver.compute(A);
    if(solver.info()!=Success)
    {
        /// decomposition failed
        if (config.verbose)          std::cerr << "\t\tERROR: CG decomposition failed with a matrix of size "<< A.rows() <<"x"<<A.cols()<<std::endl;
    }
    x = solver.solve(b);
    if(solver.info()!=Success)
    {
        /// solving failed
        if (config.verbose) std::cerr << "\t\tERROR: CG solver failed with a matrix of size "<< A.rows() <<"x"<<A.cols()<<std::endl;
    }
    if (config.verbose) std::cout << "\t\t#iterations:     " << solver.iterations() << std::endl;
    if (config.verbose) std::cout << "\t\testimated error: " << solver.error()      << std::endl;

    /// For using solve with guess refer to documentation here: http://eigen.tuxfamily.org/dox-devel/classEigen_1_1ConjugateGradient.html
}

/*template <class SparseMatrixType, class DenseVectorType>
void echo::solvers::solveWithSparseQR(const SparseMatrixType &A, const DenseVectorType &b, DenseVectorType &x, ConfigurationParameter &config)
{

    SparseQR<SparseMatrixType, COLAMDOrdering<int> > solver;
    //solver.setMaxIterations(config.max_iterations);
    //solver.setTolerance(config.tol);
    solver.compute(A);
    if(solver.info()!=Success)
    {
        /// decomposition failed
        if (config.verbose)          std::cerr << "\t\tERROR: SparseQR decomposition failed with a matrix of size "<< A.rows() <<"x"<<A.cols()<<std::endl;
    }
    x = solver.solve(b);
    if(solver.info()!=Success)
    {
        /// solving failed
        if (config.verbose) std::cerr << "\t\tERROR: SparseQR solver failed with a matrix of size "<< A.rows() <<"x"<<A.cols()<<std::endl;
    }
    //if (config.verbose) std::cout << "\t\t#iterations:     " << solver.iterations() << std::endl;
    //if (config.verbose) std::cout << "\t\testimated error: " << solver.error()      << std::endl;

    /// For using solve with guess refer to documentation here: http://eigen.tuxfamily.org/dox-devel/classEigen_1_1ConjugateGradient.html
}
*/
/*template <class SparseMatrixType, class DenseVectorType>
void echo::solvers::solveWithSPQR(const SparseMatrixType &A, const DenseVectorType &b, DenseVectorType &x, ConfigurationParameter &config)
{

    SPQR<SparseMatrixType > solver;
    //solver.setMaxIterations(config.max_iterations);
    //solver.setTolerance(config.tol);

    solver.compute(A);

    if(solver.info()!=Success)
    {
        /// decomposition failed
        if (config.verbose)          std::cerr << "\t\tERROR: SPQR decomposition failed with a matrix of size "<< A.rows() <<"x"<<A.cols()<<std::endl;
    }
    x = solver.solve(b);
    if(solver.info()!=Success)
    {
        /// solving failed
        if (config.verbose) std::cerr << "\t\tERROR: SPQR solver failed with a matrix of size "<< A.rows() <<"x"<<A.cols()<<std::endl;
    }
    //if (config.verbose) std::cout << "\t\t#iterations:     " << solver.iterations() << std::endl;
    //if (config.verbose) std::cout << "\t\testimated error: " << solver.error()      << std::endl;

    /// For using solve with guess refer to documentation here: http://eigen.tuxfamily.org/dox-devel/classEigen_1_1ConjugateGradient.html
}
*/
template <class SparseMatrixType, class DenseVectorType>
void echo::solvers::solveWithBiCGSTAB(const SparseMatrixType &A, const DenseVectorType &b, DenseVectorType &x, ConfigurationParameter &config){

    BiCGSTAB<SparseMatrixType, IncompleteLUT<double> > solver;
    solver.setMaxIterations(config.max_iterations);
    solver.setTolerance(config.tol);
    solver.compute(A);
    if(solver.info()!=Success)
    {
        /// decomposition failed
        if (config.verbose)          std::cerr << "\t\tERROR: BiCGSTAB decomposition failed with a matrix of size "<< A.rows() <<"x"<<A.cols()<<std::endl;
    }
    x = solver.solve(b);
    if(solver.info()!=Success)
    {
        /// solving failed
        if (config.verbose) std::cerr << "\t\tERROR: BiCGSTAB solver failed with a matrix of size "<< A.rows() <<"x"<<A.cols() <<std::endl;
    }
    if (config.verbose) std::cout << "\t\t#iterations:     " << solver.iterations() << std::endl;
    if (config.verbose) std::cout << "\t\testimated error: " << solver.error()      << std::endl;

}

template <class SparseMatrixType, class DenseVectorType>
void echo::solvers::solveWithGMRES(const SparseMatrixType &A, const DenseVectorType &b, DenseVectorType &x, ConfigurationParameter &config){

    GMRES<SparseMatrixType, IncompleteLUT<double> > solver;
    solver.setMaxIterations(config.max_iterations);
    solver.setTolerance(config.tol);
    solver.compute(A);
    if(solver.info()!=Success)
    {
        /// decomposition failed
        if (config.verbose)          std::cerr << "\t\tERROR: GMRES decomposition failed with a matrix of size "<< A.rows() <<"x"<<A.cols()<<std::endl;
    }
    x = solver.solve(b);
    if(solver.info()!=Success)
    {
        /// solving failed
        if (config.verbose) std::cerr << "\t\tERROR: GMRES solver failed with a matrix of size "<< A.rows() <<"x"<<A.cols()<<std::endl;
    }
    if (config.verbose) std::cout << "\t\t#iterations:     " << solver.iterations() << std::endl;
    if (config.verbose) std::cout << "\t\testimated error: " << solver.error()      << std::endl;

}
#endif /* LINEARSOLVERS_H_*/
