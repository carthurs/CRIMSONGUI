#pragma once

#include <string>
#include <exception>

#include <boost/signals2.hpp>

#include "AsyncTaskExports.h"

namespace crimson {
namespace async {

/*! \brief   The asynchronous task base class. */
class AsyncTask_EXPORT Task {
public:
    /*! \brief   Values that represent execution states. */
    enum State {
        State_Idle, ///< Task awaits for an available thread to be executed.
        State_Starting, ///< Task has been added to the thread but the execution hasn't started yet.
        State_Running, ///< Task is being executed on a thread.
        State_Cancelling, ///< Task is still running, but the cancellation request has been issued.
        
        // Terminal states 
        State_Cancelled,    ///< Task has been cancelled.
        State_Failed,       ///< An error occurred during task execution.
        State_Finished      ///< Task has successfully finished execution.
    };

    /*!
     * \brief   Check if a task state is terminal, meaning that the task has completed running.
     */
    static bool isStateTerminal(State state) { return state == State_Cancelled || state == State_Failed || state == State_Finished; }

    Task() = default;
    Task(const Task&) = default;
    virtual ~Task() {}

    /*! \name Task state change and progress signals */
    ///@{ 
    boost::signals2::signal<void(Task::State, const std::string&)> taskStateChangedSignal;  ///< The task state changed signal
    boost::signals2::signal<void(unsigned int)> stepsAddedSignal;   ///< Progress reporting: add a number of steps.
    boost::signals2::signal<void(unsigned int)> progressMadeSignal; ///< Progress reporting: steps completed.
    ///@} 


    /*! \brief   Prepares the task execution and runs the task. */
    void run();

    /*!
     * \brief   Requests the task to cancel itself. For cancellation to work, the task's runTask()
     *  method must periodically call isCancelled() and react accordingly.
     *  
     *  Can be overridden to implement custom behavior, but the base class implementation of cancel()
     *  should be called to ensure proper behavior.
     */
	virtual void cancel();

    ///@{ 
    /*!
     * \brief   Sets the task state.
     *
     * \param   state   The state.
     * \param   message The optional message associated with the state change.
     */
    void setState(State state, const std::string& message = "");

    /*!
     * \brief   Gets the current task state.
     */
    State getState() const { return _state; }
    ///@} 

    /*!
     * \brief   Gets the message associated with the last task execution state change.
     */
    const std::string& getLastStateChangeMessage() const { return _lastStateChangeMessage; }

protected:

    /*!
     * \brief   Main execution method for subclasses to override. 
     *
     * \return  One of the terminal states and a message (e.g. error message).
     */
    virtual std::tuple<State, std::string> runTask() = 0; ///< 

    /*!
     * \brief   Call this method in your override of runTask() to check if a cancellation request has been issued.
     */
    bool isCancelling() const;
    
private:
    State _state = State_Idle;
    std::string _lastStateChangeMessage;
};

}
}
