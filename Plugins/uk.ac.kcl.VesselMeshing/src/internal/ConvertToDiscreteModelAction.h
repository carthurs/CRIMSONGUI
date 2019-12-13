#pragma once

// Parent classes
#include <QObject>
#include <mitkIContextMenuAction.h>

/*! \brief   A context menu action that brings up the MeshInformationDialog. */
class ConvertToDiscreteModelAction : public QObject, public mitk::IContextMenuAction
{
    Q_OBJECT
    Q_INTERFACES(mitk::IContextMenuAction)

public:
    ConvertToDiscreteModelAction();
    ~ConvertToDiscreteModelAction();

    // IContextMenuAction
    void Run(const QList<mitk::DataNode::Pointer> &selectedNodes) override;
    void SetDataStorage(mitk::DataStorage *) override {}
    void SetSmoothed(bool) override {}
    void SetDecimated(bool) override {}
    void SetFunctionality(berry::QtViewPart*) override {}

private:
    ConvertToDiscreteModelAction(const ConvertToDiscreteModelAction &) = delete;
    ConvertToDiscreteModelAction& operator=(const ConvertToDiscreteModelAction &) = delete;
};
