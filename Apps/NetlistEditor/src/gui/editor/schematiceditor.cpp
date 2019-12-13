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


#include "model/metacircuit.h"
#include "parser/parser_factory.h"
#include "parser/crc_circuit.h"

#include "QtProperty"
#include "gui/editor/schematiceditor.h"
#include "gui/editor/schematicsceneparser.h"
#include "gui/settings.h"
#include "gui/qlogger.h"

#include <QtCore/QPointer>
#include <QtCore/QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QUndoStack>
#include <QApplication>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QFocusEvent>

#include <fstream>
#include <sstream>
#include <map>

#include "utility/qsapecngUtility.h"
#include "utility/boilerplateScripts.h"


namespace qsapecng
{


SchematicEditor::SchematicEditor(QWidget* parent, Qt::WindowFlags flags)
  : QMdiSubWindow(parent, flags), solved_(false)
{
  scene_ = new SchematicScene(this);
  init();
}


SchematicEditor::SchematicEditor(
    SchematicScene& scene,
    QWidget* parent,
    Qt::WindowFlags flags
  ) : QMdiSubWindow(parent, flags), solved_(false)
{
  scene_ = &scene;
  init();
}


QString SchematicEditor::userFriendlyCurrentFile() const
{
  return strippedName(curFile_);
}


QString SchematicEditor::currentFile() const
{
  return curFile_;
}


// bool SchematicEditor::accept(WorkPlane& workplane)
// {
//   if(!solved_) {
//     QMessageBox::StandardButton ret;
//     ret = QMessageBox::warning(this, tr("Dirty circuit"),
//       QString("'%1'").arg(userFriendlyCurrentFile()
//         + tr(" has been modified.\n Solve the circuit?")),
//       QMessageBox::Yes | QMessageBox::No
//         | QMessageBox::Cancel
//     );

//     if(ret == QMessageBox::Cancel)
//       return false;

//     if(ret == QMessageBox::Yes) {
//       solve();
//       return false;
//     }
//   }

//   std::map<std::string, double> values;
//   QList<Item*> items = scene_->activeItems();
//   while(!items.isEmpty()) {
//     Item* item = items.takeFirst();
    
//     if(SchematicScene::itemType(item) == SchematicScene::UserDefItemType) {
//       items.append(item->data(101)
//         .value< QPointer<qsapecng::SchematicScene> >()->activeItems());
//     } else {
//       if(SchematicScene::itemProperties(item)) {
//         QtProperty* props = SchematicScene::itemProperties(item);
//         QHash<QString, QString> subs;
        
//         if(props) {
//           subs.insert("__NAME", props->valueText());
//           foreach(QtProperty* prop, props->subProperties())
//             subs.insert(prop->propertyName(), prop->valueText());
//         }

//         if(subs.contains("Value"))
//           values[subs.value("__NAME").toStdString()]
//             = subs.value("Value").toDouble();
//         else if(subs.contains("M"))
//           values[subs.value("__NAME").toStdString()]
//             = subs.value("M").toDouble();

//         if(subs.contains("lp:name") && subs.contains("lp:value"))
//           values[subs.value("lp:name").toStdString()]
//             = subs.value("lp:value").toDouble();

//         if(subs.contains("ls:name") && subs.contains("ls:value"))
//           values[subs.value("ls:name").toStdString()]
//             = subs.value("ls:value").toDouble();
//       }
//     }
//   }

//   workplane.setData(
//       values,
//       solver_.raw_numerator(),
//       solver_.raw_denominator()
//     );

//   return true;
// }


bool SchematicEditor::save()
{
	try
	{
		if (isUntitled_)
		{
			return saveAs();
		}
		else
		{
			return saveFile(curFile_);
		}
	}
	catch (crimson_bct_error_t exception)
	{
		if (exception == Error_NodesNotAssigned)
		{
			QMessageBox messageBox;
			messageBox.critical(0, "Error", "You must press Assign Nodes before saving!");
			messageBox.setFixedSize(500, 200);
			return false;
		}
	}

}


bool SchematicEditor::saveAs()
{
  QString fileName = QFileDialog::getSaveFileName(
      this, tr("Save As"), Settings().workspace(),
      tr("%1;;%2")
	    .arg(tr("XML files (CRIMSON BCT readable) (*.xml)"))
        .arg(tr("Netlist files (CRIMSON Flowsolver readable) (*.dat)"))
    );

  if(fileName.isEmpty())
    return false;

  return saveFile(fileName);
}

bool SchematicEditor::writeSimFiles()
{
	QFileDialog directoryChooser(this);

  directoryChooser.setFileMode(QFileDialog::Directory);
  directoryChooser.setOption(QFileDialog::ShowDirsOnly, true);

  QStringList chosenDirectory;
  if (directoryChooser.exec())
  {
	  chosenDirectory = directoryChooser.selectedFiles();

	  // Ensure that only one directory has been chosen:
	  if (chosenDirectory.length() != 1)
	  {
		  QMessageBox messageBox;
		  messageBox.critical(0, "Error", "You must select exactly one directory for the simulation files!");
		  messageBox.setFixedSize(500, 200);
		  return false;
	  }

	  QString chosenDirectory_Qstring = chosenDirectory.back();
	  writeNetlistSurfacesDotDat(chosenDirectory_Qstring);
	  writeAnyRequiredBoilerplatePythonControlScripts(chosenDirectory_Qstring);
	  return true;
  }

  return false;
}

bool SchematicEditor::fileExists(const QString& filePath)
{
	QFile file(filePath);
	if (file.exists())
	{
		return true;
	}
	else
	{
		return false;
	}
}

void SchematicEditor::writeNetlistSurfacesDotDat(const QString& chosenDirectory_Qstring)
{
	try
	{
		// Prepare to write netlist_surfaces.dat for reading by the flowsolver:
		QString filenameBasedOnOutputDir_datFile = chosenDirectory_Qstring;
		filenameBasedOnOutputDir_datFile.append("/netlist_surfaces.dat");
		bool datFileAlreadyExists = fileExists(filenameBasedOnOutputDir_datFile);

		// Prepare to write netlist_surfaces.xml so we can reload this in the Boundary Condition Toolbox if wanted later:
		QString filenameBasedOnOutputDir_xmlFile = chosenDirectory_Qstring;
		filenameBasedOnOutputDir_xmlFile.append("/netlist_surfaces.xml");
		bool xmlFileAlreadyExists = fileExists(filenameBasedOnOutputDir_xmlFile);

		bool aFileAlreadyExists = datFileAlreadyExists || xmlFileAlreadyExists;
		QMessageBox::StandardButton usersAnswer;
		if (aFileAlreadyExists)
		{
			QMessageBox overwriteExistingFiles;
			usersAnswer = overwriteExistingFiles.question(0, QString("File Exists"), QString("The selected folder is not empty. Overwrite existing simulation files?"));
			overwriteExistingFiles.setFixedSize(500, 200);
		}

		if (!aFileAlreadyExists || usersAnswer == QMessageBox::Yes)
		{
			saveFile(filenameBasedOnOutputDir_datFile);
			saveFile(filenameBasedOnOutputDir_xmlFile);
		}
		else
		{
			QMessageBox filesNotSaved;
			filesNotSaved.information(0, QString("Files Not Saved"), QString("Files Not Saved"));
			filesNotSaved.setFixedSize(500, 200);
		}
	}
	catch (crimson_bct_error_t exception)
	{
		if (exception == Error_NodesNotAssigned)
		{
			QMessageBox messageBox;
			messageBox.critical(0, "Error", "You must press Assign Nodes before saving!");
			messageBox.setFixedSize(500, 200);
		}
	}
}

void SchematicEditor::writeAnyRequiredBoilerplatePythonControlScripts(const QString& chosenDirectory_Qstring)
{
  // Get the control system types and associated file names from the user's input
  SchematicSceneParser* parser;
  std::vector<std::pair<predefinedStrings::controlScriptTypes::controlScriptTypes, std::string>>  namesAndTypesOfUserDefinedControlScripts;
  try
  {
	   parser = new SchematicSceneParser(*scene_);
	   namesAndTypesOfUserDefinedControlScripts = parser->getNamesAndTypesOfUserDefinedControlScripts();
	   delete parser;
  }
  catch (crimson_bct_error_t exception)
  {
    if (exception == Error_NodesNotAssigned)
    {
      QMessageBox messageBox;
      messageBox.critical(0, "Save Failed!", "You must Assign Nodes after moving any circuit items or labels.");
      messageBox.setFixedSize(500, 200);
    }
  }

  for (auto controlledItem = namesAndTypesOfUserDefinedControlScripts.begin(); controlledItem != namesAndTypesOfUserDefinedControlScripts.end(); controlledItem++)
  {
    // Extract the information for the controlled item:
    predefinedStrings::controlScriptTypes::controlScriptTypes controlType = controlledItem->first;
    std::string controlFileName = controlledItem->second;

    // a string which we will use to construct the full file save name and path
    std::string filenameBasedOnOutputDir = chosenDirectory_Qstring.toStdString();
	  filenameBasedOnOutputDir.append("\\");
    if (controlType == predefinedStrings::controlScriptTypes::ControlScript_StandardPython)
    {
      filenameBasedOnOutputDir.append(controlFileName);
	    filenameBasedOnOutputDir.append(".py");
      writeBoilerplateGenericPythonControlScript(filenameBasedOnOutputDir);
    }
    else if (controlType == predefinedStrings::controlScriptTypes::ControlScript_DatFileData)
    {
      // Just a copy of the directory path, as we'll need to write two files here to the same path:
      std::string filenameBasedOnOutputDir2 = filenameBasedOnOutputDir;

      filenameBasedOnOutputDir.append(controlFileName);
      filenameBasedOnOutputDir.append(".py");
      writeBoilerplatePythonDatFilePrescriber(filenameBasedOnOutputDir);

      filenameBasedOnOutputDir2.append(controlFileName);
      filenameBasedOnOutputDir2.append(".dat.example");
      writeBoilerplatePressureOrFlowWaveformDatFile(filenameBasedOnOutputDir2);
    }
	else if (controlType != predefinedStrings::controlScriptTypes::ControlScript_None)
    {
      QMessageBox messageBox;
      messageBox.critical(0, "Internal Error", "Unknown control type. Please save your work and restart! Report this to the developers.");
      messageBox.setFixedSize(500, 200);
    }
  }

  
}

void SchematicEditor::writeBoilerplateGenericPythonControlScript(const std::string fileNameToWrite)
{
  writeStringToFile(fileNameToWrite, genericPythonScript);
}

void SchematicEditor::writeBoilerplatePythonDatFilePrescriber(const std::string fileNameToWrite)
{
  writeStringToFile(fileNameToWrite, pythonDatFileFlowPrescriber);
}

void SchematicEditor::writeBoilerplatePressureOrFlowWaveformDatFile(const std::string fileNameToWrite)
{
  writeStringToFile(fileNameToWrite, exampleFlowOrPressureFile);
}

void SchematicEditor::writeStringToFile(const std::string fileNameToWrite, const std::string stringToWrite)
{
	QString fileName_qstring(fileNameToWrite.c_str());
	QFile file(fileName_qstring);
  
  if(!file.open(QFile::WriteOnly | QFile::Text)) {
    QMessageBox::warning(this, tr("Write file"),
      tr("Unable to write file ")
        + QString("%1:\n%2.")
		.arg(fileName_qstring)
          .arg(file.errorString())
    );

    return;
  } else {
    file.close();
  }

  std::string logMessage("Saving file...");
  logMessage.append(fileName_qstring.toStdString());
  QLogger::info(QObject::tr(logMessage.c_str()));
//   QApplication::setOverrideCursor(Qt::WaitCursor);
  QCursor cur = cursor();
  setCursor(Qt::WaitCursor);

  std::ofstream out_file(QFile::encodeName(fileName_qstring));

  out_file << stringToWrite;

  return;
}



bool SchematicEditor::saveFile(const QString& fileName)
{
  QFile file(fileName);
  
  if(!file.open(QFile::WriteOnly | QFile::Text)) {
    QMessageBox::warning(this, tr("Write file"),
      tr("Unable to write file ")
        + QString("%1:\n%2.")
          .arg(fileName)
          .arg(file.errorString())
    );
    
    return false;
  } else {
    file.close();
  }

  QLogger::info(QObject::tr("Saving file..."));
//   QApplication::setOverrideCursor(Qt::WaitCursor);
  QCursor cur = cursor();
  setCursor(Qt::WaitCursor);

  QFileInfo fileInfo(file);
  std::ofstream out_file(QFile::encodeName(fileName));
  sapecng::abstract_builder* out = sapecng::builder_factory::builder(fileInfo.suffix().toStdString(), out_file);

  SchematicSceneParser* parser = new SchematicSceneParser(*scene_);

  if(out)
  {
    std::string datSuffix("dat");
    if (fileInfo.suffix().toStdString() == datSuffix)
    {
      parser->writeCrimsonFlowsolverCircuitDescriptionFile(*out);
    }
    else
    {
      parser->parse(*out);
    }
  }

  delete parser;
  delete out;

  setCurrentFile(fileName);
  externalCleanChanged_ = true;
  scene_->undoRedoStack()->setClean();
  emit fileSaved(fileName);

//   QApplication::restoreOverrideCursor();
  setCursor(cur);

  return true;
}


bool SchematicEditor::loadFile(const QString& fileName)
{
  if(maybeSave()) {
    QFile file(fileName);
    if(!file.open(QFile::ReadOnly | QFile::Text)) {
      QMessageBox::warning(this, tr("Read file"),
        tr("Unable to read file ")
        + QString("%1:\n%2.")
          .arg(fileName)
          .arg(file.errorString())
      );
      return false;
    } else {
      file.close();
    }

    scene_->clearSchematicScene();
    QLogger::info(QObject::tr("Loading file..."));
//     QApplication::setOverrideCursor(Qt::WaitCursor);
    QCursor cur = cursor();
    setCursor(Qt::WaitCursor);

    SchematicSceneBuilder* out = new SchematicSceneBuilder(*scene_);

    QFileInfo fileInfo(file);
    std::ifstream in_file(QFile::encodeName(fileName));
    sapecng::abstract_parser* parser =
      sapecng::parser_factory::parser(
        fileInfo.suffix().toStdString(), in_file);

    if(parser) {
      scene_->undoRedoStack()->beginMacro(QObject::tr("Load file"));
      parser->parse(*out);
      scene_->undoRedoStack()->endMacro();
    }

    delete parser;
    delete out;

    setCurrentFile(fileName);
    externalCleanChanged_ = true;
    scene_->undoRedoStack()->setClean();
    emit fileLoaded(fileName);

//     QApplication::restoreOverrideCursor();
    setCursor(cur);
  }

  return true;
}


void SchematicEditor::solve()
{
  if(solved_) {
    showResult();
    return;
  }

  view_->setInteractive(false);
  scene_->assignNodes();

//   QApplication::setOverrideCursor(Qt::WaitCursor);
  setCursor(Qt::WaitCursor);
  QLogger::info(QObject::tr("Generating circuit..."));

  SchematicSceneParser* parser = new SchematicSceneParser(*scene_);
  solver_.apply(*parser);
  delete parser;

  QLogger::info(QObject::tr("Solving..."));
  solver_.start();
}


void SchematicEditor::reset()
{
  scene_->clearSchematicScene();
}


void SchematicEditor::fileNameChanged(const QString& fileName)
{
  setWindowTitle(fileName + "[*]");
}


void SchematicEditor::finished()
{
//   QApplication::restoreOverrideCursor();
  setCursor(Qt::ArrowCursor);
  view_->setInteractive(true);
  solved_ = true;
  showResult();

  emit solved();
}


void SchematicEditor::stateChanged(
  Qt::WindowStates oldState, Qt::WindowStates newState)
{
  scene_->undoRedoStack()->setActive(
    newState.testFlag(Qt::WindowActive) && !isRunning());
}


void SchematicEditor::showUserDef(SchematicScene& scene)
{
  SchematicEditor* editor = new SchematicEditor(scene, this);
  editor->scene().undoRedoStack()->setClean();

  connect(editor, SIGNAL(dirtyChanged(bool)),
    this, SLOT(externalCleanChanged()));
  connect(this, SIGNAL(aboutToCloseEditor()), editor, SLOT(close()));

  emit(stackEditor(editor));
}


void SchematicEditor::externalCleanChanged()
{
  externalCleanChanged_ = false;
  cleanChanged();
}


void SchematicEditor::cleanChanged(bool clean)
{
  setWindowModified(!externalCleanChanged_ || !clean);
  setDirty();
}


void SchematicEditor::closeEvent(QCloseEvent *event)
{
  if(maybeSave()) {
    if(solver_.isRunning())
      solver_.wait();
    
    QMdiSubWindow::closeEvent(event);
    emit(aboutToCloseEditor());
    event->accept();
  } else {
    event->ignore();
  }
}


void SchematicEditor::setDirty()
{
  solved_ = false;
  emit dirtyChanged(solved_);
}


void SchematicEditor::showResult()
{
  std::string num, den, expr;

  QPlainTextEdit* result = new QPlainTextEdit;
  result->setReadOnly(true);
  result->setUndoRedoEnabled(false);
  result->setLineWrapMode(QPlainTextEdit::NoWrap);
  result->setWordWrapMode(QTextOption::NoWrap);
  result->setTextInteractionFlags(
        Qt::TextSelectableByMouse
      | Qt::TextSelectableByKeyboard
    );

  result->appendPlainText("# " + currentFile() + "\n");

  expr.clear();
  num = sapecng::metacircuit::as_string(solver_.mixed_numerator());
  den = sapecng::metacircuit::as_string(solver_.mixed_denominator());
  expr.append(num);
  expr.append("\n");
  expr.append(std::string(
    (num.size() > den.size() ? num.size() : den.size()) * 3/2, '-'));
  expr.append("\n");
  expr.append(den);
  result->appendPlainText(tr("Result:\n"));
  result->appendPlainText(QString::fromStdString(expr));

  expr.clear();
  num = sapecng::metacircuit::as_string(solver_.raw_numerator());
  den = sapecng::metacircuit::as_string(solver_.raw_denominator());
  expr.append(num);
  expr.append("\n");
  expr.append(std::string(
    (num.size() > den.size() ? num.size() : den.size()) * 3/2, '-'));
  expr.append("\n");
  expr.append(den);
  result->appendPlainText(tr("\n\nRaw:\n"));
  result->appendPlainText(QString::fromStdString(expr));

  expr.clear();
  num = sapecng::metacircuit::as_string(solver_.digit_numerator());
  den = sapecng::metacircuit::as_string(solver_.digit_denominator());
  expr.append(num);
  expr.append("\n");
  expr.append(std::string(
    (num.size() > den.size() ? num.size() : den.size()) * 3/2, '-'));
  expr.append("\n");
  expr.append(den);
  result->appendPlainText(tr("\n\nDigit:\n"));
  result->appendPlainText(QString::fromStdString(expr));

  QDialogButtonBox* button = new QDialogButtonBox(QDialogButtonBox::Ok);

  QLayout* layout = new QVBoxLayout;
  layout->addWidget(result);
  layout->addWidget(button);

  QDialog dialog(this);
  dialog.setLayout(layout);
  connect(button, SIGNAL(accepted()), &dialog, SLOT(accept()));
  dialog.exec();
}


void SchematicEditor::setCurrentFile(const QString& fileName)
{
  curFile_ = QFileInfo(fileName).canonicalFilePath();
  isUntitled_ = false;
  setWindowModified(false);
  setWindowTitle(userFriendlyCurrentFile() + "[*]");
}


QString SchematicEditor::strippedName(const QString& fullFileName) const
{
  return QFileInfo(fullFileName).fileName();
}


bool SchematicEditor::maybeSave()
{
  if(isWindowModified()) {
    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, tr("Save file"),
      tr("'%1'").arg(userFriendlyCurrentFile()
        + QString(" has been modified.\n Save the file?")),
      QMessageBox::Save | QMessageBox::Discard
        | QMessageBox::Cancel
      );
    
    if(ret == QMessageBox::Save)
      return save();
    else if(ret == QMessageBox::Cancel)
      return false;
  }

