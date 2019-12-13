#include "ui_GlobalMeshingParametersWidget.h"

#include "CGALVMTKMeshingKernelExports.h"

#include <IMeshingKernel.h>

namespace crimson
{
/*! \brief   A widget for editing face data (boundary conditions and materials). */
class CGALVMTKMeshingKernel_EXPORT GlobalMeshingParametersWidget : public QWidget
{
    Q_OBJECT
public:
    GlobalMeshingParametersWidget(IMeshingKernel::GlobalMeshingParameters& params, QWidget* parent = nullptr);
    ~GlobalMeshingParametersWidget();

private slots:
    void syncMeshingParametersToUIValues();
    void syncUIValuesToMeshingParameters();

private:
    Ui::GlobalMeshingParametersWidget _UI;

    IMeshingKernel::GlobalMeshingParameters& _params;
};
};