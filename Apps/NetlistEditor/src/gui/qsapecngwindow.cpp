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


#include "config.h"

#include "gui/qsapecngwindow.h"

#include "gui/configdialog/configdialog.h"
#include "gui/settings.h"
#include "gui/qlogger.h"
#include "gui/delegate.h"

#include "gui/editor/schematiceditor.h"
#include "gui/editor/schematicview.h"
#include "gui/editor/schematicscene.h"
#include "gui/extendedmdiarea.h"
#include "gui/sidebarmodel.h"
#include "gui/sidebarview.h"

#include "QtTreePropertyBrowser"
#include "QtButtonPropertyBrowser"

#include <QProgressBar>

#include <QMdiArea>
#include <QMdiSubWindow>

#include <QTabWidget>

#include <QMessageBox>
#include <QDialogButtonBox>

#include <QtCore/QFileInfo>
#include <QFileDialog>

#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QAction>
#include <QToolBox>
#include <QTextDocument>
#include <QGroupBox>
#include <QPlainTextEdit>

#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>

#include <QtCore/QSignalMapper>
#include <QCloseEvent>
#include <QGraphicsItem>

#include <QUndoView>
#include <QUndoGroup>
#include <QUndoStack>

#include <QFileSystemModel>
#include <QTreeView>

#include <QPrinter>
#include <QPrintDialog>

#include <QGridLayout>
#include <QVBoxLayout>
#include <QApplication>

#include <limits>


