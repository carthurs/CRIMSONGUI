#include "CRIMSONAppWorkbenchWindowAdvisor.h"
#include <CRIMSONVersion.h>

#include <QCoreApplication>

#include <QMainWindow>
#include <QToolBar>

#include <mitkLogMacros.h>

#include "CRIMSONWorkflowWidget.h"

namespace crimson {


CRIMSONAppWorkbenchWindowAdvisor::CRIMSONAppWorkbenchWindowAdvisor(berry::WorkbenchAdvisor* wbAdvisor, berry::IWorkbenchWindowConfigurer::Pointer configurer)
    : QmitkExtWorkbenchWindowAdvisor(wbAdvisor, configurer)
{
    SetProductName(QCoreApplication::applicationName() + " v." + CRIMSON_VERSION_STRING + " - ");

    MITK_INFO << "CRIMSON version: " << CRIMSON_VERSION_STRING;
}

void CRIMSONAppWorkbenchWindowAdvisor::PostWindowCreate()
{
    QmitkExtWorkbenchWindowAdvisor::PostWindowCreate();

    std::vector<crimson::CRIMSONWorkflowWidget::PerspectiveAndViewsType> perspectiveViewsList = {
            { "uk.ac.kcl.GeometryModelingPerspective", 
                {   "org.mitk.views.vesselpathplanningview", 
                    "org.mitk.views.ContourModelingView", 
                    "org.mitk.views.VesselBlendingView", 
                    "org.mitk.views.VesselDrivenResliceView" 
                } 
            },
            { "uk.ac.kcl.MeshingAndSolverSetupPerspective", 
                {   "org.mitk.views.vesselmeshingview", 
                    "org.mitk.views.SolverSetupView", 
                    "org.mitk.views.MeshAdaptView", 
                    "org.mitk.views.MeshExplorationView", 
                    "org.mitk.views.VesselDrivenResliceView",
					"org.mitk.views.ResliceView" 					
                } 
            }
    };

    berry::IWorkbenchWindow::Pointer window = this->GetWindowConfigurer()->GetWindow();
    QMainWindow* mainWindow = static_cast<QMainWindow*>(window->GetShell()->GetControl());

    QToolBar* workflowToolBar = new QToolBar;
    workflowToolBar->setObjectName("CRIMSON Workflow Toolbar");

    CRIMSONWorkflowWidget* workflowWidget = new CRIMSONWorkflowWidget(perspectiveViewsList, workflowToolBar);
    workflowToolBar->addWidget(workflowWidget);

    workflowToolBar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    mainWindow->addToolBar(workflowToolBar);
    mainWindow->insertToolBarBreak(workflowToolBar);

    workflowWidget->collapseAll();

}

} // namespace crimson
