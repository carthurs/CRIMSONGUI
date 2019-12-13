#pragma once

#include <mitkDataNode.h>
#include <AsyncTaskManager.h>
#include <VesselPathAbstractData.h>
//#include <FaceData.h>
#include <SolidData.h>
#include <FaceIdentifier.h>

#include <mitkDataStorage.h>
#include <mitkBaseRenderer.h>
#include <mitkRenderingManager.h>

namespace crimson {

/*! \brief   A collection of useful utility functions. */
class PCMRIUtils {
public:
    /*! \name Functions generating consistent async task UIDs for common operations. */
    ///@{ 
    static crimson::AsyncTaskManager::TaskUID getMappingTaskUID(mitk::DataNode* BCNode);
    ///@} 

    /*!
     * \brief   Get all the contour nodes belonging to a particular PCMRI BC node.
     */
	static mitk::DataStorage::SetOfObjects::ConstPointer getContourNodes(mitk::DataNode* bcNode);

	/*!
	* \brief   Get the MRA point node belonging to a particular PCMRI BC node.
	*/
	static mitk::DataNode::Pointer getMraPointNode(mitk::DataNode* bcNode);

	/*!
	* \brief   Get all the PCMRI point nodes belonging to a particular PCMRI BC node.
	*/
	static mitk::DataNode::Pointer getPcmriPointNode(mitk::DataNode* bcNode);

    /*!
     * \brief   Get all the contour nodes belonging to a particular PCMRI BC node sorted by their
     *  parameter value.
     */
	static std::vector<mitk::DataNode*> getContourNodesSortedByParameter(mitk::DataNode* bcNode);

    /*!
     * \brief   Get the center of planar figure's geometry.
     */
    static mitk::Point3D getPlanarFigureGeometryCenter(const mitk::DataNode* node);


	/*!
	* \brief   Get the plane geometry for the MRA reslice window based on selected faces.
	*/
	static mitk::PlaneGeometry::Pointer getMRAPlane(mitk::DataNode* BCNode, mitk::DataNode* mraNode, mitk::DataNode* solidNode, FaceIdentifier selectedFace);

	/*!
	* \brief   Compute and assign the planar figure parameter value.
	*/
	static mitk::ScalarType assignPlanarFigureParameter(mitk::DataNode* BCNode, mitk::DataNode* figureNode, mitk::BaseRenderer*  manager);

    /*!
     * \brief   Assign default planar figure node properties for contour.
     */
    static void setDefaultContourNodeProperties(mitk::DataNode* contourNode, bool selected);

	/*!
	* \brief   Assign default point node properties for corresponding points.
	*/
	static void setDefaultPointNodeProperties(mitk::DataNode* pointNode, bool selected, float* color, mitk::BaseRenderer* renderer);

    /*!
     * \brief   Update the parameter values for all the contours. 
     *
     * \param   bcNode					The PCMRI BC node.
     * \param   nodeToIgnore            A plar figure node whose parameter value should not be updated (usually the node being deleted).
     * \param   forceUpdate             If false, only the nodes with invalid parameter values will be updated.
     *
     * \return  A map from parameter value to planar figure node pointer.
     */
	static std::map<float, mitk::DataNode*> updatePlanarFigureParameters(mitk::DataNode* bcNode, const mitk::DataNode* nodeToIgnore, mitk::BaseRenderer*  manager, bool forceUpdate = false);

    /*! \name Functions to find various types of nodes */
    ///@{ 
    /*! \brief   Gets the solid node lofted from a vessel path. */
	//static mitk::DataNode* getBCSolidModelNode(mitk::DataNode* bcNode);
    ///@} 

    /*!
     * \brief   Get reslice geometry initialization parameters.
     *
     * \return  The reslice geometry parameters : Parametric delta, Reference image spacing, Time steps.
     */
    static std::tuple<double, mitk::Vector3D, unsigned int> getResliceGeometryParameters(mitk::DataNode::Pointer imageNode);
};

}