namespace qsapecng
{


QSapecNGWindow::QSapecNGWindow()
{
  Settings().load();

  windowMapper_ = new QSignalMapper(this);
  connect(windowMapper_, SIGNAL(mapped(QWidget*)),
    this, SLOT(setActiveSubWindow(QWidget*)));

  // fMapper_ = new QSignalMapper(this);
  // connect(fMapper_, SIGNAL(mapped(int)), this, SLOT(plot(int)));

  mdiArea_ = new ExtendedMdiArea;
  mdiArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  mdiArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
//   mdiArea_->setViewMode(QMdiArea::TabbedView);

  connect(static_cast<ExtendedMdiArea*>(mdiArea_),
    SIGNAL(dropFileEvent(const QString&)), this, SLOT(open(const QString&)));
  connect(mdiArea_, SIGNAL(subWindowActivated(QMdiSubWindow*)),
    this, SLOT(subWindowActivated(QMdiSubWindow*)));
  connect(mdiArea_, SIGNAL(subWindowActivated(QMdiSubWindow*)),
    this, SLOT(sceneSelectionChanged()));
  connect(mdiArea_, SIGNAL(subWindowActivated(QMdiSubWindow*)),
    this, SLOT(updateMenus()));
  connect(mdiArea_, SIGNAL(subWindowActivated(QMdiSubWindow*)),
    this, SLOT(updatePropertyBrowser()));
  connect(mdiArea_, SIGNAL(subWindowActivated(QMdiSubWindow*)),
    this, SLOT(updateDocks()));

  createLogger();
  createDockWidgets();
  createActions();
  createToolBars();
  createStatusBar();
  createMenus();

  // workplane_ = new WorkPlane;
  // workplane_->setContextMenu(workplaneMenu_);
  // workplane_->xAxisLogScale(xLogAct_->isChecked());
  // workplane_->yAxisLogScale(yLogAct_->isChecked());
  // workplane_->setData(
  //     std::map<std::string, double>(),
  //     sapecng::metacircuit::expression(),
  //     sapecng::metacircuit::expression()
  //   );
  // workplane_->plot(WorkPlane::NOOP);

  central_ = new QTabWidget;
  central_->addTab(mdiArea_, tr("Editor"));
  // central_->addTab(workplane_, tr("Workplane"));
  central_->setTabPosition(QTabWidget::East);

  connect(central_, SIGNAL(currentChanged(int)),
    this, SLOT(sceneSelectionChanged()));
  connect(central_, SIGNAL(currentChanged(int)),
    this, SLOT(updateMenus()));
  connect(central_, SIGNAL(currentChanged(int)),
    this, SLOT(updatePropertyBrowser()));
  connect(central_, SIGNAL(currentChanged(int)),
    this, SLOT(updateDocks()));

  setCentralWidget(central_);

  sideBarDock_->setObjectName("sideBarDock");
  propertyDock_->setObjectName("propertyDock");
  messageDock_->setObjectName("messageDock");
  undoRedoDock_->setObjectName("undoRedoDock");
  workspaceDock_->setObjectName("workspaceDock");

  fileToolBar_->setObjectName("fileToolBar");
  viewToolBar_->setObjectName("viewToolBar");
  runToolBar_->setObjectName("runToolBar");
  toolToolBar_->setObjectName("toolToolBar");

  readSettings();

  sceneSelectionChanged();
  updateMenus();
  updateDocks();
  loadRecentFileList();

  setWindowIcon(QIcon(":/images/icon.png"));
  setWindowTitle(QString("%1 - %2")
      .arg(PACKAGE_NAME)
      .arg(PACKAGE_VERSION)
    );
  setUnifiedTitleAndToolBarOnMac(true);
}


void QSapecNGWindow::closeEvent(QCloseEvent* event)
{
  QStringList session;
  QList<QMdiSubWindow*> subWs = mdiArea_->subWindowList();
  foreach(QMdiSubWindow* subW, subWs) {
    SchematicEditor* editor = qobject_cast<SchematicEditor*>(subW);
    if(editor && !editor->isUntitled())
      session.append(editor->currentFile());
  }

  mdiArea_->closeAllSubWindows();
  mdiArea_->activateNextSubWindow();

  if(activeEditor()) {
    event->ignore();
  } else {
    writeSettings();
    Settings().save();
    event->accept();
  }
}


void QSapecNGWindow::toggleScreenMode()
{
  setWindowState(windowState() ^ Qt::WindowFullScreen);
}


void QSapecNGWindow::configDialog()
{
  ConfigDialog configDialog(this);
  configDialog.exec();

  Settings settings;

  QApplication::setFont(settings.appFont());

  QLogger::setLevel((sapecng::Logger::LogLevel) settings.logLvl());
  policy_->setDebugColor(settings.debugColor());
  policy_->setInfoColor(settings.infoColor());
  policy_->setWarningColor(settings.warningColor());
  policy_->setErrorColor(settings.errorColor());
  policy_->setFatalColor(settings.fatalColor());
}


void QSapecNGWindow::changeWorkspace()
{
  QString dir = QFileDialog::getExistingDirectory(
    this, tr("Choose directory"), workspace_->text(),
    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

  if(!dir.isEmpty()) {
    workspace_->setText(dir);
    wfsView_->setRootIndex(wfsModel_->setRootPath(dir));
    SettingsManager().setWorkspace(dir);
  }
}


void QSapecNGWindow::zoomIn()
{
  SchematicEditor* editor = activeEditor();
  if(editor) {
    editor->view().zoomIn();
    zoomOutAct_->setEnabled(true);
    zoomNormalAct_->setEnabled(true);
  }
}


void QSapecNGWindow::zoomOut()
{
  SchematicEditor* editor = activeEditor();
  if(editor)
    editor->view().zoomOut();
}


void QSapecNGWindow::zoomNormal()
{
  SchematicEditor* editor = activeEditor();
  if(editor)
    editor->view().normalSize();
}


void QSapecNGWindow::fit()
{
  SchematicEditor* editor = activeEditor();
  if(editor)
    editor->view().fitToView();
}


// void QSapecNGWindow::wave()
// {
//   SchematicEditor* editor = activeEditor();
//   if(editor)
//       editor->accept(*workplane_)
//     ? central_->setCurrentWidget(workplane_)
//     : central_->setCurrentWidget(mdiArea_)
//     ;
// }


void QSapecNGWindow::assignNodes()
{
  SchematicEditor* editor = activeEditor();
  if(editor) {
    QLogger::info(tr("Assigning nodes..."));
    editor->scene().assignNodes();
  }
}


// void QSapecNGWindow::resolve()
// {
//   SchematicEditor* editor = activeEditor();
//   if(editor) {
//     editor->solve();
//     mdiArea_->setActiveSubWindow(0);
//     mdiArea_->setActiveSubWindow(editor);
//   }
// }


// void QSapecNGWindow::plot(int f)
// {
//   workplane_->plot(f);
// }


// void QSapecNGWindow::xAxisLogScale(bool log)
// {
//   workplane_->xAxisLogScale(log);
// }


// void QSapecNGWindow::yAxisLogScale(bool log)
// {
//   workplane_->yAxisLogScale(log);
// }


// void QSapecNGWindow::replot()
// {
//   workplane_->redraw();
// }


void QSapecNGWindow::newFile()
{
  central_->setCurrentWidget(mdiArea_);
  QMdiSubWindow* subWindow = createSchematicEditor();
  setActiveSubWindow(subWindow);
  subWindow->show();
}


void QSapecNGWindow::open(const QString& fileName)
{
  if(!fileName.isEmpty()) {
    central_->setCurrentWidget(mdiArea_);

    QMdiSubWindow* existing = findEditor(fileName);
    if(existing) {
      mdiArea_->setActiveSubWindow(existing);
      return;
    }

    SchematicEditor* editor = createSchematicEditor();
    if(editor->loadFile(fileName)) {
      updateRecentFileList(fileName);
      statusBar()->showMessage(tr("File loaded"), 2000);
      mdiArea_->setActiveSubWindow(editor);
      editor->show();
    } else {
      editor->close();
    }
  }
}


void QSapecNGWindow::open()
{
  QString fileName = QFileDialog::getOpenFileName(this,
      tr("Read file"), Settings().workspace(),
      QString("%1;;%2")
        .arg(tr("XML files (*.xml)"))
        .arg(tr("Info files (*.info)"))
    );

  open(fileName);
}


void QSapecNGWindow::openRecent()
{
  QAction* action = qobject_cast<QAction*>(sender());
  if(action) {
    central_->setCurrentWidget(mdiArea_);

    QMdiSubWindow* existing = findEditor(action->data().toString());
    if(existing) {
      mdiArea_->setActiveSubWindow(existing);
      return;
    }

    SchematicEditor* editor = createSchematicEditor();
    if(editor->loadFile(action->data().toString())) {
      updateRecentFileList(action->data().toString());
      statusBar()->showMessage(tr("File loaded"), 2000);
      mdiArea_->setActiveSubWindow(editor);
      editor->show();
    } else {
      editor->close();
    }
  }
}


void QSapecNGWindow::save()
{
  SchematicEditor* editor = activeEditor();
  if(editor && editor->save()) {
    updateRecentFileList(editor->currentFile());
    statusBar()->showMessage(tr("File saved"), 2000);
  }
}

void QSapecNGWindow::writeSimulationFiles()
{
  SchematicEditor* editor = activeEditor();
  if(editor && editor->writeSimFiles()) {
    // updateRecentFileList(editor->currentFile());
    statusBar()->showMessage(tr("Simulation files written"), 2000);
  } 
}


void QSapecNGWindow::saveAs()
{
  SchematicEditor* editor = activeEditor();
  if(editor && editor->saveAs()) {
    updateRecentFileList(editor->currentFile());
    statusBar()->showMessage(tr("File saved"), 2000);
  }
}


void QSapecNGWindow::print()
{
  QPrinter printer;
  if(central_->currentWidget() == mdiArea_) {
    SchematicEditor* editor = activeEditor();

    if(editor) {
      if(QPrintDialog(&printer).exec() != QDialog::Accepted)
        return;

      QPainter painter(&printer);
      painter.setRenderHint(QPainter::Antialiasing);
      editor->scene().render
        (&painter, QRectF(), editor->scene().itemsBoundingRect());
    }
  } else {
    if(QPrintDialog(&printer).exec() != QDialog::Accepted)
      return;

    // RK workplane_->const_plot().print(printer);
  }
}


void QSapecNGWindow::props()
{
  SchematicEditor* editor = activeEditor();

  if(editor) {
    QList<QGraphicsItem*> items = editor->scene().selectedItems();
    QtButtonPropertyBrowser* browser = new QtButtonPropertyBrowser;
    editor->scene().initializeBrowser(browser);

    foreach(QGraphicsItem* item, items)
      if(SchematicScene::itemProperties(item))
        browser->addProperty(SchematicScene::itemProperties(item));

    QDialog* dialog = new QDialog(this, Qt::Dialog);
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttons, SIGNAL(accepted()), dialog, SLOT(accept()));

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addWidget(browser);
    layout->addWidget(buttons);

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setLayout(layout);
    dialog->exec();
  }
}


void QSapecNGWindow::bin()
{
  SchematicEditor* editor = activeEditor();

  if(editor)
    editor->scene().binSelectedItems();
}


void QSapecNGWindow::cut()
{
  SchematicEditor* editor = activeEditor();

  if(editor)
    editor->scene().cutSelectedItems();
}


void QSapecNGWindow::copy()
{
  SchematicEditor* editor = activeEditor();

  if(editor)
    editor->scene().copySelectedItems();
}


void QSapecNGWindow::paste()
{
  SchematicEditor* editor = activeEditor();

  if(editor)
    editor->scene().pasteItems();
}


void QSapecNGWindow::rotate()
{
  SchematicEditor* editor = activeEditor();

  if(editor)
    editor->scene().rotateSelectedItems();
}


void QSapecNGWindow::mirror()
{
  SchematicEditor* editor = activeEditor();

  if(editor)
    editor->scene().mirrorSelectedItems();
}


void QSapecNGWindow::bringToFront()
{
  SchematicEditor* editor = activeEditor();

  if(editor)
    editor->scene().bringToFrontSelectedItem();
}


void QSapecNGWindow::sendToBack()
{
  SchematicEditor* editor = activeEditor();

  if(editor)
    editor->scene().sendToBackSelectedItem();
}


void QSapecNGWindow::license()
{
  QString title = tr("License");
  QString body =
    "<b>CRIMSON Boundary Condition Toolbox</b>,<br>"
    "Graphical design of boundary conditions for blood flow simulation.<br>"
    "Copyright (C) 2015, King's College London.<br>"
    "Developed by Christopher J. Arthurs <a href=\"mailto:christopher.arthurs@kcl.ac.uk\">christopher.arthurs@kcl.ac.uk</a>.<br>"
    "<br>"
    "This program is free software: you can redistribute it and/or modify "
    "it under the terms of the GNU General Public License as published by "
    "the Free Software Foundation, either version 3 of the License, or "
    "(at your option) any later version.<br>"
    "<br>"
    "This program is distributed in the hope that it will be useful, "
    "but WITHOUT ANY WARRANTY; without even the implied warranty of "
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
    "GNU General Public License for more details.<br>"
    "<br>"
    "You should have received a copy of the GNU General Public License "
    "along with this program.  If not, see <http://www.gnu.org/licenses/>.<br>"
    "<br>"
    "<b>This file incorporates work covered by the following copyright and "
    "permission notice:</b>"
    "<br>"
    "<br>"
    "QSapecNG, Qt-based GUI for SapecNG<br>"
    "Copyright (C) 2009-2011, Michele Caini<br>"
    "<br>"
    "This program is free software: you can redistribute it and/or modify "
    "it under the terms of the GNU General Public License as published by "
    "the Free Software Foundation, either version 3 of the License, or "
    "(at your option) any later version.<br>"
    "<br>"
    "This program is distributed in the hope that it will be useful, "
    "but WITHOUT ANY WARRANTY; without even the implied warranty of "
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
    "GNU General Public License for more details.<br>"
    "<br>"
    "You should have received a copy of the GNU General Public License "
    "along with this program.  If not, see "
    "<a href=\"http://www.gnu.org/licenses/\">http://www.gnu.org/licenses/</a>.";

  QMessageBox::about(this, title, body);
}


void QSapecNGWindow::about()
{
  QMessageBox::about(this, tr("QSapecNG"),
    QString("%1%2%3")
      .arg(tr("QSapecNG: Qt-based GUI for SapecNG\n\n"))
      .arg(tr("QSapecNG is based in part on the work of "))
      .arg(tr("the Qwt project (http://qwt.sf.net)."))
  );
}


void QSapecNGWindow::activated(const QModelIndex& index)
{
  SchematicEditor* editor = activeEditor();

  if(editor)
    editor->scene().setActiveItem(
        (SchematicScene::SupportedItemType)
        sideBarModel_->data(index, Qt::UserRole).toInt()
      );
}


void QSapecNGWindow::sceneSelectionChanged()
{
  bool hasSelectedItems = false;
  SchematicEditor* editor = activeEditor();

  if(editor)
    hasSelectedItems =
      (editor->scene().selectedItems().size() != 0);

  propsAct_->setEnabled(hasSelectedItems);
  binAct_->setEnabled(hasSelectedItems);
  cutAct_->setEnabled(hasSelectedItems);
  copyAct_->setEnabled(hasSelectedItems);
  rotateAct_->setEnabled(hasSelectedItems);
  mirrorAct_->setEnabled(hasSelectedItems);
  bringToFrontAct_->setEnabled(hasSelectedItems);
  sendToBackAct_->setEnabled(hasSelectedItems);
}


void QSapecNGWindow::updateMenus()
{
  bool hasEditor = (activeEditor() != 0);
  // bool hasWorkplane = (central_->currentWidget() == workplane_);

  saveAct_->setEnabled(hasEditor);
  writeSimulationFilesAct_->setEnabled(hasEditor);
  saveAsAct_->setEnabled(hasEditor);
  printAct_->setEnabled(hasEditor);

  pasteAct_->setEnabled(hasEditor);
  if(!hasEditor)
    undoRedoGroup_->setActiveStack(0);

  zoomInAct_->setEnabled(hasEditor);
  zoomOutAct_->setEnabled(hasEditor);
  zoomNormalAct_->setEnabled(hasEditor);
  fitAct_->setEnabled(hasEditor);

  nodeAct_->setEnabled(hasEditor);
  // resolveAct_->setEnabled(hasEditor);
  // waveAct_->setEnabled(hasEditor);

  cursorAct_->setEnabled(hasEditor);
  // userdefAct_->setEnabled(hasEditor);
  connectedWireSessionAct_->setEnabled(hasEditor);
  disconnectedWireSessionAct_->setEnabled(hasEditor);
  groundSessionAct_->setEnabled(hasEditor);
  // portSessionAct_->setEnabled(hasEditor);
  // outSessionAct_->setEnabled(hasEditor);
  // voltmeterSessionAct_->setEnabled(hasEditor);
  // ammeterSessionAct_->setEnabled(hasEditor);
  labelSessionAct_->setEnabled(hasEditor);

  closeAct_->setEnabled(hasEditor);
  closeAllAct_->setEnabled(hasEditor);
  tileAct_->setEnabled(hasEditor);
  cascadeAct_->setEnabled(hasEditor);
  nextAct_->setEnabled(hasEditor);
  previousAct_->setEnabled(hasEditor);

  // magnitudeAct_->setEnabled(hasWorkplane);
  // magnitudeRadAct_->setEnabled(hasWorkplane);
  // phaseAct_->setEnabled(hasWorkplane);
  // phaseRadAct_->setEnabled(hasWorkplane);
  // gainAct_->setEnabled(hasWorkplane);
  // gainRadAct_->setEnabled(hasWorkplane);
  // lossAct_->setEnabled(hasWorkplane);
  // lossRadAct_->setEnabled(hasWorkplane);
  // zerosPolesAct_->setEnabled(hasWorkplane);

  // xLogAct_->setEnabled(hasWorkplane);
  // yLogAct_->setEnabled(hasWorkplane);
  // replotAct_->setEnabled(hasWorkplane);
}


void QSapecNGWindow::updateWindowMenu()
{
  windowMenu_->clear();
  windowMenu_->addMenu(showMenu_);
  windowMenu_->addSeparator();
  windowMenu_->addAction(closeAct_);
  windowMenu_->addAction(closeAllAct_);
  windowMenu_->addSeparator();
  windowMenu_->addAction(tileAct_);
  windowMenu_->addAction(cascadeAct_);
  windowMenu_->addSeparator();
  windowMenu_->addAction(nextAct_);
  windowMenu_->addAction(previousAct_);
  windowMenu_->addAction(separatorAct_);

  QList<QMdiSubWindow*> windows = mdiArea_->subWindowList();
  separatorAct_->setVisible(!windows.isEmpty());

  for(int i = 0; i < windows.size(); ++i) {
    SchematicEditor* child = qobject_cast<SchematicEditor*>(windows.at(i));

    QString text;
    if(i < 9) {
      text = QString("&%1 %2").arg(i + 1)
        .arg(child->userFriendlyCurrentFile());
    } else {
      text = QString("%1 %2").arg(i + 1)
        .arg(child->userFriendlyCurrentFile());
    }
    QAction* action = windowMenu_->addAction(text);
    action->setCheckable(true);
    action->setChecked(child == activeEditor());
    connect(action, SIGNAL(triggered()), windowMapper_, SLOT(map()));
    windowMapper_->setMapping(action, windows.at(i));
  }
}


void QSapecNGWindow::updatePropertyBrowser()
{
  bool hasEditor = (activeEditor() != 0);

  treeBrowser_->clear();
  if(hasEditor)
    treeBrowser_->addProperty(activeEditor()->scene().properties());
}


void QSapecNGWindow::updateDocks()
{
  bool hasEditor = (activeEditor() != 0);

  sideBarDock_->widget()->setEnabled(hasEditor);
  undoRedoDock_->widget()->setEnabled(hasEditor);
}


void QSapecNGWindow::createLogger()
{
  policy_ = new QLogPolicy(this);
  policy_->setDebugColor(Qt::black);
  policy_->setInfoColor(Qt::black);
  policy_->setWarningColor(Qt::blue);
  policy_->setErrorColor(Qt::red);

  QLogger::setLevel(sapecng::Logger::DEBUG);
  QLogger::setPolicy(policy_);
}


void QSapecNGWindow::createActions()
{
  newAct_ = new QAction(QIcon(":/images/new.png"), tr("&New"), this);
  newAct_->setShortcuts(QKeySequence::New);
  newAct_->setStatusTip(tr("New file"));
  connect(newAct_, SIGNAL(triggered()), this, SLOT(newFile()));

  openAct_ = new QAction(QIcon(":/images/open.png"), tr("&Open..."), this);
  openAct_->setShortcuts(QKeySequence::Open);
  openAct_->setStatusTip(tr("Open"));
  connect(openAct_, SIGNAL(triggered()), this, SLOT(open()));

  saveAct_ = new QAction(QIcon(":/images/save.png"), tr("&Save"), this);
  saveAct_->setShortcuts(QKeySequence::Save);
  saveAct_->setStatusTip(tr("Save"));
  connect(saveAct_, SIGNAL(triggered()), this, SLOT(save()));

  writeSimulationFilesAct_ = new QAction(QIcon(":/images/save.png"), tr("Write Simulation Files"), this);
  // writeSimulationFilesAct_->setShortcuts(QKeySequence::Save);
  writeSimulationFilesAct_->setStatusTip(tr("Write Simulation Files"));
  connect(writeSimulationFilesAct_, SIGNAL(triggered()), this, SLOT(writeSimulationFiles()));

  saveAsAct_ = new QAction(tr("Save &As..."), this);
  saveAsAct_->setShortcuts(QKeySequence::SaveAs);
  saveAsAct_->setStatusTip(tr("Save As..."));
  connect(saveAsAct_, SIGNAL(triggered()), this, SLOT(saveAs()));

  printAct_ = new QAction(QIcon(":/images/print.png"), tr("&Print"), this);
  printAct_->setShortcuts(QKeySequence::Print);
  printAct_->setStatusTip(tr("Print"));
  connect(printAct_, SIGNAL(triggered()), this, SLOT(print()));

  quitAct_ = new QAction(QIcon(":/images/cross.png"), tr("&Quit"), this);
  quitAct_->setShortcut(tr("Ctrl+Q"));
  quitAct_->setStatusTip(tr("Quit"));
  connect(quitAct_, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));

