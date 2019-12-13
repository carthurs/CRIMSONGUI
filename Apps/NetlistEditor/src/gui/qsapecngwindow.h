/*
    QSapecNG - Qt based SapecNG GUI front-end
    Copyright (C) 2009, Michele Caini

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef QSAPECNGWINDOW_H
#define QSAPECNGWINDOW_H


#include <QMainWindow>
#include <QtCore/QModelIndex>

#include "gui/workplane/workplane.h"


class QWidget;

class QProgressBar;

class QMdiArea;
class QMdiSubWindow;

class QTabWidget;

class QTextEdit;
class QLineEdit;
class QPushButton;
class QAction;
class QMenu;

class QToolBar;
class QDockWidget;

class QSignalMapper;

class QtTreePropertyBrowser;

class QUndoView;
class QUndoGroup;

class QFileSystemModel;
class QTreeView;


namespace qsapecng
{


class QLogPolicy;

class SchematicEditor;
class SideBarModel;
class SideBarView;

class Item;


class QSapecNGWindow: public QMainWindow
{

  Q_OBJECT

public:
  QSapecNGWindow();

protected:
  void closeEvent(QCloseEvent* event);

private slots:
  void toggleScreenMode();

  void configDialog();
  void changeWorkspace();

  void zoomIn();
  void zoomOut();
  void zoomNormal();
  void fit();

  // void wave();
  void assignNodes();
  // void resolve();
  // void plot(int f);
  // void xAxisLogScale(bool log);
  // void yAxisLogScale(bool log);
  // void replot();

  void newFile();
  void open(const QString& fileName);
  void open();
  void openRecent();
  void save();
  void writeSimulationFiles();
  void saveAs();
  void print();

  void props();

  void bin();
  void cut();
  void copy();
  void paste();
  void rotate();
  void mirror();
  void bringToFront();
  void sendToBack();

  void license();
  void about();

  // void userDefRequested();
  void connectedWireSessionRequested();
  void disconnectedWireSessionRequested();
  void groundSessionRequested();
  // void portSessionRequested();
  // void outSessionRequested();
  // void voltmeterSessionRequested();
  // void ammeterSessionRequested();
  void labelSessionRequested();
  void clearSessionRequested();

  void activated(const QModelIndex& index);
  void sceneSelectionChanged();
  void updateMenus();
  void updateWindowMenu();
  void updatePropertyBrowser();
  void updateDocks();

  void setActiveSubWindow(QWidget* window);
  void subWindowActivated(QMdiSubWindow* window);
  void stackEditor(SchematicEditor* editor);
  
signals:
  void toggleGrid(bool visible);

private:
  void createLogger();
  void createActions();
  void createDockWidgets();
  void createMenus();
  void createToolBars();
  void createStatusBar();

  void createSideBarDockLayout();
  void createPropertyDockLayout();
  void createMessageDockLayout();
  void createUndoRedoDockLayout();
  void createWorkspaceDockLayout();

  void readSettings();
  void writeSettings();

  SchematicEditor* activeEditor();
  QMdiSubWindow* findEditor(const QString& fileName);
  SchematicEditor* createSchematicEditor();
  void setupSchematicEditor(SchematicEditor* editor);

  void updateRecentFileList(const QString& fileName);
  void loadRecentFileList();

private:
  QProgressBar* progressBar_;

  SideBarModel* sideBarModel_;
  SideBarView* sideBarView_;

  QtTreePropertyBrowser* treeBrowser_;

  QLogPolicy* policy_;
  QTextEdit* logViewer_;

  QUndoView* undoRedoView_;
  QUndoGroup* undoRedoGroup_;

  QLineEdit* workspace_;
  QFileSystemModel* wfsModel_;
  QTreeView* wfsView_;

  QMdiArea* mdiArea_;
  QSignalMapper* windowMapper_;

  // WorkPlane* workplane_;

  QTabWidget* central_;

  QMenu* fileMenu_;
  QMenu* recentMenu_;
  QMenu* editMenu_;
  QMenu* viewMenu_;
  QMenu* runMenu_;
  QMenu* workplaneMenu_;
  QMenu* toolMenu_;
  QMenu* settingMenu_;
  QMenu* toolbarMenu_;
  QMenu* windowMenu_;
  QMenu* showMenu_;
  QMenu* helpMenu_;
  QMenu* sceneMenu_;

  QToolBar* fileToolBar_;
  QToolBar* viewToolBar_;
  QToolBar* runToolBar_;
  QToolBar* toolToolBar_;

  QDockWidget* sideBarDock_;
  QDockWidget* propertyDock_;
  QDockWidget* messageDock_;
  QDockWidget* undoRedoDock_;
  QDockWidget* workspaceDock_;

  QAction* newAct_;
  QAction* openAct_;
  QAction* saveAct_;
  QAction* writeSimulationFilesAct_;
  QAction* saveAsAct_;
  QAction* printAct_;
  QAction* quitAct_;
  QAction* binAct_;
  QAction* undoAct_;
  QAction* redoAct_;
  QAction* propsAct_;
  QAction* cutAct_;
  QAction* copyAct_;
  QAction* pasteAct_;
  QAction* rotateAct_;
  QAction* mirrorAct_;
  QAction* bringToFrontAct_;
  QAction* sendToBackAct_;
  QAction* zoomInAct_;
  QAction* zoomOutAct_;
  QAction* zoomNormalAct_;
  QAction* fitAct_;
  QAction* nodeAct_;
  // QAction* resolveAct_;
  // QAction* waveAct_;
  QAction* toggleScreenModeAct_;
  QAction* configureAppAct_;
  QAction* closeAct_;
  QAction* closeAllAct_;
  QAction* tileAct_;
  QAction* cascadeAct_;
  QAction* nextAct_;
  QAction* previousAct_;
  QAction* separatorAct_;
  QAction* licenseAct_;
  QAction* aboutAct_;
  QAction* aboutQtAct_;
  // QAction* userdefAct_;
  QAction* connectedWireSessionAct_;
  QAction* disconnectedWireSessionAct_;
  QAction* groundSessionAct_;
  // QAction* portSessionAct_;
  // QAction* outSessionAct_;
  // QAction* voltmeterSessionAct_;
  // QAction* ammeterSessionAct_;
  QAction* labelSessionAct_;
  QAction* cursorAct_;
  QAction* toggleGridAct_;

  QSignalMapper* fMapper_;
  // QAction* magnitudeAct_;
  // QAction* magnitudeRadAct_;
  // QAction* phaseAct_;
  // QAction* phaseRadAct_;
  // QAction* gainAct_;
  // QAction* gainRadAct_;
  // QAction* lossAct_;
  // QAction* lossRadAct_;
  // QAction* zerosPolesAct_;
  /* plotter noop acts */
  // QAction* xLogAct_;
  // QAction* yLogAct_;
  // QAction* replotAct_;

  enum { maxRecentFiles = 10 };
  QAction* openRecentActs_[maxRecentFiles];

};


}


#endif // QSAPECNGWINDOW_H
