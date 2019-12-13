#pragma once

// Parent classes
#include <QObject>
#include <mitkIContextMenuAction.h>

/*! \brief   A context menu action allowing to export the vessel paths and contours in the vessel tree as a set of polylines. 
 * 
 *  This functionality is used for interfacing with Matlab code (see the matlab folder in the repository).
 */
class ExportVesselsAction : public QObject, public mitk::IContextMenuAction
{
    Q_OBJECT
    Q_INTERFACES(mitk::IContextMenuAction)

public:
    ExportVesselsAction();
    ~ExportVesselsAction();

    // IContextMenuAction
    void Run(const QList<mitk::DataNode::Pointer> &selectedNodes) override;
    void SetDataStorage(mitk::DataStorage *) override { }
    void SetSmoothed(bool) override {}
    void SetDecimated(bool) override {}
    void SetFunctionality(berry::QtViewPart*) override {}

private:
    ExportVesselsAction(const ExportVesselsAction &) = delete;
    ExportVesselsAction& operator=(const ExportVesselsAction &) = delete;
};