  binAct_ = new QAction(QIcon(":/images/bin.png"), tr("&Delete"), this);
  binAct_->setShortcut(QKeySequence::Delete);
  binAct_->setStatusTip(tr("Delete"));
  connect(binAct_, SIGNAL(triggered()), this, SLOT(bin()));

  undoAct_ = new QAction(QIcon(":/images/undo.png"), tr("&Undo"), this);
  undoAct_->setShortcut(QKeySequence::Undo);
  undoAct_->setStatusTip(tr("Undo"));
  undoAct_->setEnabled(undoRedoGroup_->canUndo());
  connect(undoAct_, SIGNAL(triggered()), undoRedoGroup_, SLOT(undo()));
  connect(
      undoRedoGroup_, SIGNAL(canUndoChanged(bool)),
      undoAct_, SLOT(setEnabled(bool))
    );

  redoAct_ = new QAction(QIcon(":/images/redo.png"), tr("&Redo"), this);
  redoAct_->setShortcut(QKeySequence::Redo);
  redoAct_->setStatusTip(tr("Redo"));
  redoAct_->setEnabled(undoRedoGroup_->canRedo());
  connect(redoAct_, SIGNAL(triggered()), undoRedoGroup_, SLOT(redo()));
  connect(
      undoRedoGroup_, SIGNAL(canRedoChanged(bool)),
      redoAct_, SLOT(setEnabled(bool))
    );

