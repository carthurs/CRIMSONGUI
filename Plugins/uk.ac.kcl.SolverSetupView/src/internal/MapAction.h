#pragma once

// Parent classes
#include <QObject>
#include <mitkIContextMenuAction.h>

#include <CreateDataNodeAsyncTask.h>

/*! \brief   A context menu action that performs lofting of the vessel paths. */
class MapAction : public QObject, public mitk::IContextMenuAction
{
    Q_OBJECT
    Q_INTERFACES(mitk::IContextMenuAction)

public:
	MapAction();
	~MapAction();

    // IContextMenuAction
    void Run(const QList<mitk::DataNode::Pointer> &selectedNodes) override;
    void SetDataStorage(mitk::DataStorage *) override {}
    void SetSmoothed(bool) override {}
    void SetDecimated(bool) override {}
    void SetFunctionality(berry::QtViewPart*) override {}

    std::shared_ptr<crimson::CreateDataNodeAsyncTask> Run(const mitk::DataNode::Pointer& node);

private:
	MapAction(const MapAction &) = delete;
	MapAction& operator=(const MapAction &) = delete;

	void interpolateContour(const mitk::DataNode* contourNodes[2]);

};
