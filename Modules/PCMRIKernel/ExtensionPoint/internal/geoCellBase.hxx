#ifndef ECHOGEOCELLBASE_H_
#define ECHOGEOCELLBASE_H_

#include <Point_tools.hxx>

namespace echo
{

namespace geo{

template< typename CoordType, unsigned int InputDimension=2>
class CellBase
{
public:

    typedef CoordType CoordinateType;
    typedef echo::Point<CoordType, InputDimension> PointType;

    virtual bool contains(const PointType &p) = 0;

private:

protected: /// These will be inherited
    std::vector <PointType *> m_vertices;
};


} /// end namespace geo

} /// end namespace echo

#endif //ECHOGEOCELLBASE_H_
