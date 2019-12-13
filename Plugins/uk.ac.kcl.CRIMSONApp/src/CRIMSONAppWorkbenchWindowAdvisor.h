#pragma once

#include <QmitkExtWorkbenchWindowAdvisor.h>

namespace crimson {

class CRIMSONAppWorkbenchWindowAdvisor : public QmitkExtWorkbenchWindowAdvisor
{
    Q_OBJECT

public:
    CRIMSONAppWorkbenchWindowAdvisor(berry::WorkbenchAdvisor* wbAdvisor, berry::IWorkbenchWindowConfigurer::Pointer configurer);

    void PostWindowCreate() override;
};

} // namespace crimson
