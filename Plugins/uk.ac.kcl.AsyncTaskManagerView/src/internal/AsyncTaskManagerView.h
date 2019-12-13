#pragma once

#include <QmitkAbstractView.h>

#include <AsyncTaskManager.h>

#include "ui_AsyncTaskManagerView.h"

/*!
 * \brief   AsyncTaskManagerView displays the running async tasks, their progress and a way to
 *  cancel them.
 */
class AsyncTaskManagerView : public QmitkAbstractView
{
    Q_OBJECT

public:
    static const std::string VIEW_ID;

    AsyncTaskManagerView();
    ~AsyncTaskManagerView();


private slots:
    void cancelAllTasks();
    void cancelTaskByButton();
    void taskStateChanged(const crimson::AsyncTaskManager::TaskUID& uid, crimson::async::Task::State state);
    void taskAdded(const crimson::AsyncTaskManager::TaskUID& taskUid);
    void taskProgressAddSteps(const crimson::AsyncTaskManager::TaskUID& taskUid, unsigned int steps);
    void taskProgressMade(const crimson::AsyncTaskManager::TaskUID& taskUid, unsigned int steps);
    void taskCompleted(const crimson::AsyncTaskManager::TaskUID& taskUid, crimson::async::Task::State finalState);

private:
    void CreateQtPartControl(QWidget *parent) override;
    void SetFocus() override;

    void _updateUI();

    int _findRowByUid(const crimson::AsyncTaskManager::TaskUID& uid);
    crimson::AsyncTaskManager::TaskUID _getUidForRow(int row);

    enum {
        ColumnName = 0,
        ColumnProgress,
        ColumnCancelButton
    };

private:
    // Ui and main widget of this view
    Ui::AsyncTaskManagerWidget _UI;
};
