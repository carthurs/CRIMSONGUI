#include "AsyncTask.h"


namespace crimson {
namespace async {

    void Task::run()
    {
        if (isCancelling()) {
            setState(State_Cancelled, std::string("Operation cancelled."));
            return;
        }

        setState(State_Running);
        try {
            State resultState;
            std::string message; 
            std::tie(resultState, message) = runTask();

            // In case the actual task didn't react to cancellation request, process it here
            setState(isCancelling() ? State_Cancelled : resultState, message);
        }
        catch (std::exception& exc) {
            setState(State_Failed, exc.what());
        }
        catch (...) {
            setState(State_Failed, "Unhandled exception during task execution");
        }
    }

    void Task::cancel()
    {
        if (!isStateTerminal(_state)) {
            setState(State_Cancelling);
        }
    }

    bool Task::isCancelling() const
    {
        return _state == State_Cancelling;
    }

    void Task::setState(State state, const std::string& message)
    {
        _state = state;
        _lastStateChangeMessage = message;
        taskStateChangedSignal(state, message);
    }

}
}

