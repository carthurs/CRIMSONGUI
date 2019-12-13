#pragma once

#include <berryIApplication.h>

/// Qt
#include <QObject>
#include <QScopedPointer>

namespace crimson {

class CRIMSONAppWorkbenchAdvisor;

class CRIMSONApplication : public QObject, public berry::IApplication
{
    Q_OBJECT
    Q_INTERFACES(berry::IApplication)

public:

    /** Standard constructor.*/
    CRIMSONApplication();

    /** Standard destructor.*/
    ~CRIMSONApplication();

    /** Starts the application.*/
    QVariant Start(berry::IApplicationContext* context) override;

    /** Exits the application.*/
    void Stop() override;

private:

    /** The WorkbenchAdvisor for the CRIMSONApplication application.*/
    QScopedPointer<CRIMSONAppWorkbenchAdvisor> wbAdvisor;
};

} // namespace crimson