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


#include "gui/sidebarview.h"

#include <QDrag>
#include <QPixmap>


namespace qsapecng
{


SideBarView::SideBarView(QWidget* parent)
  : QListView(parent)
{
  setDragEnabled(true);
  setDragDropMode(QAbstractItemView::DragOnly);
  setSelectionMode(QAbstractItemView::SingleSelection);
  setViewMode(QListView::ListMode);
  setMovement(QListView::Free);
  setAlternatingRowColors(true);
  setEditTriggers(QAbstractItemView::NoEditTriggers);
  setResizeMode(QListView::Adjust);
}


void SideBarView::startDrag(Qt::DropActions supportedActions)
{
  QDrag* drag = new QDrag(this);

  QModelIndex index;
  QModelIndexList indexes = selectedIndexes();
  if(!indexes.isEmpty())
    index = indexes.first();

  if(index.isValid()) {
    indexes.clear();
    indexes << index;
    drag->setMimeData(model()->mimeData(indexes));
    drag->setPixmap(qvariant_cast<QPixmap>(
      model()->itemData(index)[Qt::DecorationRole])
    );

    drag->setHotSpot(QPoint(
      drag->pixmap().width(),
      drag->pixmap().height())
    );
  }

  drag->exec(supportedActions);
  clearSelection();
}


}
