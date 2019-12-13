#pragma once

#include <mitkDataNode.h>
#include <MeshingParametersData.h>
#include <AsyncTaskManager.h>

namespace crimson {

/*! \brief   A collection of useful functions for meshing. */
class MeshingUtils {
public:

    /*!
     * \brief   Finds the meshing parameters for a solid in the hierarchy. Creates the node if it
     *  doesn't exist.
     */
    static MeshingParametersData* getMeshingParametersForSolid(mitk::DataNode* solidNode);

    /*!
     * \brief   Generates an async task UIDs for meshing operation.
     */
    static crimson::AsyncTaskManager::TaskUID getMeshingTaskUID(mitk::DataNode* solidNode);
};

}