#include <mitkNodePredicateProperty.h>

#include "VesselPathAbstractData.h"
#include "NodeDependentView.h"

#include <HierarchyManager.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>


NodeDependentView::NodeDependentView(const crimson::HierarchyManager::NodeType& nodeType, bool searchParent, QString title, bool keepOldNodeOnInvalidSelection)
: _observerTag(0)
, _selectedNodeLineEdit(nullptr)
, _currentNode(nullptr)
, _nodeType(nodeType)
, _searchParent(searchParent)
, _title(title)
, _keepOldNodeOnInvalidSelection(keepOldNodeOnInvalidSelection)
{
}

NodeDependentView::~NodeDependentView()
{
    if (_currentNode) {
        _currentNode->GetData()->RemoveObserver(_observerTag);
    }
}

void NodeDependentView::initializeCurrentNode()
{
    OnSelectionChanged(GetSite()->GetPart(), GetDataManagerSelection());
}

void NodeDependentView::NodeAdded(const mitk::DataNode* /*node*/)
{
}

void NodeDependentView::NodeChanged(const mitk::DataNode* node)
{
    if (node == _currentNode && node->GetName() != _selectedNodeLineEdit->text().toStdString()) {
        _updateLineEditText();
    }
}

void NodeDependentView::NodeRemoved(const mitk::DataNode* node)
{
    if (_currentNode == node) {
        setCurrentNode(nullptr);
    }
}

void NodeDependentView::OnSelectionChanged(berry::IWorkbenchPart::Pointer /*part*/, const QList<mitk::DataNode::Pointer> &nodes)
{
    auto hm = crimson::HierarchyManager::getInstance();
    // Try to find selected tracked node
    for (const mitk::DataNode::Pointer& node : nodes) {
        if (!GetDataStorage()->Exists(node)) {
            continue;
        }

        if (hm->getPredicate(_nodeType)->CheckNode(node)) {
            setCurrentNode(node);
            return;
        }
    }

    // Try to find tracked node among selected nodes' parents
    if (_searchParent) {
        for (const mitk::DataNode::Pointer& node : nodes) {
            if (!GetDataStorage()->Exists(node)) {
                continue;
            }

            mitk::DataNode::Pointer parent = hm->getAncestor(node, _nodeType);
            if (parent) {
                setCurrentNode(parent);
                return;
            }
        }
    }

    if (!_keepOldNodeOnInvalidSelection) {
        setCurrentNode(nullptr);
    }
}

void NodeDependentView::setCurrentNode(const mitk::DataNode* vesselPathNode)
{
    if (_currentNode == vesselPathNode) {
        return;
    }

    mitk::DataNode* prevVesselPathNode = _currentNode;

    // Cleanup old connections
    if (_currentNode) {
        // Unsubscribe from old change events
        _currentNode->GetData()->RemoveObserver(_observerTag);
        _observerTag = 0;
    }

    _currentNode = const_cast<mitk::DataNode*>(vesselPathNode);

    // Listen to change events
    if (_currentNode) {
        // Subscribe to new change events
        auto modifiedCommand = itk::SimpleMemberCommand<NodeDependentView>::New();
        modifiedCommand->SetCallbackFunction(this, &NodeDependentView::currentNodeModified);
        _observerTag = _currentNode->GetData()->AddObserver(itk::ModifiedEvent(), modifiedCommand);
    }

    _updateLineEditText();
    currentNodeChanged(prevVesselPathNode);
}

void NodeDependentView::_updateLineEditText()
{
    if (_selectedNodeLineEdit) {
        _selectedNodeLineEdit->setText(_currentNode ? QString::fromStdString(_currentNode->GetName()) : "");
        _selectedNodeLineEdit->setToolTip(_currentNode ? _selectedNodeLineEdit->text() : _selectedNodeLineEdit->placeholderText());
    }
}

QFrame* NodeDependentView::createSelectedNodeWidget(QWidget* parent)
{
    QFrame* frame = new QFrame(parent);
    frame->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout* layout = new QHBoxLayout(frame);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(new QLabel(_title + ":", parent));
    if (!_selectedNodeLineEdit) {
        _selectedNodeLineEdit = new QLineEdit(parent);
    }
    _selectedNodeLineEdit->setPlaceholderText("Select " + _title.toLower());
    _selectedNodeLineEdit->setReadOnly(true);
    layout->addWidget(_selectedNodeLineEdit);
    return frame;
}