  propsAct_ = new QAction(tr("Properties..."), this);
  propsAct_->setStatusTip(tr("Properties..."));
  connect(propsAct_, SIGNAL(triggered()), this, SLOT(props()));

  cutAct_ = new QAction(QIcon(":/images/cut.png"), tr("Cu&t"), this);
  cutAct_->setShortcut(QKeySequence::Cut);
  cutAct_->setStatusTip(tr("Cut"));
  connect(cutAct_, SIGNAL(triggered()), this, SLOT(cut()));

  copyAct_ = new QAction(QIcon(":/images/copy.png"), tr("&Copy"), this);
  copyAct_->setShortcut(QKeySequence::Copy);
  copyAct_->setStatusTip(tr("Copy"));
  connect(copyAct_, SIGNAL(triggered()), this, SLOT(copy()));

  pasteAct_ = new QAction(QIcon(":/images/paste.png"), tr("&Paste"), this);
  pasteAct_->setShortcut(QKeySequence::Paste);
  pasteAct_->setStatusTip(tr("Paste"));
  connect(pasteAct_, SIGNAL(triggered()), this, SLOT(paste()));

  rotateAct_ = new QAction(QIcon(":/images/rotate.png"), tr("R&otate"), this);
  rotateAct_->setStatusTip(tr("Rotate"));
  connect(rotateAct_, SIGNAL(triggered()), this, SLOT(rotate()));

  mirrorAct_ = new QAction(QIcon(":/images/mirror.png"), tr("&Mirror"), this);
  mirrorAct_->setStatusTip(tr("Mirror"));
  connect(mirrorAct_, SIGNAL(triggered()), this, SLOT(mirror()));

  bringToFrontAct_ =
    new QAction(QIcon(":/images/bringfront.png"), tr("Bring to &front"), this);
  bringToFrontAct_->setStatusTip(tr("Bring to front"));
  connect(bringToFrontAct_, SIGNAL(triggered()), this, SLOT(bringToFront()));

  sendToBackAct_ =
    new QAction(QIcon(":/images/sendback.png"), tr("Send to &back"), this);
  sendToBackAct_->setStatusTip(tr("Send to back"));
  connect(sendToBackAct_, SIGNAL(triggered()), this, SLOT(sendToBack()));

  zoomInAct_ = new QAction(QIcon(":/images/zoomin.png"), tr("Zoom &In"), this);
  zoomInAct_->setShortcuts(QKeySequence::ZoomIn);
  zoomInAct_->setStatusTip(tr("Zoom In"));
  connect(zoomInAct_, SIGNAL(triggered()), this, SLOT(zoomIn()));

  zoomOutAct_ = new QAction(QIcon(":/images/zoomout.png"), tr("Zoom &Out"), this);
  zoomOutAct_->setShortcuts(QKeySequence::ZoomOut);
  zoomOutAct_->setStatusTip(tr("Zoom Out"));
  connect(zoomOutAct_, SIGNAL(triggered()), this, SLOT(zoomOut()));

  zoomNormalAct_ = new QAction(QIcon(":/images/zoomnormal.png"), tr("Zoom &Normal"), this);
  zoomNormalAct_->setStatusTip(tr("Zoom Normal"));
  connect(zoomNormalAct_, SIGNAL(triggered()), this, SLOT(zoomNormal()));

  fitAct_ = new QAction(QIcon(":/images/fit.png"), tr("&Fit"), this);
  fitAct_->setStatusTip(tr("Fit"));
  connect(fitAct_, SIGNAL(triggered()), this, SLOT(fit()));

  nodeAct_ = new QAction(QIcon(":/images/node.png"), tr("Assign &nodes"), this);
  nodeAct_->setShortcut(tr("F7"));
  nodeAct_->setStatusTip(tr("Assign nodes"));
  connect(nodeAct_, SIGNAL(triggered()), this, SLOT(assignNodes()));

  // resolveAct_ = new QAction(QIcon(":/images/resolve.png"), tr("&Resolve..."), this);
  // resolveAct_->setShortcut(tr("F8"));
  // resolveAct_->setStatusTip(tr("Resolve"));
  // connect(resolveAct_, SIGNAL(triggered()), this, SLOT(resolve()));

  // waveAct_ = new QAction(QIcon(":/images/wave.png"), tr("&Workplane..."), this);
  // waveAct_->setShortcut(tr("F9"));
  // waveAct_->setStatusTip(tr("Workplane"));
  // connect(waveAct_, SIGNAL(triggered()), this, SLOT(wave()));

  toggleScreenModeAct_ = new QAction(tr("F&ull Screen Mode"), this);
  toggleScreenModeAct_->setStatusTip(tr("Switch to Full Screen Mode"));
  toggleScreenModeAct_->setCheckable(true);
  connect(toggleScreenModeAct_, SIGNAL(triggered()),
    this, SLOT(toggleScreenMode()));

