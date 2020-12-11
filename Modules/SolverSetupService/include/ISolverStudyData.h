#pragma once

#include <mitkBaseData.h>

#include "SolutionData.h"
#include <ImmutableRanges.h>

namespace crimson
{

/*! \brief   A data provider interface for discovery of data by the UID's. */
class IDataProvider {
public:
    virtual ~IDataProvider() = default; 

    virtual mitk::BaseData* findDataByUID(gsl::cstring_span<> uid) const = 0;

    // TODO: this method only exists because BC set doesn't know the BC's that it owns. Should be fixed.
    virtual std::vector<mitk::BaseData*> getChildrenData(gsl::cstring_span<> parentUID) const = 0;
};

/*! \brief   A solver study data interface. */
class ISolverStudyData : public mitk::BaseData
{
public:
    mitkClassMacro(ISolverStudyData, BaseData);

    ///@{ 
    /*!
     * \brief   Gets mesh node UID used in the study.
     */
    virtual gsl::cstring_span<> getMeshNodeUID() const = 0;

    /*!
     * \brief   Sets mesh node UID to use in the study.
     */
    virtual void setMeshNodeUID(gsl::cstring_span<> nodeUID) = 0;
    ///@} 

    ///@{ 
    /*!
     * \brief   Gets solver parameters node UID used in the study.
     */
    virtual gsl::cstring_span<> getSolverParametersNodeUID() const = 0;

    /*!
     * \brief   Sets solver parameters node UID to use in the study.
     */
    virtual void setSolverParametersNodeUID(gsl::cstring_span<> nodeUID) = 0;
    ///@} 

    ///@{ 
    /*!
     * \brief   Gets the UID's of boundary condition set nodes used in the study.
     */
    virtual ImmutableValueRange<gsl::cstring_span<>> getBoundaryConditionSetNodeUIDs() const = 0;

    /*!
     * \brief   Sets the UID's of boundary condition set nodes to use in the study.
     */
    virtual void setBoundaryConditionSetNodeUIDs(ImmutableValueRange<gsl::cstring_span<>> nodeUIDs) = 0;
    ///@} 

    ///@{ 
    /*!
    * \brief   Gets the UID's of material nodes used in the study.
    */
    virtual ImmutableValueRange<gsl::cstring_span<>> getMaterialNodeUIDs() const = 0;
    /*!
    * \brief   Sets the UID's of material nodes to use in the study.
    */
    virtual void setMaterialNodeUIDs(ImmutableValueRange<gsl::cstring_span<>> nodeUIDs) = 0;
    ///@} 

	///@{ 
	/*!
	* \brief   Gets the UID's of particle bolus mesh nodes used in the study.
	*/
	virtual gsl::cstring_span<> getParticleBolusMeshNodeUID() const = 0;
	/*!
	* \brief   Sets the UID's of particle bolus mesh nodes to use in the study.
	*/
	virtual void setParticleBolusMeshNodeUID(gsl::cstring_span<> nodeUID) = 0;
	///@}

	///@{ 
	/*!
	* \brief   Gets the UID's of particle bin mesh nodes used in the study.
	*/
	virtual ImmutableValueRange<gsl::cstring_span<>> getParticleBinMeshNodeUIDs() const = 0;
	/*!
	* \brief   Sets the UID's of particle bin mesh nodes to use in the study.
	*/
	virtual void setParticleBinMeshNodeUIDs(ImmutableValueRange<gsl::cstring_span<>> nodeUIDs) = 0;
	///@} 

    /*!
     * \brief   Writes a solver setup.
     *
     * \param   dataProvider        The data provider.
     * \param   vesselForestData    The vessel tree data (may be null).
     * \param   solidModelData      The solid model.
     * \param   solutions           The solution data objects to be written alongside the solver output.
     */
    virtual bool writeSolverSetup(const IDataProvider& dataProvider, const mitk::BaseData* vesselForestData,
                                  gsl::not_null<const mitk::BaseData*> solidModelData,
								  gsl::span<const SolutionData*> solutions, const bool setupParticleProblem) = 0;
	/*!
	* \brief   Runs the simulation using a flowsolver.
	*/
	virtual bool runFlowsolver() = 0;

	/*!
	* \brief   Gets the particle tracking simulation folder, and may do some additional internal path setup.
	*/
	virtual std::string setupParticleTrackingPathsAndGetParticleTrackingFolder() = 0;

    /*!
     * \brief   Loads the computed solution.
     */
    virtual std::vector<SolutionData::Pointer> loadSolution() = 0;

    /*!
     * \brief   Computes the material values.
     *
     * \param   dataProvider        The data provider.
     * \param   vesselForestData    The vessel tree data (may be null).
     * \param   solidModelData      The solid model.
     */
    virtual std::vector<SolutionData::Pointer> computeMaterials(const IDataProvider& dataProvider,
                                                                const mitk::BaseData* vesselForestData,
                                                                gsl::not_null<const mitk::BaseData*> solidModelData) = 0;

protected:
    ISolverStudyData() { mitk::BaseData::InitializeTimeGeometry(1); }
    virtual ~ISolverStudyData() {}

    ISolverStudyData(const Self& other) = default;

    void SetRequestedRegion(const itk::DataObject*) override {}
    void SetRequestedRegionToLargestPossibleRegion() override {}
    bool RequestedRegionIsOutsideOfTheBufferedRegion() override { return true; }
    bool VerifyRequestedRegion() override { return true; }
};

} // end namespace crimson
