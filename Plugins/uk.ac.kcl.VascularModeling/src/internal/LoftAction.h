#pragma once

// Parent classes
#include <QObject>
#include <mitkIContextMenuAction.h>

#include <CreateDataNodeAsyncTask.h>

/*! \brief   A context menu action that performs lofting of the vessel paths. */
class LoftAction : public QObject, public mitk::IContextMenuAction
{
    Q_OBJECT
    Q_INTERFACES(mitk::IContextMenuAction)

public:
    LoftAction(bool preview = false, double interContourDistance = 1.0);
    ~LoftAction();

    // IContextMenuAction
    void Run(const QList<mitk::DataNode::Pointer> &selectedNodes) override;
    void SetDataStorage(mitk::DataStorage *) override {}
    void SetSmoothed(bool) override {}
    void SetDecimated(bool) override {}
    void SetFunctionality(berry::QtViewPart*) override {}

    std::shared_ptr<crimson::CreateDataNodeAsyncTask> Run(const mitk::DataNode::Pointer& node);

private:
    LoftAction(const LoftAction &) = delete;
    LoftAction& operator=(const LoftAction &) = delete;

    bool _preview;
    double _interContourDistance;
};
