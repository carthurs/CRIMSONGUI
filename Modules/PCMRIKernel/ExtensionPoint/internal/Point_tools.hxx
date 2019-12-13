// -------------------------------------------------------------------------
/**
*     \class                  Point
*     \brief                  This class encapsulated usual functionality of 3D points in 3D images
*     \details					This is part of the definitions library
*
*
*     \author                 Alberto G&oacute;mez <alberto.gomez@kcl.ac.uk>
*     \date             January 2010
*     \note             Copyright (c) King's College London, The Rayne Institute.
*                             Division of Imaging Sciences, 4th floow Lambeth Wing, St Thomas' Hospital.
*                             All rights reserved.
*/
// -------------------------------------------------------------------------

#ifndef POINT_H_
#define POINT_H_
#include <iostream>
//#include "vnl/vnl_matrix.h"
//#include "vnl/vnl_matrix.txx"
#include <vector>
#include <algorithm>
#include <cmath>

namespace echo
{

template <class T, unsigned int NDIMS=3>
class Point : public std::vector<T>
{

    friend std::ostream& operator<<(std::ostream& output, const Point<T,NDIMS>& p)
    {
        for(typename Point<T,NDIMS>::const_iterator it = p.begin(); it != p.end(); ++it)
            output <<  *it << " ";
        return output;
    }



public:

    friend void swap(Point<T,NDIMS>& first, Point<T,NDIMS>& second) // nothrow
    {
        first.swap( second);
        // If there were more members:
        // std::swap(first.mArray, second.mArray);
    }

    typedef T ScalarType;

    Point() : std::vector<T>(NDIMS,0)
    {
        /// Nothing to be done. This function initialises the point to 0

    }

    Point(const T *val) : std::vector<T>(NDIMS)
    {
        for(typename std::vector<T>::iterator it = this->begin(); it != this->end(); ++it)
            *it=*(val++);
    }

    Point(const T val) : std::vector<T>(NDIMS)
    {
        for(typename std::vector<T>::iterator it = this->begin(); it != this->end(); ++it)
            *it=val;
    }



    Point(unsigned int *val) : std::vector<T>(NDIMS)
    {
        for(typename std::vector<T>::iterator it = this->begin(); it != this->end(); ++it)
            *it=(T) *(val++);
    }

    Point(unsigned long int *val) : std::vector<T>(NDIMS)
    {
        for(typename std::vector<T>::iterator it = this->begin(); it != this->end(); ++it)
            *it=(T) *(val++);
    }

    ~Point()
    {
        // delete[] a;
    }

    static unsigned int GetDimensions(){
        return NDIMS;
    }

    inline T prod() const{
        T value=1;
        typename std::vector<T>::const_iterator cit;
        for (cit=this->begin(); cit!=this->end(); ++cit)
            value *= *cit;
        return value;
    }

    inline T sum() const{
        T value=0;
        typename std::vector<T>::const_iterator cit;
        for (cit=this->begin(); cit!=this->end(); ++cit)
            value += *cit;
        return value;
    }

    inline Point<T,NDIMS> isnz() const{
        Point<T,NDIMS> output;
        typename std::vector<T>::const_iterator cit;
        typename std::vector<T>::iterator it;
        for (cit=this->begin(), it=output.begin(); cit!=this->end(); ++cit, ++it)
            *it = (T) *cit!=0;
        return output;
    }


    Point<T,NDIMS> & operator=( Point<T,NDIMS> rhs)
    {
		this->resize(rhs.size());
        std::copy(rhs.begin(), rhs.end(), this->begin());
        return *this;
    }

    //    Point<T,NDIMS> & operator=( T rhs)
    //    {
    //        typename std::vector<T>::iterator it;
    //        for (it=this->begin(); it!=this->end();  ++it)
    //            *it = rhs;
    //        return *this;
    //    }

    //    // operator overload
    //    Point<T,NDIMS> & operator=(const Point &rhs)
    //    {
    //        // Check for self-assignment!
    //        if (this != &rhs)       // Same object?
    //        {