  return true;
}


void SchematicEditor::init()
{
  view_ = new SchematicView(this);

  view_->setScene(scene_);
  connect(scene_, SIGNAL(sceneRectChanged(const QRectF&)),
    view_, SLOT(updateSceneRect(const QRectF&)));
//   scene_->setSceneRect(0, 0, 1024, 768);
  scene_->setSceneRect(QRectF());
  scene_->addRect(0, 0, 1E-99, 1E-99)->setVisible(false);

  setWidget(view_);

  connect(scene_, SIGNAL(propertyChanged()),
    this, SLOT(externalCleanChanged()));
  connect(scene_, SIGNAL(showUserDef(SchematicScene&)),
    this, SLOT(showUserDef(SchematicScene&)));
  connect(scene_->undoRedoStack(), SIGNAL(cleanChanged(bool)),
    this, SLOT(cleanChanged(bool)));
  connect(scene_->undoRedoStack(), SIGNAL(indexChanged(int)),
    this, SLOT(cleanChanged()));

  connect(&solver_, SIGNAL(finished()), this, SLOT(finished()));

  connect(this, SIGNAL(windowStateChanged(Qt::WindowStates, Qt::WindowStates)),
    this, SLOT(stateChanged(Qt::WindowStates, Qt::WindowStates)));

  curFile_ = tr("Untitled");
  isUntitled_ = true;
  externalCleanChanged_ = true;

  setGeometry(0, 0, 640, 480);
  setWindowTitle(curFile_ + "[*]");
  setWindowIcon(QIcon(":/images/grid.png"));
  setAttribute(Qt::WA_DeleteOnClose);
}


}
