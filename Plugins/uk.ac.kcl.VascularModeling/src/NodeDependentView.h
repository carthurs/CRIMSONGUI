#pragma once

#include <QmitkAbstractView.h>

#include <HierarchyManager.h>

#include "uk_ac_kcl_VascularModeling_Export.h"

namespace mitk {
    class DataNode;
}

class QLineEdit;
class QFrame;

/*! \brief   A convenience base class for all the views which track the selection of nodes of a particular type. */
class VASCULARMODELING_EXPORT NodeDependentView : public QmitkAbstractView
{
public:

    /*!
     * \brief   Constructor.
     *
     * \param   nodeType                        Type of the node to track.
     * \param   searchParent                    If set to true, the tracked node type is considered selected whenever any of its children is selected.
     * \param   title                           The node type name.
     * \param   keepOldNodeOnInvalidSelection   true to keep the previously selected tracked node when the new selection is invalid.
     */
    NodeDependentView(const crimson::HierarchyManager::NodeType& nodeType, bool searchParent, QString title, bool keepOldNodeOnInvalidSelection = false);
    ~NodeDependentView();


protected:
    virtual void currentNodeChanged(mitk::DataNode* /*prevNode*/) {}
    virtual void currentNodeModified() {}

    /*!
     * \brief   Creates 'selected node' widget, which contains a QLabel with the node type name and a QLineEdit displaying the current tracked node.
     */
    QFrame* createSelectedNodeWidget(QWidget* parent);

protected slots:
    void initializeCurrentNode();
    void setCurrentNode(const mitk::DataNode* node);

protected:
    void OnSelectionChanged(berry::IWorkbenchPart::Pointer part, const QList<mitk::DataNode::Pointer> &nodes) override;
    void NodeAdded(const mitk::DataNode* node) override;
    void NodeChanged(const mitk::DataNode* node) override;
    void NodeRemoved(const mitk::DataNode* node) override;
    mitk::DataNode* currentNode() const { return _currentNode; }


private:
    void _updateLineEditText();
    unsigned long _observerTag;

    QLineEdit* _selectedNodeLineEdit;
    mitk::DataNode* _currentNode;
    const crimson::HierarchyManager::NodeType& _nodeType;
    bool _searchParent;
    QString _title;
    bool _keepOldNodeOnInvalidSelection;
};