    //            typename std::vector<T>::const_iterator cit;
    //            typename std::vector<T>::iterator it;
    //            for( it = this->begin(), cit = rhs.begin(); it != this->end(); ++it, ++cit)
    //                *it=*cit;
    //        }
    //        return *this;
    //    }

    inline Point<T,3> extract3DPoint() const{
        Point<T,3> output;
        if (NDIMS>3){

            typename std::vector<T>::const_iterator cit1;
            typename std::vector<T>::iterator it;
            for (cit1 = this->begin(), it = output.begin(); it!=output.end(); ++cit1, ++it)
                *it= *cit1;
        }
        return output;

    }



    /**
     * Checks if a point is inside some bounds
     */
    inline bool isIn(double *bounds) const
    {
        for (typename std::vector<T>::const_iterator citer=this->begin(); citer!=this->end(); ++citer){
            if (*citer < bounds[2*(citer-this->begin())] || *citer > bounds[2*(citer-this->begin())+1])
                return false;
        }
        return true;
    }

    inline double norm() const
    {
        double n2 = this->norm2();
        if (n2>=0){
            double n = std::sqrt(n2);
            return n;
        }else{
            std::cout << "ERROR, PointType::norm is negative"<<std::endl;
            return -1;
        }
    }

    inline double norm2() const
    {
        T squared_norm=0;
        typename std::vector<T>::const_iterator cit;
        for( cit = this->begin(); cit != this->end(); ++cit)
            squared_norm += *cit **cit;

        return squared_norm;
    }

    /**
     * Makes a floor operation in each dimension
     */

    inline Point<int,NDIMS> floor() const
    {
        Point<int,NDIMS> output;
        typename std::vector<T>::const_iterator cit;
        typename std::vector<int>::iterator it;
        for( cit = this->begin(), it = output.begin(); cit != this->end(); ++cit, ++it)
            *it = std::floor(*cit );

        return output;
    }

    inline T min() const
    {
        return *std::min_element(this->begin(), this->end());
    }

    inline T min(unsigned int start, unsigned int end) const
    {
        return *std::min_element(this->begin()+start, this->begin()+end);
    }

    inline T max() const
    {
        return *std::max_element(this->begin(), this->end());
    }

    inline T max(unsigned int start, unsigned int end) const
    {
        return *std::max_element(this->begin()+start, this->begin()+end);
    }

    inline T dot(const Point &v) const
    {
        T output = 0;
        typename std::vector<T>::const_iterator cit1, cit2;
        for( cit1 = this->begin(), cit2 = v.begin(); cit1 != this->end(); ++cit1, ++cit2)
            output +=*cit1 **cit2;

        return output;
    }
    /**
     * @brief cross
     * If the point is less than 3 dimensions, it is padded with zeros. Output is always of 3 dimensions
     * @param v
     * @return
     */
    inline Point<double,3> cross(const Point &v) const
    {
        if (this->size()>3 || v.size()>3)
        {
            std::cerr<<"ERROR: Point<int,NDIMS>::cross -cross product requires points of 3 dimensions"<<std::endl;
        }

        Point<T,3> tmp1, tmp2;

        for (int i=0; i<this->size(); i++){
            tmp1[i]=(*this)[i];
        }

        for (int i=0; i<v.size(); i++){
            tmp2[i]=v[i];
        }

        Point<double,3> output;

        output[0]=tmp1[1]*tmp2[2]-tmp1[2]*tmp2[1];
        output[1]=tmp1[2]*tmp2[0]-tmp1[0]*tmp2[2];
        output[2]=tmp1[0]*tmp2[1]-tmp1[1]*tmp2[0];
        return output;
    }



    /// Operator overload -----------------------------------------------------------------------------

    /**
     * Element wise division (always returns double point)
     */
    inline Point<double,NDIMS> operator/(const Point &v) const
    {
        Point<double,NDIMS> output;
        typename std::vector<T>::const_iterator cit1, cit2;
        typename std::vector<double>::iterator it;
        for (cit1 = this->begin(), cit2 = v.begin(), it = output.begin(); cit1!=this->end(); ++cit1, ++cit2, ++it)
            *it=(double) *cit1 / (double) *cit2;

        return output;
    }

