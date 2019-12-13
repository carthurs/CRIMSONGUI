#include <Poco/Util/MapConfiguration.h>

#include <QApplication>
#include <QMessageBox>
#include <QtSingleApplication>
#include <QTime>
#include <QDir>
#include <QDesktopServices>
#include <QSplashScreen>
#include <QTimer>

#include <Windows.h>
#include <Winuser.h>

#include <usModuleSettings.h>

#include <mitkBaseApplication.h>

int main(int argc, char** argv)
{
    mitk::BaseApplication app(argc, argv);

    app.setApplicationName("CRIMSON");
    app.setOrganizationName("KCL");

    app.initializeQt();

#ifndef _DEBUG
    QPixmap pixmap(":/splash/splashscreen.png");
    QSplashScreen splash(pixmap);

    bool showSplashScreen = true;
    if (showSplashScreen) {
        splash.show();

        qApp->sendPostedEvents();
        qApp->processEvents();
        qApp->flush();

        QTimer::singleShot(3000, &splash, SLOT(close()));
    }
#endif


#if QT_VERSION >= QT_VERSION_CHECK(5,6,0)
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#else
	qputenv("QT_DEVICE_PIXEL_RATIO", QByteArray("1"));
#endif // QT_VERSION


    QStringList preloadLibs;
    preloadLibs << "liborg_mitk_gui_qt_ext";
    app.setPreloadLibraries(preloadLibs);


    app.setProperty(mitk::BaseApplication::PROP_PRODUCT, "uk.ac.kcl.CRIMSONApp.CRIMSON");

    return app.run();
}