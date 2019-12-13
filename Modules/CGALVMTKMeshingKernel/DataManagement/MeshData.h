#pragma once

#include <array>

#include <mitkBaseData.h>
#include <mitkSurface.h>
#include <mitkUnstructuredGrid.h>

#include "CGALVMTKMeshingKernelExports.h"

#include <FaceIdentifier.h>

namespace crimson
{

class MeshDataIO;

/*! \brief   MeshData represents a simulation mesh stored as a  mesh. */
class CGALVMTKMeshingKernel_EXPORT MeshData : public mitk::BaseData
{
public:
    mitkClassMacro(MeshData, BaseData);
    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);
    mitkCloneMacro(Self);

public:
    ////////////////////////////////////////////////////////////
    // mitk::BaseData interface implementation
    ////////////////////////////////////////////////////////////
    void UpdateOutputInformation() override;
    void SetRequestedRegionToLargestPossibleRegion() override {}
    bool RequestedRegionIsOutsideOfTheBufferedRegion() override { return false; }
    bool VerifyRequestedRegion() override { return true; }
    void SetRequestedRegion(const itk::DataObject*) override {}
    void PrintSelf(std::ostream& os, itk::Indent indent) const override;

    ///@{ 
    /*!
     * \brief   Sets the FaceIdentifierMap (see SolidData and FaceIdentifierMap documentation for
     *  details)
     */
    void setFaceIdentifierMap(FaceIdentifierMap faceIdMap) { _faceIdentifierMap = std::move(faceIdMap); }

    /*!
     * \brief   Get the FaceIdentifierMap (see SolidData and FaceIdentifierMap documentation for
     *  details)
     */
    const FaceIdentifierMap& getFaceIdentifierMap() const { return _faceIdentifierMap; }
    ///@} 

    ///@{ 
    /*!
     * \brief   Gets the representation of the surface of simulation mesh as a mitk::Surface.
     */
    mitk::Surface::Pointer getSurfaceRepresentation() const;

	/*!
	* \brief   Gets the representation of the surface of simulation mesh face defined by FaceIdentifier as a mitk::Surface.
	*/
	mitk::Surface::Pointer getSurfaceRepresentationForFace(crimson::FaceIdentifier face) const;

    /*!
     * \brief   Gets the representation of the simulation mesh as a mitk::UnstructuredGrid.
     */
    mitk::UnstructuredGrid::Pointer getUnstructuredGridRepresentation() const;

    void setUnstructuredGrid(mitk::UnstructuredGrid::Pointer data, bool fixOrdering);

    /*!
     * \brief   Gets the storage for the data associated with the simulation mesh (e.g. velocity and
     *  pressure) as vtkPointData.
     */
    vtkPointData* getPointData() const;
    ///@} 

    /*!
     * \brief   Gets the aspect ratios of all the elements of the mesh.
     */
    std::vector<double> getElementAspectRatios();

    //////////////////////////////////////////////////////////////////////////

    ///@{ 
    /*!
     * \brief   Gets number of nodes.
     */
    int getNNodes() const;

    /*!
     * \brief   Gets number of edges.
     */
    int getNEdges() const;

    /*!
     * \brief   Gets number of element faces.
     */
    int getNFaces() const;

    /*!
     * \brief   Gets number of elements.
     */
    int getNElements() const;

    /*!
     * \brief   Gets coordinates of the node with index nodeIndex.
     */
    mitk::Point3D getNodeCoordinates(int nodeIndex) const;

    /*!
     * \brief   Gets node id's of the element with index elementIndex.
     */
    std::vector<int> getElementNodeIds(int elementIndex) const;

    /*!
     * \brief   Gets element id's of the elements sharing a face with element with index elementIndex.
     */
    std::vector<int> getAdjacentElements(int elementIndex) const;

	/*!
	* \brief   Gets the node id's for a model face with face identifier faceId.
	*/
	std::vector<int> getBoundaryNodeIdsForFace(const FaceIdentifier& faceId) const;

    /*!
     * \brief   Gets the node id's for a model face with face identifier faceId.
     */
    std::vector<int> getNodeIdsForFace(const FaceIdentifier& faceId) const;

    /*!
     * \brief   A structure containing information about a simulation mesh face. 
     */
    struct MeshFaceInfo {
        int elementId;  ///< The element index the face belongs to
        int globalFaceId;   ///< Index of the face in mesh
        int nodeIds[3]; ///< The indices of the nodes belonging to the face.
    };

    /*!
     * \brief   Gets MeshFaceInfo for all mesh faces belnging to the model face with face identifier
     *  faceId.
     */
    std::vector<MeshFaceInfo> getMeshFaceInfoForFace(const FaceIdentifier& faceId) const;

	/*!
	* \brief   Calculates the area of a face with face identifier faceId by calculating the area
	*   of each mesh face belonging to that model face
	*/
	double calculateArea(const FaceIdentifier& faceId);
    ///@} 


protected:
    MeshData();
    virtual ~MeshData();

    MeshData(const Self& other);

private:
    friend class MeshDataIO;

    FaceIdentifierMap _faceIdentifierMap;

    mutable mitk::Surface::Pointer _surfaceRepresentation = nullptr;
    mitk::UnstructuredGrid::Pointer _unstructuredGridRepresentation = nullptr;
    int _firstTriangleCellId;
    int _nFaces;
    int _nEdges;
};

} // namespace crimson
