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


#include "gui/editor/wire.h"
#include "gui/editor/graphicsnode.h"
#include "gui/editor/schematicscene.h"

#include <QtCore/QStringList>
#include <QPainter>
#include <QColor>


namespace qsapecng
{


Wire::Wire(
      QGraphicsItem* parent,
      SchematicScene* scene
    )
  : Item(parent, scene), connectedJunctions_(false), mod_(false)
{
  setAcceptDrops(false);
  setWire(QLineF(0, 0, 0, 0));
  itemPen_.setColor(Qt::black);
}


Wire::Wire(
      const QLineF& wire,
      bool connectedJunctions,
      QGraphicsItem* parent,
      SchematicScene* scene
    )
  : Item(parent, scene), connectedJunctions_(connectedJunctions), mod_(false)
{
  setAcceptDrops(false);
  setWire(wire);
  itemPen_.setColor(Qt::black);
}


Wire::~Wire()
{
  foreach(GraphicsNode* node, nodes_) {
    if(node->scene())
      node->scene()->removeItem(node);

    delete node;
  }

  nodes_.clear();
}


QRectF Wire::boundingRect() const
{
  qreal offset = penWidth() + COLOR_BORDER;

  if(orientation_ == Qt::Horizontal)
    return
      QRectF(0 - offset, 0 - offset,
        path_.boundingRect().width() + 2 * offset,
        SchematicScene::GridStep / 2 + 2 * offset);
  else
    return
      QRectF(0 - offset, 0 - offset,
        SchematicScene::GridStep / 2 + 2 * offset,
        path_.boundingRect().height() + 2 * offset);
}


void Wire::paint(
    QPainter* painter,
    const QStyleOptionGraphicsItem* option,
    QWidget* widget
  )
{
  painter->save();

  if(isSelected()) {
    painter->save();

    painter->setPen(defaultPen_);
    double opacity = painter->opacity();
    painter->setOpacity(opacity / OpacityFactor);
    painter->drawRect(boundingRect());

    painter->restore();
  }

  painter->setPen(itemPen_);
  painter->drawPath(path_);

  painter->restore();
}


Qt::Orientation Wire::orientation() const
{
  return orientation_;
}


void Wire::setWire(const QLineF& wire)
{
  if(qAbs(wire.angle()) == 0 || qAbs(wire.angle()) == 180) {
    orientation_ = Qt::Horizontal;
  } else if(qAbs(wire.angle()) == 90 || qAbs(wire.angle()) == 270) {
    orientation_ = Qt::Vertical;
  } else {
    if(qAbs(wire.dx()) > qAbs(wire.dy())) {
      orientation_ = Qt::Horizontal;
      wire.p2().setY(wire.p1().y());
    } else {
      orientation_ = Qt::Vertical;
      wire.p2().setX(wire.p1().x());
    }
  }

  QLineF realWire = wire.translated(-wire.p1());
  if(realWire.length() < SchematicScene::GridStep) {
    if(orientation_ == Qt::Horizontal)
      realWire.setLine(0, 0, SchematicScene::GridStep, 0);
    else
      realWire.setLine(0, 0, 0, SchematicScene::GridStep);
  }

  QPainterPath path;
  path.moveTo(0, 0);
  path.lineTo(
    qAbs(realWire.x2()),
    qAbs(realWire.y2())
  );

  path_ = path;
  //pathAsItem_.setPath(path_); // CA1
  //scene->addItem(&pathAsItem_); // CA1
  refreshNodeList();
}


QPointF Wire::fromPoint() const
{
  return QPointF(0, 0);
}


QPointF Wire::toPoint() const
{
  if(orientation_ == Qt::Horizontal)
    return QPointF(path_.boundingRect().width(), 0);
  else
    return QPointF(0, path_.boundingRect().height());
}


bool Wire::isJunctionsConnected() const
{
  return connectedJunctions_;
}


void Wire::setConnectedJunctions(bool state)
{
  connectedJunctions_ = state;
  refreshNodeList();
}


void Wire::rotate()
{
  qreal tmp;
  QPointF from = fromPoint();
  QPointF to = toPoint();

  tmp = from.x();
  from.setX(from.y());
  from.setY(tmp);

  tmp = to.x();
  to.setX(to.y());
  to.setY(tmp);

  bool selected = isSelected();
  setVisible(false);
  setWire(QLineF(from, to));
  setVisible(true);
  setSelected(selected);
}


void Wire::rotateBack()
{
  rotate();
}


void Wire::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
  if(schematicScene())
    schematicScene()->modifyWire(this);
  else
    Item::mouseDoubleClickEvent(event);
}


void Wire::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  QList<QGraphicsItem*> collidingItemList = collidingItems();

  foreach(GraphicsNode* node, nodes_)
    collidingItemList.removeAll(node);

  foreach(QGraphicsItem* graphicsItem, collidingItemList) {
    Item* item = qgraphicsitem_cast<Item*>(graphicsItem);
    if(item && itemType() == Item::Wire) {

      Wire* wire =
        static_cast<Wire*>(graphicsItem);

      if(wire->orientation() == orientation_ && schematicScene())
        schematicScene()->joinWires(this, wire);
    }
  }

  Item::mouseReleaseEvent(event);
}


