#include "vesselPathTableViewWidget.h"

#include <QKeyEvent>
#include <QHeaderView>
#include <QStyledItemDelegate>
#include <QLineEdit>
#include <QDoubleValidator>

class ValidatorDelegate : public QStyledItemDelegate {
public:
    ValidatorDelegate(QObject* parent)
        : QStyledItemDelegate(parent)
        , validator(new QDoubleValidator(this))
    {}

    QWidget* createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const override
    {
        QWidget* editor = QStyledItemDelegate::createEditor(parent, option, index);
        QLineEdit* lineEditEditor = qobject_cast<QLineEdit*>(editor);
        if (lineEditEditor) {
            lineEditEditor->setValidator(validator);
        }

        return editor;
    }
private:
    QDoubleValidator* validator;
};

VesselPathTableViewWidget::VesselPathTableViewWidget(QWidget* parent)
    : QTableView(parent)
{
    this->setItemDelegate(new ValidatorDelegate(this));
    horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

void VesselPathTableViewWidget::setModel(QAbstractItemModel* newModel)
{
    if (model() == newModel) {
        return;
    }


    QTableView::setModel(newModel);
}

void VesselPathTableViewWidget::removeSelectedPoints()
{
    QList<QPersistentModelIndex> persistentIndices;
    Q_FOREACH(const QModelIndex& vesselIndex, selectionModel()->selectedRows()) {
        persistentIndices << vesselIndex;
    }

    if (persistentIndices.empty() && selectionModel()->currentIndex().isValid()) {
        persistentIndices << selectionModel()->currentIndex();
    }


    QListIterator<QPersistentModelIndex> it(persistentIndices);
    for (it.toBack(); it.hasPrevious(); ) {
        const QPersistentModelIndex& index = it.previous();
        model()->removeRow(index.row(), index.parent());
    }
}

void VesselPathTableViewWidget::keyPressEvent(QKeyEvent *event)
{
    // Remove points upon Delete key press
    if (event->key() == Qt::Key_Delete) {
        removeSelectedPoints();
        event->accept();
    }
    QTableView::keyPressEvent(event);
}
