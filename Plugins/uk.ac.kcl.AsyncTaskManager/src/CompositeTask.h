#pragma once

#include <AsyncTask.h>

#include <QWaitCondition>
#include <QMutex>

#include <vector>
#include <memory>

namespace crimson {

/*! \brief   A composite execution strategy interface. */
class ICompositeExecutionStrategy {
public:
    virtual ~ICompositeExecutionStrategy() {}

    /*!
     * \brief   Gets all the tasks in a composite task.
     */
    virtual const std::vector<std::shared_ptr<crimson::async::Task>>& allTasks() const = 0;

    /*!
     * \brief   Gets the tasks that should be started originally.
     */
    virtual std::vector<std::shared_ptr<crimson::async::Task>> startingTasks() = 0;

    /*!
     * \brief   Gets the next task to be executed when one of the tasks has finished.
     */
    virtual std::shared_ptr<crimson::async::Task> nextTask(const std::shared_ptr<crimson::async::Task>& finishedTask) = 0;
};

/*! \brief   A simple strategy that relies of the task scheduler to execute tasks. */
class StartAllExecutionStrategy : public ICompositeExecutionStrategy {
public:
    StartAllExecutionStrategy(const std::vector<std::shared_ptr<crimson::async::Task>>& tasks) : _tasks(tasks) {}


    const std::vector<std::shared_ptr<crimson::async::Task>>& allTasks() const override { return _tasks; }
    std::vector<std::shared_ptr<crimson::async::Task>> startingTasks() override { return _tasks; }
    std::shared_ptr<crimson::async::Task> nextTask(const std::shared_ptr<crimson::async::Task>&) override { return nullptr; }

private:
    std::vector<std::shared_ptr<crimson::async::Task>> _tasks;
};

/*! \brief   A simple strategy that executes the tasks in sequential order. */
class SequentialExecutionStrategy : public ICompositeExecutionStrategy {
public:
    SequentialExecutionStrategy(const std::vector<std::shared_ptr<crimson::async::Task>>& tasks) : _tasks(tasks) {}

    const std::vector<std::shared_ptr<crimson::async::Task>>& allTasks() const override { return _tasks; }
    std::vector<std::shared_ptr<crimson::async::Task>> startingTasks() override { return{ _tasks[0] }; }
    std::shared_ptr<crimson::async::Task> nextTask(const std::shared_ptr<crimson::async::Task>&) override { return (++currentTask == _tasks.size()) ? nullptr : _tasks[currentTask]; }

private:
    std::vector<std::shared_ptr<crimson::async::Task>> _tasks;
    size_t currentTask = 0;
};

/*!
 * \brief   Composite task runs its child tasks and waits for their completion
 */
class ASYNCTASKMANAGER_EXPORT CompositeTask : public crimson::async::Task {
public:
    CompositeTask(std::shared_ptr<ICompositeExecutionStrategy> executionStrategy);

    // crimson::async::Task
    std::tuple<State, std::string> runTask() override;
    void cancel() override;

private:
    void childTaskStateChanged(const std::shared_ptr<crimson::async::Task>& childTask, Task::State newState);

private:
    std::shared_ptr<ICompositeExecutionStrategy> executionStrategy;
    size_t nTasksRemaining;
    QWaitCondition waitCondition;
    QMutex mutex;
    bool tasksCancelled = false;
};

}