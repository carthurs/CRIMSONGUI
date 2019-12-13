#include <assert.h>
#include <set>

#include "AsyncTaskManager.h"

#include <mitkLogMacros.h>
#include <mitkProgressBar.h>

#include <QRunnable>
#include <QThreadPool>
#include <QMessageBox>

namespace crimson {

AsyncTaskManager* AsyncTaskManager::_instance = nullptr;

bool AsyncTaskManager::init()
{
    assert(_instance == nullptr);
    _instance = new AsyncTaskManager();
    return true;
}

void AsyncTaskManager::term()
{
    assert(_instance != nullptr);
    delete _instance;
    _instance = nullptr;
}

AsyncTaskManager* AsyncTaskManager::getInstance()
{
    return _instance;
}

AsyncTaskManager::AsyncTaskManager()
{
    qRegisterMetaType<crimson::async::Task::State>("crimson::async::Task::State");
    qRegisterMetaType<crimson::AsyncTaskManager::TaskUID>("crimson::AsyncTaskManager::TaskUID");
    auto tp = QThreadPool::globalInstance();
    tp->setMaxThreadCount(std::max(2, tp->maxThreadCount()));
}

AsyncTaskManager::~AsyncTaskManager()
{
    // TODO:
//     for (auto nameTaskPair : _tasks) {
//         cancelTask(nameTaskPair.first);
//     }
// 
//     QThreadPool::globalInstance()->waitForDone();
}

class AsyncTaskQWrapper : public QRunnable {
public:
    AsyncTaskQWrapper(const std::shared_ptr<async::Task>& task) : _task(task) { }