QVariant Wire::itemChange(GraphicsItemChange change, const QVariant& value)
{
  if(change == ItemPositionHasChanged
        || change == ItemTransformHasChanged
        || change == ItemSceneHasChanged
      )
    foreach(GraphicsNode* node, nodes_)
      node->updateItemSet();

  return Item::itemChange(change, value);
}


Wire* Wire::joined(Wire* wire)
{
  bool positionChanged = false;

  QPointF wireFromPoint = mapFromItem(wire, wire->fromPoint());
  QPointF wireToPoint = mapFromItem(wire, wire->toPoint());

  switch(orientation_)
  {
  case Qt::Horizontal:
    if(fromPoint().x() < wireFromPoint.x())
      wireFromPoint = fromPoint();
    else
      positionChanged = true;

    if(toPoint().x() > wireToPoint.x())
      wireToPoint = toPoint();

    wireFromPoint.setY(fromPoint().y());
    wireToPoint.setY(toPoint().y());

    break;
  case Qt::Vertical:
    if(fromPoint().y() < wireFromPoint.y())
      wireFromPoint = fromPoint();
    else
      positionChanged = true;

    if(toPoint().y() > wireToPoint.y())
      wireToPoint = toPoint();

    wireFromPoint.setX(fromPoint().x());
    wireToPoint.setX(toPoint().x());

    break;
  }

  Wire* joined = static_cast<Wire*>(
    SchematicScene::itemByType(SchematicScene::WireItemType));

  joined->setWire(QLineF(wireFromPoint, wireToPoint));
  joined->setConnectedJunctions(connectedJunctions_);
  if(positionChanged)
    joined->setPos(mapToScene(wireFromPoint));
  else
    joined->setPos(mapToScene(fromPoint()));

  return joined;
}


int Wire::propagate(int value)
{
  if(mod_)
    return value;

  mod_ = true;

  int act;
  bool found = false;

  foreach(GraphicsNode* node, nodes_) {
    if(node->node()->isValid()) {
      act = node->node()->value();
      found = true;
    }
  }

  if(!found)
    act = value++;

  foreach(GraphicsNode* node, nodes_) {
    node->node()->setValue(act);

    foreach(GraphicsNode* setnode, node->itemSet())
      if(setnode->owner()->itemType() == Item::Wire)
        static_cast<Wire*>(setnode->owner())->propagate(act);

  }

  if(!found)
    nodes_.front()->showNodeValue();

  QList<QColor> colors;
  colors.push_back(QColor(Qt::darkCyan));
  colors.push_back(QColor(Qt::darkRed));
  colors.push_back(QColor(Qt::darkMagenta));
  colors.push_back(QColor(Qt::darkGreen));
  colors.push_back(QColor(Qt::darkYellow));
  colors.push_back(QColor(Qt::darkBlue));
  colors.push_back(QColor(Qt::darkGray));

  itemPen_.setWidth(itemPen_.width() + COLOR_BORDER);
  itemPen_.setColor(colors.at(act % colors.size()));
  update();

  return value;
}


void Wire::invalidate()
{
  foreach(GraphicsNode* node, nodes_) {
    node->node()->setDirty();
    node->hideNodeValue();
  }

  if(mod_) {
    itemPen_.setWidth(itemPen_.width() - COLOR_BORDER);
    itemPen_.setColor(Qt::black);
    mod_ = false;
    update();
  }
}


void Wire::refreshNodeList()
{
  foreach(GraphicsNode* node, nodes_) {
    if(node->scene())
      node->scene()->removeItem(node);

    delete node;
  }

  nodes_.clear();

  if(connectedJunctions_) {
    if(orientation_ == Qt::Horizontal) {
      int size =
        qAbs((toPoint().x() - fromPoint().x()) / SchematicScene::GridStep) + 1;

      for(int x = 0; x < size; ++x) {
        GraphicsNode* node = new GraphicsNode(this);
        node->setOwner(this);
        node->setPos(QPointF(x * SchematicScene::GridStep, 0));
        nodes_.append(node);
      }
    } else {
      int size =
        qAbs((toPoint().y() - fromPoint().y()) / SchematicScene::GridStep) + 1;

      for(int y = 0; y < size; ++y) {
        GraphicsNode* node = new GraphicsNode(this);
        node->setOwner(this);
        node->setPos(QPointF(0, y * SchematicScene::GridStep));
        nodes_.append(node);
      }
    }
  } else {
    GraphicsNode* from = new GraphicsNode(this);
    GraphicsNode* to = new GraphicsNode(this);
    from->setOwner(this);
    to->setOwner(this);
    from->setPos(fromPoint());
    to->setPos(toPoint());
    nodes_.append(from);
    nodes_.append(to);
  }
}


}
