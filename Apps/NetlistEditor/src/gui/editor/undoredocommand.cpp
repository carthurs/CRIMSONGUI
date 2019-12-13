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


#include "parser/parser_factory.h"
#include "gui/editor/schematicsceneparser.h"
#include "gui/editor/undoredocommand.h"
#include "gui/editor/wire.h"
#include "gui/editor/component.h"
#include "gui/editor/item.h"

#include <QtCore/QSet>
#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QGraphicsItem>

#include <sstream>
#include <memory>


namespace qsapecng
{


AddItems::AddItems(
      SchematicScene* scene,
      QList<QGraphicsItem*> items,
      QUndoCommand* parent
    )
  : QUndoCommand(parent), scene_(scene), items_(items)
{
  setText(QObject::tr("Add generic item(s)"));
  foreach(QGraphicsItem* item, items_)
    pos_.push_back(item->pos());
}


AddItems::AddItems(
      SchematicScene* scene,
      QGraphicsItem* item,
      QUndoCommand* parent
    )
  : QUndoCommand(parent), scene_(scene)
{
  setText(QObject::tr("Add generic item(s)"));
  items_.append(item);
  pos_.append(item->pos());
}


AddItems::AddItems(
      SchematicScene* scene,
      QList<QGraphicsItem*> items,
      const QList<QPointF>& pos,
      QUndoCommand* parent
    )
  : QUndoCommand(parent), scene_(scene)
{
  setText(QObject::tr("Add generic item(s)"));
  if(items.size() == pos.size()) {
    items_ = items;
    pos_ = pos;
  }
}


AddItems::AddItems(
      SchematicScene* scene,
      QGraphicsItem* item,
      const QPointF& pos,
      QUndoCommand* parent
    )
  : QUndoCommand(parent), scene_(scene)
{
  setText(QObject::tr("Add generic item(s)"));
  items_.append(item);
  pos_.append(pos);
}


AddItems::~AddItems()
{
  foreach(QGraphicsItem* item, items_) {
    if(item->scene()) {
      if(item->scene() == scene_)
        scene_->clearSupportedItem(item);
      else
        item->scene()->removeItem(item);
    }

    delete item;
  }
}


void AddItems::undo()
{
  scene_->resetStatus();
  foreach(QGraphicsItem* item, items_)
    if(item->scene() && item->scene() == scene_)
      scene_->removeSupportedItem(item);
}


void AddItems::redo()
{
  for(int i = 0; i < items_.size(); ++i) {
    if(!items_.at(i)->scene()) {
      scene_->addSupportedItem(items_.at(i));
      items_.at(i)->setPos(scene_->closestGridPoint(pos_.at(i)));
    }
  }
}


AddSupportedItem::AddSupportedItem(
      SchematicScene* scene,
      SchematicScene::SupportedItemType type,
      const QPointF& pos,
      QUndoCommand* parent
    )
  : QUndoCommand(parent), scene_(scene), pos_(pos)
{
  setText(QObject::tr("Add item: ") + SchematicScene::itemNameByType(type));
  item_ = scene->itemByType(type);
}


AddSupportedItem::~AddSupportedItem()
{
  scene_->clearSupportedItem(item_);
  delete item_;
}


void AddSupportedItem::undo()
{
  scene_->resetStatus();
  scene_->removeSupportedItem(item_);
}


void AddSupportedItem::redo()
{
  scene_->addSupportedItem(item_);
  item_->setPos(scene_->closestGridPoint(pos_));
}


Item* AddSupportedItem::item() const
{
  return item_;
}


// AddUserDefItem::AddUserDefItem(
//       SchematicScene* scene,
//       QByteArray md5,
//       int size,
//       std::string info,
//       const QPointF& pos,
//       QUndoCommand* parent
//     )
//   : QUndoCommand(parent), scene_(scene), pos_(pos)
// {
//   setText(QObject::tr("Add item: ") + SchematicScene::itemNameByType(SchematicScene::UserDefItemType));
//   item_ = scene->itemByType(SchematicScene::UserDefItemType);
//   static_cast<Component*>(item_)->setPath(SchematicScene::userDefPath(size));
//   static_cast<Component*>(item_)->addNodes(SchematicScene::userDefNodes(size));
//   item_->setData(99, md5);
  
//   SchematicScene* rep = new SchematicScene;
//   sapecng::abstract_builder* builder = new SchematicSceneBuilder(*rep);
    
//   std::istringstream in_file(info);
//   sapecng::abstract_parser* parser =
//     sapecng::parser_factory::parser(sapecng::parser_factory::INFO, in_file);

//   if(parser)
//     parser->parse(*builder);

//   delete parser;
//   delete builder;
  
//   QPointer<qsapecng::SchematicScene> smart(rep);
//   item_->setData(101, qVariantFromValue(smart));
// }


// AddUserDefItem::~AddUserDefItem()
// {
//   scene_->clearSupportedItem(item_);
//   delete item_;
// }


// void AddUserDefItem::undo()
// {
//   scene_->resetStatus();
//   scene_->removeSupportedItem(item_);
// }


// void AddUserDefItem::redo()
// {
//   scene_->addSupportedItem(item_);
//   item_->setPos(scene_->closestGridPoint(pos_));
// }


// Item* AddUserDefItem::item() const
// {
//   return item_;
// }


AddWire::AddWire(
      SchematicScene* scene,
      const QPointF& pos,
      const QLineF& wire,
      bool connectedJunctions,
      QUndoCommand* parent
    )
  : QUndoCommand(parent), scene_(scene), pos_(pos)
{
  setText(QObject::tr("Add wire"));
  Wire* wireItem =
    static_cast<Wire*>(scene->itemByType(SchematicScene::WireItemType));
  wireItem->setConnectedJunctions(connectedJunctions);
  wireItem->setWire(wire);
  wire_ = wireItem;
}


AddWire::~AddWire()
{
  scene_->clearSupportedItem(wire_);
  delete wire_;
}


void AddWire::undo()
{
  scene_->resetStatus();
  scene_->removeSupportedItem(wire_);
}


void AddWire::redo()
{
  scene_->addSupportedItem(wire_);
  wire_->setPos(scene_->closestGridPoint(pos_));
}


JoinWires::JoinWires(
      Wire* w1,
      Wire* w2,
      QUndoCommand* parent
    )
  : QUndoCommand(parent), w1_(w1), w2_(w2)
{
  setText(QObject::tr("Join wires"));
  scene_ = w1_->schematicScene();
  wire_ = w1->joined(w2);
}


JoinWires::~JoinWires()
{
  scene_->clearSupportedItem(wire_);
  delete wire_;
}


void JoinWires::undo()
{
  scene_->resetStatus();
  scene_->addSupportedItem(w1_);
  scene_->addSupportedItem(w2_);
  scene_->removeSupportedItem(wire_);
}


void JoinWires::redo()
{
  scene_->addSupportedItem(wire_);
  scene_->removeSupportedItem(w2_);
  scene_->removeSupportedItem(w1_);
}


RemoveItems::RemoveItems(
      SchematicScene* scene,
      QList<QGraphicsItem*> items,
      QUndoCommand* parent
    )
  : QUndoCommand(parent), scene_(scene), items_(items)
{
  setText(QObject::tr("Remove item(s)"));
}


RemoveItems::RemoveItems(
      SchematicScene* scene,
      QGraphicsItem* item,
      QUndoCommand* parent
    )
  : QUndoCommand(parent), scene_(scene)
{
  setText(QObject::tr("Remove item"));
  items_.append(item);
}


void RemoveItems::undo()
{
  scene_->resetStatus();
  foreach(QGraphicsItem* graphicsItem, items_) {
    Item* item = qgraphicsitem_cast<Item*>(graphicsItem);

    if(item)
      scene_->addSupportedItem(item);
    else
      scene_->addItem(graphicsItem);
  }
}


void RemoveItems::redo()
{
  foreach(QGraphicsItem* graphicsItem, items_) {
    Item* item = qgraphicsitem_cast<Item*>(graphicsItem);

    if(item)
      scene_->removeSupportedItem(item);
    else
      scene_->removeItem(graphicsItem);
  }
}


RotateItems::RotateItems(
      QList<QGraphicsItem*> items,
      QUndoCommand* parent
    )
  : QUndoCommand(parent), items_(items)
{
  setText(QObject::tr("Rotate item(s)"));
}


RotateItems::RotateItems(
      QGraphicsItem* item,
      QUndoCommand* parent
    )
  : QUndoCommand(parent)
{
  setText(QObject::tr("Rotate item"));
  items_.append(item);
}


void RotateItems::undo()
{
  foreach(QGraphicsItem* graphicsItem, items_) {
    Item* item = qgraphicsitem_cast<Item*>(graphicsItem);
    if(item)
      item->rotateBack();
    else
      graphicsItem->setTransform(QTransform().rotate(-90), true);;
  }
}


void RotateItems::redo()
{
  foreach(QGraphicsItem* graphicsItem, items_) {
    Item* item = qgraphicsitem_cast<Item*>(graphicsItem);
    if(item)
      item->rotate();
    else
      graphicsItem->setTransform(QTransform().rotate(90), true);
  }
}


MirrorItems::MirrorItems(
      QList<QGraphicsItem*> items,
      QUndoCommand* parent
    )
  : QUndoCommand(parent), items_(items)
{
  setText(QObject::tr("Mirror item(s)"));
}


MirrorItems::MirrorItems(
      QGraphicsItem* item,
      QUndoCommand* parent
    )
  : QUndoCommand(parent)
{
  setText(QObject::tr("Mirror item"));
  items_.append(item);
}


void MirrorItems::undo()
{
  foreach(QGraphicsItem* graphicsItem, items_) {
    Item* item = qgraphicsitem_cast<Item*>(graphicsItem);
    if(item)
      item->mirror();
    else
      graphicsItem->setTransform(QTransform().scale(-1, 1), true);
  }
}


void MirrorItems::redo()
{
  foreach(QGraphicsItem* graphicsItem, items_) {
    Item* item = qgraphicsitem_cast<Item*>(graphicsItem);
    if(item)
      item->mirror();
    else
      graphicsItem->setTransform(QTransform().scale(-1, 1), true);;
  }
}


BringToFrontItem::BringToFrontItem(
      QGraphicsItem* item,
      QUndoCommand* parent
    )
  : QUndoCommand(parent), item_(item)
{
  setText(QObject::tr("Bring to front item"));
  oldZValue_ = item_->zValue();

  QList<QGraphicsItem*> overlapItems =
    item_->collidingItems();

  qreal zValue_ = 0;
  foreach(QGraphicsItem* overlapItem, overlapItems) {
    if(overlapItem->zValue() >= zValue_)
      zValue_ = overlapItem->zValue() + 0.1;
  }
}


void BringToFrontItem::undo()
{
  item_->setZValue(oldZValue_);
}


void BringToFrontItem::redo()
{
  item_->setZValue(zValue_);
}


bool BringToFrontItem::mergeWith(const QUndoCommand* command)
{
  if(command->id() != id())
    return false;

  if(static_cast<const BringToFrontItem*>(command)->item_ == item_)
    return true;

  return false;
}


SendToBackItem::SendToBackItem(
      QGraphicsItem* item,
      QUndoCommand* parent
    )
  : QUndoCommand(parent), item_(item)
{
  setText(QObject::tr("Send to back item"));
  oldZValue_ = item_->zValue();

  QList<QGraphicsItem*> overlapItems =
    item_->collidingItems();

  qreal zValue_ = 0;
  foreach(QGraphicsItem* overlapItem, overlapItems) {
    if(overlapItem->zValue() <= zValue_)
      zValue_ = overlapItem->zValue() - 0.1;
  }
}


void SendToBackItem::undo()
{
  item_->setZValue(oldZValue_);
}


void SendToBackItem::redo()
{
  item_->setZValue(zValue_);
}


bool SendToBackItem::mergeWith(const QUndoCommand* command)
{
  if(command->id() != id())
    return false;

  if(static_cast<const SendToBackItem*>(command)->item_ == item_)
    return true;

  return false;
}


MoveItems::MoveItems(
      SchematicScene* scene,
      QList<QGraphicsItem*> items,
      const QPointF& offset,
      QUndoCommand* parent
    )
  : QUndoCommand(parent), scene_(scene), items_(items), offset_(offset)
{
  setText(QObject::tr("Move item(s)"));
  items_ = items;
  foreach(QGraphicsItem* item, items_)
    oldPos_.append(item->pos());
}


MoveItems::MoveItems(
      SchematicScene* scene,
      QGraphicsItem* item,
      const QPointF& offset,
      QUndoCommand* parent
    )
  : QUndoCommand(parent), scene_(scene), offset_(offset)
{
  setText(QObject::tr("Move item(s)"));
  items_.append(item);
  oldPos_.append(item->pos());
}


void MoveItems::undo()
{
  scene_->resetStatus();
  for(int i = 0; i < items_.size(); ++i)
    items_.at(i)->setPos(scene_->closestGridPoint(oldPos_.at(i)));
}


void MoveItems::redo()
{
  for(int i = 0; i < items_.size(); ++i)
    items_.at(i)->setPos(
        scene_->closestGridPoint(
          oldPos_.at(i) + offset_
    ));
}


bool MoveItems::mergeWith(const QUndoCommand* command)
{
  if(command->id() != id())
    return false;

  if(static_cast<const MoveItems*>(command)->items_.toSet() == items_.toSet()) {
    offset_ += static_cast<const MoveItems*>(command)->offset_;
    return true;
  }

  return false;
}


}
