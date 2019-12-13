#pragma once

#include <QObject>
#include <mitkIContextMenuAction.h>

/*! \brief   A context menu action allowing to import the vessel paths and contours stored as a set of polylines.
*
*  This functionality is used for interfacing with Matlab code (see the matlab folder in the repository).
*/
class ImportVesselsAction : public QObject, public mitk::IContextMenuAction
{
    Q_OBJECT
    Q_INTERFACES(mitk::IContextMenuAction)

public:
    ImportVesselsAction();
    ~ImportVesselsAction();

    // IContextMenuAction
    void Run(const QList<mitk::DataNode::Pointer> &selectedNodes) override;
    void SetDataStorage(mitk::DataStorage *) override { }
    void SetSmoothed(bool) override {}
    void SetDecimated(bool) override {}
    void SetFunctionality(berry::QtViewPart*) override {}

private:
    ImportVesselsAction(const ImportVesselsAction &) = delete;
    ImportVesselsAction& operator=(const ImportVesselsAction &) = delete;
};
