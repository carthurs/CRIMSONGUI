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

#pragma once
#pragma warning( disable : 4355 )

#include <iostream>
#include <boost/numeric/ublas/banded.hpp>

namespace ublas = boost::numeric::ublas;

/***********************************************************
************************************************************
**********************  BestFitIO **************************
************************************************************
***********************************************************/

struct BestFitIO
{
	int numPoints;
	double *points;
	int verbosity;
	int numOutputFields;
	double outputFields[5];
	bool wantAdjustedObs;
	bool wantResiduals;
	double *residuals;

	enum { LineGradient, LineYIntercept };
	enum { CircleCentreX, CircleCentreY, CircleRadius };
	enum { EllipseCentreX, EllipseCentreY, EllipseMajor, EllipseMinor, EllipseRotation };

	BestFitIO()
	 : numPoints(0)
	 , points(NULL)
	 , verbosity(1)
	 , numOutputFields(0)
	 , wantAdjustedObs(false)
	 , wantResiduals(false)
	 , residuals(NULL)
	 {}
};

/***********************************************************
************************************************************
**********************  NullStream *************************
************************************************************
***********************************************************/

class NullStreamBuf : public std::streambuf
{
	char buffer[64];
protected:
	virtual int overflow(int c) 
	{
		setp(buffer, buffer + sizeof(buffer));
		return (c == traits_type::eof()) ? '\0' : c;
	}
};

class NullStream : private NullStreamBuf, public std::ostream
{
	public:
		NullStream() : std::ostream(this) {}
};


/***********************************************************
************************************************************
**********************  BestFit ****************************
************************************************************
***********************************************************/

class BestFit
{
	// Construction
public:
	BestFit(int unknowns);
	BestFit(int unknowns, std::ostream &oStream);
	virtual ~BestFit();
private:
protected:

	// Implementation
public:
	bool Compute(BestFitIO &in, BestFitIO &out);
	virtual double SolveAt(double x, double y) const = 0;

private:
	virtual void GenerateProvisionals() = 0;
	virtual void FormulateMatrices() = 0;
	virtual void EvaluateFinalResiduals(int point, double &vxi, double &vyi) const;
	virtual void OutputAdjustedUnknowns(std::ostream &oStream) const = 0;
	virtual void NormaliseAdjustedUnknowns() {}

	void SetVerbosity(int verbosity);
	bool Compute();
	void ResizeMatrices();
	void AddObservation(int count, double x, double y);
	bool HasConverged() const;
	bool IsDegenerate(int iteration) const;
	bool InvertMatrix(const ublas::matrix<double> &input, ublas::matrix<double> &inverse);
	bool EvaluateUnknowns();
	void EvaluateResiduals();
	void EvaluateAdjustedUnknowns();
	void EvaluateAdjustedObservations();
	void GlobalCheck();
	void ErrorAnalysis(int iterations);
	void OutputSimpleSolution() const;
	void FillOutput(BestFitIO &out) const;
	void LowerTriangularModifyInversion(const ublas::matrix<double> &l, ublas::matrix<double> &m);
	bool CholeskyInversion(const ublas::matrix<double> &input, ublas::matrix<double> &inverse);
protected:

	// Variables
public:
private:
	int m_verbosity;
	NullStream m_nullStream;
	std::ostream &m_oStream;
	ublas::matrix<double> m_solution;
protected:
	ublas::matrix<double> m_residuals;
	ublas::matrix<double> m_design;
	ublas::matrix<double> m_l;
	ublas::banded_matrix<double> m_qweight;
	ublas::matrix<double> m_observations;
	ublas::matrix<double> m_provisionals;
	ublas::matrix<double> m_b;

	int m_numObs;
	int m_numUnknowns;
	double m_minx;
	double m_maxx;
	double m_miny;
	double m_maxy;
};

/***********************************************************
************************************************************
**********************  BestFitLine ************************
************************************************************
***********************************************************/

class BestFitLine : public BestFit
{
public:
	static const int kLineUnknowns;
	BestFitLine();
	BestFitLine(std::ostream &oStream);

private:
	void GenerateProvisionals();
	void FormulateMatrices();
	double SolveAt(double x, double y) const;
	void EvaluateFinalResiduals(int point, double &vxi, double &vyi) const;
	void OutputAdjustedUnknowns(std::ostream &oStream) const;
};

/***********************************************************
************************************************************
**********************  BestFitCircle **********************
************************************************************
***********************************************************/

class BestFitCircle : public BestFit
{
public:
	static const int kCircleUnknowns;
	BestFitCircle();
	BestFitCircle(std::ostream &oStream);

private:
	void GenerateProvisionals();
	void FormulateMatrices();
	double SolveAt(double x, double y) const;
	void OutputAdjustedUnknowns(std::ostream &oStream) const;
};

/***********************************************************
************************************************************
**********************  BestFitEllipse *********************
************************************************************
***********************************************************/

class BestFitEllipse : public BestFit
{
public:
	static const int kEllipseUnknowns;
	BestFitEllipse();
	BestFitEllipse(std::ostream &oStream);

private:
	void GenerateProvisionals();
	void FormulateMatrices();
	double SolveAt(double x, double y) const;
	void OutputAdjustedUnknowns(std::ostream &oStream) const;
	void NormaliseAdjustedUnknowns();
};

/***********************************************************
************************************************************
**********************  BestFitFactory *********************
************************************************************
***********************************************************/

struct BestFitFactory
{
	static BestFit *Create(int type);
	static BestFit *Create(int type, std::ostream &oStream);

	enum { Line, Circle, Ellipse };
};

/***********************************************************
************************************************************
**********************  Double *****************************
************************************************************
***********************************************************/

class Double
{
	static const double Accuracy;

public:
	static bool IsZero(double a);
	static bool IsNotZero(double a);
	static bool AreEqual(double a, double b);
	static bool AreNotEqual(double a, double b);
	static bool IsLessThan(double a, double b);
	static bool IsGreaterThan(double a, double b);
	static bool IsLessThanOrEquals(double a, double b);
	static bool IsGreaterThanOrEquals(double a, double b);
	static bool IsLessThanZero(double a);
	static bool IsGreaterThanZero(double a);
	static bool IsLessThanOrEqualsZero(double a);
	static bool IsGreaterThanOrEqualsZero(double a);
};