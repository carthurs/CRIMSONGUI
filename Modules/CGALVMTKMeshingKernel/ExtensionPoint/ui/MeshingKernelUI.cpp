#include <ui/MeshingKernelUI.h>

#include <ui/GlobalMeshingParametersWidget.h>
#include <ui/LocalMeshingParametersDialog.h>

namespace crimson
{
    QWidget* MeshingKernelUI::createGlobalMeshingParameterWidget(IMeshingKernel::GlobalMeshingParameters& params, QWidget* parent /*= nullptr*/) {
        return new GlobalMeshingParametersWidget(params, parent);
    }

    QDialog* MeshingKernelUI::createLocalMeshingParametersDialog(
        const std::vector<IMeshingKernel::LocalMeshingParameters*>& params,
        bool editingGlobal, 
        IMeshingKernel::LocalMeshingParameters& defaultLocalParameters,
        QWidget* parent /*= nullptr*/
    ) {
        return new LocalMeshingParametersDialog(params, editingGlobal, defaultLocalParameters, parent);
    }

} // namespace crimson

