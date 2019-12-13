#include "PythonSolverSetupPreferencePage.h"

#include <berryIPreferencesService.h>
#include <berryPlatform.h>

#include <QFileDialog>

namespace crimson
{

const char* PythonSolverSetupPreferencePage::preferenceNodeName = "/uk.ac.kcl.PythonSolverSetup";
const char* PythonSolverSetupPreferencePage::separator = ";;";
const char* PythonSolverSetupPreferencePage::directoriesKeyName = "Directories";

PythonSolverSetupPreferencePage::PythonSolverSetupPreferencePage()
    : _mainControl(nullptr)
{
}

PythonSolverSetupPreferencePage::~PythonSolverSetupPreferencePage() {}

void PythonSolverSetupPreferencePage::Init(berry::IWorkbench::Pointer) {}

void PythonSolverSetupPreferencePage::CreateQtControl(QWidget* parent)
{
    _mainControl = new QWidget(parent);
    _ui.setupUi(_mainControl);

    connect(_ui.addNewDirectoryButton, &QAbstractButton::clicked, this, &PythonSolverSetupPreferencePage::_addNewDirectory);
    connect(_ui.openNewDirectoryButton, &QAbstractButton::clicked, this, &PythonSolverSetupPreferencePage::_openNewDirectory);
    connect(_ui.removeDirectoryButton, &QAbstractButton::clicked, this,
            &PythonSolverSetupPreferencePage::_deleteSelectedDirectories);

    berry::IPreferencesService* prefService = berry::Platform::GetPreferencesService();
    _pythonSolverSetupPreferencesNode = prefService->GetSystemPreferences()->Node(preferenceNodeName);

    this->Update();
}

QWidget* PythonSolverSetupPreferencePage::GetQtControl() const { return _mainControl; }

bool PythonSolverSetupPreferencePage::PerformOk()
{
    QStringList dirList;
    dirList.reserve(_ui.directoryListWidget->count());
    for (int i = 0; i < _ui.directoryListWidget->count(); ++i) {
        dirList.push_back(_ui.directoryListWidget->item(i)->text());
    }


    _pythonSolverSetupPreferencesNode->Put(directoriesKeyName, dirList.join(separator));
    return true;
}

void PythonSolverSetupPreferencePage::PerformCancel() {}

void PythonSolverSetupPreferencePage::Update()
{
    QStringList dirList = _pythonSolverSetupPreferencesNode->Get(directoriesKeyName, "").split(separator);

    _ui.directoryListWidget->clear();
    for (const auto& dir : dirList) {
        _addDirectoryToList(dir);
    }
}

void PythonSolverSetupPreferencePage::_addNewDirectory() { _ui.directoryListWidget->editItem(_addDirectoryToList("")); }

void PythonSolverSetupPreferencePage::_openNewDirectory()
{
    QString existingDir = QFileDialog::getExistingDirectory(_mainControl, "Select a directory");
    if (!existingDir.isEmpty()) {
        _addDirectoryToList(existingDir);
    }
}

void PythonSolverSetupPreferencePage::_deleteSelectedDirectories() { qDeleteAll(_ui.directoryListWidget->selectedItems()); }

QListWidgetItem* PythonSolverSetupPreferencePage::_addDirectoryToList(const QString& dir)
{
    auto newItem = new QListWidgetItem{dir, _ui.directoryListWidget};
    newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);
    _ui.directoryListWidget->addItem(newItem);
    return newItem;
}
}