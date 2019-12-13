#pragma once

// Parent classes
#include <QObject>
#include <mitkIContextMenuAction.h>

/*! \brief   A context menu action that performs meshing of a solid model. */
class MeshAction : public QObject, public mitk::IContextMenuAction
{
    Q_OBJECT
    Q_INTERFACES(mitk::IContextMenuAction)

public:
    MeshAction();
    ~MeshAction();

    // IContextMenuAction
    void Run(const QList<mitk::DataNode::Pointer> &selectedNodes) override;
    void SetDataStorage(mitk::DataStorage*) override {}
    void SetSmoothed(bool) override {}
    void SetDecimated(bool) override {}
    void SetFunctionality(berry::QtViewPart*) override {}

private:
    MeshAction(const MeshAction &) = delete;
    MeshAction& operator=(const MeshAction &) = delete;
};
