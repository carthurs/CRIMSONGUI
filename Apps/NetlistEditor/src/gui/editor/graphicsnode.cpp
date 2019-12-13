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


#include "gui/editor/graphicsnode.h"

#include <QGraphicsTextItem>
#include <QGraphicsScene>
#include <QPainter>


namespace qsapecng
{


GraphicsNode::GraphicsNode(
      QGraphicsItem* parent,
      QGraphicsScene* scene
    )
  : QGraphicsItem(parent/*, scene RK - NEEDED? */)
{
  owner_ = qgraphicsitem_cast<Item*>(parent);

  dataPointer_ = new Node;
  colliding_ = false;
  label_ = 0;

  path_.addEllipse(
    QRectF(
      QPointF(-Radius, -Radius),
      QPointF(Radius, Radius)
    )
  );

  setAcceptDrops(false);
  setFlags(0);
}


GraphicsNode::~GraphicsNode()
{
  foreach(GraphicsNode* item, itemSet_)
    item->detach(this);

  dataPointer_.detach();
  itemSet_.clear();
}


void GraphicsNode::paint(
    QPainter* painter,
    const QStyleOptionGraphicsItem* option,
    QWidget* widget
  )
{
  if(isColliding()) {
    painter->save();
    painter->setBrush(QBrush(Qt::black, Qt::SolidPattern));
    painter->drawPath(path_);
    painter->restore();
  }
}


QRectF GraphicsNode::boundingRect() const
{
  return path_.boundingRect();
}


QPainterPath GraphicsNode::shape() const
{
  return path_;
}


void GraphicsNode::attach(GraphicsNode* item)
{
  if(item && item != this) {
    setNode(item->node());
    itemSet_.insert(item);
    colliding_ = true;
  }
}


void GraphicsNode::detach(GraphicsNode* item)
{
  itemSet_.remove(item);
  colliding_ = !itemSet_.isEmpty();
}


void GraphicsNode::setNode(Node* node)
{
  dataPointer_ = node;
}


const Node* GraphicsNode::constNode() const
{
  return dataPointer_.constData();
}


Node* GraphicsNode::node() const
{
  return dataPointer_.data();
}


void GraphicsNode::updateItemSet()
{
  foreach(GraphicsNode* item, itemSet_)
    item->detach(this);

  dataPointer_.detach();
  itemSet_.clear();

  QList<QGraphicsItem*> collidingItemList = collidingItems();
  foreach(QGraphicsItem* graphicsItem, collidingItemList) {
    GraphicsNode* item =
      qgraphicsitem_cast<GraphicsNode*>(graphicsItem);

    if(item) {
      item->attach(this);
      itemSet_.insert(item);
    }
  }

  colliding_ = !itemSet_.isEmpty();
}


const QSet<GraphicsNode*>& GraphicsNode::itemSet() const
{
  return itemSet_;
}


bool GraphicsNode::isColliding() const
{
  return colliding_;
}


QVariant GraphicsNode::itemChange(GraphicsItemChange change, const QVariant& value)
{
  if(change == ItemPositionHasChanged)
    updateItemSet();

  return QGraphicsItem::itemChange(change, value);
}


void GraphicsNode::showNodeValue()
{
  if(!label_)
    label_ = new QGraphicsTextItem(this);

  label_->setPlainText(QString::number(node()->value()));
  updateLabelPosition();
  label_->setVisible(true);
}


void GraphicsNode::hideNodeValue()
{
  if(label_)
    label_->setVisible(false);
}


void GraphicsNode::setOwner(Item* owner)
{
  owner_ = owner;
}


Item* GraphicsNode::owner() const
{
  return owner_;
}


void GraphicsNode::updateLabelPosition()
{
  QPointF top = QPointF(
      (boundingRect().width() - label_->boundingRect().width()) / 2,
      -3/2 * label_->boundingRect().height()
    );
  label_->setPos(top);
  if(label_->collidingItems().size() == 0)
    return;

  QPointF left = QPointF(
      -label_->boundingRect().width(),
      (boundingRect().height() - label_->boundingRect().height()) / 2
    );
  label_->setPos(left);
  if(label_->collidingItems().size() == 0)
    return;

  QPointF bottom = QPointF(
      (boundingRect().width() - label_->boundingRect().width()) / 2,
      boundingRect().height() + 1/2 * label_->boundingRect().height()
    );
  label_->setPos(bottom);
  if(label_->collidingItems().size() == 0)
    return;

  QPointF right = QPointF(
      boundingRect().width(),
      (boundingRect().height() - label_->boundingRect().height()) / 2
    );
  label_->setPos(right);
  if(label_->collidingItems().size() == 0)
    return;

  QPointF topLeft = QPointF(
      -label_->boundingRect().width(),
      -3/2 * label_->boundingRect().height()
    );
  label_->setPos(topLeft);
  if(label_->collidingItems().size() == 0)
    return;

  QPointF bottomLeft = QPointF(
      -label_->boundingRect().width(),
      boundingRect().height() + 1/2 * label_->boundingRect().height()
    );
  label_->setPos(bottomLeft);
  if(label_->collidingItems().size() == 0)
    return;

  QPointF topRight = QPointF(
      boundingRect().width(),
      -3/2 * label_->boundingRect().height()
    );
  label_->setPos(topRight);
  if(label_->collidingItems().size() == 0)
    return;

  QPointF bottomRight = QPointF(
      boundingRect().width(),
      boundingRect().height() + 1/2 * label_->boundingRect().height()
    );
  label_->setPos(bottomRight);
  if(label_->collidingItems().size() == 0)
    return;

  label_->setPos(top);
}


}