  configureAppAct_ = new QAction(QIcon(":/images/configuration.png"),
    tr("&Configure QSapecNG..."), this);
  configureAppAct_->setStatusTip(tr("Configure QSapecNG..."));
  connect(configureAppAct_, SIGNAL(triggered()),
    this, SLOT(configDialog()));

  closeAct_ = new QAction(tr("&Close"), this);
  closeAct_->setShortcut(tr("Ctrl+F4"));
  closeAct_->setStatusTip(tr("Close active window"));
  connect(closeAct_, SIGNAL(triggered()),
    mdiArea_, SLOT(closeActiveSubWindow()));

  closeAllAct_ = new QAction(tr("Close &All"), this);
  closeAllAct_->setStatusTip(tr("Close all windows"));
  connect(closeAllAct_, SIGNAL(triggered()),
    mdiArea_, SLOT(closeAllSubWindows()));

  tileAct_ = new QAction(tr("&Tile"), this);
  tileAct_->setStatusTip(tr("Tile"));
  connect(tileAct_, SIGNAL(triggered()), mdiArea_, SLOT(tileSubWindows()));

  cascadeAct_ = new QAction(tr("Casca&de"), this);
  cascadeAct_->setStatusTip(tr("Cascade"));
  connect(cascadeAct_, SIGNAL(triggered()), mdiArea_, SLOT(cascadeSubWindows()));

  nextAct_ = new QAction(tr("&Next"), this);
  nextAct_->setShortcuts(QKeySequence::NextChild);
  nextAct_->setStatusTip(tr("Next window"));
  connect(nextAct_, SIGNAL(triggered()),
    mdiArea_, SLOT(activateNextSubWindow()));

  previousAct_ = new QAction(tr("&Previous"), this);
  previousAct_->setShortcuts(QKeySequence::PreviousChild);
  previousAct_->setStatusTip(tr("Previous window"));
  connect(previousAct_, SIGNAL(triggered()),
    mdiArea_, SLOT(activatePreviousSubWindow()));

  separatorAct_ = new QAction(this);
  separatorAct_->setSeparator(true);

  licenseAct_ = new QAction(QIcon(":/images/license.png"), tr("&License"), this);
  licenseAct_->setStatusTip(tr("Show License"));
  connect(licenseAct_, SIGNAL(triggered()), this, SLOT(license()));

  aboutAct_ = new QAction(QIcon(":/images/info.png"), tr("&About"), this);
  aboutAct_->setStatusTip(tr("About QSapecNG"));
  connect(aboutAct_, SIGNAL(triggered()), this, SLOT(about()));

  aboutQtAct_ = new QAction(QIcon(":/images/qt-logo.png"), tr("&Qt"), this);
  aboutQtAct_->setStatusTip(tr("About Qt"));
  connect(aboutQtAct_, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

  // userdefAct_ =
  //   new QAction(QIcon(":/images/symbols/userdef.png"), tr("&User def"), this);
  // userdefAct_->setStatusTip(tr("User def"));
  // connect(userdefAct_, SIGNAL(triggered()), this, SLOT(userDefRequested()));

  connectedWireSessionAct_ =
    new QAction(QIcon(":/images/symbols/wirecross.png"), tr("Wire c&ross"), this);
  connectedWireSessionAct_->setStatusTip(tr("Wire Cross Session Request"));
  connect(connectedWireSessionAct_, SIGNAL(triggered()),
    this, SLOT(connectedWireSessionRequested()));

  disconnectedWireSessionAct_ =
    new QAction(QIcon(":/images/symbols/wire.png"), tr("&Wire"), this);
  disconnectedWireSessionAct_->setStatusTip(tr("Wire Session Request"));
  connect(disconnectedWireSessionAct_, SIGNAL(triggered()),
    this, SLOT(disconnectedWireSessionRequested()));

  groundSessionAct_ =
    new QAction(QIcon(":/images/symbols/ground.png"), tr("&Ground"), this);
  groundSessionAct_->setStatusTip(tr("Ground Session Request"));
  connect(groundSessionAct_, SIGNAL(triggered()),
    this, SLOT(groundSessionRequested()));

  // portSessionAct_ =
  //   new QAction(QIcon(":/images/symbols/port.png"), tr("&Port"), this);
  // portSessionAct_->setStatusTip(tr("Port Session Request"));
  // connect(portSessionAct_, SIGNAL(triggered()),
  //   this, SLOT(portSessionRequested()));

  // outSessionAct_ =
  //   new QAction(QIcon(":/images/symbols/out.png"), tr("&Out"), this);
  // outSessionAct_->setStatusTip(tr("Out Session Request"));
  // connect(outSessionAct_, SIGNAL(triggered()),
  //   this, SLOT(outSessionRequested()));

  // voltmeterSessionAct_ =
  //   new QAction(QIcon(":/images/symbols/voltmeter.png"), tr("&Voltmeter"), this);
  // voltmeterSessionAct_->setStatusTip(tr("Voltmeter Session Request"));
  // connect(voltmeterSessionAct_, SIGNAL(triggered()),
  //   this, SLOT(voltmeterSessionRequested()));

  // ammeterSessionAct_ =
  //   new QAction(QIcon(":/images/symbols/ammeter.png"), tr("&Ammeter"), this);
  // ammeterSessionAct_->setStatusTip(tr("Ammeter Session Request"));
  // connect(ammeterSessionAct_, SIGNAL(triggered()),
  //   this, SLOT(ammeterSessionRequested()));

  labelSessionAct_ =
    new QAction(QIcon(":/images/label.png"), tr("&Label"), this);
  labelSessionAct_->setStatusTip(tr("Insert Label"));
  connect(labelSessionAct_, SIGNAL(triggered()),
    this, SLOT(labelSessionRequested()));

  cursorAct_ = new QAction(QIcon(":/images/cursor.png"), tr("&Cursor"), this);
  cursorAct_->setStatusTip(tr("Clear pending request"));
  connect(cursorAct_, SIGNAL(triggered()), this, SLOT(clearSessionRequested()));

  toggleGridAct_ =
    new QAction(QIcon(":/images/grid.png"), tr("&Toggle grid"), this);
  toggleGridAct_->setStatusTip(tr("Toggle grid"));
  toggleGridAct_->setCheckable(true);
  connect(toggleGridAct_, SIGNAL(triggered(bool)),
    this, SIGNAL(toggleGrid(bool)));
  toggleGridAct_->setChecked(true);

  // magnitudeAct_ = new QAction(tr("&Magnitude"), this);
  // magnitudeAct_->setStatusTip(tr("Magnitude function"));
  // connect(magnitudeAct_, SIGNAL(triggered()), fMapper_, SLOT(map()));
  // fMapper_->setMapping(magnitudeAct_, WorkPlane::MAGNITUDE);

  // magnitudeRadAct_ = new QAction(tr("Magnitude (rad/s)"), this);
  // magnitudeRadAct_->setStatusTip(tr("Magnitude function"));
  // connect(magnitudeRadAct_, SIGNAL(triggered()), fMapper_, SLOT(map()));
  // fMapper_->setMapping(magnitudeRadAct_, WorkPlane::MAGNITUDE_RAD);

  // phaseAct_ = new QAction(tr("&Phase"), this);
  // phaseAct_->setStatusTip(tr("Phase function"));
  // connect(phaseAct_, SIGNAL(triggered()), fMapper_, SLOT(map()));
  // fMapper_->setMapping(phaseAct_, WorkPlane::PHASE);

  // phaseRadAct_ = new QAction(tr("Phase (rad/s)"), this);
  // phaseRadAct_->setStatusTip(tr("Phase function"));
  // connect(phaseRadAct_, SIGNAL(triggered()), fMapper_, SLOT(map()));
  // fMapper_->setMapping(phaseRadAct_, WorkPlane::PHASE_RAD);

  // gainAct_ = new QAction(tr("&Gain"), this);
  // gainAct_->setStatusTip(tr("Gain function"));
  // connect(gainAct_, SIGNAL(triggered()), fMapper_, SLOT(map()));
  // fMapper_->setMapping(gainAct_, WorkPlane::GAIN);

  // gainRadAct_ = new QAction(tr("Gain (rad/s)"), this);
  // gainRadAct_->setStatusTip(tr("Gain function"));
  // connect(gainRadAct_, SIGNAL(triggered()), fMapper_, SLOT(map()));
  // fMapper_->setMapping(gainRadAct_, WorkPlane::GAIN_RAD);

  // lossAct_ = new QAction(tr("&Loss"), this);
  // lossAct_->setStatusTip(tr("Loss function"));
  // connect(lossAct_, SIGNAL(triggered()), fMapper_, SLOT(map()));
  // fMapper_->setMapping(lossAct_, WorkPlane::LOSS);

  // lossRadAct_ = new QAction(tr("Loss (rad/s)"), this);
  // lossRadAct_->setStatusTip(tr("Loss function"));
  // connect(lossRadAct_, SIGNAL(triggered()), fMapper_, SLOT(map()));
  // fMapper_->setMapping(lossRadAct_, WorkPlane::LOSS_RAD);

  // zerosPolesAct_ = new QAction(tr("&Zeros/Poles"), this);
  // zerosPolesAct_->setStatusTip(tr("Zero/Poles function"));
  // connect(zerosPolesAct_, SIGNAL(triggered()), fMapper_, SLOT(map()));
  // fMapper_->setMapping(zerosPolesAct_, WorkPlane::ZEROS);

  // xLogAct_ = new QAction(tr("&x-axis log scale"), this);
  // xLogAct_->setStatusTip(tr("Toggle x-scale"));
  // xLogAct_->setCheckable(true);
  // connect(xLogAct_, SIGNAL(triggered(bool)), this, SLOT(xAxisLogScale(bool)));
  // xLogAct_->setChecked(true);

  // yLogAct_ = new QAction(tr("&y-axis log scale"), this);
  // yLogAct_->setStatusTip(tr("Toggle y-scale"));
  // yLogAct_->setCheckable(true);
  // connect(yLogAct_, SIGNAL(triggered(bool)), this, SLOT(yAxisLogScale(bool)));
  // yLogAct_->setChecked(false);

  // replotAct_ = new QAction(tr("&Replot"), this);
  // replotAct_->setStatusTip(tr("Replot function"));
  // connect(replotAct_, SIGNAL(triggered()), this, SLOT(replot()));

  for(int i = 0; i < maxRecentFiles; ++i) {
    openRecentActs_[i] = new QAction(this);
    openRecentActs_[i]->setVisible(false);
    connect(openRecentActs_[i], SIGNAL(triggered()), this, SLOT(openRecent()));
  }
}


void QSapecNGWindow::createDockWidgets()
{
  setDockOptions(QMainWindow::AllowTabbedDocks);
  setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::North);
  setTabPosition(Qt::RightDockWidgetArea, QTabWidget::North);
  setTabPosition(Qt::BottomDockWidgetArea, QTabWidget::South);

