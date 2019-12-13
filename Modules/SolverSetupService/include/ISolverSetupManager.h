#pragma once

#include <ImmutableRanges.h>

#include "ISolverStudyData.h"
#include "ISolverParametersData.h"
#include "IBoundaryConditionSet.h"
#include "IMaterialData.h"

namespace crimson
{

/*! \brief   Solver setup manager interface. */
class ISolverSetupManager
{
public:

    /*!
     * \brief   Gets the name of the SolverSetupManager.
     */
    virtual gsl::cstring_span<> getName() const = 0;

    ///@{ 
    /*!
     * \brief   Gets list of names of types of boundary condition sets.
     */
    virtual ImmutableValueRange<gsl::cstring_span<>> getBoundaryConditionSetNameList() const = 0;

    /*!
     * \brief   Creates a boundary condition set with a particular type name selected from
     *  getBoundaryConditionSetNameList().
     */
    virtual IBoundaryConditionSet::Pointer createBoundaryConditionSet(gsl::cstring_span<> name) = 0;
    ///@} 

    ///@{ 
    /*!
    * \brief   Gets list of names of types of boundary conditions.
    */
    virtual ImmutableValueRange<gsl::cstring_span<>>
    getBoundaryConditionNameList(gsl::not_null<IBoundaryConditionSet*> ownerBCSet) const = 0;

    /*!
    * \brief   Creates a boundary condition with a particular type name selected from
    *  getBoundaryConditionNameList().
    */
    virtual IBoundaryCondition::Pointer createBoundaryCondition(gsl::not_null<IBoundaryConditionSet*> ownerBCSet,
                                                                gsl::cstring_span<> name) = 0;
    ///@} 

    ///@{ 
    /*!
    * \brief   Gets list of names of types of materials.
    */
    virtual ImmutableValueRange<gsl::cstring_span<>> getMaterialNameList() const = 0;

    /*!
    * \brief   Creates a material with a particular type name selected from
    *  getMaterialNameList().
    */
    virtual IMaterialData::Pointer createMaterial(gsl::cstring_span<> name) = 0;
    ///@} 

    ///@{ 
    /*!
    * \brief   Gets list of names of types of solver parameters.
    */
    virtual ImmutableValueRange<gsl::cstring_span<>> getSolverParametersNameList() const = 0;
    /*!
    * \brief   Creates a solver parameters object with a particular type name selected from
    *  getSolverParametersNameList().
    */
    virtual ISolverParametersData::Pointer createSolverParameters(gsl::cstring_span<> name) = 0;
    ///@} 

    ///@{ 
    /*!
    * \brief   Gets list of names of types of solver studies.
    */
    virtual ImmutableValueRange<gsl::cstring_span<>> getSolverStudyNameList() const = 0;
    /*!
    * \brief   Creates a solver study with a particular type name selected from
    *  getSolverStudyNameList().
    */
    virtual ISolverStudyData::Pointer createSolverStudy(gsl::cstring_span<> name) = 0;
    ///@} 

    virtual ~ISolverSetupManager() {}
};

} // end namespace crimson
