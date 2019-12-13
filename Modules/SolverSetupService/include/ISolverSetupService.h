#pragma once

#include <gsl.h>
#include <memory>

namespace crimson {

class ISolverSetupManager;

/*! \brief   A solver setup service interface. */
class ISolverSetupService
{
public:

    /*!
     * \brief   Gets solver setup manager.
     */
    virtual gsl::not_null<ISolverSetupManager*> getSolverSetupManager() const = 0;
    virtual ~ISolverSetupService() {}
};

}