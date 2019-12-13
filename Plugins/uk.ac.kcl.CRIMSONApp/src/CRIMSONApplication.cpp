#include "CRIMSONApplication.h"
#include "CRIMSONAppWorkbenchAdvisor.h"

#include <CRIMSONVersion.h>

#include <berryPlatformUI.h>

#include <QSettings>
#include <QDate>
#include <QMessageBox>
#include <QCoreApplication>

namespace crimson {


CRIMSONApplication::CRIMSONApplication()
{
}

CRIMSONApplication::~CRIMSONApplication()
{
}

QVariant CRIMSONApplication::Start(berry::IApplicationContext* context)
{
#ifdef CRIMSON_BUILD_TRIAL_VERSION
    static auto TRIAL_COUNT = 365;
    QSettings settings("King's College London", QString("CRIMSON ") + CRIMSON_VERSION_STRING);

    if (!settings.allKeys().contains("trial_left")) {
        settings.setValue("trial_left", TRIAL_COUNT);
        settings.setValue("trial_last_date", QDate::currentDate());
        settings.setValue("first_run_date", QDate::currentDate());
    }

    QDate firstRunDate = qvariant_cast<QDate>(settings.value("first_run_date"));

    int trialDaysLeft = qvariant_cast<int>(settings.value("trial_left"));
    if (QDate::currentDate() != qvariant_cast<QDate>(settings.value("trial_last_date"))) {
        trialDaysLeft = qMax(0, trialDaysLeft - 1);
        trialDaysLeft = qMin(qMax(0, TRIAL_COUNT - static_cast<int>(firstRunDate.daysTo(QDate::currentDate()))), trialDaysLeft);

        settings.setValue("trial_left", trialDaysLeft);
        settings.setValue("trial_last_date", QDate::currentDate());
    }

    if (trialDaysLeft == 0) {
        QMessageBox::information(nullptr, "Trial has ended", "Your CRIMSON trial period has ended.\nPlease contact King's College London for a renewal.", QMessageBox::Ok);
        return EXIT_OK;
    }

    QCoreApplication::setApplicationName(QCoreApplication::applicationName() + " (Trial, " + QString::number(trialDaysLeft) + " days left)");
#endif

  berry::Display* display = berry::PlatformUI::CreateDisplay();

  wbAdvisor.reset(new CRIMSONAppWorkbenchAdvisor);
  int code = berry::PlatformUI::CreateAndRunWorkbench(display, wbAdvisor.data());

  // exit the application with an appropriate return code
  return code == berry::PlatformUI::RETURN_RESTART
              ? EXIT_RESTART : EXIT_OK;
}

void CRIMSONApplication::Stop()
{
}

} // namespace crimson