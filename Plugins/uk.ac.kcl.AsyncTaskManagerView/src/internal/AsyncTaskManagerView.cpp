// Qt
#include <QToolButton>
#include <QProgressBar>

// Main include
#include "AsyncTaskManagerView.h"

// Module includes
#include <AsyncTaskManager.h>

const std::string AsyncTaskManagerView::VIEW_ID = "org.mitk.views.AsyncTaskManagerView";
static const int TaskUidRole = Qt::UserRole + 1;

AsyncTaskManagerView::AsyncTaskManagerView()
{
}

AsyncTaskManagerView::~AsyncTaskManagerView()
{
}

void AsyncTaskManagerView::SetFocus()
{
}

void AsyncTaskManagerView::CreateQtPartControl(QWidget *parent)
{
    // create GUI widgets from the Qt Designer's .ui file
    _UI.setupUi(parent);

    _UI.taskTableWidget->horizontalHeader()->setSectionResizeMode(ColumnName, QHeaderView::ResizeToContents);
    _UI.taskTableWidget->horizontalHeader()->setSectionResizeMode(ColumnProgress, QHeaderView::Stretch);
    _UI.taskTableWidget->horizontalHeader()->setSectionResizeMode(ColumnCancelButton, QHeaderView::ResizeToContents);

    connect(_UI.cancelAllButton, &QAbstractButton::clicked, this, &AsyncTaskManagerView::cancelAllTasks);

    connect(crimson::AsyncTaskManager::getInstance(), &crimson::AsyncTaskManager::taskStateChanged,
        this, &AsyncTaskManagerView::taskStateChanged);
    connect(crimson::AsyncTaskManager::getInstance(), &crimson::AsyncTaskManager::taskAdded,
        this, &AsyncTaskManagerView::taskAdded);
    connect(crimson::AsyncTaskManager::getInstance(), &crimson::AsyncTaskManager::taskProgressAddSteps,
        this, &AsyncTaskManagerView::taskProgressAddSteps);
    connect(crimson::AsyncTaskManager::getInstance(), &crimson::AsyncTaskManager::taskProgressMade,
        this, &AsyncTaskManagerView::taskProgressMade);
    connect(crimson::AsyncTaskManager::getInstance(), &crimson::AsyncTaskManager::taskCompleted,
        this, &AsyncTaskManagerView::taskCompleted);

    // Fill the widget with tasks already running
    for (auto nameTaskPair : crimson::AsyncTaskManager::getInstance()->getAllTasks()) {
        taskAdded(nameTaskPair.first);
    }

    // Initialize the UI
    _updateUI();
}

void AsyncTaskManagerView::_updateUI()
{
    bool anyTaskNotCancelling = false;
    for (auto nameTaskPair : crimson::AsyncTaskManager::getInstance()->getAllTasks()) {
        if (nameTaskPair.second->getTask()->getState() != crimson::async::Task::State_Cancelling) {
            anyTaskNotCancelling = true;
            break;
        }
    }

    _UI.cancelAllButton->setEnabled(anyTaskNotCancelling);
}

void AsyncTaskManagerView::cancelAllTasks()
{
    for (auto nameTaskPair : crimson::AsyncTaskManager::getInstance()->getAllTasks()) {
        crimson::AsyncTaskManager::getInstance()->cancelTask(nameTaskPair.first);
    }

    _updateUI();
}

void AsyncTaskManagerView::taskStateChanged(const crimson::AsyncTaskManager::TaskUID& uid, crimson::async::Task::State state)
{
    _UI.taskTableWidget->cellWidget(_findRowByUid(uid), ColumnCancelButton)->setEnabled(state != crimson::async::Task::State_Cancelling);
    _updateUI();
}

void AsyncTaskManagerView::taskAdded(const crimson::AsyncTaskManager::TaskUID& taskUid)
{
    int newRow = _UI.taskTableWidget->rowCount();
    _UI.taskTableWidget->insertRow(newRow);
    
    std::shared_ptr<crimson::QAsyncTaskAdapter> task = crimson::AsyncTaskManager::getInstance()->findTask(taskUid);
    auto nameItem = new QTableWidgetItem(QString::fromStdString(task->getDescription()));
    nameItem->setData(TaskUidRole, QString::fromStdString(taskUid));
    _UI.taskTableWidget->setItem(newRow, ColumnName, nameItem);

    auto progressBar = new QProgressBar();
    progressBar->setRange(0, task->stepsTotal());
    progressBar->setValue(task->stepsMade());
    _UI.taskTableWidget->setCellWidget(newRow, ColumnProgress, progressBar);

    auto cancelButton = new QToolButton();
    cancelButton->setIcon(QIcon(":/icons/icons/stop.png"));
    cancelButton->setToolTip(tr("Cancel task"));
    connect(cancelButton, &QAbstractButton::clicked, this, &AsyncTaskManagerView::cancelTaskByButton);
    _UI.taskTableWidget->setCellWidget(newRow, ColumnCancelButton, cancelButton);
}

void AsyncTaskManagerView::taskProgressAddSteps(const crimson::AsyncTaskManager::TaskUID& taskUid, unsigned int steps)
{
    int row = _findRowByUid(taskUid);

    auto progressBar = static_cast<QProgressBar*>(_UI.taskTableWidget->cellWidget(row, 1));
    progressBar->setMaximum(progressBar->maximum() + steps);
}

void AsyncTaskManagerView::taskProgressMade(const crimson::AsyncTaskManager::TaskUID& taskUid, unsigned int steps)
{
    int row = _findRowByUid(taskUid);

    auto progressBar = static_cast<QProgressBar*>(_UI.taskTableWidget->cellWidget(row, ColumnProgress));
    progressBar->setValue(progressBar->value() + steps);
}

void AsyncTaskManagerView::taskCompleted(const crimson::AsyncTaskManager::TaskUID& taskUid, crimson::async::Task::State)
{
    _UI.taskTableWidget->removeRow(_findRowByUid(taskUid));
    _updateUI();
}

void AsyncTaskManagerView::cancelTaskByButton()
{
    for (int row = 0; row < _UI.taskTableWidget->rowCount(); ++row) {
        if (_UI.taskTableWidget->cellWidget(row, ColumnCancelButton) == sender()) {
            crimson::AsyncTaskManager::getInstance()->cancelTask(_getUidForRow(row));
            break;
        }
    }
}

int AsyncTaskManagerView::_findRowByUid(const crimson::AsyncTaskManager::TaskUID& uid)
{
    for (int row = 0; row < _UI.taskTableWidget->rowCount(); ++row) {
        if (_getUidForRow(row) == uid) {
            return row;
        }
    }

    return -1;
}

crimson::AsyncTaskManager::TaskUID AsyncTaskManagerView::_getUidForRow(int row)
{
    return _UI.taskTableWidget->item(row, ColumnName)->data(TaskUidRole).value<QString>().toStdString();
}