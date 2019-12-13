/*
    QSapecNG - Qt based SapecNG GUI front-end
    Copyright (C) 2009, Michele Caini

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "gui/delegate.h"

#include <QDoubleSpinBox>
#include <limits>


namespace qsapecng
{


QWidget* QDoubleSpinBoxDelegate::createEditor(
    QWidget* parent,
    const QStyleOptionViewItem& option,
    const QModelIndex& index
  ) const
{
  QDoubleSpinBox* editor = new QDoubleSpinBox(parent);
  editor->setMinimum(std::numeric_limits<double>::min());
  editor->setMaximum(std::numeric_limits<double>::max());
  editor->setDecimals(5);
  editor->setValue(1.);

  return editor;
}


void QDoubleSpinBoxDelegate::setEditorData(
    QWidget* editor,
    const QModelIndex& index
  ) const
{
  double value = index.model()->data(index, Qt::EditRole).toDouble();

  QDoubleSpinBox* spinBox = static_cast<QDoubleSpinBox*>(editor);
  spinBox->setValue(value);
}


void QDoubleSpinBoxDelegate::setModelData(
    QWidget* editor,
    QAbstractItemModel* model,
    const QModelIndex& index
  ) const
{
  QDoubleSpinBox* spinBox = static_cast<QDoubleSpinBox*>(editor);
  spinBox->interpretText();
  double value = spinBox->value();

  model->setData(index, value, Qt::EditRole);
}


void QDoubleSpinBoxDelegate::updateEditorGeometry(
    QWidget* editor,
    const QStyleOptionViewItem& option,
    const QModelIndex& index
  ) const
{
  editor->setGeometry(option.rect);
}


}
