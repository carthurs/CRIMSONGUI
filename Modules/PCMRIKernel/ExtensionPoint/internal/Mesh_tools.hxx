#ifndef MESHTOOLS_H_
#define MESHTOOLS_H_

#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkMatrix4x4.h>
#include <vtkPointSet.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPoints2D.h>
#include <vtkDataSetWriter.h>
#include <vtkUnstructuredGrid.h>
#include <vtkUnstructuredGridWriter.h>
#include <vtkPolyDataWriter.h>
#include <vtkTransform.h>
#include <vtkCellArray.h>
#include <vtkTriangle.h>

#include <itkCellInterface.h>
#include <itkTriangleCell.h>
#include <itkQuadrilateralCell.h>

#include <Point_tools.hxx>
#include <String_tools.hxx>
#include <geoMeshField.hxx>

/// Smart Pointers from boost library
#include <boost/shared_ptr.hpp>

namespace echo
{

namespace geo{

template<class CellType, unsigned int InputDimension=2>
class Mesh
{
public:

    friend void swap(Mesh<CellType, InputDimension>& first, Mesh<CellType, InputDimension>& second) // nothrow
    {
        first.swap( second);
    }

    typedef struct {
        std::vector<unsigned int> cells;
        /**
         * @brief index
         *index of the vertex where angle a is to be measured.
         */
        std::vector<unsigned int> index;
    } OneRingCellType;

    static const  unsigned int NVERTICES = CellType::NVERTICES;
    typedef typename CellType::CoordinateType CoordinateType;
	typedef typename boost::shared_ptr<Mesh<CellType, InputDimension> > Pointer;

    typedef  echo::Point<CoordinateType, InputDimension> PointType;
    //typedef  echo::Point<unsigned int, CellType::NVERTICES> CellTopologyType;
    typedef  echo::CircularPoint<unsigned int, CellType::NVERTICES> CellTopologyType;

    typedef  struct {
        int triangle_index;
        echo::Point<double, CellType::NVERTICES> coordinates;
    } BarycentricCoordinatesType;

    typedef MeshField<double> MeshDoubleFieldType;
    typedef MeshField<bool> MeshBoolFieldType;

    /// Matrix types
    // typedef Eigen::Matrix<CoordinateType, Eigen::Dynamic,Eigen::Dynamic> DenseMatrixType;
    // typedef Eigen::SparseMatrix<CoordinateType> SparseMatrixType; /// Sparse and Col major.
    // typedef Eigen::SparseVector<CoordinateType> SparseVectorType;
    // typedef Eigen::Matrix<CoordinateType, Eigen::Dynamic,1> DenseVectorType;
    // typedef Eigen::Triplet<CoordinateType> TripletType;

    /// Constructors
    Mesh(){}
	Mesh(const typename Mesh<CellType, InputDimension>::Pointer other);
    Mesh(const std::vector< CellTopologyType > & topology, const std::vector<PointType> & points);
    ~Mesh() {}

    /**
     * @brief computeAngles
     * Calculate all the angles of all triangles
     */
    void computeAngles();

    /**
     * @brief computeAreas
     */
    void computeAreas();

    /**
     * @brief SetPoints
     * @param points
     */
    void SetPoints(const std::vector<PointType> & points);

    /**
     * @brief SetTopology
     * @param topology
     */
    void SetTopology(const std::vector<CellTopologyType> & topology);

    /**
     * @brief writeToFile
     * @param filename
     * @return
     */
    int  writeToFile(std::string &filename);

    /**
     * @brief toVTKMesh
     * @return
     */
    vtkSmartPointer<vtkPolyData> toVTKMesh();

    Mesh<CellType, InputDimension> & operator=( Mesh<CellType, InputDimension> rhs)
    {
        swap(*this,rhs);
        return *this;
    }

    /**
     * @brief addDoubleField
     * @param data
     * @param name
     */
    void addDoubleField(const std::vector< double > & data, std::string & name);
    void addDoubleField(const MeshDoubleFieldType & f);


    /**
     * @brief GetBarycentricCoordinates
     * Find the barycentric coordinates in the mesh
     * @param points
     * @return
     */
    std::vector<BarycentricCoordinatesType> GetBarycentricCoordinates(const std::vector<PointType> & points);
    BarycentricCoordinatesType GetBarycentricCoordinates(const PointType & points);

