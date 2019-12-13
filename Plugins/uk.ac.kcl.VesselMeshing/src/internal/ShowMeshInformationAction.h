#pragma once

// Parent classes
#include <QObject>
#include <mitkIContextMenuAction.h>

#include "MeshInformationDialog.h"

/*! \brief   A context menu action that brings up the MeshInformationDialog. */
class ShowMeshInformationAction : public QObject, public mitk::IContextMenuAction
{
    Q_OBJECT
    Q_INTERFACES(mitk::IContextMenuAction)

public:
    ShowMeshInformationAction();
    ~ShowMeshInformationAction();

    // IContextMenuAction
    void Run(const QList<mitk::DataNode::Pointer> &selectedNodes) override;
    void SetDataStorage(mitk::DataStorage *) override {}
    void SetSmoothed(bool) override {}
    void SetDecimated(bool) override {}
    void SetFunctionality(berry::QtViewPart*) override {}

private:
    ShowMeshInformationAction(const ShowMeshInformationAction &) = delete;
    ShowMeshInformationAction& operator=(const ShowMeshInformationAction &) = delete;

    static std::unique_ptr<MeshInformationDialog> _meshInformationDialog;
};
