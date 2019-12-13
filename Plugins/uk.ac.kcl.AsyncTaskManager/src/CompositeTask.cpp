#include <AsyncTaskManager.h>
#include <assert.h>

#include "CompositeTask.h"
#include <QMutexLocker>
#include <mitkLogMacros.h>

namespace crimson {

CompositeTask::CompositeTask(std::shared_ptr<ICompositeExecutionStrategy> executionStrategy)
    : executionStrategy(executionStrategy)
    , nTasksRemaining(executionStrategy->allTasks().size())
{

}

std::tuple<async::Task::State, std::string> CompositeTask::runTask()
{
    std::vector<std::shared_ptr<crimson::async::Task>> startingTasks = executionStrategy->startingTasks();
    if (startingTasks.empty()) {
        return std::make_tuple(async::Task::State_Finished, std::string());
    }

    stepsAddedSignal(static_cast<int>(nTasksRemaining));

    std::vector<boost::signals2::scoped_connection> connections;
    for (const std::shared_ptr<crimson::async::Task>& taskPtr : executionStrategy->allTasks()) {
        connections.push_back(taskPtr->taskStateChangedSignal.connect(
            [this, taskPtr](Task::State newState, const std::string&)
            {
                childTaskStateChanged(taskPtr, newState);
            }
            ));
    }

    for (const std::shared_ptr<crimson::async::Task>& taskPtr : startingTasks) {
        crimson::AsyncTaskManager::getInstance()->runTask(taskPtr);
    }

    mutex.lock();
    if (nTasksRemaining > 0) {
        waitCondition.wait(&mutex);
    }
    mutex.unlock();

    auto returnState = std::make_tuple(async::Task::State_Finished, std::string("Completed successfully."));
    for (const std::shared_ptr<crimson::async::Task>& taskPtr : executionStrategy->allTasks()) {
        switch (taskPtr->getState()) {
        case async::Task::State_Failed:
            return std::make_tuple(async::Task::State_Finished, "One of the tasks has failed\n" + taskPtr->getLastStateChangeMessage());
        case async::Task::State_Cancelled:
            std::get<0>(returnState) = async::Task::State_Cancelled;
            std::get<1>(returnState) = "Operation cancelled.";
            break;
        case async::Task::State_Finished:
            break;
        default:
            assert(false); // All tasks should be finished by now
        }
    }

    return returnState;
}

void CompositeTask::childTaskStateChanged(const std::shared_ptr<crimson::async::Task>& childTask, Task::State newState)
{
    if (isStateTerminal(newState)) {
        QMutexLocker locker(&mutex);

        progressMadeSignal(1);

        if (--nTasksRemaining == 0) {
            // All tasks complete
            waitCondition.wakeOne();
        }

        std::shared_ptr<crimson::async::Task> nextTask = executionStrategy->nextTask(childTask);

        locker.unlock();

        if (nextTask) {
            crimson::AsyncTaskManager::getInstance()->runTask(nextTask);
        }
    }
}

void CompositeTask::cancel()
{
    for (const std::shared_ptr<crimson::async::Task>& taskPtr : executionStrategy->allTasks()) {
        taskPtr->cancel();
    }

    crimson::async::Task::cancel();
}

}
