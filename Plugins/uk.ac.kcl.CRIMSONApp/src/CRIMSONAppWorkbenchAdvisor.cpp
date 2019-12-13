#include <QPoint>

#include "CRIMSONAppWorkbenchAdvisor.h"
#include "CRIMSONAppWorkbenchWindowAdvisor.h"
#include "internal/uk_ac_kcl_CRIMSONApp_Activator.h"

#include <berryIQtStyleManager.h>

namespace crimson {


const QString CRIMSONAppWorkbenchAdvisor::DEFAULT_PERSPECTIVE_ID = "uk.ac.kcl.GeometryModelingPerspective";

// //! [WorkbenchAdvisorCreateWindowAdvisor]
berry::WorkbenchWindowAdvisor* CRIMSONAppWorkbenchAdvisor::CreateWorkbenchWindowAdvisor(berry::IWorkbenchWindowConfigurer::Pointer configurer)
{
    auto wwAdvisor = new CRIMSONAppWorkbenchWindowAdvisor(this, configurer);

    // Exclude the help perspective from org.blueberry.ui.qt.help from
    // the normal perspective list.
    // The perspective gets a dedicated menu entry in the help menu
    QList<QString> excludePerspectives;
    excludePerspectives.push_back("org.blueberry.perspectives.help");
    wwAdvisor->SetPerspectiveExcludeList(excludePerspectives);

    // Exclude some views from the normal view list
    QList<QString> excludeViews;
    excludeViews.push_back("org.mitk.views.modules");
    wwAdvisor->SetViewExcludeList(excludeViews);

    wwAdvisor->SetWindowIcon(":/CRIMSON-logo.png"); // Replace default icon taken from ico file due to transparency

//    std::vector<std::string> viewExcludeList;

    configurer->SetInitialSize(QPoint(1000, 770));

//     wwAdvisor->ShowViewMenuItem(false);
//     wwAdvisor->ShowNewWindowMenuItem(false);
//     wwAdvisor->ShowClosePerspectiveMenuItem(false);
//     wwAdvisor->SetPerspectiveExcludeList(perspExcludeList);
//     wwAdvisor->SetViewExcludeList(viewExcludeList);
//     wwAdvisor->ShowViewToolbar(false);
//     wwAdvisor->ShowPerspectiveToolbar(true);
    wwAdvisor->ShowVersionInfo(false);
    wwAdvisor->ShowMitkVersionInfo(false);
//     wwAdvisor->SetWindowIcon(":/org.mitk.gui.qt.diffusionimagingapp/app-icon.png");
    return wwAdvisor;
}
// //! [WorkbenchAdvisorCreateWindowAdvisor]

CRIMSONAppWorkbenchAdvisor::~CRIMSONAppWorkbenchAdvisor()
{
}
// //! [WorkbenchAdvisorInit]
void CRIMSONAppWorkbenchAdvisor::Initialize(berry::IWorkbenchConfigurer::Pointer configurer)
{
    ctkPluginContext* pluginContext = uk_ac_kcl_CRIMSONApp_Activator::GetPluginContext();
    ctkServiceReference serviceReference = pluginContext->getServiceReference<berry::IQtStyleManager>();

    //always granted by org.blueberry.ui.qt
    Q_ASSERT(serviceReference);

    berry::IQtStyleManager* styleManager = pluginContext->getService<berry::IQtStyleManager>(serviceReference);
    Q_ASSERT(styleManager);

    QString styleName = "CRIMSON Style";
    styleManager->AddStyle(":/vmStyle.qss", styleName);
    berry::QtWorkbenchAdvisor::Initialize(configurer);
    styleManager->SetStyle(":/vmStyle.qss");



    configurer->SetSaveAndRestore(true);
}
// //! [WorkbenchAdvisorInit]
QString CRIMSONAppWorkbenchAdvisor::GetInitialWindowPerspectiveId()
{
    return DEFAULT_PERSPECTIVE_ID;
}

} // namespace crimson
