#include "ReparentAction.h"

#include "ui_ReparentDialog.h"

#include <HierarchyManager.h>
#include <QtWidgets/QMessageBox>
#include <QmitkNodeDescriptor.h>
#include <QmitkNodeDescriptorManager.h>

Q_DECLARE_METATYPE(mitk::DataNode*)

ReparentAction::ReparentAction()
{
}

ReparentAction::~ReparentAction()
{
}


void ReparentAction::Run(const QList<mitk::DataNode::Pointer> &selectedNodes)
{
    QDialog dialog;
    Ui::ReparentDialog dialogUI;
    dialogUI.setupUi(&dialog);

    auto hm = crimson::HierarchyManager::getInstance();

    // Find the potential parents
    auto potentialParents = std::unordered_set<mitk::DataNode*>{};

    for (const auto& node : selectedNodes) {
        auto nodeType = hm->findFittingNodeType(node);

        if (!nodeType) {
            continue;
        }

        auto moreParents = hm->findPotentialParents(node, nodeType.get());
        potentialParents.insert(moreParents.begin(), moreParents.end());
    }

    if (potentialParents.empty()) {
        QMessageBox::information(nullptr, "No potential parents found", "The current parent node is the only possible parent node.");
        return;
    }

    // Let user select the new parent
    dialog.setWindowTitle(dialog.windowTitle() + QString::fromStdString(selectedNodes[0]->GetName()));

    static const int DataNodeRole = Qt::UserRole + 1;

    for (mitk::DataNode* parent : potentialParents) {
        auto item = new QListWidgetItem(QString::fromStdString(parent->GetName()));
        item->setData(DataNodeRole, QVariant::fromValue(parent));

        QmitkNodeDescriptor* nodeDescriptor
            = QmitkNodeDescriptorManager::GetInstance()->GetDescriptor(parent);
        item->setIcon(nodeDescriptor->GetIcon());

        dialogUI.newParentNodeListWidget->addItem(item);
    }

    if (dialog.exec() == QDialog::Accepted) {
        auto newParent = qvariant_cast<mitk::DataNode*>(dialogUI.newParentNodeListWidget->selectedItems().first()->data(DataNodeRole));
        auto newParentNodeType = hm->findFittingNodeType(newParent);

        if (!newParentNodeType) {
            MITK_ERROR << "Failed to detect the new parent node type";
            return;
        }

        for (const auto& node : selectedNodes) {
            auto nodeType = hm->findFittingNodeType(node);
            if (!nodeType) {
                MITK_WARN << "Failed to detect the node type for node " << node->GetName() << ". Skipping";
                continue;
            }

            if (!hm->canAddNode(newParent, newParentNodeType.get(), nodeType.get(), false)) {
                MITK_WARN << "Cannot make the node " << node->GetName() << " a child of new parent. Skipping";
                continue;
            }
            hm->reparentNode(node, newParent);
        }
    }
}