#pragma once

#include <QObject>
#include <QAbstractButton>
#include <QAction>
#include <AsyncTaskManager.h>

#include "uk_ac_kcl_AsyncTaskManager_Export.h"

namespace crimson {

    /*! \ brief A utility class for pairs of QAbstractButtons controlling the execution of an asynchronous task in UI. 
     *  
     *  The main purpose is to enable/disable the buttons according to the task state.
     */
    class ASYNCTASKMANAGER_EXPORT TaskStateObserver : public QObject {
        Q_OBJECT
    public:
        TaskStateObserver(QObject* runTaskButtonOrAction, QObject* cancelTaskButtonOrAction = nullptr, QObject* parent = nullptr)
            : QObject(parent) 
            , _runTaskButton(qobject_cast<QAbstractButton*>(runTaskButtonOrAction))
            , _cancelTaskButton(qobject_cast<QAbstractButton*>(cancelTaskButtonOrAction))
            , _runTaskAction(qobject_cast<QAction*>(runTaskButtonOrAction))
            , _cancelTaskAction(qobject_cast<QAction*>(cancelTaskButtonOrAction))
        {
            _connectIfNotNull(_runTaskButton, &QAbstractButton::clicked, &TaskStateObserver::runTaskRequested);
            _connectIfNotNull(_cancelTaskButton, &QAbstractButton::clicked, &TaskStateObserver::cancelTask);
            _connectIfNotNull(_runTaskAction, &QAction::triggered, &TaskStateObserver::runTaskRequested);
            _connectIfNotNull(_cancelTaskAction, &QAction::triggered, &TaskStateObserver::cancelTask);

            connect(AsyncTaskManager::getInstance(), &AsyncTaskManager::taskStateChanged, this, &TaskStateObserver::onTaskStateChanged);
            _updateButtonEnabledStates();
        }
        
        void setEnabled(bool enabled)
        {
            _enabled = enabled;
            _updateButtonEnabledStates();
        }
        
        void setPrimaryObservedUID(const AsyncTaskManager::TaskUID& uid)
        {
            _primaryObservedUID = uid;
            _updateButtonEnabledStates();
        }

        void setSecondaryObservedUIDs(const std::vector<AsyncTaskManager::TaskUID>& uids)
        {
            _secondaryObservedUIDs = uids;
            _updateButtonEnabledStates();
        }
        
    signals:
        void runTaskRequested();
        void taskFinished(async::Task::State finalState);
        
    private slots:
        void onTaskStateChanged(const AsyncTaskManager::TaskUID& uid, async::Task::State state)
        {
            if (uid == _primaryObservedUID) {
                if (async::Task::isStateTerminal(state)) {
                    emit taskFinished(state);
                }
            }

            _updateButtonEnabledStates();
        }

        void cancelTask()
        {
            AsyncTaskManager::getInstance()->cancelTask(_primaryObservedUID);
        }
        
    private:
        enum EnableState {
            esNoChange,
            esEnable,
            esDisable
        };

        void _updateButtonEnabledStates()
        {
            EnableState enableRunTaskButton = esEnable;
            EnableState enableCancelTaskButton = esNoChange;

            if (!_enabled) {
                enableRunTaskButton = esDisable;
            } else {
                std::shared_ptr<QAsyncTaskAdapter> primaryTask = AsyncTaskManager::getInstance()->findTask(_primaryObservedUID);
                if (primaryTask) {
                    async::Task::State primaryTaskState = primaryTask->getTask()->getState();
                    enableCancelTaskButton = (primaryTaskState == async::Task::State_Starting || primaryTaskState == async::Task::State_Running) ? esEnable : esDisable;
                    enableRunTaskButton = async::Task::isStateTerminal(primaryTaskState) ? esEnable : esDisable;
                }

                for (const AsyncTaskManager::TaskUID& secondaryTaskUID : _secondaryObservedUIDs) {
                    std::shared_ptr<QAsyncTaskAdapter> task = AsyncTaskManager::getInstance()->findTask(secondaryTaskUID);

                    if (task && !async::Task::isStateTerminal(task->getTask()->getState())) {
                        enableRunTaskButton = esDisable;
                        break;
                    }
                }
            }

            _enableIfNotNull(_runTaskButton, enableRunTaskButton);
            _enableIfNotNull(_cancelTaskButton, enableCancelTaskButton);
            _enableIfNotNull(_runTaskAction, enableRunTaskButton);
            _enableIfNotNull(_cancelTaskAction, enableCancelTaskButton);
        }

        template<typename T>
        void _enableIfNotNull(T* object, EnableState enableState)
        {
            if (object && enableState != esNoChange) {
                object->setEnabled(enableState == esEnable);
            }
        }

        template<typename T, typename Func1, typename Func2>
        void _connectIfNotNull(T* object, Func1 signal, Func2 slot)
        {
            if (object) {
                connect(object, signal, this, slot);
            }
        }

        bool _enabled = false;

        AsyncTaskManager::TaskUID _primaryObservedUID;
        std::vector<AsyncTaskManager::TaskUID> _secondaryObservedUIDs;
        QAbstractButton* _runTaskButton;
        QAbstractButton* _cancelTaskButton;
        QAction* _runTaskAction;
        QAction* _cancelTaskAction;
    };

}