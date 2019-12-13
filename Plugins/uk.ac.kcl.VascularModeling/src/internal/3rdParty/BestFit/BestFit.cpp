/*****************************************************************************/
/*                                                                           */
/* Best-fit                                                                  */
/*                                                                           */
/* Copyright 2014                                                            */
/* Alasdair Craig                                                            */
/* ac@acraig.za.net                                                          */
/* License: Code Project Open License 1.02                                   */
/* http://www.codeproject.com/info/cpol10.aspx                               */
/*                                                                           */
/*****************************************************************************/

#ifdef _MSC_VER
#pragma warning( disable : 4244 ) // suppress possible loss-of-data warning arising from boost
#endif


#include "BestFit.h"
#include "cholesky.h"

#include <limits>
#include <cassert>
#include <stdexcept>
#include <iomanip>
#include <boost/numeric/ublas/lu.hpp>
#include <boost/numeric/ublas/io.hpp>


#define CONVERGENCE_CRITERIA 0.000000001
#define MAX_ITERATIONS 50

BestFit::BestFit(int unknowns)
	: m_verbosity(1)
	, m_oStream(m_nullStream)
	, m_solution(unknowns,1)
	, m_provisionals(unknowns,1)
	, m_numObs(0)
	, m_numUnknowns(unknowns)
	, m_minx( std::numeric_limits<double>::max())
	, m_maxx(-std::numeric_limits<double>::max())
	, m_miny( std::numeric_limits<double>::max())
	, m_maxy(-std::numeric_limits<double>::max())
{
}

BestFit::BestFit(int unknowns, std::ostream &oStream)
	: m_verbosity(1)
	, m_oStream(oStream)
	, m_solution(unknowns,1)
	, m_provisionals(unknowns,1)
	, m_numObs(0)
	, m_numUnknowns(unknowns)
	, m_minx( std::numeric_limits<double>::max())
	, m_maxx(-std::numeric_limits<double>::max())
	, m_miny( std::numeric_limits<double>::max())
	, m_maxy(-std::numeric_limits<double>::max())
{
}

BestFit::~BestFit()
{
}

// Set the level of detail required for output to stdout
void BestFit::SetVerbosity(int verbosity)
{
	m_verbosity = verbosity;
}

bool BestFit::Compute(BestFitIO &in, BestFitIO &out)
{
	SetVerbosity(in.verbosity);

	if (!in.points || in.numPoints == 0)
		{
		m_oStream << "No solution. No input points." << std::endl;
		return false;
		}
	if (in.numPoints < m_numUnknowns + 1)
		{
		m_oStream << "No solution. Too few input points, need " << m_numUnknowns + 1 - in.numPoints << " or more." << std::endl;
		return false;
		}

	m_numObs = in.numPoints;

	ResizeMatrices();

	for (int i = 0; i < m_numObs; ++i)
	{
		double x = in.points[i * 2 + 0];
		double y = in.points[i * 2 + 1];
		AddObservation(i, x, y);
	}

	Compute();

	FillOutput(out);

	return true;
}

void BestFit::ResizeMatrices()
{
	m_residuals.resize(m_numObs, 1);
	m_design.resize(m_numObs, m_numUnknowns);
	m_l.resize(m_numObs, 1);
	m_qweight.resize(m_numObs, m_numObs, 0, 0);
	m_observations.resize(m_numObs, 2);
	m_b.resize(m_numObs, m_numObs * 2);
}

// Add another observable (coordinate) to the computation object
void BestFit::AddObservation(int count, double x, double y)
{
	m_observations(count, 0) = x;
	m_observations(count, 1) = y;

	m_minx = std::min<double>(m_minx, x);
	m_maxx = std::max<double>(m_maxx, x);
	m_miny = std::min<double>(m_miny, y);
	m_maxy = std::max<double>(m_maxy, y);
}

// Do the least-squares adjustment
bool BestFit::Compute()
{
	if (m_verbosity > 1)
		m_oStream << "Observations:    " << m_observations << std::endl;

	GenerateProvisionals();

	bool successful = true;

	int iteration = 0;

	while (true)
		{
		FormulateMatrices();

		if (m_verbosity > 1)
			{
			m_oStream << "Provisionals:    " << m_provisionals << std::endl;
			m_oStream << "l-matrix:        " << m_l << std::endl;
			}

		// evaluate the unknowns - small corrections to be applied to the provisional unknowns
		if (EvaluateUnknowns())
			{
			++iteration;

			// add the solution to the provisional unknowns
			EvaluateAdjustedUnknowns();

			if (HasConverged())
				break;
			if (IsDegenerate(iteration))
				break;
			}
		else
			{
			successful = false;
			break;
			}
		}

	successful = successful && (iteration > 0 && iteration < MAX_ITERATIONS);
	if (successful)
		{
		// Give ellipse chance to normalise it's axes. 
		NormaliseAdjustedUnknowns();

		// evaluate the residuals
		EvaluateResiduals();

		// add the residuals to the provisional observations
		EvaluateAdjustedObservations();

		// Check that the adjusted unknowns and adjusted observations satisfy
		// the original line/circle/ellipse equation.
		GlobalCheck();

		// Subsequent error analysis and statistical output
		ErrorAnalysis(iteration);

		if (0 == m_verbosity)
			OutputSimpleSolution();
		}

	return successful;
}

