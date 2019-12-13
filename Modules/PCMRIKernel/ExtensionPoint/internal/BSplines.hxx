// -------------------------------------------------------------------------
/**
*     \class                  Bsplines
*
*     \author                 Alberto G&oacute;mez <alberto.gomez@kcl.ac.uk>
*     \date             March 2010
*     \note             Copyright (c) King's College London, The Rayne Institute.
*                             Division of Imaging Sciences, 4th floow Lambeth Wing, St Thomas' Hospital.
*                             All rights reserved.
*/
// -------------------------------------------------------------------------

#ifndef BSPLINES_H_
#define BSPLINES_H_

#include <iostream>
#include <cmath>

namespace echo
{

double Bspline(unsigned int DEGREE, double x);
double B1(double x);
double B3(double x);

template<class ArrayType>
ArrayType Bspline(unsigned int DEGREE, const ArrayType &x);
template<class ArrayType>
ArrayType B1(const ArrayType &x);
template<class ArrayType>
ArrayType B3(const ArrayType &x);

/// Derivatives of the B-spline

double dBspline(unsigned int DEGREE, double x);
double dB1(double x);
double dB3(double x);

template<class ArrayType>
ArrayType dBspline(unsigned int DEGREE, const ArrayType &x);
template<class ArrayType>
ArrayType dB1(const ArrayType &x);
template<class ArrayType>
ArrayType dB3(const ArrayType &x);

/// Convolutions of the B-spline
template <class ArrayIn, class ArrayOut>
ArrayOut BsplineConv(unsigned int DEGREE, ArrayIn &x);
double BsplineConv(unsigned int DEGREE, int x);
double Bc1(int x);
double Bc3(int x);
/// Convolutions of the derivative of B-spline
template <class ArrayIn, class ArrayOut>
ArrayOut dBsplineConv(unsigned int DEGREE, ArrayIn &x);
double dBsplineConv(unsigned int DEGREE, int x);
double dBc1(int x);
double dBc3(int x);
/// Convolutions of a B-spline with the derivative of B-spline
template <class ArrayIn, class ArrayOut>
ArrayOut BsplineCrossedConv(unsigned int DEGREE, ArrayIn &x, int order);
double BsplineCrossedConv(unsigned int DEGREE, int x, int order);
double Bcc1(int x);
double Bcc3(int x);

///---------------------------------------------------------------------------///
///-----------------------------IMPLEMENTATION--------------------------------///
///---------------------------------------------------------------------------///

///----------------- Bspline ------------------------------
/**
 * Version optimized for arrays. Arrays should have the iteration mechanism of stl vector
 */
template<class ArrayType>
ArrayType Bspline(unsigned int DEGREE, const ArrayType &x)
{
    switch (DEGREE)
    {
    case 1:
        return B1<ArrayType>(x);
        break;
    case 3:
        return B3<ArrayType>(x);
        break;
    default:
        std::cerr << "BSpline error:: wrong degree "<< DEGREE<< std::endl;
        return x;
    }
}


inline double Bspline(unsigned int DEGREE, double x)
{
    switch (DEGREE)
    {
    case 1:
        return B1(x);
        break;
    case 3:
        return B3(x);
        break;
    default:
        std::cerr << "BSpline error:: wrong degree "<< DEGREE<< std::endl;
        return 0;
    }
}

template<class ArrayType>
inline ArrayType B1(const ArrayType &x)
{
    ArrayType out;
    typename ArrayType::const_iterator const_iter;
    typename ArrayType::iterator iter;
    for ( iter = out.begin(), const_iter = x.begin(); const_iter!=x.end(); ++const_iter,  ++iter)
    {
        if (*const_iter>-1 && *const_iter<0)
            *iter =  *const_iter+1;
        else if (*const_iter>=0 && *const_iter<1)
            *iter =  1-*const_iter;
        else
            *iter =0;
    }
    return out;
}

template<class ArrayType>
inline ArrayType B3(const ArrayType &x)
{
    ArrayType out;
    typename ArrayType::const_iterator const_iter;
    typename ArrayType::iterator iter;
    for ( iter = out.begin(), const_iter = x.begin(); const_iter!=x.end(); ++const_iter,  ++iter)
    {

        if (*const_iter>-2 && *const_iter<-1)
            *iter = (*const_iter **const_iter**const_iter+6* *const_iter**const_iter+12* *const_iter+8)/6;
        else if (*const_iter>=-1 && *const_iter<0)
            *iter = -(3* *const_iter**const_iter**const_iter+6* *const_iter**const_iter-4)/6;
        else if (*const_iter>=0 && *const_iter<1)
            *iter = (3* *const_iter**const_iter**const_iter-6* *const_iter**const_iter+4)/6;
        else if (*const_iter>=1 && *const_iter<2)
            *iter = -(*const_iter**const_iter**const_iter-6* *const_iter**const_iter+12* *const_iter-8)/6;
        else
            *iter = 0;
    }
    return out;
}


inline double B1(double x)
{
    if (x>-1 && x<0)
        return x+1;
    else if (x>=0 && x<1)
        return 1-x;
    else
        return 0;
}

inline double B3(double x)
{

    double x2 = x*x;
    double x3 = x*x*x;

    if (x>-2 && x<-1)
        return (x3+6*x2+12*x+8)/6;
    else if (x>=-1 && x<0)
        return -(3*x3+6*x2-4)/6;
    else if (x>=0 && x<1)
        return (3*x3-6*x2+4)/6;
    else if (x>=1 && x<2)
        return -(x3-6*x2+12*x-8)/6;
    else
        return 0;
}

///----------------- dBspline ------------------------------
template<class ArrayType>
ArrayType dBspline(unsigned int DEGREE, const ArrayType &x)
{
    switch (DEGREE)
    {
    case 1:
        return dB1<ArrayType>(x);
        break;
    case 3:
        return dB3<ArrayType>(x);
        break;
    default:
        std::cerr << "BSpline error:: wrong degree "<< DEGREE<< std::endl;
        return x;
    }
}

inline double dBspline(unsigned int DEGREE, double x)
{
    switch (DEGREE)
    {
    case 1:
        return dB1(x);
        break;
    case 3:
        return dB3(x);
        break;
    default:
        std::cerr << "BSpline error:: wrong degree "<< DEGREE<< std::endl;
        return 0;
    }
}


template<class ArrayType>
inline ArrayType dB1(const ArrayType &x)
{
    ArrayType out;
    typename ArrayType::const_iterator const_iter;
    typename ArrayType::iterator iter;
    for ( iter = out.begin(), const_iter = x.begin(); const_iter!=x.end(); ++const_iter,  ++iter)
    {
        if (*const_iter>-1 && *const_iter<0)
            *iter =  1;
        else if (*const_iter>=0 && *const_iter<1)
            *iter =  -1;
        else
            *iter =0;
    }
    return out;
}

template<class ArrayType>
inline ArrayType dB3(const ArrayType &x)
{
    ArrayType out;
    typename ArrayType::const_iterator const_iter;
    typename ArrayType::iterator iter;
    for ( iter = out.begin(), const_iter = x.begin(); const_iter!=x.end(); ++const_iter,  ++iter)
    {

        if (*const_iter>-2 && *const_iter<-1)
            *iter = (*const_iter **const_iter+4* *const_iter+4)/2;
        else if (*const_iter>=-1 && *const_iter<0)
            *iter = -(3* *const_iter**const_iter+4* *const_iter)/2;
        else if (*const_iter>=0 && *const_iter<1)
            *iter = (3* *const_iter**const_iter-4* *const_iter)/2;
        else if (*const_iter>=1 && *const_iter<2)
            *iter = -(*const_iter**const_iter-4* *const_iter+4)/2;
        else
            *iter = 0;
    }
    return out;
}

inline double dB1(double x)
{
    if (x>-1 && x<0)
        return 1;
    else if (x>=0 && x<1)
        return -1;
    else
        return 0;
}

inline double dB3(double x)
{

    double x2 = x*x;

    if (x>-2 && x<-1)
        return (x2+4*x + 4)/2;
    else if (x>=-1 && x<0)
        return -(3*x2+4*x)/2;
    else if (x>=0 && x<1)
        return (3*x2-4*x)/2;
    else if (x>=1 && x<2)
        return -(x2-4*x+4)/2;
    else
        return 0;
}


///----------------- BsplineConv ------------------------------
template <class ArrayIn, class ArrayOut>
ArrayOut BsplineConv(unsigned int DEGREE, ArrayIn &x)
{
    ArrayOut out;
    for (int k=0; k<x.size(); k++)
    {
        out[k]=BsplineConv(DEGREE, x[k]);
    }
    return out;
}

inline double BsplineConv(unsigned int DEGREE, int x)
{
    switch (DEGREE)
    {
    case 1:
        return Bc1(x);
        break;
    case 3:
        return Bc3(x);
        break;
    default:
        std::cerr << "BSpline error:: wrong degree "<< DEGREE<< std::endl;
        return 0;
    }
}

inline double Bc1(int x_)
{

    /*
    double x = 1.0*x_;
    double x2 = x*x;
    double x3 = x*x*x;
    if (x_==-2)
        return  (x3+6*x2+12*x+8)/6;
    else if (x_==-1)
        return -(3*x3+6*x2-4)/6;
    else if (x_==0)
        return (3*x3-6*x2+4)/6;
    else if (x_==1 || x_==2)
        return -(x3-6*x2+12*x-8)/6;
    else
        return 0;
    */
    ///  The cases (x_==-2),  (x_==2) give 0, so we save time by not evaluating them. Also,
    /// we now the values for the other cases, so we can hardcode them
    if (x_==-1)
        return 0.16666666666666665741;
    else if (x_==0)
        return 0.66666666666666662966;
    else if (x_==1 )
        return 0.16666666666666665741;
    else
        return 0;

}

inline double Bc3(int x_)
{
    /*
    double x = 1.0*x_;

    double x2 = x*x;
    double x3 = x*x*x;
    double x4 = x*x*x*x;
    double x5 = x*x*x*x*x;
    double x6 = x*x*x*x*x*x;
    double x7 = x*x*x*x*x*x*x;


    if (x_==-3)
        return -(7*x7+140*x6+1176*x5+5320*x4+13720*x3+19320*x2+12152*x+1112)/5040;
    else if (x_==-2)
        return (21*x7+252*x6+1176*x5+2520*x4+1960*x3-504*x2+392*x+2472)/5040;
    else if (x_==-1)
        return -(35*x7+140*x6-560*x4+1680*x2-2416)/5040;
    else if (x_==0)
        return (35*x7-140*x6+560*x4-1680*x2+2416)/5040;
    else if (x_==1)
        return  -(21*x7-252*x6+1176*x5-2520*x4+1960*x3+504*x2+392*x-2472)/5040;
    else if (x_==2 || x_==3)
        return (7*x7-140*x6+1176*x5-5320*x4+13720*x3-19320*x2+12152*x-1112)/5040;
    else
        return 0;
    */

    if (x_==-3)
        return 0.00019841269841269841;
    else if (x_==-2)
        return 0.02380952380952380820;
    else if (x_==-1)
        return 0.23630952380952380265;
    else if (x_==0)
        return 0.47936507936507938288;
    else if (x_==1)
        return  0.23630952380952380265;
    else if (x_==2)
        return 0.02380952380952380820;
    else if (x_==3)
        return 0.00019841269841269841;
    else
        return 0;



}

///----------------- dBsplineConv ------------------------------
template <class ArrayIn, class ArrayOut>
ArrayOut dBsplineConv(unsigned int DEGREE, ArrayIn &x)
{
    ArrayOut out;
    for (int k=0; k<x.size(); k++)
    {
        out[k]=dBsplineConv(DEGREE, x[k]);
    }
    return out;
}

inline double dBsplineConv(unsigned int DEGREE, int x)
{
    switch (DEGREE)
    {
    case 1:
        return dBc1(x);
        break;
    case 3:
        return dBc3(x);
        break;
    default:
        std::cerr << "BSpline error:: wrong degree "<< DEGREE<< std::endl;
        return 0;
    }
}

inline double dBc1(int x_)
{
    /*
    double x = 1.0*x_;
    if (x_==-2)
        return -x-2;
    else if (x_==-1)
        return 3*x+2;
    else if (x_==0)
        return 2-3*x;
    else if (x_==1)
        return x-2;
    else
        return 0;
        */

    if (x_==-1)
        return -1.0;
    else if (x_==0)
        return 2.0;
    else if (x_==1)
        return -1.0;
    else
        return 0;
}

inline double dBc3(int x_)
{
    double x = 1.0*x_;

    /*
      double x2 = x*x;
    double x3 = x*x*x;
    double x4 = x*x*x*x;
    double x5 = x*x*x*x*x;
    if (x>=-3 && x<-2)
        return (7*x5+100*x4+560*x3+1520*x2+1960*x+920)/120;
    else if (x>=-2 && x<-1)
        return -(21*x5+180*x4+560*x3+720*x2+280*x-24)/120;
    else if (x>=-1 && x<0)
        return (7*x5+20*x4-32*x2+16)/24;
    else if (x>=0 && x<1)
        return -(7*x5-20*x4+32*x2-16)/24;
    else if (x>=1 && x<2)
        return (21*x5-180*x4+560*x3-720*x2+280*x+24)/120;
    else if (x>=2 && x<=3)
        return -(7*x5-100*x4+560*x3-1520*x2+1960*x-920)/120;
    else
        return 0;
    */


    if (x_==-3)
        return -0.00833333333333333322;
    else if (x_==-2)
        return -0.20000000000000001110;
    else if (x_==-1)
        return -0.12500000000000000000;
    else if (x_==0)
        return 0.66666666666666662966;
    else if (x_==1)
        return -0.12500000000000000000;
    else if (x_==2)
        return -0.20000000000000001110;
    else if (x_==3)
        return -0.00833333333333333322;
    else
        return 0;
}

///----------------- dBsplineCrossedConv ------------------------------
template <class ArrayIn, class ArrayOut>
ArrayOut BsplineCrossedConv(unsigned int DEGREE, ArrayIn &x, int order)
{
    ArrayOut out;
    for (int k=0; k<x.size(); k++)
    {
        out[k]=BsplineCrossedConv(DEGREE, x[k],order);
    }
    return out;
}

inline double BsplineCrossedConv(unsigned int DEGREE, int x, int order)
{
    switch (DEGREE)
    {
    case 1:
        return Bcc1(x)* (double) order;
        break;
    case 3:
        return Bcc3(x)* (double) order;
        break;
    default:
        std::cerr << "BSpline error:: wrong degree "<< DEGREE<< std::endl;
        return 0;
    }
}

inline double Bcc1(int x_)
{
    /*double x = 1.0*x_;
    double x2 = x*x;

    if (x_==-2)
        return -(x2+4*x+4)/2;
    else if (x_==-1)
        return (3*x2+4*x)/2;
    else if (x_==0)
        return -(3*x2-4*x)/2;
    else if (x_==1 || x_==2)
        return (x2-4*x+4)/2;
    else
        return 0;*/

    if (x_==-1)
        return -0.5;
    else if (x_==1)
        return 0.5;
    else
        return 0;
}

inline double Bcc3(int x_)
{
    /*
    double x = 1.0*x_;
    double x2 = x*x;
    double x3 = x*x*x;
    double x4 = x*x*x*x;
    double x5 = x*x*x*x*x;
    double x6 = x*x*x*x*x*x;

    if (x_==-3)
        return -(7*x6+120*x5+840*x4+3040*x3+5880*x2+5520*x+1736)/720;
    else if (x_==-2)
        return (21*x6+216*x5+840*x4+1440*x3+840*x2-144*x+56)/720;
    else if (x_==-1)
        return -(7*x6+24*x5-64*x3+96*x)/144;
    else if (x_==0)
        return (7*x6-24*x5+64*x3-96*x)/144;
    else if (x_==1)
        return -(21*x6-216*x5+840*x4-1440*x3+840*x2+144*x+56)/720;
    else if (x_==2 || x_==3)
        return (7*x6-120*x5+840*x4-3040*x3+5880*x2-5520*x+1736)/720;
    else
        return 0;
    */


    if (x_==-3)
        return 0.00138888888888888894;
    else if (x_==-2)
        return 0.07777777777777777901;
    else if (x_==-1)
        return 0.34027777777777779011;
    else if (x_==1)
        return -0.34027777777777779011;
    else if (x_==2)
        return -0.07777777777777777901;
    else if (x_ == 3)
        return -0.00138888888888888894;
    else
        return 0;
}


}


#endif /* BSPLINES_H_ */

