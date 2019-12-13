#pragma once

#include <berryIPreferences.h>
#include <berryIQtPreferencePage.h>

#include "ui_PythonSolverSetupPreferencePage.h"

namespace crimson
{

class PythonSolverSetupPreferencePage : public QObject, public berry::IQtPreferencePage
{
    Q_OBJECT
    Q_INTERFACES(berry::IPreferencePage)

public:
    PythonSolverSetupPreferencePage();
    ~PythonSolverSetupPreferencePage();

    void Init(berry::IWorkbench::Pointer workbench) override;

    void CreateQtControl(QWidget* widget) override;

    QWidget* GetQtControl() const override;

    ///
    /// \see IPreferencePage::PerformOk()
    ///
    virtual bool PerformOk() override;

    ///
    /// \see IPreferencePage::PerformCancel()
    ///
    virtual void PerformCancel() override;

    ///
    /// \see IPreferencePage::Update()
    ///
    virtual void Update() override;

    static const char* preferenceNodeName;
    static const char* separator;
    static const char* directoriesKeyName;

private slots:
    void _addNewDirectory();
    void _openNewDirectory();
    void _deleteSelectedDirectories();

private:
    QListWidgetItem* _addDirectoryToList(const QString& dir);

private:
    QWidget* _mainControl;
    Ui::PythonSolverSetupPreferencePage _ui;

    berry::IPreferences::Pointer _pythonSolverSetupPreferencesNode;
};
}