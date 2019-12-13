// This will be an interface for meshing service and blueberry extension point etc.

#pragma once

#include <map>

#include <mitkBaseData.h>
#include <FaceIdentifier.h>
#include <VesselForestData.h>
#include <AsyncTaskWithResult.h>
#include <OCCBRepData.h>

#include <boost/optional.hpp>

#include "CGALVMTKMeshingKernelExports.h"

namespace crimson
{

class CGALVMTKMeshingKernel_EXPORT IMeshingKernel
{
public:
    struct LocalMeshingParameters {
        boost::optional<double> size;       // = 0.01;
        boost::optional<bool> sizeRelative; // = true;

        bool useBoundaryLayers; // = bltNone
        boost::optional<double> thickness;
        int numSubLayers;
        double subLayerRatio;

        // Set the default values - i.e. all optionals are defined
        void setDefaults()
        {
            size = 0.01;
            sizeRelative = true;
            useBoundaryLayers = false;
            thickness = 1;
            numSubLayers = 1;
            subLayerRatio = 0.5;
        }

        bool anyOptionSet() const
        {
            return size || sizeRelative || thickness;
        }
    };

    struct GlobalMeshingParameters {
        bool meshSurfaceOnly = false;
        int surfaceOptimizationLevel = 5;

        double maxRadiusEdgeRatio = 2;
        double minDihedralAngle = 5;
        double maxDihedralAngle = 165;
        int volumeOptimizationLevel = 2;

        LocalMeshingParameters defaultLocalParameters;

        GlobalMeshingParameters() { defaultLocalParameters.setDefaults(); }
    };

    /*!
     * \brief   Creates an asynchronous task that computes a mesh for a solid model.
     *
     * \param   solid               The solid model to be meshed.
     * \param   params              The meshing parameters for the whole model.
     * \param   localParams         The map from face identifiers to overriding local meshing
     *                              parameters.
     * \param   vesselUIDtoNameMap  a map from vessel path UID to data node name - used for debugging
     *                              output and formatting error messages.
     */
	
    static std::shared_ptr<async::TaskWithResult<mitk::BaseData::Pointer>>
		createMeshSolidTask(const mitk::BaseData::Pointer& solid, const GlobalMeshingParameters& params,
                        const std::map<FaceIdentifier, LocalMeshingParameters>& localParams,
                        const std::map<VesselPathAbstractData::VesselPathUIDType, std::string>& vesselUIDtoNameMap);




    /*!
     * \brief   Creates an asynchronous task that adapts a mesh based on the error indicator obtained
     *  after initial simulation.
     *
     * \param   originalMesh            The original mesh.
     * \param   factor                  The factor by which the average error should be reduced.
     * \param   hmin                    The minimum edge size in the resulting mesh.
     * \param   hmax                    The maximum edge size in the resulting mesh.
     * \param   errorIndicatorArrayName The name of the array which contains the error indicator data.
     */
    static std::shared_ptr<async::TaskWithResult<mitk::BaseData::Pointer>>
    createAdaptMeshTask(const mitk::BaseData::Pointer& originalMesh, double factor, double hmin, double hmax,
                        const std::string& errorIndicatorArrayName);
};

} // namespace crimson
