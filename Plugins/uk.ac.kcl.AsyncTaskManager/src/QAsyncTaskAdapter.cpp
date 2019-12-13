#include "QAsyncTaskAdapter.h"
#include <QMetaType>

namespace crimson {

QAsyncTaskAdapter::QAsyncTaskAdapter()
{
}

QAsyncTaskAdapter::QAsyncTaskAdapter(const std::shared_ptr<async::Task>& task) 
{
    setTask(task);
}

QAsyncTaskAdapter::~QAsyncTaskAdapter()
{
}

void QAsyncTaskAdapter::setTask(const std::shared_ptr<async::Task>& task)
{
    _task = task;
    _connections[0] = _task->taskStateChangedSignal.connect(
        [this](async::Task::State state, const std::string& message)
        {
            QMetaObject::invokeMethod(this, "queuedTaskStateChangeHandler", Qt::QueuedConnection, 
                Q_ARG(crimson::async::Task::State, state), 
                Q_ARG(QString, QString::fromStdString(message)));
        });

    _connections[1] = _task->progressMadeSignal.connect(
        [this](unsigned int steps)
        {
            QMetaObject::invokeMethod(this, "queuedProgressMade", Qt::QueuedConnection, Q_ARG(unsigned int, steps));
        });

    _connections[2] = _task->stepsAddedSignal.connect(
        [this](unsigned int steps)
        {
            QMetaObject::invokeMethod(this, "queuedProgressStepsAdded", Qt::QueuedConnection, Q_ARG(unsigned int, steps));
        });
}

void QAsyncTaskAdapter::queuedTaskStateChangeHandler(crimson::async::Task::State newState, QString message)
{
    emit taskStateChanged(newState, message);
}

void QAsyncTaskAdapter::queuedProgressMade(unsigned int steps)
{
    _stepsMade += steps;
    emit progressMade(steps);
}

void QAsyncTaskAdapter::queuedProgressStepsAdded(unsigned int steps)
{
    _stepsTotal += steps;
    emit progressStepsAdded(steps);
}


}