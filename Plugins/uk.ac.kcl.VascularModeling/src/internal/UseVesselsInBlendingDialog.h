#pragma once

#include <VesselForestData.h>
#include "VascularModelingUtils.h"

#include <QDialog>
#include "ui_UseVesselsInBlendingDialog.h"

/*! \brief   Dialog for setting the use vessels in blending flags. */
class UseVesselsInBlendingDialog : public QDialog {
    Q_OBJECT
public:
    UseVesselsInBlendingDialog(mitk::DataNode* vesselForestNode, QWidget* parent = nullptr)
        : QDialog(parent)
        , _vesselForest(static_cast<crimson::VesselForestData*>(vesselForestNode->GetData())) 
    { 
        _ui.setupUi(this); 

        // Fill the list widget
        for (const auto& vesselUID : _vesselForest->getVessels()) {
            mitk::DataNode* vesselNode = crimson::VascularModelingUtils::getVesselPathNodeByUID(vesselForestNode, vesselUID);
            if (!vesselNode) {
                continue;
            }
            
            QListWidgetItem* item = new QListWidgetItem();
            item->setText(QString::fromStdString(vesselNode->GetName()));
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            item->setCheckState(_vesselForest->getVesselUsedInBlending(vesselUID) ? Qt::Checked : Qt::Unchecked);
            item->setData(Qt::UserRole, QVariant::fromValue(QString::fromStdString(vesselUID)));
            _ui.vesselsListWidget->addItem(item);
        }
        
        connect(_ui.vesselsListWidget, &QListWidget::itemChanged, this, &UseVesselsInBlendingDialog::syncSelectionStatus);
        connect(_ui.chooseAllButton, &QAbstractButton::clicked, this, &UseVesselsInBlendingDialog::chooseAll);
        connect(_ui.chooseNoneButton, &QAbstractButton::clicked, this, &UseVesselsInBlendingDialog::chooseNone);
    }
    
    void accept() override 
    {
        for (int row = 0; row < _ui.vesselsListWidget->count(); ++row) {
            QListWidgetItem* item = _ui.vesselsListWidget->item(row);
            _vesselForest->setVesselUsedInBlending(qvariant_cast<QString>(item->data(Qt::UserRole)).toStdString(), item->checkState() == Qt::Checked);
        }
        QDialog::accept();
    }

private slots:
    void syncSelectionStatus(QListWidgetItem* itemToSyncWith)
    {
        if (!itemToSyncWith->isSelected()) {
            return;
        }
        disconnect(_ui.vesselsListWidget, &QListWidget::itemChanged, this, &UseVesselsInBlendingDialog::syncSelectionStatus);

        for (QListWidgetItem* item : _ui.vesselsListWidget->selectedItems()) {
            item->setCheckState(itemToSyncWith->checkState());
        }
        
        connect(_ui.vesselsListWidget, &QListWidget::itemChanged, this, &UseVesselsInBlendingDialog::syncSelectionStatus);
    }
    
    void chooseAll()
    {
        disconnect(_ui.vesselsListWidget, &QListWidget::itemChanged, this, &UseVesselsInBlendingDialog::syncSelectionStatus);

        for (int row = 0; row < _ui.vesselsListWidget->count(); ++row) {
            _ui.vesselsListWidget->item(row)->setCheckState(Qt::Checked);
        }
        
        connect(_ui.vesselsListWidget, &QListWidget::itemChanged, this, &UseVesselsInBlendingDialog::syncSelectionStatus);
    }
    
    void chooseNone()
    {
        disconnect(_ui.vesselsListWidget, &QListWidget::itemChanged, this, &UseVesselsInBlendingDialog::syncSelectionStatus);

        for (int row = 0; row < _ui.vesselsListWidget->count(); ++row) {
            _ui.vesselsListWidget->item(row)->setCheckState(Qt::Unchecked);
        }

        connect(_ui.vesselsListWidget, &QListWidget::itemChanged, this, &UseVesselsInBlendingDialog::syncSelectionStatus);
    }

private:
    Ui::UseVesselsInBlendingDialog _ui;
    crimson::VesselForestData* _vesselForest;
};