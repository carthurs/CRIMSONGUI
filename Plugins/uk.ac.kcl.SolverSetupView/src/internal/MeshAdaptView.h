#pragma once

#include <NodeDependentView.h>

#include "ui_MeshAdaptView.h"

#include <AsyncTaskManager.h>
#include <AsyncTaskWithResult.h>

class QtProperty;

namespace crimson
{
class TaskStateObserver;
}

/*! \brief   MeshAdaptView allows to create optimized mesh based on the original mesh and the existing solution. */
class MeshAdaptView : public NodeDependentView
{
    Q_OBJECT

public:
    static const std::string VIEW_ID;

    MeshAdaptView();
    ~MeshAdaptView();

    void OnSelectionChanged(berry::IWorkbenchPart::Pointer /*source*/, const QList<mitk::DataNode::Pointer>& nodes) override;

private slots:
    void adaptMesh();
    void meshAdaptFinished(crimson::async::Task::State state);

private:
    void CreateQtPartControl(QWidget* parent) override;
    void SetFocus() override;

    void currentNodeChanged(mitk::DataNode* prevNode) override;
    void currentNodeModified() override;

    void _fillAdaptationPropertyValues();
    void _setSpinBoxValueFromProperty(QDoubleSpinBox* spinBox, const char* propertyName);
    void _setPropertyValueFromSpinBox(QDoubleSpinBox* spinBox, const char* propertyName);
    void _setPropertyFromCurrentErrorIndicatorNode();
    void _setCurrentErrorIndicatorNodeFromProperty();
    void _updateUI();

    std::string _getMeshAdaptTaskUID();

    void _setupDependentNodes();
    mitk::DataNode* _getMeshNode();
    mitk::DataNode* _getAdaptationParametersNode();

    void NodeAdded(const mitk::DataNode* node) override;
    void NodeRemoved(const mitk::DataNode* node) override;

private:
    // Ui and main widget of this view
    Ui::MeshAdaptWidget _UI;
    QMetaObject::Connection _errorIndicatorComboBoxConnection;

    crimson::TaskStateObserver* _meshAdaptTaskStateObserver = nullptr;

    mitk::DataNode* _currentMeshNode = nullptr;
    mitk::DataNode* _currentAdaptationParametersNode = nullptr;
};

namespace detail
{
/*! \brief   An asynchronous task adapter which duplicates a solver study if adaptation finishes successfully. */
class CreateNewStudyDataAsyncTask : public crimson::QAsyncTaskAdapter
{
    Q_OBJECT
public:
    CreateNewStudyDataAsyncTask(std::shared_ptr<crimson::async::TaskWithResult<mitk::BaseData::Pointer>> childDataCreatorTask,
                                std::map<std::string, mitk::BaseProperty::Pointer> propertiesForNewNode,
                                mitk::DataNode::Pointer originalStudyNode, mitk::DataNode::Pointer originalMeshNode,
                                std::vector<mitk::DataNode::Pointer> solutionNodes);

protected slots:
    void processTaskStateChange(crimson::async::Task::State state, QString);

protected:
    std::shared_ptr<crimson::async::TaskWithResult<mitk::BaseData::Pointer>> task;
    std::map<std::string, mitk::BaseProperty::Pointer> propertiesForNewNode;
    mitk::DataNode::Pointer originalStudyNode;
    mitk::DataNode::Pointer originalMeshNode;
    std::vector<mitk::DataNode::Pointer> solutionNodes;
};
}