  sideBarDock_ = new QDockWidget(tr("Components"), this);
  sideBarDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  sideBarDock_->setTitleBarWidget(0);
  createSideBarDockLayout();
  sideBarDock_->setFocusPolicy(Qt::NoFocus);

  propertyDock_ = new QDockWidget(tr("Properties"), this);
  propertyDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  propertyDock_->setTitleBarWidget(0);
  createPropertyDockLayout();

  messageDock_ = new QDockWidget(tr("Messages"), this);
  messageDock_->setAllowedAreas(Qt::BottomDockWidgetArea);
  messageDock_->setTitleBarWidget(0);
  createMessageDockLayout();

  undoRedoDock_ = new QDockWidget(tr("Undo/Redo"), this);
  undoRedoDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  undoRedoDock_->setTitleBarWidget(0);
  createUndoRedoDockLayout();

  workspaceDock_ = new QDockWidget(tr("Workspace"), this);
  workspaceDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  workspaceDock_->setTitleBarWidget(0);
  createWorkspaceDockLayout();

  addDockWidget(Qt::LeftDockWidgetArea, workspaceDock_);
  addDockWidget(Qt::LeftDockWidgetArea, sideBarDock_);
  addDockWidget(Qt::LeftDockWidgetArea, propertyDock_);
  addDockWidget(Qt::BottomDockWidgetArea, messageDock_);
  addDockWidget(Qt::RightDockWidgetArea, undoRedoDock_);
  tabifyDockWidget(sideBarDock_, propertyDock_);
}


