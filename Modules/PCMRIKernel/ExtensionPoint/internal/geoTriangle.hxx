#ifndef ECHOGEOTRIANGLE_H_
#define ECHOGEOTRIANGLE_H_

#include <Mesh/geoCellBase.hxx>

namespace echo
{

namespace geo{


const double VERY_SMALL_DOUBLE  = 0;

/**
 * @brief The Triangle class
 * Topology is always vertex 0 connects to vertex 1 connects to vertex 2 connects to vertex 0
 */
template< typename CoordType, unsigned int InputDimension=2>
class Triangle : echo::geo::CellBase< CoordType, InputDimension>
{
public:

    const static unsigned int NVERTICES=3;

    typedef echo::geo::CellBase< CoordType, InputDimension> ParentType;
    typedef typename ParentType::CoordinateType CoordinateType;
    typedef typename ParentType::PointType PointType;
    typedef echo::Point<CoordType, NVERTICES> BarycentricCoordinatesType;
    /// Constructors
    Triangle();
    //Triangle(std::vector<PointType> & points);
    ~Triangle() {}

    /**
     * @brief contains
     * Whether a point is contained inside the triangle or not.
     * If the input dimension is greater than 2, then it first verifies if the point is in the triangle plane.
     * Algorithm taken from http://www.blackpawn.com/texts/pointinpoly/
     * @param p
     * @return
     */
    bool contains(const PointType &p);

    /**
     * @brief barycentricCoordinates
     * Calculate the barycentric coordinates of a point. First checks that the point is in the triangle.
     * If the first element of the output vector is negative then the point is outside the triangle.
     * @param p
     * @return a three element std::vector with the coordinates
     */
    BarycentricCoordinatesType barycentricCoordinates(const PointType &p);

    /**
     * @brief calculateAngles
     * calculate the internal triangles
     */
    void computeAngles();

    double computeArea();

    PointType *GetVertex(unsigned int i){return this->m_vertices[i];}

    /**
     * @brief addPoint
     * @param p
     */
    void insertPoint(unsigned int idx, PointType &p);

    inline double getAngle(int i){
        return this->m_angles[i];
    }

    inline double getArea(){
        return this->m_area;
    }





private:

    bool sameSide(const PointType p1, const PointType p2, const PointType a, const PointType b);
    BarycentricCoordinatesType m_angles;
    double m_area;

};

template<typename  CoordType, unsigned int InputDimension>
Triangle< CoordType, InputDimension>::Triangle(){

    this->m_vertices.resize(NVERTICES);
}


template<typename  CoordType, unsigned int InputDimension>
void Triangle< CoordType, InputDimension>::computeAngles(){

    double a,b,c;
    echo::get_angles<PointType>(*(this->m_vertices[0]),*(this->m_vertices[1]),*(this->m_vertices[2]),a,b,c);

    this->m_angles[0] = a;
    this->m_angles[1] = b;
    this->m_angles[2] = c;

}

template<typename  CoordType, unsigned int InputDimension>
double Triangle< CoordType, InputDimension>::computeArea(){

    PointType A = *(this->m_vertices[0]);
    PointType B = *(this->m_vertices[1]);
    PointType C = *(this->m_vertices[2]);

    PointType AB = B-A;
    PointType AC = C-A;

    this->m_area = (AB.cross(AC)).norm()/2;
    return this->m_area;

}





template<typename  CoordType, unsigned int InputDimension>
bool Triangle< CoordType, InputDimension>::contains(const PointType &p){


    if (InputDimension >2){
        std::cout << "TODO: Currently only 2D triangular meshes supported"<<std::endl;
        return false;
    }

    /// Using barycentric coordinates from http://www.blackpawn.com/texts/pointinpoly/
    BarycentricCoordinatesType bc = this->barycentricCoordinates(p);
    bool isin =(bc[0]+VERY_SMALL_DOUBLE >= 0) && (bc[1]+VERY_SMALL_DOUBLE >= 0) && (bc[0] + bc[1] - VERY_SMALL_DOUBLE < 1);
    return isin;

}

template<typename  CoordType, unsigned int InputDimension>
bool Triangle< CoordType, InputDimension>::sameSide(const PointType p1, const PointType p2, const PointType a, const PointType b){

    typename echo::Point<CoordType, 3> cp1 = (b-a).cross(p1-a);
    typename echo::Point<CoordType, 3> cp2 = (b-a).cross(p2-a);
    if (cp1.dot(cp2) >=0){
        return true;
    }
    return false;
}


template<typename  CoordType, unsigned int InputDimension>
void Triangle< CoordType, InputDimension>::insertPoint(unsigned int idx, PointType &p){
    this->m_vertices[idx] = &p;
}

/**
 * Implementation borrowed from here http://www.blackpawn.com/texts/pointinpoly/
 */
template<typename  CoordType, unsigned int InputDimension>
typename Triangle<CoordType, InputDimension>::BarycentricCoordinatesType
Triangle< CoordType, InputDimension>::barycentricCoordinates(const PointType &p){

    BarycentricCoordinatesType coordinates;

    // Compute vectors
    PointType v0 = *(this->m_vertices[2]) - *(this->m_vertices[0]);
    PointType v1 = *(this->m_vertices[1]) - *(this->m_vertices[0]);
    PointType v2 = p - *(this->m_vertices[0]);

    // Compute dot products
    double dot00 = v0.dot(v0);
    double dot01 = v0.dot(v1);
    double dot02 = v0.dot(v2);
    double dot11 = v1.dot(v1);
    double dot12 = v1.dot(v2);

    // Compute barycentric coordinates
    double invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
    coordinates[1] = (dot00 * dot12 - dot01 * dot02) * invDenom;
    coordinates[2] = (dot11 * dot02 - dot01 * dot12) * invDenom;
    coordinates[0] = 1-coordinates[1]-coordinates[2];

    return coordinates;
}


} /// end namespace geo

} /// end namespace echo

#endif //ECHOGEOTRIANGLE_H_
