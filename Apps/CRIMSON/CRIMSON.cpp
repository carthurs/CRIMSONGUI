#include <Poco/Util/MapConfiguration.h>

#include <QApplication>
#include <QMessageBox>
#include <QtSingleApplication>
#include <QTime>
#include <QDir>
#include <QDesktopServices>
#include <QSplashScreen>
#include <QTimer>

#include <iostream>

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

	// switch up the font to size 9 - size 8 was the default
	QFont default = QApplication::font();
	default.setPointSize(9);
	QApplication::setFont(default);

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

//#ifdef Q_OS_WIN
//	//SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
//	SetProcessDPIAware(); // call before the main event loop
//#endif // Q_OS_WIN 

#if QT_VERSION >= QT_VERSION_CHECK(5,6,0)
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#else
	qputenv("QT_DEVICE_PIXEL_RATIO", QByteArray("1"));
#endif // QT_VERSION

//    QString storageDir = handleNewAppInstance(&app, argc, argv, "");
//
//    if (storageDir.isEmpty())
//    {
//        // This is a new instance and no other instance is already running. We specify
//        // the storage directory here (this is the same code as in berryInternalPlatform.cpp
//        // so that we can re-use the location for the persistent data location of the
//        // the CppMicroServices library.
//
//        // Append a hash value of the absolute path of the executable to the data location.
//        // This allows to start the same application from different build or install trees.
//#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
//        storageDir = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + '_';
//#else
//        storageDir = QDesktopServices::storageLocation(QDesktopServices::DataLocation) + '_';
//#endif
//        storageDir += QString::number(qHash(QCoreApplication::applicationDirPath())) + QDir::separator();
//    }
//    us::ModuleSettings::SetStoragePath((storageDir + QString("us") + QDir::separator()).toStdString());

    // These paths replace the .ini file and are tailored for installation
    // packages created with CPack. If a .ini file is presented, it will
    // overwrite the settings in MapConfiguration
//    Poco::Path basePath(argv[0]);
//    basePath.setFileName("");
//
//    Poco::Path provFile(basePath);
//    provFile.setFileName("CRIMSON.provisioning");
//
//    Poco::Path extPath(basePath);
//    extPath.pushDirectory("ExtBundles");
//
//    std::string pluginDirs = extPath.toString();
//
//    Poco::Util::MapConfiguration* extConfig(new Poco::Util::MapConfiguration());
//    if (!storageDir.isEmpty())
//    {
//        extConfig->setString(berry::Platform::ARG_STORAGE_DIR, storageDir.toStdString());
//    }
//    extConfig->setString(berry::Platform::ARG_PLUGIN_DIRS, pluginDirs);
//    extConfig->setString(berry::Platform::ARG_PROVISIONING, provFile.toString());
//    extConfig->setString(mitk::BaseApplication::PROP_PRODUCT, "uk.ac.kcl.CRIMSONApp");

//    QStringList preloadLibs;
//
//    // Preload the org.mitk.gui.qt.ext plug-in (and hence also QmitkExt) to speed
//    // up a clean-cache start. This also works around bugs in older gcc and glibc implementations,
//    // which have difficulties with multiple dynamic opening and closing of shared libraries with
//    // many global static initializers. It also helps if dependent libraries have weird static
//    // initialization methods and/or missing de-initialization code.
//    preloadLibs << "liborg_mitk_gui_qt_ext";
//
//    QMap<QString, QString> preloadLibVersion;
//
//#ifdef Q_OS_MAC
//    const QString libSuffix = ".dylib";
//#elif defined(Q_OS_UNIX)
//    const QString libSuffix = ".so";
//#elif defined(Q_OS_WIN)
//    const QString libSuffix = ".dll";
//#else
//    const QString libSuffix;
//#endif
//
//    for (QStringList::Iterator preloadLibIter = preloadLibs.begin(),
//        iterEnd = preloadLibs.end(); preloadLibIter != iterEnd; ++preloadLibIter)
//    {
//        QString& preloadLib = *preloadLibIter;
//        // In case the application is started from an install directory
//        QString tempLibraryPath = QCoreApplication::applicationDirPath() + "/plugins/" + preloadLib + libSuffix;
//        QFile preloadLibrary(tempLibraryPath);
//#ifdef Q_OS_MAC
//        if (!preloadLibrary.exists())
//        {
//            // In case the application is started from a build tree
//            QString relPath = "/../../../plugins/" + preloadLib + libSuffix;
//            tempLibraryPath = QCoreApplication::applicationDirPath() + relPath;
//            preloadLibrary.setFileName(tempLibraryPath);
//        }
//#endif
//        if (preloadLibrary.exists())
//        {
//            preloadLib = tempLibraryPath;
//        }
//        // Else fall back to the QLibrary search logic
//    }
//
//    QString preloadConfig;
//    Q_FOREACH(const QString& preloadLib, preloadLibs)
//    {
//        preloadConfig += preloadLib + preloadLibVersion[preloadLib] + ",";
//    }
//    preloadConfig.chop(1);
//
//    extConfig->setString(berry::Platform::ARG_PRELOAD_LIBRARY, preloadConfig.toStdString());
//
//    // Seed the random number generator, once at startup.
//    QTime time = QTime::currentTime();
//    qsrand((uint)time.msec());
//
//    // Run the workbench.
//    return berry::Starter::Run(argc, argv, extConfig);
    QStringList preloadLibs;
    preloadLibs << "liborg_mitk_gui_qt_ext";
    app.setPreloadLibraries(preloadLibs);

    //app.setProperty(mitk::BaseApplication::PROP_APPLICATION, "org.mitk.qt.diffusionimagingapp");
    app.setProperty(mitk::BaseApplication::PROP_PRODUCT, "uk.ac.kcl.CRIMSONApp.CRIMSON");

	return app.run();
}