void QSapecNGWindow::createMenus()
{
  fileMenu_ = menuBar()->addMenu(tr("&File"));
  fileMenu_->addAction(newAct_);
  fileMenu_->addAction(openAct_);
  recentMenu_ = fileMenu_->addMenu(QIcon(":/images/open.png"), tr("Open &Recent"));
  for(int i = 0; i < maxRecentFiles; ++i)
    recentMenu_->addAction(openRecentActs_[i]);
  fileMenu_->addAction(saveAct_);
  fileMenu_->addAction(saveAsAct_);
  fileMenu_->addAction(writeSimulationFilesAct_);
  fileMenu_->addSeparator();
  fileMenu_->addAction(printAct_);
  fileMenu_->addSeparator();
  fileMenu_->addAction(quitAct_);

  editMenu_ = menuBar()->addMenu(tr("&Edit"));
  editMenu_->addAction(undoAct_);
  editMenu_->addAction(redoAct_);
  editMenu_->addSeparator();
  editMenu_->addAction(propsAct_);
  editMenu_->addSeparator();
  editMenu_->addAction(cutAct_);
  editMenu_->addAction(copyAct_);
  editMenu_->addAction(pasteAct_);
  editMenu_->addSeparator();
  editMenu_->addAction(rotateAct_);
  editMenu_->addAction(mirrorAct_);
  editMenu_->addAction(bringToFrontAct_);
  editMenu_->addAction(sendToBackAct_);
  editMenu_->addSeparator();
  editMenu_->addAction(binAct_);

  viewMenu_ = menuBar()->addMenu(tr("&View"));
  viewMenu_->addAction(zoomInAct_);
  viewMenu_->addAction(zoomOutAct_);
  viewMenu_->addAction(zoomNormalAct_);
  viewMenu_->addSeparator();
  viewMenu_->addAction(fitAct_);

  runMenu_ = menuBar()->addMenu(tr("&Run"));
  runMenu_->addAction(nodeAct_);
  // runMenu_->addAction(resolveAct_);
  // runMenu_->addAction(waveAct_);

  // workplaneMenu_ = menuBar()->addMenu(tr("W&orkPlane"));
  // workplaneMenu_->addAction(magnitudeAct_);
  // workplaneMenu_->addAction(magnitudeRadAct_);
  // workplaneMenu_->addAction(phaseAct_);
  // workplaneMenu_->addAction(phaseRadAct_);
  // workplaneMenu_->addAction(gainAct_);
  // workplaneMenu_->addAction(gainRadAct_);
  // workplaneMenu_->addAction(lossAct_);
  // workplaneMenu_->addAction(lossRadAct_);
  // workplaneMenu_->addAction(zerosPolesAct_);
  // workplaneMenu_->addSeparator();
  // workplaneMenu_->addAction(xLogAct_);
  // workplaneMenu_->addAction(yLogAct_);
  // workplaneMenu_->addSeparator();
  // workplaneMenu_->addAction(replotAct_);

  toolMenu_ = menuBar()->addMenu(tr("&Tools"));
  toolMenu_->addAction(cursorAct_);
  toolMenu_->addAction(labelSessionAct_);
  toolMenu_->addSeparator();
  // toolMenu_->addAction(userdefAct_);
  toolMenu_->addAction(connectedWireSessionAct_);
  toolMenu_->addAction(disconnectedWireSessionAct_);
  toolMenu_->addAction(groundSessionAct_);
  // toolMenu_->addAction(portSessionAct_);
  // toolMenu_->addAction(outSessionAct_);
  // toolMenu_->addAction(voltmeterSessionAct_);
  // toolMenu_->addAction(ammeterSessionAct_);
  toolMenu_->addSeparator();
  toolMenu_->addAction(toggleGridAct_);

  settingMenu_ = menuBar()->addMenu(tr("&Settings"));
  toolbarMenu_ = settingMenu_->addMenu(tr("Toolbars"));
  toolbarMenu_->addAction(fileToolBar_->toggleViewAction());
  toolbarMenu_->addAction(viewToolBar_->toggleViewAction());
  toolbarMenu_->addAction(runToolBar_->toggleViewAction());
  toolbarMenu_->addAction(toolToolBar_->toggleViewAction());
  settingMenu_->addAction(toggleScreenModeAct_);
  settingMenu_->addSeparator();
  settingMenu_->addAction(configureAppAct_);

  windowMenu_ = menuBar()->addMenu(tr("&Window"));
  showMenu_ = windowMenu_->addMenu(tr("&Show"));
  updateWindowMenu();
  connect(windowMenu_, SIGNAL(aboutToShow()), this, SLOT(updateWindowMenu()));
  showMenu_->addAction(sideBarDock_->toggleViewAction());
  showMenu_->addAction(propertyDock_->toggleViewAction());
  showMenu_->addAction(messageDock_->toggleViewAction());
  showMenu_->addAction(undoRedoDock_->toggleViewAction());
  showMenu_->addAction(workspaceDock_->toggleViewAction());

  menuBar()->addSeparator();

  helpMenu_ = menuBar()->addMenu(tr("&Help"));
  helpMenu_->addAction(licenseAct_);
  helpMenu_->addSeparator();
  helpMenu_->addAction(aboutAct_);
  helpMenu_->addAction(aboutQtAct_);

  sceneMenu_ = new QMenu(this);
  sceneMenu_->addMenu(editMenu_);
  sceneMenu_->addMenu(viewMenu_);
  sceneMenu_->addMenu(runMenu_);
  sceneMenu_->addMenu(toolMenu_);
}


void QSapecNGWindow::createToolBars()
{
  fileToolBar_ = addToolBar(tr("File"));
  fileToolBar_->addAction(newAct_);
  fileToolBar_->addAction(openAct_);
  fileToolBar_->addAction(saveAct_);
  fileToolBar_->addAction(writeSimulationFilesAct_);
  fileToolBar_->addSeparator();
  fileToolBar_->addAction(printAct_);

  viewToolBar_ = addToolBar(tr("View"));
  viewToolBar_->addAction(zoomInAct_);
  viewToolBar_->addAction(zoomOutAct_);
  viewToolBar_->addAction(zoomNormalAct_);
  viewToolBar_->addSeparator();
  viewToolBar_->addAction(fitAct_);

  runToolBar_ = addToolBar(tr("Run"));
  runToolBar_->addAction(nodeAct_);
  // runToolBar_->addAction(resolveAct_);
  // runToolBar_->addAction(waveAct_);

  toolToolBar_ = addToolBar(tr("Tools"));
  toolToolBar_->addAction(cursorAct_);
  toolToolBar_->addAction(labelSessionAct_);
  toolToolBar_->addSeparator();
  // toolToolBar_->addAction(userdefAct_);
  toolToolBar_->addAction(connectedWireSessionAct_);
  toolToolBar_->addAction(disconnectedWireSessionAct_);
  toolToolBar_->addAction(groundSessionAct_);
  // toolToolBar_->addAction(portSessionAct_);
  // toolToolBar_->addAction(outSessionAct_);
  // toolToolBar_->addAction(voltmeterSessionAct_);
  // toolToolBar_->addAction(ammeterSessionAct_);
  toolToolBar_->addSeparator();
  toolToolBar_->addAction(toggleGridAct_);
}


void QSapecNGWindow::createStatusBar()
{
  statusBar()->showMessage(tr("Ready"));
  
  progressBar_ = new QProgressBar;
  statusBar()->insertPermanentWidget(0, progressBar_, 0);
  progressBar_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  progressBar_->reset();
}


void QSapecNGWindow::createSideBarDockLayout()
{
  sideBarModel_ = new SideBarModel(this);
  sideBarView_ = new SideBarView;
  sideBarView_->setSpacing(6.0);
  sideBarView_->setModel(sideBarModel_);
  sideBarView_->setFont(QFont("Times", 10, QFont::Light));
  sideBarDock_->setWidget(sideBarView_);

  connect(sideBarView_, SIGNAL(activated(const QModelIndex&)),
    this, SLOT(activated(const QModelIndex&)));
}


void QSapecNGWindow::createPropertyDockLayout()
{
  treeBrowser_ = new QtTreePropertyBrowser(this);
  treeBrowser_->setAlternatingRowColors(true);
  treeBrowser_->setResizeMode(QtTreePropertyBrowser::Interactive);
  treeBrowser_->setPropertiesWithoutValueMarked(true);
  treeBrowser_->setRootIsDecorated(true);
  propertyDock_->setWidget(treeBrowser_);
}


void QSapecNGWindow::createMessageDockLayout()
{
  logViewer_ = new QTextEdit;
  logViewer_->setAcceptRichText(false);
  logViewer_->setLineWrapMode(QTextEdit::NoWrap);
  logViewer_->setWordWrapMode(QTextOption::NoWrap);
  logViewer_->setFontPointSize(QApplication::font().pointSize() - 1);
  logViewer_->setReadOnly(true);
  logViewer_->setUndoRedoEnabled(false);
  logViewer_->setTextInteractionFlags(
        Qt::TextSelectableByMouse
      | Qt::TextSelectableByKeyboard
    );

  connect(policy_, SIGNAL(log(const QString&)),
    logViewer_, SLOT(append(const QString&)));
  connect(policy_, SIGNAL(textColorChanged(const QColor&)),
    logViewer_, SLOT(setTextColor(const QColor&)));

  messageDock_->setWidget(logViewer_);
}


void QSapecNGWindow::createUndoRedoDockLayout()
{
  undoRedoGroup_ = new QUndoGroup(this);
  undoRedoView_ = new QUndoView(undoRedoGroup_, this);
  undoRedoView_->setCleanIcon(QIcon(":/images/save.png"));

  undoRedoDock_->setWidget(undoRedoView_);
}


