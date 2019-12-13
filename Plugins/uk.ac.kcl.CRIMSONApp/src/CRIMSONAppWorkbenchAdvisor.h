#pragma once

#include <berryQtWorkbenchAdvisor.h>

namespace crimson {

class CRIMSONAppWorkbenchAdvisor : public berry::QtWorkbenchAdvisor
{
public:
    /**
     * Holds the ID-String of the initial window perspective.
     */
    static const QString DEFAULT_PERSPECTIVE_ID;

    berry::WorkbenchWindowAdvisor* CreateWorkbenchWindowAdvisor(berry::IWorkbenchWindowConfigurer::Pointer) override;
    ~CRIMSONAppWorkbenchAdvisor();

    /**
     * Gets the style manager (berry::IQtStyleManager), adds and initializes a Qt-Stylesheet-File (.qss).
     */
    void Initialize(berry::IWorkbenchConfigurer::Pointer) override;

    /**
     * Returns the ID-String of the initial window perspective.
     */
    QString GetInitialWindowPerspectiveId() override;

private:
    // QScopedPointer<berry::WorkbenchWindowAdvisor> wwAdvisor;
};

} // namespace crimson