    /**
     * Element wise product
     */
    inline Point<T,NDIMS> operator*(const Point<T,NDIMS> &v) const
    {
        Point<T,NDIMS> output;
        typename std::vector<T>::const_iterator cit1, cit2;
        typename std::vector<T>::iterator it;
        for (cit1 = this->begin(), cit2 = v.begin(), it = output.begin(); cit1!=this->end(); ++cit1, ++cit2, ++it)
            *it= *cit1 **cit2;
        return output;
    }



    /**
     * @brief operator *
     * @param v
     * @return product by a scalar
     */

    inline Point<T,NDIMS> operator*(const T &v) const
    {
        Point<T,NDIMS> output;
        typename std::vector<T>::const_iterator cit;
        typename std::vector<T>::iterator it;
        for (cit = this->begin(),  it = output.begin(); cit!=this->end(); ++cit, ++it)
            *it= *cit *  v;
        return output;
    }

    inline Point<T,NDIMS> & operator*=(T v)
    {
        typename std::vector<T>::iterator it;
        for (it=this->begin(); it!=this->end(); ++it)
            *it *= v;

        return *this;
    }

    template <class T2>
    inline Point<T,NDIMS> & operator*=( const Point<T2,NDIMS> &v)
    {
        typename std::vector<T2>::const_iterator cit;
        typename std::vector<T>::iterator it;
        for (it=this->begin(), cit = v.begin(); it!=this->end(); ++it, ++cit)
            *it *= (T) *cit;

        return *this;
    }


    inline Point<T,NDIMS> operator+(const T &v) const
    {
        Point<T,NDIMS> output;
        typename std::vector<T>::const_iterator cit;
        typename std::vector<T>::iterator it;
        for (cit = this->begin(),  it = output.begin(); cit!=this->end(); ++cit, ++it)
            *it= *cit +  v;
        return output;
    }


    inline Point<T,NDIMS> operator+(const Point &v) const
    {
        Point<T,NDIMS> output;
        typename std::vector<T>::const_iterator cit1, cit2;
        typename std::vector<T>::iterator it;
        for (cit1 = this->begin(), cit2 = v.begin(), it = output.begin(); cit1!=this->end(); ++cit1, ++cit2, ++it)
            *it= *cit1 +  *cit2;
        return output;
    }

    /**
     * If all elements are < then true
     */
    inline bool operator<(const Point &v) const
    {
        typename std::vector<T>::const_iterator cit1, cit2;
        for (cit1 = this->begin(), cit2 = v.begin(); cit1!=this->end(); ++cit1, ++cit2)
        {
            if ( *cit1 >=  *cit2)
                return false;
        }
        return true;
    }
    /**
     * If one element is != then true
     */
    inline bool operator!=(const Point<T,NDIMS> &v) const
    {
        typename std::vector<T>::const_iterator cit1, cit2;
        for (cit1 = this->begin(), cit2 = v.begin(); cit1!=this->end(); ++cit1, ++cit2)
        {
            if ( *cit1 !=  *cit2)
                return true;
        }
        return false;
    }

    /**
     * If all element is == then true
     */
    inline bool operator==(const Point &v) const
    {
        typename std::vector<T>::const_iterator cit1, cit2;
        for (cit1 = this->begin(), cit2 = v.begin(); cit1!=this->end(); ++cit1, ++cit2)
        {
            if ( *cit1 !=  *cit2)
                return false;
        }
        return true;
    }

    /**
     * If all elements are <= then true
     */
    inline bool operator<=(const Point &v) const
    {
        typename std::vector<T>::const_iterator cit1, cit2;
        for (cit1 = this->begin(), cit2 = v.begin(); cit1!=this->end(); ++cit1, ++cit2)
        {
            if ( *cit1 >  *cit2)
                return false;
        }
        return true;
    }
    /**
     * If all elements are > then true
     */
    inline bool operator>(const Point &v) const
    {
        typename std::vector<T>::const_iterator cit1, cit2;
        for (cit1 = this->begin(), cit2 = v.begin(); cit1!=this->end(); ++cit1, ++cit2)
        {
            if ( *cit1 <=  *cit2)
                return false;
        }
        return true;
    }

