#pragma once

#include <mitkDataNode.h>
#include <AsyncTaskManager.h>
#include <VesselPathAbstractData.h>

#include <mitkDataStorage.h>

namespace crimson {

/*! \brief   A collection of useful utility functions. */
class VascularModelingUtils {
public:
    /*! \name Functions generating consistent async task UIDs for common operations. */
    ///@{ 
    static crimson::AsyncTaskManager::TaskUID getLoftingTaskUID(mitk::DataNode* vesselPathNode);
    static crimson::AsyncTaskManager::TaskUID getPreviewLoftingTaskUID(mitk::DataNode* vesselPathNode);
    static crimson::AsyncTaskManager::TaskUID getBlendingTaskUID(mitk::DataNode* vesselForestNode);
    ///@} 

    /*!
     * \brief   Get all the contour nodes belonging to a particular vessel path.
     */
    static mitk::DataStorage::SetOfObjects::ConstPointer getVesselContourNodes(mitk::DataNode* vesselPathNode);

    /*!
     * \brief   Get all the contour nodes belonging to a particular vessel path sorted by their
     *  parameter value.
     */
    static std::vector<mitk::DataNode*> getVesselContourNodesSortedByParameter(mitk::DataNode* vesselPathNode);

    /*!
     * \brief   Get the center of planar figure's geometry.
     */
    static mitk::Point3D getPlanarFigureGeometryCenter(const mitk::DataNode* node);

    /*!
     * \brief   Compute and assign the planar figure parameter value.
     */
    static mitk::ScalarType assignPlanarFigureParameter(mitk::DataNode* vesselPathNode, mitk::DataNode* figureNode);

    /*!
     * \brief   Assign default planar figure node properties for contour.
     */
    static void setDefaultContourNodeProperties(mitk::DataNode* contourNode, bool selected);

    /*!
     * \brief   Update the parameter values for all the contours. 
     *
     * \param   vesselPathNode          The vessel path node.
     * \param   nodeToIgnore            A plar figure node whose parameter value should not be updated (usually the node being deleted).
     * \param   forceUpdate             If false, only the nodes with invalid parameter values will be updated.
     *
     * \return  A map from parameter value to planar figure node pointer.
     */
    static std::map<float, mitk::DataNode*> updatePlanarFigureParameters(mitk::DataNode* vesselPathNode, const mitk::DataNode* nodeToIgnore, bool forceUpdate = false);

    /*! \name Functions to find various types of nodes */
    ///@{ 
    /*! \brief   Finds the vessel path node by the VesselPathAbstractData it contains. */
    static mitk::DataNode* getVesselPathNodeByData(VesselPathAbstractData* vessel);

     /*! \brief   Finds the vessel path node in a vessel tree by UID. */
    static mitk::DataNode* getVesselPathNodeByUID(mitk::DataNode* vesselForestNode, const VesselPathAbstractData::VesselPathUIDType& vessel);

    /*! \brief   Gets the solid node lofted from a vessel path. */
    static mitk::DataNode* getVesselSolidModelNode(mitk::DataNode* vesselNode);
    ///@} 

    /*!
     * \brief   Get reslice geometry initialization parameters.
     *
     * \return  The reslice geometry parameters : Parametric delta, Reference image spacing, Time steps.
     */
    static std::tuple<double, mitk::Vector3D, unsigned int> getResliceGeometryParameters(mitk::DataNode* vesselNode);
};

}