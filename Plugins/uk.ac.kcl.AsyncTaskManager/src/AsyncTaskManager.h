#pragma once

#include <map>
#include <queue>
#include <chrono>

#include "QAsyncTaskAdapter.h"


namespace crimson {

/*! \brief   QThreadPool-based asynchronous task executor. */
class ASYNCTASKMANAGER_EXPORT AsyncTaskManager : public QObject {
    Q_OBJECT
public:
    typedef std::string TaskUID;

    /*! \name Singleton interface */
    ///@{ 
    static bool init();
    static AsyncTaskManager* getInstance();
    static void term();
    ///@} 

    /*!
     * \brief   Run the task using thread pool AsyncTaskManager handles the sequential execution of
     *  tasks if task->getSequentialExecutionTag() is not -1 If a task with taskUid is already
     *  running, nothing is done.
     */
    bool addTask(const std::shared_ptr<QAsyncTaskAdapter>& task, const TaskUID& taskUid);

    /*!
     * \brief   Run an external task using thread pool. No additional actions are taken.
     */
    void runTask(const std::shared_ptr<async::Task>& task);

    /*!
     * \brief   Cancels the task by UID.
     */
    void cancelTask(const TaskUID& id);

    /*!
     * \brief   Gets the current task state by UID.
     */
    async::Task::State getTaskState(const TaskUID& id); 

    /*!
     * \brief   Gets the list of all tasks in a form of a map from UID to task pointer.
     */
    const std::map<TaskUID, std::shared_ptr<QAsyncTaskAdapter>>& getAllTasks() const { return _tasks; }

    /*!
     * \brief   Finds task by UID.
     */
    std::shared_ptr<QAsyncTaskAdapter> findTask(const TaskUID& id);

signals:
    void taskAdded(const crimson::AsyncTaskManager::TaskUID& taskUid);
    void taskStateChanged(const crimson::AsyncTaskManager::TaskUID& taskUid, crimson::async::Task::State newState, QString message);
    void taskProgressAddSteps(const crimson::AsyncTaskManager::TaskUID& taskUid, unsigned int steps);
    void taskProgressMade(const crimson::AsyncTaskManager::TaskUID& taskUid, unsigned int steps);
    void taskCompleted(const crimson::AsyncTaskManager::TaskUID& taskUid, crimson::async::Task::State finalState, QString message);

private:
    AsyncTaskManager();
    ~AsyncTaskManager();

    AsyncTaskManager(const AsyncTaskManager&) = delete;
    AsyncTaskManager& operator=(const AsyncTaskManager&) = delete;

private slots:
    void handleTaskStateChange(crimson::async::Task::State, QString);
    void globalProgressAddSteps(unsigned int);
    void globalProgressMade(unsigned int);

private:
    bool _findTaskUIDBySignalSender(QObject* sender, TaskUID& uid);
    void _tryStartSequentialTask(int tag);

    static AsyncTaskManager* _instance;

    std::map<TaskUID, std::shared_ptr<QAsyncTaskAdapter>> _tasks;
    std::map<const QAsyncTaskAdapter*, std::chrono::high_resolution_clock::time_point> _taskStartTimes;
    std::map<int, std::queue<std::shared_ptr<QAsyncTaskAdapter>>> _sequentialTasks;
};

} // namespace crimson