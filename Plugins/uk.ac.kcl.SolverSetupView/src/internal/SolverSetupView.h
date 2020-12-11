#pragma once

#include <gsl.h>


#include <QmitkAbstractView.h>

#include "ui_SolverSetupView.h"
#include "ui_BoundaryConditionTypeSelectionDialog.h"

#include <HierarchyManager.h>

#include <ImmutableRanges.h>

#include "SolverStudyUIDsTableModel.h"

#include <AsyncTaskManager.h>
#include <AsyncTaskWithResult.h>


namespace crimson
{
class TaskStateObserver;
class ISolverSetupManager;
}

/*! \brief   SolverSetupView allows the user to set up the solver and to write the prepared simulation to disk. */
class SolverSetupView : public QmitkAbstractView
{
    Q_OBJECT

public:
    static const std::string VIEW_ID;

    SolverSetupView();
    ~SolverSetupView();

    void OnSelectionChanged(berry::IWorkbenchPart::Pointer source, const QList<mitk::DataNode::Pointer>& nodes) override;
    void NodeRemoved(const mitk::DataNode* node) override;
    void NodeChanged(const mitk::DataNode* node) override;

private slots:
    void writeSolverSetup();
	void runSimulation();
	void runLagrangianParticleTrackingAnalysis();
	void setupLagrangianParticleTrackingAnalysis();
	void finishedLagrangianParticleTrackingAnalysis(crimson::async::Task::State state);
    void loadSolution();
    void showSolution(bool show);
    void createSolverRoot();
    void createSolverParameters();
    void createSolverStudy();
    void createBoundaryConditionSet();
    void createBoundaryCondition();
    void createMaterial();
    void showMaterial();
    void exportMaterials();
    void setMeshNodeForStudy(const mitk::DataNode*);
	void setParticleBolusNodeForStudy(const mitk::DataNode*);
    void setSolverParametersNodeForStudy(const mitk::DataNode*);

private:
    void CreateQtPartControl(QWidget* parent) override;
    void SetFocus() override;

    void _updateUI();

    template <class T, class... Args>
    void createSolverObject(mitk::DataNode* ownerNode, Args... additionalArgs);

private:
    // Ui and main widget of this view
    Ui::SolverSetupWidget _UI;

	void _setupAdvancedParticleTrackingOptionsUI();

    void _setCurrentRootNode(mitk::DataNode* node);
    void _setCurrentSolverRootNode(mitk::DataNode* node);

    void _findCurrentSolverSetupManager();

    void _setCurrentBoundaryConditionSetNode(mitk::DataNode* node);
    void _setCurrentBoundaryConditionNode(mitk::DataNode* node);
    void _setCurrentMaterialNode(mitk::DataNode* node);
    void _setCurrentSolverParametersNode(mitk::DataNode* node);
    void _setCurrentSolverStudyNode(mitk::DataNode* node);
    void _setupSolverStudyComboBoxes();

	void _setupSimulationProblem(const bool isParticleProblem);

    void _selectDataNode(mitk::DataNode* node);

    void _serviceChanged(const us::ServiceEvent event);

    void _solverStudyNodeModified();
    unsigned long _solverStudyObserverTag = -1;

    void _ensureApplyToAllWalls(mitk::DataNode* node) const;
    gsl::cstring_span<> _getTypeNameToCreate(crimson::ImmutableValueRange<gsl::cstring_span<>> options);

    mitk::DataNode* _currentSolverRootNode = nullptr;
    mitk::DataNode* _currentRootNode = nullptr;
    crimson::HierarchyManager::NodeType _currentRootNodeType;
    mitk::DataNode* _currentVesselTreeNode = nullptr;
    mitk::DataNode* _currentBoundaryConditionSetNode = nullptr;
    mitk::DataNode* _currentBoundaryConditionNode = nullptr;
    mitk::DataNode* _currentMaterialNode = nullptr;
    mitk::DataNode* _currentSolverStudyNode = nullptr;
    mitk::DataNode* _currentSolverParametersNode = nullptr;
    mitk::DataNode* _currentSolidNode = nullptr;
    crimson::ISolverSetupManager* _currentSolverSetupManager = nullptr;

    struct RemoveNodeDeleter {
        void operator()(mitk::DataNode* node);
    };

    std::unique_ptr<mitk::DataNode, RemoveNodeDeleter> _materialVisNodePtr;

    Ui::boundaryConditionTypeSelectionDialog _typeSelectionDialogUi;
    QDialog _typeSelectionDialog;


    crimson::SolverStudyUIDsTableModel _solverStudyBCSetsModel;
    crimson::SolverStudyUIDsTableModel _solverStudyMaterialsModel;
	crimson::SolverStudyUIDsTableModel _particleTrackingGeometriesModel;

	crimson::TaskStateObserver* _particleSimulationTaskStateObserver = nullptr;

	std::shared_ptr<crimson::async::TaskWithResult<bool>> createParticleTrackingTask(const bool runReductionStep,
		const bool writeParticleConfigJson,
		const bool convertFluidSimToHdf5,
		const bool runActualParticleTrackingStep,
		const std::string config_file);

	bool _runReductionStep = true;
	bool _writeParticleConfigJson = true;
	bool _convertFluidSimToHdf5 = true;
	bool _runActualParticleTrackingStep = true;
};

namespace detail {
	bool checkCrimsonParticlesAvailable();

	class ParticleTrackingTask : public crimson::async::TaskWithResult<bool>
	{
	public:
		ParticleTrackingTask(const bool runReductionStep,
			const bool writeParticleConfigJson,
			const bool convertFluidSimToHdf5,
			const bool runActualParticleTrackingStep,
			const std::string config_file)
			: _runReductionStep(runReductionStep)
			, _writeParticleConfigJson(writeParticleConfigJson)
			, _convertFluidSimToHdf5(convertFluidSimToHdf5)
			, _runActualParticleTrackingStep(runActualParticleTrackingStep)
			, _config_file(config_file) {};

	private:

		std::tuple<State, std::string> runTask() override;

		const bool _runReductionStep;
		const bool _writeParticleConfigJson;
		const bool _convertFluidSimToHdf5;
		const bool _runActualParticleTrackingStep;
		const std::string _config_file;
	};
}