// Matrix inversion routine using LU decomposition
bool BestFit::InvertMatrix(const ublas::matrix<double> &input, ublas::matrix<double> &inverse)
{
	// create a working copy of the input
	ublas::matrix<double> A(input);

	// create a permutation matrix for the LU-factorization
	ublas::permutation_matrix<double> pm(A.size1());

	// perform LU-factorization
	auto res = ublas::lu_factorize(A, pm);
	if (res != 0)
		return false;

	// create identity matrix of "inverse"
	inverse.assign(ublas::identity_matrix<double>(A.size1()));

	try
		{
		// back-substitute to get the inverse
		ublas::lu_substitute(A, pm, inverse);
		}
	catch (std::logic_error &)
		{
		return false;
		}

	return true;
}

// Determine solution vector
bool BestFit::EvaluateUnknowns()
{
	ublas::matrix<double> pa = ublas::prod(m_qweight, m_design);
	ublas::matrix<double> atpa = ublas::prod(ublas::trans(m_design), pa);

	ublas::matrix<double> inverse(atpa.size1(), atpa.size2());
	//if (CholeskyInversion(atpa, inverse))
	if (InvertMatrix(atpa, inverse))
		{
		ublas::matrix<double> pl = ublas::prod(m_qweight, m_l);
		ublas::matrix<double> atpl = ublas::prod(ublas::trans(m_design), pl);

		m_solution = ublas::prod(inverse, atpl);

		if (m_verbosity > 2)
			m_oStream << "Iter. solution   " << m_solution << std::endl;
		return true;
		}
	else
		m_oStream << "No solution. Cannot invert matrix." << std::endl;
	return false;
}


// Add solution to the provisionals
void BestFit::EvaluateAdjustedUnknowns()
{
	m_provisionals += m_solution;

	if (m_verbosity > 2)
		m_oStream << "Iter adj unknowns" << m_provisionals << std::endl;
}

// Residuals to the initial observations
void BestFit::EvaluateResiduals()
{
	// v = Ax-l, quasi-residuals in the case of circle and ellipse
	m_residuals = ublas::prod(m_design, m_solution) - m_l;

	if (m_verbosity > 1)
		m_oStream << "Residuals:       " << m_residuals << std::endl;
}

void BestFit::EvaluateFinalResiduals(int point, double &vxi, double &vyi) const
{
	// In the case of the quasi-parametric case (circle and ellipse) the actual
	// residuals for both the x and y coordinate need to be extracted.
	vxi = m_design(point, 0) * m_qweight(point, point) * m_residuals(point, 0);
	vyi = m_design(point, 1) * m_qweight(point, point) * m_residuals(point, 0);

	//// Method 1: ref (2:35)
	//ublas::matrix<double> bt = ublas::trans(m_b);
	//ublas::matrix<double> pv = ublas::prod(m_qweight, m_residuals);
	//ublas::matrix<double> btpv = ublas::prod(bt, pv);
	//double avxi = -btpv(point * 2 + 0,0);
	//double avyi = -btpv(point * 2 + 1,0);
	//m_oStream << "New residuals (Method 1) differ by: " << avxi - vxi << "," << avyi - vyi << std::endl;

	//// Method 2: ref (2:28)
	//double dx = m_observations(point, 0) - m_provisionals(0, 0);
	//double dy = m_observations(point, 1) - m_provisionals(1, 0);
	//double r = m_provisionals(2, 0);
	//double quasiv = -2.0 * dx * m_solution(0, 0) - 2.0 * dy * m_solution(1, 0) - 2.0 * r * m_solution(2, 0) - m_l(point, 0);
	//double bvxi = - (quasiv * 2.0 * dx) / (4.0 * (dx * dx + dy * dy));
	//double bvyi = - (quasiv * 2.0 * dy) / (4.0 * (dx * dx + dy * dy));
	//m_oStream << "New residuals (Method 2) differ by: " << bvxi - vxi << "," << bvyi - vyi << std::endl;
}

// Add residuals to initial observations
void BestFit::EvaluateAdjustedObservations()
{
	for (int i = 0; i < m_numObs; ++i)
		{
		double vxi = 0.0;
		double vyi = 0.0;
		EvaluateFinalResiduals(i, vxi, vyi); // overridden for normal (non-quasi-parametric) BestFitLine

		m_observations(i, 0) += vxi;
		m_observations(i, 1) += vyi;
		}

	if (m_verbosity > 1)
		m_oStream << "Adj. observations" << m_observations << std::endl;
}

