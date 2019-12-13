#pragma once

#include "AsyncTask.h"

#include <boost/optional.hpp>

namespace crimson {
namespace async {

/*! \brief   An asynchronous task which stores the result of task execution. */
template<typename T>
class TaskWithResult : public Task {
public:

    /*!
     * \brief   Gets the result of an asynchronous operation.
     *
     * \return  The result or an empty optional if operation failed.
     */
    boost::optional<T> getResult() { return _result; }

protected:
    void setResult(const T& result)
    {
        _result = result;
    }

private:
    boost::optional<T> _result;
};

}
}