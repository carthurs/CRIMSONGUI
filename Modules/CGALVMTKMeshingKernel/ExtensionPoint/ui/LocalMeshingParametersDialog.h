#pragma once

#include <QDialog>
#include "ui_LocalMeshingParametersDialog.h"

#include <IMeshingKernel.h>

#include "CGALVMTKMeshingKernelExports.h"

class OverrideStatusButtonHandler;

/*! \brief   Dialog for setting the mesh options. */
class CGALVMTKMeshingKernel_EXPORT LocalMeshingParametersDialog : public QDialog
{
    Q_OBJECT
public:

    /*!
     * \brief   Constructor.
     *
     * \param   params                  List of parameters to edit.
     * \param   editingGlobal           true to show that global meshing parameters are edited.
     * \param   defaultLocalParameters  The default values for parameters.
     * \param   parent                  (Optional) If non-null, the parent.
     */
    LocalMeshingParametersDialog(const std::vector<crimson::IMeshingKernel::LocalMeshingParameters*>& params, bool editingGlobal,
                      crimson::IMeshingKernel::LocalMeshingParameters& defaultLocalParameters, QWidget* parent = nullptr);

    void accept() override;

private:
    Ui::LocalMeshingParametersDialog _ui;
    const std::vector<crimson::IMeshingKernel::LocalMeshingParameters*>& _params;
    crimson::IMeshingKernel::LocalMeshingParameters& _defaultLocalParameters;
    bool _editingGlobal;

    OverrideStatusButtonHandler* meshSizeStatusHandler;
    OverrideStatusButtonHandler* sizeRelativeStatusHandler;
    OverrideStatusButtonHandler* curvatureRefinementStatusHandler;
    OverrideStatusButtonHandler* curvatureRefinementValueStatusHandler;
    OverrideStatusButtonHandler* minCurvatureSizeFactorStatusHandler;
    OverrideStatusButtonHandler* propagationDistanceStatusHandler;
    OverrideStatusButtonHandler* boundaryLayerThicknessStatusHandler;
};