    /**
    * If all elements are > then true
    */
    inline bool operator>=(const Point &v) const
    {
        typename std::vector<T>::const_iterator cit1, cit2;
        for (cit1 = this->begin(), cit2 = v.begin(); cit1!=this->end(); ++cit1, ++cit2)
        {
            if ( *cit1 <  *cit2)
                return false;
        }
        return true;
    }


    inline Point<T,NDIMS> & operator-=( const Point<T,NDIMS> &v)
    {
        typename std::vector<T>::const_iterator cit;
        typename std::vector<T>::iterator it;
        for (it=this->begin(), cit = v.begin(); it!=this->end(); ++it, ++cit)
            *it -= *cit;

        return *this;
    }

    template <class T2>
    inline Point<T,NDIMS> & operator-=( const Point<T2,NDIMS> &v)
    {
        typename std::vector<T2>::const_iterator cit;
        typename std::vector<T>::iterator it;
        for (it=this->begin(), cit = v.begin(); it!=this->end(); ++it, ++cit)
            *it -= (T) *cit;

        return *this;
    }


    inline Point<T,NDIMS> & operator-=( const T &val)
    {
        typename std::vector<T>::iterator it;
        for (it=this->begin(); it!=this->end(); ++it)
            *it -= val;

        return *this;
    }

    inline Point<T,NDIMS> & operator+=( const Point<T,NDIMS> &v)
    {
        typename std::vector<T>::const_iterator cit;
        typename std::vector<T>::iterator it;
        for (it=this->begin(), cit = v.begin(); it!=this->end(); ++it, ++cit)
            *it += *cit;
        return *this;
    }

    inline Point<T,NDIMS> & operator+=( T &val)
    {
        typename std::vector<T>::iterator it;
        for (it=this->begin(); it!=this->end(); ++it)
            *it += val;
        return *this;
    }





    inline Point<T,NDIMS> operator/(double v) const
    {
        Point<T,NDIMS> output;
        typename std::vector<T>::const_iterator cit;
        typename std::vector<T>::iterator it;
        for (cit=this->begin(), it = output.begin(); cit!=this->end(); ++it, ++cit)
            *it = *cit/v;

        return output;
    }

