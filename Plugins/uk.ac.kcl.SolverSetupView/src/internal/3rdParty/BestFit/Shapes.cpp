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

#include "BestFit.h"

#include <iomanip>

#define HALF_PI 1.57079632679
#define TWO_PI  6.28318530718

/***********************************************************
************************************************************
**********************  BestFitLine ************************
************************************************************
***********************************************************/

const int BestFitLine::kLineUnknowns = 2; // unknowns for a line are slope, intercept

BestFitLine::BestFitLine()
	: BestFit(kLineUnknowns)
{
}

BestFitLine::BestFitLine(std::ostream &oStream)
	: BestFit(kLineUnknowns, oStream)
{
}

void BestFitLine::GenerateProvisionals()
{
	assert(0.0 != (m_maxx - m_minx));
	if (0.0 == (m_maxx - m_minx))
		throw std::out_of_range("division by zero");

	double slope = (m_maxy - m_miny) / (m_maxx - m_minx);
	double intercept = m_miny - (slope * m_minx);

	m_provisionals(0, 0) = slope;
	m_provisionals(1, 0) = intercept;
}

void BestFitLine::FormulateMatrices()
{
	double slope = m_provisionals(0, 0);
	double intercept = m_provisionals(1, 0);

	for (int i = 0; i < m_numObs; i++)
		{
		double x = m_observations(i, 0);
		double y = m_observations(i, 1);

		m_design(i, 0) = x;
		m_design(i, 1) = 1.0;

		m_qweight(i, i) = 1.0; // parametric case, not quasi-parametric
		m_l(i, 0) = y - ((x * slope) + intercept);
		}
}

double BestFitLine::SolveAt(double x, double y) const
{
	double slope = m_provisionals(0, 0);
	double intercept = m_provisionals(1, 0);
	return y - (x * slope + intercept);
}

void BestFitLine::EvaluateFinalResiduals(int point, double &vxi, double &vyi) const
{
	vxi = 0.0;
	vyi = m_residuals(point, 0);
}

void BestFitLine::OutputAdjustedUnknowns(std::ostream &oStream) const
{
	oStream << std::setprecision(6) << std::setiosflags(std::ios::fixed)
		<< "Adj. line gradient                 " << m_provisionals(0, 0) << std::endl
		<< "Adj. line Y-intercept              " << m_provisionals(1, 0) << std::endl;
}

/***********************************************************
************************************************************
**********************  BestFitCircle **********************
************************************************************
***********************************************************/

const int BestFitCircle::kCircleUnknowns = 3; // unknowns for a circle are centrex, centrey, radius

BestFitCircle::BestFitCircle()
	: BestFit(kCircleUnknowns)
{
}

BestFitCircle::BestFitCircle(std::ostream &oStream)
	: BestFit(kCircleUnknowns, oStream)
{
}

void BestFitCircle::GenerateProvisionals()
{
	double centrex = 0.5 * (m_maxx + m_minx);
	double centrey = 0.5 * (m_maxy + m_miny);
	double radius =  0.5 * std::max<double>(m_maxx - m_minx, m_maxy - m_miny);

	m_provisionals(0, 0) = centrex;
	m_provisionals(1, 0) = centrey;
	m_provisionals(2, 0) = radius;
}

void BestFitCircle::FormulateMatrices()
{
	double x0 = m_provisionals(0, 0);
	double y0 = m_provisionals(1, 0);
	double radius = m_provisionals(2, 0);

	for (int i = 0; i < m_numObs; i++)
		{
		double dx = m_observations(i, 0) - x0;
		double dy = m_observations(i, 1) - y0;
		double dxsqr = dx * dx;
		double dysqr = dy * dy;

		m_design(i, 0) = -2.0 * dx;
		m_design(i, 1) = -2.0 * dy;
		m_design(i, 2) = -2.0 * radius;
		m_qweight(i, i) = 1.0 / (4.0 * (dxsqr + dysqr));
		m_l(i, 0) = (radius * radius) - dxsqr - dysqr;
		m_b(i, i * 2 + 0) = 2.0 * dx;
		m_b(i, i * 2 + 1) = 2.0 * dy;
		}
}

double BestFitCircle::SolveAt(double x, double y) const
{
	double dx = x - m_provisionals(0, 0);
	double dy = y - m_provisionals(1, 0);
	double rsqr = m_provisionals(2, 0) * m_provisionals(2, 0);

	return rsqr - dx * dx - dy * dy;
}

void BestFitCircle::OutputAdjustedUnknowns(std::ostream &oStream) const
{
	oStream << std::setprecision(6) << std::setiosflags(std::ios::fixed)
		<< "Adj. circle centre X               " << m_provisionals(0, 0) << std::endl
		<< "Adj. circle centre Y               " << m_provisionals(1, 0) << std::endl
		<< "Adj. circle radius                 " << m_provisionals(2, 0) << std::endl;
}

/***********************************************************
************************************************************
**********************  BestFitEllipse *********************
************************************************************
***********************************************************/

const int BestFitEllipse::kEllipseUnknowns = 5; // unknowns for an ellipse are centrex, centrey, major, minor, rotation

BestFitEllipse::BestFitEllipse()
	: BestFit(kEllipseUnknowns)
{
}

