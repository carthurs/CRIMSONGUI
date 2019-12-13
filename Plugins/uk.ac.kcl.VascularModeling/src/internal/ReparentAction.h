#pragma once

// Parent classes
#include <QObject>
#include <mitkIContextMenuAction.h>

/*! \brief   A context menu action that allows changing the parent node. */
class ReparentAction : public QObject, public mitk::IContextMenuAction
{
    Q_OBJECT
    Q_INTERFACES(mitk::IContextMenuAction)

public:
    ReparentAction();
    ~ReparentAction();

    // IContextMenuAction
    void Run(const QList<mitk::DataNode::Pointer> &selectedNodes) override;
    void SetDataStorage(mitk::DataStorage *) override { }
    void SetSmoothed(bool) override {}
    void SetDecimated(bool) override {}
    void SetFunctionality(berry::QtViewPart*) override {}

private:
    ReparentAction(const ReparentAction &) = delete;
    ReparentAction& operator=(const ReparentAction &) = delete;
};