void QSapecNGWindow::createWorkspaceDockLayout()
{
  QPushButton* wpChange = new QPushButton("...");
  connect(wpChange, SIGNAL(clicked(bool)), this, SLOT(changeWorkspace()));

  workspace_ = new QLineEdit(Settings().workspace());
  workspace_->setReadOnly(true);

  wfsModel_ = new QFileSystemModel;
  wfsView_ = new QTreeView;

  QStringList filters;
  filters << "*.info";
  filters << "*.xml";

  wfsModel_->setNameFilters(filters);
  wfsModel_->setRootPath(workspace_->text());
  wfsModel_->setResolveSymlinks(false);

  wfsView_->setModel(wfsModel_);
  wfsView_->setDragEnabled(true);
  wfsView_->setDragDropMode(QAbstractItemView::DragOnly);
  wfsView_->setRootIndex(wfsModel_->index(wfsModel_->rootPath()));
  wfsView_->setSelectionMode(QAbstractItemView::SingleSelection);
  wfsView_->sortByColumn(0, Qt::AscendingOrder);
  wfsView_->setAlternatingRowColors(true);
  wfsView_->setSortingEnabled(true);

  QGridLayout* wpLayout = new QGridLayout;;
  wpLayout->addWidget(workspace_, 0, 0);
  wpLayout->addWidget(wpChange, 0, 1);
  wpLayout->addWidget(wfsView_, 1, 0, 1, 2);

  QWidget* widget = new QWidget;
  widget->setLayout(wpLayout);

  workspaceDock_->setWidget(widget);
}


void QSapecNGWindow::readSettings()
{
  Settings settings;

  QApplication::setFont(settings.appFont());

  move(settings.mwPos());
  resize(settings.mwSize());
  restoreState(settings.mwState());

  QLogger::setLevel((sapecng::Logger::LogLevel) settings.logLvl());
  policy_->setDebugColor(settings.debugColor());
  policy_->setInfoColor(settings.infoColor());
  policy_->setWarningColor(settings.warningColor());
  policy_->setErrorColor(settings.errorColor());
  policy_->setFatalColor(settings.fatalColor());
}


void QSapecNGWindow::writeSettings()
{
  SettingsManager manager;

  manager.setMWPos(pos());
  manager.setMWSize(size());
  manager.setMWState(saveState());
}


SchematicEditor* QSapecNGWindow::activeEditor()
{
  if(mdiArea_->activeSubWindow()) {
    SchematicEditor* editor =
      qobject_cast<SchematicEditor*>(mdiArea_->activeSubWindow());
    if(editor && !editor->isRunning())
      return editor;
  }

  return 0;
}


QMdiSubWindow* QSapecNGWindow::findEditor(const QString& fileName)
{
  QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

  foreach (QMdiSubWindow* window, mdiArea_->subWindowList()) {
    SchematicEditor* child = qobject_cast<SchematicEditor*>(window);
    if (child->currentFile() == canonicalFilePath)
      return window;
  }

  return 0;
}


SchematicEditor* QSapecNGWindow::createSchematicEditor()
{
  QApplication::setOverrideCursor(Qt::WaitCursor);

  SchematicEditor* editor = new SchematicEditor;
  setupSchematicEditor(editor);

  QApplication::restoreOverrideCursor();

  return editor;
}


void QSapecNGWindow::setupSchematicEditor(SchematicEditor* editor)
{
  editor->scene().initializeBrowser(treeBrowser_);
  editor->scene().setGridVisible(toggleGridAct_->isChecked());
  editor->scene().setContextMenu(sceneMenu_);

  undoRedoGroup_->addStack(editor->scene().undoRedoStack());
  editor->scene().undoRedoStack()->setActive(true);

  connect(editor, SIGNAL(stackEditor(SchematicEditor*)),
    this, SLOT(stackEditor(SchematicEditor*)));
  connect(&(editor->scene()), SIGNAL(selectionChanged()),
      this, SLOT(sceneSelectionChanged()));
  connect(this, SIGNAL(toggleGrid(bool)),
      &(editor->scene()), SLOT(setGridVisible(bool)));

  editor->setAttribute(Qt::WA_DeleteOnClose);
  editor->setWindowIcon(QIcon(":/images/grid.png"));
  mdiArea_->addSubWindow(editor, Qt::SubWindow);
}


void QSapecNGWindow::updateRecentFileList(const QString& fileName)
{
  QStringList files = Settings().recentFiles();

  files.removeAll(fileName);
  files.prepend(fileName);
  while (files.size() > maxRecentFiles)
    files.removeLast();

  SettingsManager().setRecentFiles(files);
  loadRecentFileList();
}


void QSapecNGWindow::loadRecentFileList()
{
  QStringList files = Settings().recentFiles();

  int numRecentFiles = qMin(files.size(), (int) maxRecentFiles);
  for (int i = 0; i < numRecentFiles; ++i) {
    QString text = QString("&%1 %2").arg(i + 1).arg(files[i]);
    openRecentActs_[i]->setText(text);
    openRecentActs_[i]->setData(files[i]);
    openRecentActs_[i]->setVisible(true);
  }
  for (int j = numRecentFiles; j < maxRecentFiles; ++j)
    openRecentActs_[j]->setVisible(false);
}


// void QSapecNGWindow::userDefRequested()
// {
//   SchematicEditor* editor = activeEditor();

//   if(editor)
//     editor->scene().setUserDefRequest();
// }


void QSapecNGWindow::connectedWireSessionRequested()
{
  SchematicEditor* editor = activeEditor();

  if(editor)
    editor->scene().setWireSessionRequest(true);
}


void QSapecNGWindow::disconnectedWireSessionRequested()
{
  SchematicEditor* editor = activeEditor();

  if(editor)
    editor->scene().setWireSessionRequest(false);
}


void QSapecNGWindow::groundSessionRequested()
{
  SchematicEditor* editor = activeEditor();

  if(editor)
    editor->scene().setActiveItem(SchematicScene::GroundItemType);
}


// void QSapecNGWindow::portSessionRequested()
// {
//   SchematicEditor* editor = activeEditor();

//   if(editor)
//     editor->scene().setActiveItem(SchematicScene::PortItemType);
// }


// void QSapecNGWindow::outSessionRequested()
// {
//   SchematicEditor* editor = activeEditor();

//   if(editor)
//     editor->scene().setActiveItem(SchematicScene::OutItemType);
// }


// void QSapecNGWindow::voltmeterSessionRequested()
// {
//   SchematicEditor* editor = activeEditor();

//   if(editor)
//     editor->scene().setActiveItem(SchematicScene::VoltmeterItemType);
// }


// void QSapecNGWindow::ammeterSessionRequested()
// {
//   SchematicEditor* editor = activeEditor();

//   if(editor)
//     editor->scene().setActiveItem(SchematicScene::AmmeterItemType);
// }


void QSapecNGWindow::labelSessionRequested()
{
  SchematicEditor* editor = activeEditor();

  if(editor)
    editor->scene().createLabel();
}


void QSapecNGWindow::clearSessionRequested()
{
  SchematicEditor* editor = activeEditor();

  if(editor)
    editor->scene().resetStatus();
}


void QSapecNGWindow::setActiveSubWindow(QWidget* window)
{
  if (!window)
    return;

  mdiArea_->setActiveSubWindow(qobject_cast<QMdiSubWindow*>(window));
}


void QSapecNGWindow::subWindowActivated(QMdiSubWindow* window)
{
  progressBar_->setRange(0, 1);
  progressBar_->reset();

  if(window) {
    SchematicEditor* editor = qobject_cast<SchematicEditor*>(window);
    if(editor && editor->isRunning()) {
      progressBar_->setRange(0, 0);
      undoRedoGroup_->setActiveStack(0);
    }
  }
}


void QSapecNGWindow::stackEditor(SchematicEditor* editor)
{
  setupSchematicEditor(editor);
  central_->setCurrentWidget(mdiArea_);
  setActiveSubWindow(editor);
  editor->show();

  statusBar()->showMessage(tr("Component loaded"), 2000);
}


}