    inline Point<T,NDIMS> & operator/=(double v)
    {
        typename std::vector<T>::iterator it;
        for (it=this->begin(); it!=this->end(); ++it)
            *it /= v;

        return *this;
    }

};



template <class T, unsigned int NDIMS>
inline  Point<T,NDIMS> operator-(Point<T,NDIMS> lhs, const Point<T,NDIMS> &v)
{
    lhs-= v;
    return lhs;
}

template <class T, unsigned int NDIMS, class T2>
inline Point<T,NDIMS> operator-(Point<T,NDIMS> lhs, const Point<T2,NDIMS> &v)
{
    lhs-= v;
    return lhs;
}

template <class T, unsigned int NDIMS, class T2>
inline Point<T,NDIMS> operator*(Point<T,NDIMS> lhs, const Point<T2,NDIMS> &v)
{
    lhs*= v;
    return lhs;
}


template <class T, unsigned int NDIMS>
inline Point<T,NDIMS> operator-(Point<T,NDIMS> lhs, const T &v)
{
    lhs-=v;
    return lhs;
};

template <class T, unsigned int NDIMS>
inline Point<T,NDIMS> operator+(Point<T,NDIMS> lhs, const Point<T,NDIMS> &v)
{
    lhs+=v;
    return lhs;
};

/**
 * Add two point lists
 */

template <class T, unsigned int NDIMS>
inline std::vector<Point<T,NDIMS> > operator+(std::vector< Point<T,NDIMS> > lhs, const std::vector< Point<T,NDIMS> > &rhs)
{


    typename std::vector<Point<T, NDIMS>  >::const_iterator cit ;
    typename std::vector<Point<T, NDIMS>  >::iterator it ;

    for (it = lhs.begin(), cit = rhs.begin() ; it!=lhs.end(); ++it, ++cit){

        *it += *cit;

    }

    return lhs;
}


/**
 * Calculate the angles of a triangle given the three points (ND points)
 */
template <class PointType>
void get_angles(const PointType & A, const PointType & B, const PointType & C, double & a, double & b, double & c){

    PointType AB = B-A;
    AB/=AB.norm();
    PointType AC = C-A;
    AC/=AC.norm();
    PointType BC = C-B;
    BC/=BC.norm();

    double cosa = AB.dot(AC);
    double sina = (AB.cross(AC)).norm();
    double cosb = BC.dot(AB*-1.0);
    double sinb = (BC.cross(AB*-1.0)).norm();

    a = atan2(sina,cosa);
    b = atan2(sinb,cosb);
    /*
    /// Make sure these are the internal angles
    if ((a+b>M_PI) && (a>b)){
        a=M_PI-a;
    } else if ((a+b>M_PI) && (a<=b)){
        b=M_PI-b;
    }
    */
    c = M_PI - (a+b);
}

template <class PointType>
void writePointListToFile(const std::vector< PointType > &point_list, std::string &filename)
{
    std::ofstream myfileW(filename.c_str());
    if (myfileW.is_open())
    {
        for (int i = 0; i < point_list.size(); i++)
        {
            myfileW << point_list[i]<<std::endl;
        }
        myfileW.close();
    }
    else
    {
        std::cerr << "Could not open file " << filename << " for writing."<< std::endl;
    }
}

template <class PointType>
void readPointListFromFile(const std::string &filename, std::vector< PointType > &point_list)
{
    std::fstream myfileR;
    myfileR.open(filename.c_str(), std::fstream::in);
    if (myfileR.is_open())
    {
        double dummy;
        while(myfileR.eof() == false){
            PointType p;
            for (int j=0; j< PointType::GetDimensions(); j++){
                myfileR >> dummy;
                p[j]=dummy;
            }
            point_list.push_back(p);
        }
        myfileR.close();
    }
    else
    {
        std::cerr << "Could not open file " << filename << " for reading."<< std::endl;
    }
}

template <class PointType>
void getBounds(const std::vector< PointType > &point_list, double *bounds)
{
    for (int i=0; i<point_list[0].size(); i++){
        double max = std::numeric_limits<double>::min();
        double min = std::numeric_limits<double>::max();

        typename std::vector<PointType>::const_iterator cit ;
        for (cit = point_list.begin(); cit!=point_list.end(); ++cit){
            if ((*cit)[i]>max)
                max=(*cit)[i];

            if ((*cit)[i]<min)
                min=(*cit)[i];
        }
        bounds[i*2]=min;
        bounds[i*2+1]=max;
    }
}


template <class T, unsigned int NDIMS=3>
class CircularPoint : public Point<T,NDIMS>
{
public:

    typedef Point<T,NDIMS> ParentType;
    typedef typename ParentType::ScalarType ScalarType;

    CircularPoint() : Point<T,NDIMS>() {}
    CircularPoint(const T *val) : Point<T,NDIMS>(val) {}
    CircularPoint(unsigned int *val) : Point<T,NDIMS>(val){}
    CircularPoint(unsigned long int *val) : Point<T,NDIMS>(val){}

    ~CircularPoint(){}

    T next_index ( int current, int incr=1) const{
        if (current+incr < NDIMS){
            return current+incr;
        } else {
            int newcurrent = current-NDIMS;
            this->next_index(newcurrent);
        }
    }



};

template <class T>
class CircularVector : public std::vector<T>
{
public:

    typedef std::vector<T> ParentType;
    typedef typename ParentType::value_type value_type;
    typedef typename ParentType::const_iterator const_iterator;
    typedef typename ParentType::iterator iterator;

    CircularVector() : std::vector<T>() {}
    ~CircularVector(){}

    int next_index( int current, int incr=1) const{

        int last = this->size();

        if (current+incr < last){
            return current+incr;
        } else {
            int newcurrent = current-last;
            this->next_index(newcurrent);
        }
    }


};


inline double cotan(double i) { return(1 / tan(i)); }


} /// end namespace echo
#endif /* POINT_H_ */
