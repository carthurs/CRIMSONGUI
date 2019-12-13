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


#ifndef SCHEMATICVIEW_H
#define SCHEMATICVIEW_H


#include <QGraphicsView>
#include <QResizeEvent>


namespace qsapecng
{


class SchematicScene;


class SchematicView: public QGraphicsView
{

  Q_OBJECT

public:
  SchematicView(QWidget* parent = 0): QGraphicsView(parent)
  {
    setCacheMode(QGraphicsView::CacheBackground);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    setDragMode(QGraphicsView::RubberBandDrag);
//     setDragMode(QGraphicsView::ScrollHandDrag);
//     setAlignment(Qt::AlignCenter);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setAcceptDrops(true);
    setMouseTracking(true);
    setAttribute(Qt::WA_DeleteOnClose);
    adjustSize();
    scale(2,2); //set a greater level of initial zoom
  }

public slots:
  void zoomIn()
    { scale(1.2, 1.2); }

  void zoomOut()
    { scale(1/1.2, 1/1.2); }

  void normalSize()
    { resetTransform(); }

  void fitToView()
  {
    fitInView(sceneRect(), Qt::KeepAspectRatio);
    ensureVisible(sceneRect());
    centerOn(sceneRect().center());
  }

protected:
//   void resizeEvent(QResizeEvent* event)
//   {
//     setSceneRect(0, 0, event->size().width(), event->size().height());
//     QGraphicsView::resizeEvent(event);
//   }

};


}


#endif // SCHEMATICVIEW_H