    void run() override { _task->run(); }

private:
    std::shared_ptr<async::Task> _task;
};

bool AsyncTaskManager::addTask(const std::shared_ptr<QAsyncTaskAdapter>& task, const TaskUID& taskUid)
{
    if (_tasks.find(taskUid) != _tasks.end()) {
        MITK_ERROR << "Task with UID " << taskUid << " is already running. Task rejected.";
        return false;
    }

    connect(task.get(), &QAsyncTaskAdapter::taskStateChanged, this, &AsyncTaskManager::handleTaskStateChange);
    connect(task.get(), &QAsyncTaskAdapter::progressStepsAdded, this, &AsyncTaskManager::globalProgressAddSteps);
    connect(task.get(), &QAsyncTaskAdapter::progressMade, this, &AsyncTaskManager::globalProgressMade);

    _tasks[taskUid] = task;

    emit taskAdded(taskUid);

    if (task->getSequentialExecutionTag() == -1) {
        runTask(task->getTask());
    }
    else {
        _sequentialTasks[task->getSequentialExecutionTag()].push(task);
        _tryStartSequentialTask(task->getSequentialExecutionTag());
    }

    return true;
}

void AsyncTaskManager::_tryStartSequentialTask(int tag)
{
    if (!_sequentialTasks[tag].empty()) {
        crimson::async::Task::State state = _sequentialTasks[tag].front()->getTask()->getState();
        if (state == crimson::async::Task::State_Idle || state == crimson::async::Task::State_Cancelling) {
            const std::shared_ptr<QAsyncTaskAdapter>& taskToStart = _sequentialTasks[tag].front();
            runTask(taskToStart->getTask());
        }
    }
}

void AsyncTaskManager::runTask(const std::shared_ptr<async::Task>& task)
{
    if (task->getState() == async::Task::State_Cancelling) {
        task->setState(async::Task::State_Cancelled);
        return;
    }
    task->setState(async::Task::State_Starting);

    auto wrapper = new AsyncTaskQWrapper(task);

    QThreadPool::globalInstance()->start(wrapper);
}

void AsyncTaskManager::cancelTask(const TaskUID& id)
{
    if (_tasks.find(id) != _tasks.end()) {
        MITK_INFO << "Cancelling task " << _tasks[id]->getDescription();
        _tasks[id]->getTask()->cancel();
    }
}

async::Task::State AsyncTaskManager::getTaskState(const TaskUID& id)
{
    if (_tasks.find(id) == _tasks.end()) {
        return async::Task::State_Finished;
    }

    return _tasks[id]->getTask()->getState();
}

void AsyncTaskManager::handleTaskStateChange(async::Task::State state, QString message)
{
    TaskUID uid;
    if (_findTaskUIDBySignalSender(sender(), uid)) {
        emit taskStateChanged(uid, state, message);
        if (async::Task::isStateTerminal(state)) {
            auto taskFinishedTime = std::chrono::high_resolution_clock::now();

            // If the task requires sequential executation - try start the next task in queue
            std::shared_ptr<QAsyncTaskAdapter> taskPtr = _tasks[uid];
            if (taskPtr->getSequentialExecutionTag() != -1) {
                _sequentialTasks[taskPtr->getSequentialExecutionTag()].pop();
                _tryStartSequentialTask(taskPtr->getSequentialExecutionTag());
            }

            if (state == crimson::async::Task::State::State_Failed && !taskPtr->isSilentFail()) {
                QMessageBox::critical(nullptr, QString::fromStdString(taskPtr->getDescription()) + " failed", message);
                MITK_ERROR << taskPtr->getDescription() << " failed: " << message.toStdString();
            }

            if (state == crimson::async::Task::State::State_Finished) {
                MITK_INFO << taskPtr->getDescription() << " successfully finished in " 
                    << std::chrono::duration_cast<std::chrono::milliseconds>(taskFinishedTime - _taskStartTimes[taskPtr.get()]).count() / 1000.0 << " seconds";
            }

            globalProgressMade(taskPtr->stepsTotal() - taskPtr->stepsMade());

            emit taskCompleted(uid, state, message);

            _taskStartTimes.erase(taskPtr.get());
            _tasks.erase(uid);

            disconnect(taskPtr.get(), &QAsyncTaskAdapter::taskStateChanged, this, &AsyncTaskManager::handleTaskStateChange);
            disconnect(taskPtr.get(), &QAsyncTaskAdapter::progressStepsAdded, this, &AsyncTaskManager::globalProgressAddSteps);
            disconnect(taskPtr.get(), &QAsyncTaskAdapter::progressMade, this, &AsyncTaskManager::globalProgressMade);
        } else if (state == async::Task::State_Running) {
            _taskStartTimes[_tasks[uid].get()] = std::chrono::high_resolution_clock::now();
        }
    }
}

void AsyncTaskManager::globalProgressAddSteps(unsigned int steps)
{
    TaskUID uid;
    if (_findTaskUIDBySignalSender(sender(), uid)) {
        mitk::ProgressBar::GetInstance()->AddStepsToDo(steps);
        emit taskProgressAddSteps(uid, steps);
    }
}

void AsyncTaskManager::globalProgressMade(unsigned int steps)
{
    TaskUID uid;
    if (_findTaskUIDBySignalSender(sender(), uid)) {
        mitk::ProgressBar::GetInstance()->Progress(steps);
        emit taskProgressMade(uid, steps);
    }
}

std::shared_ptr<QAsyncTaskAdapter> AsyncTaskManager::findTask(const TaskUID& id)
{
    auto iter = _tasks.find(id);
    if (iter == _tasks.end()) {
        return std::shared_ptr<QAsyncTaskAdapter>();
    }

    return iter->second;
}

bool AsyncTaskManager::_findTaskUIDBySignalSender(QObject* sender, TaskUID& uid)
{
    for (const std::pair<TaskUID, std::shared_ptr<QAsyncTaskAdapter>>& uidTaskPtrPair : _tasks) {
        if (uidTaskPtrPair.second.get() == sender) {
            uid = uidTaskPtrPair.first;
            return true;
        }
    }

    return false;
}


} // namespace crimson
