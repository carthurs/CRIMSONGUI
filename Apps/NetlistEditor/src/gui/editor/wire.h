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


#ifndef WIRE_H
#define WIRE_H


#include "gui/editor/item.h"
#include "gui/editor/vpiface.h"

#include <QtCore/QLineF>
#include <QPainterPath>
#include <QPen>


namespace qsapecng
{


class SchematicScene;
class GraphicsNode;


class Wire: public Item, public ValuePropagationInterface
{

public:
  Wire(
    QGraphicsItem* parent = 0,
    SchematicScene* scene = 0
  );

  Wire(
    const QLineF& wire,
    bool connectedJunctions = true,
    QGraphicsItem* parent = 0,
    SchematicScene* scene = 0
  );

  ~Wire();

  inline ItemType itemType() const { return Item::Wire; }

  QRectF boundingRect() const;
  void paint(
    QPainter* painter,
    const QStyleOptionGraphicsItem* option,
    QWidget* widget
  );

  Qt::Orientation orientation() const;
  void setWire(const QLineF& wire);
  QPointF fromPoint() const;
  QPointF toPoint() const;

  bool isJunctionsConnected() const;
  void setConnectedJunctions(bool state);

  void mirror() { }
  void rotate();
  void rotateBack();

  Wire* joined(Wire* wire);

public:
  int propagate(int value);
  void invalidate();

protected:
  void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event);
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
  QVariant itemChange(GraphicsItemChange change, const QVariant& value);

private:
  void refreshNodeList();

private:
  enum { COLOR_BORDER = 2 };

  QPainterPath path_;
  //QGraphicsPathItem pathAsItem_; // CA1 This is (?) needed because QPainterPaths don't work for collision detection (for intersecting wires)

  Qt::Orientation orientation_;
  QList<GraphicsNode*> nodes_;
  bool connectedJunctions_;
  bool mod_;

};


}


#endif // WIRE_H
