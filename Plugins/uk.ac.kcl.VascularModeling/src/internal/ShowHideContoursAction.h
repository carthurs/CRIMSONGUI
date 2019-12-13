#pragma once

// Parent classes
#include <QObject>
#include <mitkIContextMenuAction.h>

#include <unordered_set>

/*! \brief   A context menu action that allows showing and hiding all the contours belonging to a vessel path. */
class ShowHideContoursAction : public QObject, public mitk::IContextMenuAction
{
    Q_OBJECT
    Q_INTERFACES(mitk::IContextMenuAction)
public:
    ShowHideContoursAction(bool show);
    ~ShowHideContoursAction();

    // IContextMenuAction
    void Run(const QList<mitk::DataNode::Pointer> &selectedNodes) override;
    void SetDataStorage(mitk::DataStorage*) override { }
    void SetSmoothed(bool) override {}
    void SetDecimated(bool) override {}
    void SetFunctionality(berry::QtViewPart*) override {}

    // Additional functionality
    static void setContourVisibility(mitk::DataNode* contourNode, bool show);
    static void setAllVesselContoursVisibility(mitk::DataNode* vesselPathNode, bool show);

private:
    ShowHideContoursAction(const ShowHideContoursAction &) = delete;
    ShowHideContoursAction& operator=(const ShowHideContoursAction &) = delete;

    bool _show;
};

class ShowContoursAction : public ShowHideContoursAction {
    Q_OBJECT
    Q_INTERFACES(mitk::IContextMenuAction)
public:
    ShowContoursAction() : ShowHideContoursAction(true) {}
private:
    ShowContoursAction(const ShowHideContoursAction &) = delete;
    ShowContoursAction& operator=(const ShowHideContoursAction &) = delete;
};

class HideContoursAction : public ShowHideContoursAction {
    Q_OBJECT
    Q_INTERFACES(mitk::IContextMenuAction)
public:
    HideContoursAction() : ShowHideContoursAction(false) {}
private:
    HideContoursAction(const ShowHideContoursAction &) = delete;
    HideContoursAction& operator=(const ShowHideContoursAction &) = delete;
};
