#pragma once

// Parent classes
#include <QObject>
#include <mitkIContextMenuAction.h>

#include <unordered_set>

/*! \brief   A context menu action that performs blending of the vessels in a vessel tree. */
class BlendAction : public QObject, public mitk::IContextMenuAction
{
    Q_OBJECT
    Q_INTERFACES(mitk::IContextMenuAction)

public:
    BlendAction();
    ~BlendAction();

    // IContextMenuAction
    void Run(const QList<mitk::DataNode::Pointer> &selectedNodes) override;
    void SetDataStorage(mitk::DataStorage *) override { }
    void SetSmoothed(bool) override {}
    void SetDecimated(bool) override {}
    void SetFunctionality(berry::QtViewPart*) override {}

    /*!
     * \brief   Runs the blending operation for a vessel tree.
     *
     * \return  A set of visible loft model nodes actually used in blending.
     */
    std::unordered_set<mitk::DataNode*> Run(mitk::DataNode* node);

private:
    BlendAction(const BlendAction &) = delete;
    BlendAction& operator=(const BlendAction &) = delete;
};