    /**
     * @brief inWhichCell
     * @param p
     * @return -1 if not found
     */
    int inWhichCell(const PointType & p);

    /**
     * @brief GetPosition
     * Use barycentric coordinates to Get true coordinates in the mesh
     * @param bc
     * @return
     */
    std::vector<PointType> GetPosition(const std::vector<BarycentricCoordinatesType> & bc);
    PointType GetPosition(const BarycentricCoordinatesType & bc);

    /**
     * @brief Get1Ring
     * Get the sorted indices (couter-clockwise) of the 1-ring neighbours of a node.
     * For efficiency, the first time this method is called all the 1-ring neighbourhood are calculated.
     * @param i
     * @return
     */
    std::vector<unsigned int> Get1Ring(unsigned int i);
    OneRingCellType Get1RingCells(unsigned int i);

    inline unsigned int GetNumberOfDoubleFields(){ return this->m_doubleFields.size();}
    MeshDoubleFieldType GetDoubleField(unsigned int i){return this->m_doubleFields[i];}
    inline unsigned int GetNumberOfBoolFields(){ return this->m_boolFields.size();}
    MeshBoolFieldType GetBoolField(unsigned int i){return this->m_boolFields[i];}

    inline unsigned int GetNumberOfNodes(){return this->m_points.size();}
    inline unsigned int GetNumberOfCells(){return this->m_cells.size();}



    /**
     * @brief m_cells
     * Array of cells. Each cell does not contain the points but have a pointer to the points.
     * The points are stored in m_points.
     */
    std::vector<CellType> m_cells;

    /**
     * @brief m_topology
     * Array with the incides of the vertices that for each cell
     */
    std::vector< CellTopologyType > m_topology;

    /**
     * @brief m_points
     * Array of the points that form each cell
     */
    std::vector<PointType> m_points;


private:

    std::vector< MeshDoubleFieldType > m_doubleFields;
    std::vector< MeshBoolFieldType > m_boolFields;

    /**
     * @brief m_onering
     * sorted indices of vertices that are in the onering
     */
    std::vector< echo::CircularVector<unsigned int> > m_onering;
    /**
     * @brief m_oneringcell
     * sorted indices of cells that are in the one ring
     */
    std::vector< echo::CircularVector<unsigned int> > m_oneringcell;

    /**
     * @brief m_oneringidx
     * For each cell in m_oneringcell, this contains the position of the 'a' angle
     */
    std::vector< echo::CircularVector<unsigned int> > m_oneringidx;

    /**
     * @brief recalculateOneRing
     * Recalculates the arrays one ring and one ring cells
     */
    void recalculateOneRing();

    template<typename mypair>
    static bool pair_comparator ( const mypair& l, const mypair& r)
    { return l.first < r.first; }