BestFitEllipse::BestFitEllipse(std::ostream &oStream)
	: BestFit(kEllipseUnknowns, oStream)
{
}

void BestFitEllipse::GenerateProvisionals()
{
	double centrex = 0.5 * (m_maxx + m_minx);
	double centrey = 0.5 * (m_maxy + m_miny);
	double dx = m_maxx - m_minx;
	double dy = m_maxy - m_miny;
	double major = 0.5 * std::max<double>(dx, dy);
	double minor = 0.5 * std::min<double>(dx, dy);
	double rotation = atan(dy / dx);

	m_provisionals(0, 0) = centrex;
	m_provisionals(1, 0) = centrey;
	m_provisionals(2, 0) = major;
	m_provisionals(3, 0) = minor;
	m_provisionals(4, 0) = rotation;
}


void BestFitEllipse::FormulateMatrices()
{
	double x0 = m_provisionals(0, 0);
	double y0 = m_provisionals(1, 0);
	double a = m_provisionals(2, 0);
	double b = m_provisionals(3, 0);

	double sinr = sin(m_provisionals(4, 0));
	double cosr = cos(m_provisionals(4, 0));
	double asqr = a * a;
	double bsqr = b * b;

	for (int i = 0; i < m_numObs; i++)
		{
		double x = m_observations(i, 0);
		double y = m_observations(i, 1);

		double d1 = ((y - y0) * cosr - (x - x0) * sinr);
		double d2 = ((x - x0) * cosr + (y - y0) * sinr);

		m_design(i, 0) = 2.0 * ((asqr * d1 * sinr) - (bsqr * d2 * cosr));
		m_design(i, 1) = -2.0 * ((bsqr * d2 * sinr) + (asqr * d1 * cosr));
		m_design(i, 2) = 2.0 * a * ((d1 * d1) - bsqr);
		m_design(i, 3) = 2.0 * b * ((d2 * d2) - asqr);
		m_design(i, 4) = 2.0 * d1 * d2 * (bsqr - asqr);

		double p = 2.0 * ((asqr * d1 * sinr) + (bsqr * d2 * cosr));
		double q = -2.0 * ((bsqr * d2 * sinr) + (asqr * d1 * cosr));
		m_qweight(i, i) = 1.0 / (p * p + q * q);
		m_l(i, 0) = -((asqr * d1 * d1) + (bsqr * d2 * d2) - (asqr * bsqr));
		}
}

double BestFitEllipse::SolveAt(double x, double y) const
{
	double x0 = m_provisionals(0, 0);
	double y0 = m_provisionals(1, 0);
	double asqr = m_provisionals(2, 0) * m_provisionals(2, 0);
	double bsqr = m_provisionals(3, 0) * m_provisionals(3, 0);
	double sinr = sin(m_provisionals(4, 0));
	double cosr = cos(m_provisionals(4, 0));
	double dx = x - x0;
	double dy = y - y0;
	double d1 = ( dx * cosr + dy * sinr);
	double d2 = (-dx * sinr + dy * cosr);

	return ((d1 * d1) / asqr) + ((d2 * d2) / bsqr) - 1.0;
}

void BestFitEllipse::OutputAdjustedUnknowns(std::ostream &oStream) const
{
	oStream << std::setprecision(6) << std::setiosflags(std::ios::fixed)
		<< "Adj. ellipse centre X              " << m_provisionals(0, 0) << std::endl
		<< "Adj. ellipse centre Y              " << m_provisionals(1, 0) << std::endl
		<< "Adj. ellipse semi-major axis       " << m_provisionals(2, 0) << std::endl
		<< "Adj. ellipse semi-minor axis       " << m_provisionals(3, 0) << std::endl
		<< "Adj. ellipse rotation              " << m_provisionals(4, 0) << std::endl;
}

// Give ellipse chance to normalise major >= minor and 0 <= rotation angle < 2PI.
void BestFitEllipse::NormaliseAdjustedUnknowns()
{
	double theta = std::abs(m_provisionals(4, 0));
	bool negative = Double::IsLessThanZero(m_provisionals(4, 0));

	while (theta >= TWO_PI)
		theta -= TWO_PI;
	if (negative)
		theta = TWO_PI - theta;

	double major = m_provisionals(2, 0);
	double minor = m_provisionals(3, 0);
	if (Double::IsGreaterThan(minor, major))
	{
		// swap axes and make angle the perpendicular
		m_provisionals(2, 0) = minor;
		m_provisionals(3, 0) = major;

		theta -= HALF_PI;
		if (Double::IsLessThanZero(theta))
			theta += TWO_PI;
	}

	m_provisionals(4, 0) = theta;
}

/***********************************************************
************************************************************
**********************  BestFitFactory *********************
************************************************************
***********************************************************/

BestFit *BestFitFactory::Create(int type)
	{
	switch (type)
	{
		case 0: return new BestFitLine(); break;
		case 1: return new BestFitCircle(); break;
		case 2: return new BestFitEllipse(); break;
	}

	return NULL;
	}

BestFit *BestFitFactory::Create(int type, std::ostream &oStream)
{
	switch (type)
	{
		case 0: return new BestFitLine(oStream); break;
		case 1: return new BestFitCircle(oStream); break;
		case 2: return new BestFitEllipse(oStream); break;
	}

	return NULL;
}
