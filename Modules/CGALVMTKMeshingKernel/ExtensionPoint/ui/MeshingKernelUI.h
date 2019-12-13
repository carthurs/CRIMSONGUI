#pragma once

#include <IMeshingKernel.h>

#include <QWidget>
#include <QDialog>

namespace crimson
{

class CGALVMTKMeshingKernel_EXPORT MeshingKernelUI {
public:
    static QWidget* createGlobalMeshingParameterWidget(IMeshingKernel::GlobalMeshingParameters& params, QWidget* parent = nullptr);
    static QDialog* createLocalMeshingParametersDialog(const std::vector<IMeshingKernel::LocalMeshingParameters*>& params, bool editingGlobal,
        IMeshingKernel::LocalMeshingParameters& defaultLocalParameters, QWidget* parent = nullptr);
};

} // namespace crimson