// Global check on the quality of the adjustment
void BestFit::GlobalCheck()
{
	ublas::vector<double> global(m_numObs, 0);
	bool pass = (m_numObs > 0);

	for (int i = 0; i < m_numObs; ++i)
		{
		double x = m_observations(i, 0);
		double y = m_observations(i, 1);

		global(i) = SolveAt(x, y); // should be zero
		pass = pass && fabs(global(i)) < 0.01; // TODO: Is this too lax?
		}

	if (m_verbosity > 1)
		m_oStream << "Global check:    " << global << std::endl;
	if (m_verbosity > 0)
		m_oStream << "Global check of the adjustment     ***" << (pass ? "PASSES***" : "FAILS***") << std::endl;

	// Secondary test is to check that aTPv is zero too, really only neccessary
	// if the above global check fails.
	ublas::matrix<double> atp = ublas::prod(ublas::trans(m_design), m_qweight);
	ublas::matrix<double> atpv = ublas::prod(atp, m_residuals);

	pass = true;
	for (int j = 0; j < m_numUnknowns; ++j)
		pass = pass && Double::IsZero(atpv(j, 0));

	if (!pass && m_verbosity > 1)
		m_oStream << "aTPv             " << atpv << std::endl;
	if (m_verbosity > 0)
		m_oStream << "Check of the evaluated unknowns    ***" << (pass ? "PASSES***" : "FAILS***") << std::endl;
}

// Predicate for seeing whether the solution has become sufficiently small
bool BestFit::HasConverged() const
{
	for (int i = 0; i < m_numUnknowns; ++i)
		{
		if (fabs(m_solution(i, 0)) > CONVERGENCE_CRITERIA)
			return false;
		}
	return true;
}

// Predicate for seeing whether the solution is diverging?
bool BestFit::IsDegenerate(int iteration) const
{
	bool degenerate = (iteration >= MAX_ITERATIONS);
	if (degenerate)
		m_oStream << "No solution. Does not converge." << std::endl;

	return degenerate;
}

// Output stats
// TODO: Variance-covariance matrices
void BestFit::ErrorAnalysis(int iterations)
{
	ublas::matrix<double> pv = ublas::prod(m_qweight, m_residuals);
	ublas::matrix<double> vtpv = ublas::prod(ublas::trans(m_residuals), pv);
	int degreesFreedom = m_numObs - m_numUnknowns;
	double variance = vtpv(0, 0) / degreesFreedom;
	double stddev = sqrt(variance);

	if (m_verbosity > 0)
		{
		m_oStream << std::setprecision(6) << std::setiosflags(std::ios::fixed)
			<< "Number of observations             " << m_numObs << std::endl
			<< "Number of unknowns                 " << m_numUnknowns << std::endl
			<< "Degrees of freedom                 " << degreesFreedom << std::endl
			<< "Iterations until convergence       " << iterations << std::endl
			<< "Variance                           " << variance << std::endl
			<< "Std. dev. observation unit weight  " << stddev << std::endl;

		m_oStream << "***********************************" << std::endl;

		OutputAdjustedUnknowns(m_oStream);
		}
}

// Output at it's very simplest
void BestFit::OutputSimpleSolution() const
{
	for (int i = 0; i < m_numUnknowns; ++i)
		m_oStream << m_provisionals(i, 0) << std::endl;
}

// Return selected values back to caller
void BestFit::FillOutput(BestFitIO &out) const
{
	out.numPoints = m_numObs;
	out.numOutputFields = m_numUnknowns;
	out.verbosity = m_verbosity;

	//memset(out.outputFields, 0, sizeof(outputFields));
	for (int i = 0; i < m_numUnknowns; ++i)
		out.outputFields[i] = m_provisionals(i, 0);

	if (out.wantResiduals)
		out.residuals = new double[m_numObs];

	for (int j = 0; j < m_numObs; ++j)
	{
		if (out.wantAdjustedObs)
		{
			out.points[j * 2 + 0] = m_observations(j, 0);
			out.points[j * 2 + 1] = m_observations(j, 1);
		}
		if (out.wantResiduals)
			out.residuals[j] = m_residuals(j, 0);
	}
}

void BestFit::LowerTriangularModifyInversion(const ublas::matrix<double> &l, ublas::matrix<double> &m)
{
	typedef ublas::matrix<double>::size_type INDEX;
	INDEX i, j, n = l.size1();

	for (i = 0; i < n; ++i)
		m(i, i) = 1.0 / l(i, i);

	for (j = 0; j < n; ++j)
	{
		for (i = j + 1; i < n; ++i)
		{
			assert(i > j);

			double sum = 0.0;
			for (INDEX k = j; k <= i - 1; ++k)
				sum += l(i, k) * m(k, j);

			m(i, j) = -m(i, i) * sum;
		}
	}
}

bool BestFit::CholeskyInversion(const ublas::matrix<double> &input, ublas::matrix<double> &inverse)
{
	ublas::matrix<double> l(input.size1(), input.size2());
	ublas::matrix<double> m(input.size1(), input.size2(), 0.0);

	if (0 == cholesky_decompose(input, l))
	{
		LowerTriangularModifyInversion(l, m);

		inverse = ublas::prod(ublas::trans(m), m);

		return true;
	}

	return false;
}

