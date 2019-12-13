#include "BooleanOperationItemDelegate.h"
#include <QComboBox>

BooleanOperationItemDelegate::BooleanOperationItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}


BooleanOperationItemDelegate::~BooleanOperationItemDelegate()
{
}


QWidget* BooleanOperationItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // ComboBox ony in column 2
    if (index.column() != 3)
        return QStyledItemDelegate::createEditor(parent, option, index);

    // Create the combobox and populate it
    QComboBox *cb = new QComboBox(parent);
    cb->addItem(getTextForBOPType(crimson::VesselForestData::bopFuse), static_cast<int>(crimson::VesselForestData::bopFuse));
    cb->addItem(getTextForBOPType(crimson::VesselForestData::bopCut), static_cast<int>(crimson::VesselForestData::bopCut));
    cb->addItem(getTextForBOPType(crimson::VesselForestData::bopCommon), static_cast<int>(crimson::VesselForestData::bopCommon));
    return cb;
}

void BooleanOperationItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (QComboBox* cb = qobject_cast<QComboBox*>(editor)) {
        // get the index of the text in the combobox that matches the current value of the itenm
        auto bopType = index.data(Qt::UserRole).toInt();
        cb->setCurrentIndex(cb->findData(bopType));
    }
    else {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void BooleanOperationItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    if (QComboBox* cb = qobject_cast<QComboBox*>(editor)) {
        // save the current text of the combo box as the current value of the item
        auto text = cb->currentText();
        auto bopType = cb->itemData(cb->currentIndex());

        model->setData(index, text, Qt::EditRole);
        model->setData(index, bopType, Qt::UserRole);
    }
    else {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

QString BooleanOperationItemDelegate::getTextForBOPType(crimson::VesselForestData::BooleanOperationType type)
{
    static std::map<crimson::VesselForestData::BooleanOperationType, QString> names = { 
        { crimson::VesselForestData::bopFuse, tr("Fuse") }, 
        { crimson::VesselForestData::bopCut, tr("Cut") },
        { crimson::VesselForestData::bopCommon, tr("Common") },
    };

    return names[type];
}