    /// Could do other types such as int, point (vector), etc

};



template<class CellType, unsigned int InputDimension>
Mesh<CellType, InputDimension>::Mesh(const typename Mesh<CellType, InputDimension>::Pointer other){

    /// Copy the points
    this->m_points.resize(other->GetNumberOfNodes());
    this->m_onering.resize(this->m_points.size());
    this->m_oneringcell.resize(this->m_points.size());
    this->m_oneringidx.resize(this->m_points.size());
    typename std::vector< PointType>::const_iterator cit_points;
    typename std::vector< PointType>::iterator it_points;
    for( cit_points = other->m_points.begin(), it_points = this->m_points.begin(); cit_points!= other->m_points.end(); ++cit_points, ++it_points){
        *it_points = *cit_points;
    }

    /// Copy the topology and create the cell list
    this->m_topology.resize(other->m_topology.size());
    this->m_cells.resize(other->m_topology.size());
    typename std::vector< CellTopologyType>::const_iterator cit_topology;
    typename std::vector< CellTopologyType>::iterator it_topology;
    typename std::vector< CellType>::iterator it_cells;
    for( cit_topology = other->m_topology.begin(), it_topology = this->m_topology.begin(), it_cells = this->m_cells.begin();
         cit_topology != other->m_topology.end(); ++cit_topology, ++it_topology, ++it_cells){
        *it_topology = *cit_topology;

        CellType triangle;
        triangle.insertPoint(0, this->m_points[(*it_topology)[0]]);
        triangle.insertPoint(1, this->m_points[(*it_topology)[1]]);
        triangle.insertPoint(2, this->m_points[(*it_topology)[2]]);
        *it_cells = triangle;
    }

    this->m_onering.resize(this->m_points.size());
    this->m_oneringcell.resize(this->m_points.size());
    this->m_oneringidx.resize(this->m_points.size());

    /// Copy double fields
    for (int i=0; i<other->GetNumberOfDoubleFields(); i++){
        MeshDoubleFieldType v = other->GetDoubleField(i);
        this->addDoubleField(v);
    }

}


template<class CellType, unsigned int InputDimension>
Mesh<CellType, InputDimension>::Mesh(const std::vector< CellTopologyType > & topology, const std::vector<PointType> & points){

    /// Copy the points
    this->m_points.resize(points.size());
    this->m_onering.resize(this->m_points.size());
    this->m_oneringcell.resize(this->m_points.size());
    this->m_oneringidx.resize(this->m_points.size());
    typename std::vector< PointType>::const_iterator cit_points;
    typename std::vector< PointType>::iterator it_points;
    for( cit_points = points.begin(), it_points = this->m_points.begin(); cit_points!= points.end(); ++cit_points, ++it_points){
        *it_points = *cit_points;
    }

    /// Copy the topology and create the cell list
    this->m_topology.resize(topology.size());
    this->m_cells.resize(topology.size());
    typename std::vector< CellTopologyType>::const_iterator cit_topology;
    typename std::vector< CellTopologyType>::iterator it_topology;
    typename std::vector< CellType>::iterator it_cells;
    for( cit_topology = topology.begin(), it_topology = this->m_topology.begin(), it_cells = this->m_cells.begin();
         cit_topology != topology.end(); ++cit_topology, ++it_topology, ++it_cells){
        *it_topology = *cit_topology;

        CellType triangle;
        triangle.insertPoint(0, this->m_points[(*it_topology)[0]]);
        triangle.insertPoint(1, this->m_points[(*it_topology)[1]]);
        triangle.insertPoint(2, this->m_points[(*it_topology)[2]]);
        *it_cells = triangle;
    }
}

template<class CellType, unsigned int InputDimension>
void Mesh<CellType, InputDimension>::addDoubleField(const std::vector< double > & data, std::string & name){

    MeshDoubleFieldType field;
    typename std::vector<double>::const_iterator cit;
    field.data.reserve(data.size());
    for (cit = data.begin(); cit != data.end(); ++cit){
        field.data.push_back(*cit);
    }
    field.name = name;

    this->m_fields.push_back(field);
}

template<class CellType, unsigned int InputDimension>
void Mesh<CellType, InputDimension>::addDoubleField(const MeshDoubleFieldType & f){

    MeshDoubleFieldType field;
    typename std::vector<double>::const_iterator cit;
    field.data.reserve(f.data.size());
    for (cit = f.data.begin(); cit != f.data.end(); ++cit){
        field.data.push_back(*cit);
    }
    field.name = f.name;

    this->m_doubleFields.push_back(field);
}

template<class CellType, unsigned int InputDimension>
void Mesh<CellType, InputDimension>::computeAngles(){
    typename std::vector< CellType>::iterator it_cells;
    for( it_cells = this->m_cells.begin(); it_cells != this->m_cells.end();  ++it_cells){
        it_cells->computeAngles();
    }
}

template<class CellType, unsigned int InputDimension>
void Mesh<CellType, InputDimension>::computeAreas(){
    typename std::vector< CellType>::iterator it_cells;
    for( it_cells = this->m_cells.begin(); it_cells != this->m_cells.end();  ++it_cells){
        it_cells->computeArea();
    }
}



template<class CellType, unsigned int InputDimension>
std::vector<unsigned int> Mesh<CellType, InputDimension>::Get1Ring(unsigned int i){

    if (!this->m_onering[i].size()){
        /// recalculate al the 1-ring
        this->recalculateOneRing();
    }

    return this->m_onering[i];


}

template<class CellType, unsigned int InputDimension>
typename Mesh<CellType, InputDimension>::OneRingCellType Mesh<CellType, InputDimension>::Get1RingCells(unsigned int i){

    if (!this->m_onering[i].size()){
        /// recalculate al the 1-ring
        this->recalculateOneRing();
    }

    OneRingCellType orc;

    orc.cells =  this->m_oneringcell[i];
    orc.index=  this->m_oneringidx[i];
    return orc;


}

template<class CellType, unsigned int InputDimension>
void Mesh<CellType, InputDimension>::recalculateOneRing(){

    /// All neighbours will store:
    /// [0] the first neighbour
    /// [1] the second neighbour clockwise
    /// [2] the triangle id
    /// [4] The index (starting at zero) of the vertex at the center of the 1-ring
    typedef echo::Point<unsigned int, 4> NeighbourTupleType;
    std::vector< std::vector<  NeighbourTupleType>  > all_neighbours(this->m_points.size());

    /// Calculate all 1-neighbours of each vertex

    typename std::vector<CellTopologyType>::const_iterator current_cell;
    for (current_cell = this->m_topology.begin(); current_cell!=this->m_topology.end(); ++current_cell){
        unsigned int current_cell_idx = current_cell - this->m_topology.begin(); /// Index of the current triangle
        /// Iterate over all the vertices of the current triangle
        for (int i=0; i<current_cell->size(); i++){ /// i is the index of the current vertex
            unsigned int next_index = current_cell->next_index(i); /// next_index is the index to the first neighbour
            unsigned int next_index2 = current_cell->next_index(next_index); /// next_index2 is the index to the second neighbour, clockwise
            NeighbourTupleType p;
            p[0] = (*current_cell)[next_index];
            p[1] = (*current_cell)[next_index2];
            p[2] = current_cell_idx;
            p[3] = i;
            all_neighbours[(*current_cell)[i]].push_back(p);
        }
    }

    /// Now sort the neighbours counter clockwise and remove duplicates
    std::vector< std::vector<NeighbourTupleType> >::iterator neighbours_iterator;
    std::vector< echo::CircularVector<unsigned int> >::iterator onering_iterator;
    std::vector< echo::CircularVector<unsigned int> >::iterator oneringcell_iterator;
    std::vector< echo::CircularVector<unsigned int> >::iterator oneringidx_iterator ;


    MeshBoolFieldType borderField("border");
    borderField.data.reserve(this->m_points.size());

    for (neighbours_iterator = all_neighbours.begin(),
         onering_iterator = this->m_onering.begin(),
         oneringcell_iterator = this->m_oneringcell.begin(),
         oneringidx_iterator = this->m_oneringidx.begin();
         neighbours_iterator!= all_neighbours.end();
         ++neighbours_iterator, ++onering_iterator, ++oneringcell_iterator, ++oneringidx_iterator){
        //unsigned int current_vertex = neighbours_iterator-all_neighbours.begin(); // This is the current vertex
        unsigned int first_vertex, last_vertex; /// First and last sorted neighbours, to allow for bi-directional insertion
        /// I think the following should rather be a while loop like:
        ///     while (there are neighbours in the list that have not been added)
        /// and at each iteration if the neighbour is added, then remove it from the list, if it is not added then put it at the end of the list
        //NeighbourTupleType unsorted_neighbour_vector = *neighbours_iterator;
        while (neighbours_iterator->size()){
            unsigned int neighbour = (*neighbours_iterator)[0][0]; // This is the current neighbour of current_vertex
            if (!onering_iterator->size()){
                onering_iterator->push_back(neighbour);/// Add arbitrarily the first node
                unsigned int neighbour2 = (*neighbours_iterator)[0][1]; // This is the second neighbour
                onering_iterator->push_back(neighbour2);
                first_vertex = neighbour;
                last_vertex = neighbour2;
                /// Now the list of cells
                unsigned int cellidx = (*neighbours_iterator)[0][2];
                oneringcell_iterator->push_back(cellidx); /// push back the cell number
                unsigned int index_of_angle_a = (*neighbours_iterator)[0][3];
                oneringidx_iterator->push_back(index_of_angle_a);

            }  else {
                unsigned int neighbour2 = (*neighbours_iterator)[0][1]; // This is the second neighbour
                unsigned int cellidx = (*neighbours_iterator)[0][2];
                unsigned int index_of_angle_a = (*neighbours_iterator)[0][3];
                if ( neighbour == last_vertex ){ /// Inser at the end
                    ///onering_iterator->push_back(neighbour); // not necesasry because is the same as the last vertex
                    onering_iterator->push_back(neighbour2);
                    /// first_vertex = neighbour; I have added at the end so the first vertex does not change
                    last_vertex = neighbour2;
                    oneringcell_iterator->push_back(cellidx); /// push back the cell number
                    oneringidx_iterator->push_back(index_of_angle_a);
                } else if ( neighbour2 == first_vertex ){ /// insert at the begining
                    onering_iterator->insert(onering_iterator->begin(), neighbour);
                    ///onering_iterator->insert(onering_iterator->begin()+1,neighbour2); // not necesasry because is the same as the first vertex
                    first_vertex = neighbour;
                    ///last_vertex = neighbour2; I have added at the begining so the last vertex does not change
                    oneringcell_iterator->insert(oneringcell_iterator->begin(), cellidx); /// push back the cell number
                    oneringidx_iterator->insert(oneringidx_iterator->begin(),index_of_angle_a);
                } else { /// this current neighbour will come later -move it at the end of the unsorted neighbour list
                    neighbours_iterator->push_back((*neighbours_iterator)[0]);
                }
            }
            neighbours_iterator->erase(neighbours_iterator->begin());/// And delete it from the beginning of the list
        }
        /// Now it is a good moment to check if the current vertex belongs to a borderr or not!
        if (first_vertex==last_vertex){
            borderField.data.push_back(false);
            /// remove the last neighbour
            onering_iterator->pop_back();
        } else {
            borderField.data.push_back(true);
        }
    }
    this->m_boolFields.push_back(borderField);
}


template<class CellType, unsigned int InputDimension>
vtkSmartPointer<vtkPolyData> Mesh<CellType, InputDimension>::toVTKMesh(){

    vtkSmartPointer<vtkPolyData> gr = vtkSmartPointer<vtkPolyData> ::New();
    vtkSmartPointer<vtkPoints> points= vtkSmartPointer<vtkPoints>::New();
    switch (InputDimension)
    {
    case 2:
    {

        for (int i=0; i<this->m_points.size(); i++){
            points->InsertNextPoint(this->m_points[i][0],this->m_points[i][1],0.0 );
        }
    }
        break;
    case 3:
    {
        for (int i=0; i<this->m_points.size(); i++){
            points->InsertNextPoint(&(this->m_points[i][0]));
        }

    }
        break;
    default:
    {
        std::cerr<< "ERROR  in Mesh::toVTKMesh() : only 2D or 3D meshes are accepted "<<std::endl;
    }
    }

    // Create a triangles
    vtkSmartPointer<vtkCellArray> triangles = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkTriangle> triangle = vtkSmartPointer<vtkTriangle>::New();
    for (int i=0; i<this->m_topology.size(); i++){
        for (int j=0; j<NVERTICES; j++){
            triangle->GetPointIds()->SetId(j,this->m_topology[i][j]);
        }
        triangles->InsertNextCell(triangle);
    }

    /// TODO add scalars

    gr->SetPoints(points);
    gr->SetPolys(triangles);

    return gr;

}

template<class CellType, unsigned int InputDimension>
std::vector<typename Mesh<CellType, InputDimension>::BarycentricCoordinatesType>
Mesh<CellType, InputDimension>::GetBarycentricCoordinates(const std::vector<PointType> & points){

    std::vector<BarycentricCoordinatesType> bc(points.size());

    typename std::vector<PointType>::const_iterator cit;
    typename std::vector<BarycentricCoordinatesType>::iterator it;

    for (cit = points.begin(), it = bc.begin(); cit!=points.end(); ++cit, ++it){
        *it = this->GetBarycentricCoordinates(*cit);
    }
    return bc;

}

template<class CellType, unsigned int InputDimension>
typename Mesh<CellType, InputDimension>::BarycentricCoordinatesType
Mesh<CellType, InputDimension>::GetBarycentricCoordinates(const PointType & point){
    /// Calculate in which triangle this point lies
    BarycentricCoordinatesType bc;
    bc.triangle_index = this->inWhichCell(point);
    if (bc.triangle_index<0){
        /// Point is outside of the convex hull
        bc.coordinates = 0.0;
        bc.triangle_index = 0;
    } else {
        bc.coordinates= this->m_cells[bc.triangle_index].barycentricCoordinates(point);
    }

    return bc;
}

template<class CellType, unsigned int InputDimension>
std::vector<typename Mesh<CellType, InputDimension>::PointType>
Mesh<CellType, InputDimension>::GetPosition(const std::vector<BarycentricCoordinatesType> & bc){

    std::vector<PointType> p(bc.size());
    typename std::vector<BarycentricCoordinatesType>::const_iterator cit;
    typename std::vector<PointType>::iterator it;

    for (it = p.begin(), cit = bc.begin(); cit!=bc.end(); ++cit, ++it){
        *it = this->GetPosition(*cit);
    }
    return p;
}

template<class CellType, unsigned int InputDimension>
typename Mesh<CellType, InputDimension>::PointType
Mesh<CellType, InputDimension>::GetPosition(const BarycentricCoordinatesType & bc){
    PointType p;
    CellType cell = this->m_cells[bc.triangle_index];
    for (int i=0;i<CellType::NVERTICES; i++){
        PointType v = *(cell.GetVertex(i));
        p+=v*bc.coordinates[i];
    }
    return p;
}

template<class CellType, unsigned int InputDimension>
int Mesh<CellType, InputDimension>::inWhichCell(const PointType & p){

    typename std::vector<CellType>::iterator cit;
    for (cit = this->m_cells.begin(); cit != this->m_cells.end(); ++cit){
        bool isInTriangle = cit->contains(p);
        if (isInTriangle){
            return cit-this->m_cells.begin();
        }
    }

    return -1;

}

template<class CellType, unsigned int InputDimension>
void Mesh<CellType, InputDimension>::SetPoints(const std::vector<PointType> & points){
    /// Copy the points
    this->m_points.resize(points.size());
    typename std::vector< PointType>::const_iterator cit_points;
    typename std::vector< PointType>::iterator it_points;
    for( cit_points = points.begin(), it_points = this->m_points.begin(); cit_points!= points.end(); ++cit_points, ++it_points){
        *it_points = *cit_points;
    }

    /// Update the cell list
    typename std::vector< CellTopologyType>::const_iterator cit_topology;
    typename std::vector< CellType>::iterator it_cells;
    for( cit_topology = this->m_topology.begin(), it_cells = this->m_cells.begin();
         cit_topology != this->m_topology.end(); ++cit_topology, ++it_cells){

        CellType triangle;
        triangle.insertPoint(0, this->m_points[(*cit_topology)[0]]);
        triangle.insertPoint(1, this->m_points[(*cit_topology)[1]]);
        triangle.insertPoint(2, this->m_points[(*cit_topology)[2]]);
        *it_cells = triangle;
    }
}

template<class CellType, unsigned int InputDimension>
void Mesh<CellType, InputDimension>::SetTopology(const std::vector< CellTopologyType > & topology){

    /// Copy the topology and create the cell list
    this->m_topology.resize(topology.size());
    this->m_cells.resize(topology.size());
    typename std::vector< CellTopologyType>::const_iterator cit_topology;
    typename std::vector< CellTopologyType>::iterator it_topology;
    typename std::vector< CellType>::iterator it_cells;
    for( cit_topology = topology.begin(), it_topology = this->m_topology.begin(), it_cells = this->m_cells.begin();
         cit_topology != topology.end(); ++cit_topology, ++it_topology, ++it_cells){
        *it_topology = *cit_topology;

        CellType triangle;
        triangle.insertPoint(0, this->m_points[(*it_topology)[0]]);
        triangle.insertPoint(1, this->m_points[(*it_topology)[1]]);
        triangle.insertPoint(2, this->m_points[(*it_topology)[2]]);
        *it_cells = triangle;
    }
}

template<class CellType, unsigned int InputDimension>
int   Mesh<CellType, InputDimension>::writeToFile(std::string &filename){

    /// See what the extension is

    std::vector<std::string > tokens =  echo::str::Tokenize(filename, std::string("."));
    std::string extension = tokens[tokens.size()-1];

    if (!strcmp(extension.c_str(),"vtk")){
       // std::cout << "Write as a vtk file"<<std::endl;
        vtkSmartPointer<vtkPolyData> vtkGrid = this->toVTKMesh();
        vtkSmartPointer<vtkPolyDataWriter> writer =  vtkSmartPointer<vtkPolyDataWriter>::New();
        writer->SetInputData(vtkGrid);
        writer->SetFileName(filename.c_str());
        writer->Write();
    }
}

}
///----------------------------------------------------------------------------------------------------///
/// End of class Mesh ---------------------------------------------------------------------------------///
///----------------------------------------------------------------------------------------------------///

inline vtkSmartPointer<vtkPolyData> transformMesh(vtkSmartPointer<vtkPolyData> inputMesh,     vtkSmartPointer<vtkMatrix4x4> matrix,  bool debug=false, bool inverse=false);
inline void writeMesh(vtkSmartPointer<vtkPointSet> mesh, std::string &filename);

inline void readMeshesFile(const std::string filename, std::vector<std::string> & mesh_files, std::vector<double> & timestamps);

/****************************************************************
*                          IMPLEMENTATION                       *
****************************************************************/

template <class ValueType>
void polyDataToScalarValuesList(const vtkSmartPointer<vtkPointSet> &polydata, const std::string &arrayName, std::vector<ValueType> &point_list){

    vtkSmartPointer<vtkDataArray> values =  polydata->GetPointData()->GetScalars(arrayName.c_str());
    int npoints = values->GetNumberOfTuples();
    point_list.resize(npoints);
    for (int i=0; i<npoints; i++)
    {
        point_list[i] = values->GetTuple(i)[0];
    }
}


template <class PointType>
void polyDataToPointList(const vtkSmartPointer<vtkPointSet> &polydata, std::vector<PointType> &point_list, double t_position=0.0, double coordinates_factor =1.0)
{
    vtkSmartPointer<vtkPoints> points =   vtkSmartPointer<vtkPoints>::New();
    points = polydata->GetPoints();
    int npoints = points->GetNumberOfPoints();
    point_list.resize(npoints);
    double pt[4]; /// vtkPoints are always 3D points

    PointType tmp;
    unsigned int ndims = tmp.GetDimensions();

    for (int i=0; i<npoints; i++)
    {
        points->GetPoint(i,&pt[0]);
        pt[0]*=coordinates_factor;
        pt[1]*=coordinates_factor;
        pt[2]*=coordinates_factor;
        if ( ndims==4){
            pt[3]=t_position;
        }
        PointType pt_(pt);
        point_list[i]=pt_;

    }

}


void writeMesh(vtkSmartPointer<vtkPointSet> mesh, std::string &filename){

    vtkSmartPointer<vtkUnstructuredGridWriter> writer =  vtkSmartPointer<vtkUnstructuredGridWriter>::New();
    writer->SetInputData(mesh);
    writer->SetFileName(filename.c_str());
    writer->Write();
}

vtkSmartPointer<vtkPolyData> transformMesh(vtkSmartPointer<vtkPolyData> inputMesh,     vtkSmartPointer<vtkMatrix4x4> matrix,  bool debug, bool inverse)
{

    vtkSmartPointer<vtkPointSet> pointset   = vtkPointSet::SafeDownCast(inputMesh);
    vtkSmartPointer<vtkPoints> inputpoints  = pointset->GetPoints();
    vtkSmartPointer<vtkPoints> outputpoints = vtkSmartPointer<vtkPoints>::New();

    vtkSmartPointer<vtkMatrix4x4> vtkmatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    vtkmatrix->DeepCopy(matrix);

    if (!inverse)
        vtkmatrix->Invert();

    if (debug) std::cout << "Matrix "<<*vtkmatrix << std::endl;

    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->SetMatrix (vtkmatrix);

    transform->TransformPoints (inputpoints, outputpoints);
    pointset->SetPoints (outputpoints);

    vtkSmartPointer<vtkPolyData> outputMesh = vtkPolyData::SafeDownCast(pointset);

    return outputMesh;

}

void readMeshesFile(const std::string filename, std::vector<std::string> & mesh_files, std::vector<double> & timestamps){

    /// count the lines
    std::ifstream inFile(filename.c_str());
    unsigned int nlines = std::count(std::istreambuf_iterator<char>(inFile), std::istreambuf_iterator<char>(), '\n');
    if (nlines==0){
        std::cerr << "File "<< filename <<" does not exist or is empty"<<std::endl;
        return;
    }

    std::fstream filereader;
    filereader.open(filename.c_str(), std::fstream::in);
    mesh_files.resize(nlines);
    timestamps.resize(nlines);
    for (int i=0; i<nlines; i++){
        filereader >> timestamps[i] >> mesh_files[i];
    }
    filereader.close();
}

template <class ITKMeshType>
typename ITKMeshType::Pointer ITKMeshFromVTKPolyData(vtkPolyData* grid)
{
    /// Create a new mesh
    typename ITKMeshType::Pointer mesh(ITKMeshType::New());
    /// Get the points from vtk
    vtkPoints* vtkpoints = grid->GetPoints();
    int numPoints = vtkpoints->GetNumberOfPoints();
    /// Create a compatible point container for the mesh
    /// the mesh is created with a null points container
    typename ITKMeshType::PointsContainer::Pointer points =ITKMeshType::PointsContainer::New();
    /// Resize the point container to be able to fit the vtk points
    points->Reserve(numPoints);
    /// Set the point container on the mesh
    mesh->SetPoints(points);
    for(int i =0; i < numPoints; i++)
    {
        double* apoint = vtkpoints->GetPoint(i);
        mesh->SetPoint(i, typename ITKMeshType::PointType(apoint));
    }

    typename ITKMeshType::CellsContainerPointer cells =  ITKMeshType::CellsContainer::New();
    mesh->SetCells(cells);
    /// extract the cell id's from the vtkPolyData
    int numcells = grid->GetNumberOfCells();
    int* vtkCellTypes = new int[numcells];
    int cellId =0;
    for(; cellId < numcells; cellId++)
    {
        vtkCellTypes[cellId] = grid->GetCellType(cellId);
    }

    cells->Reserve(numcells);
    vtkIdType npts;
    vtkIdType* pts;
    cellId = 0;
    vtkCellArray* vtkcells = grid->GetPolys();
    for(vtkcells->InitTraversal(); vtkcells->GetNextCell(npts, pts); cellId++)
        //  for(; cellId<numcells; cellId++)
    {
        typename ITKMeshType::CellAutoPointer c;
        switch(vtkCellTypes[cellId])
        {
        case VTK_TRIANGLE:
        {
            typedef itk::CellInterface<double, typename ITKMeshType::CellTraits> CellInterfaceType;
            typedef itk::TriangleCell<CellInterfaceType> TriangleCellType;
            TriangleCellType * t = new TriangleCellType;
            typename TriangleCellType::PointIdentifier itkPts[3];
            for (int ii = 0; ii < npts; ++ii)
            {
                itkPts[ii] = static_cast<typename TriangleCellType::PointIdentifier>(pts[ii]);
            }
            t->SetPointIds(itkPts);
            c.TakeOwnership( t );
            break;
        }
        case VTK_QUAD:
        {
            typedef itk::CellInterface<double, typename ITKMeshType::CellTraits> CellInterfaceType;
            typedef itk::QuadrilateralCell<CellInterfaceType> QuadrilateralCellType;
            QuadrilateralCellType * t = new QuadrilateralCellType;
            typename QuadrilateralCellType::PointIdentifier itkPts[3];
            for (int ii = 0; ii < npts; ++ii)
            {
                itkPts[ii] = static_cast<typename QuadrilateralCellType::PointIdentifier>(pts[ii]);
            }
            t->SetPointIds(itkPts);
            c.TakeOwnership( t );
            break;
        }
        case VTK_EMPTY_CELL:
        case VTK_VERTEX:
        case VTK_POLY_VERTEX:
        case VTK_LINE:
        case VTK_POLY_LINE:
        case VTK_TRIANGLE_STRIP:
        case VTK_POLYGON:
        case VTK_PIXEL:
        case VTK_TETRA:
        case VTK_VOXEL:
        case VTK_HEXAHEDRON:
        case VTK_WEDGE:
        case VTK_PYRAMID:
        case VTK_PARAMETRIC_CURVE:
        case VTK_PARAMETRIC_SURFACE:
        default:
            std::cerr << "Warning unhandled cell type "
                      << vtkCellTypes[cellId] << std::endl;
            ;
        }
        mesh->SetCell(cellId, c);
    }

    // mesh->Print(std::cout);
    return mesh;
}


} /// end namespace echo

#endif ///  MESHTOOLS_H_
