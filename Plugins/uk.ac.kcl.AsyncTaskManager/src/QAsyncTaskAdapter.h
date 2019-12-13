#pragma once

#include "uk_ac_kcl_AsyncTaskManager_Export.h"

#include <AsyncTask.h>
#include <QObject>

#include <boost/signals2/connection.hpp>

#include <memory>
#include <array>

namespace crimson {

class QAsyncTaskAdapterProgressObserver;

/*! \brief   An adapter for the asynchronous tasks to be executed in a QThreadPool. */
class ASYNCTASKMANAGER_EXPORT QAsyncTaskAdapter : public QObject {
    Q_OBJECT
public:
    QAsyncTaskAdapter();
    QAsyncTaskAdapter(const std::shared_ptr<async::Task>& task);

    ~QAsyncTaskAdapter();

    ///@{
    /*!
     * \brief   Sets the actual task.
     */
    void setTask(const std::shared_ptr<async::Task>& task);

    /*!
     * \brief   Gets the task.
     */
    const std::shared_ptr<async::Task>& getTask() const { return _task; }
    ///@} 

    ///@{ 
    /*!
     * \brief   Sets the task description.
     */
    void setDescription(const std::string& description) { _description = description; }

    /*!
     * \brief   Gets the task description.
     */
    const std::string& getDescription() const { return _description; }
    ///@} 

    ///@{ 
    /*!
     * \brief   Gets the total number of progress steps added by the task.
     */
    unsigned int stepsTotal() const { return _stepsTotal; }

    /*!
     * \brief   Gets the total number of progress steps that the task has executed.
     */
    unsigned int stepsMade() const { return _stepsMade; }
    ///@} 
    
    ///@{ 
    /*!
     * The sequential execution tag groups the tasks that cannot be executed in parallel.
     * -1 means no limits on task parallelism are set otherwise all tasks with same tag will
     *  execute one after another.
     */
    void setSequentialExecutionTag(int tag) { _sequentialExecutionTag = tag; }
    int getSequentialExecutionTag() const { return _sequentialExecutionTag;  }
    ///@} 

    ///@{ 
    /*! The silent fail flag suppresses a message box in case the task fails.
     */
    void setSilentFail(bool silent) { _silentFail = silent; }
    bool isSilentFail() const { return _silentFail; }
    ///@} 

signals:
    // Connect to these signals if you want to have some code executed in the caller (normally, GUI) thread
    // after the task is finished
    void progressStepsAdded(unsigned int);
    void progressMade(unsigned int);
    void taskStateChanged(crimson::async::Task::State, QString);

private slots :
    ///@{ 
    /*! These slots are used to process the task signals in a GUI thread. */
    void queuedTaskStateChangeHandler(crimson::async::Task::State, QString);
    void queuedProgressMade(unsigned int);
    void queuedProgressStepsAdded(unsigned int);
    ///@} 

private:
    std::shared_ptr<async::Task> _task;

    unsigned int _stepsTotal = 0;
    unsigned int _stepsMade = 0;
    std::string _description;

    int _sequentialExecutionTag = -1;
    bool _silentFail = false;

    std::array<boost::signals2::scoped_connection, 3> _connections;
};


}
