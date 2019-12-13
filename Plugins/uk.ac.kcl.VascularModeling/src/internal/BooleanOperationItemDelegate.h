#pragma once

#include <QStyledItemDelegate>
#include <VesselForestData.h>

/*! \brief   A QStyledItemDelegate which represnts a boolean operation type with a combo box. */
class BooleanOperationItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    BooleanOperationItemDelegate(QObject *parent = 0);
    ~BooleanOperationItemDelegate();

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    static QString getTextForBOPType(crimson::VesselForestData::BooleanOperationType type);

};