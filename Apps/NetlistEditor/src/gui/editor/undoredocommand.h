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


#ifndef UNDOREDO_H
#define UNDOREDO_H


#include "gui/editor/schematicscene.h"

#include <QUndoCommand>

#include <QtCore/QPointF>
#include <QtCore/QLineF>

#include <QtCore/QVariant>
#include <QtCore/QList>


class QGraphicsScene;
class QGraphicsItem;


namespace qsapecng
{


class Wire;
class Item;


class AddItems: public QUndoCommand
{

public:
  AddItems(
      SchematicScene* scene,
      QList<QGraphicsItem*> items,
      QUndoCommand* parent = 0
    );
  AddItems(
      SchematicScene* scene,
      QGraphicsItem* item,
      QUndoCommand* parent = 0
    );
  AddItems(
      SchematicScene* scene,
      QList<QGraphicsItem*> items,
      const QList<QPointF>& pos,
      QUndoCommand* parent = 0
    );
  AddItems(
      SchematicScene* scene,
      QGraphicsItem* item,
      const QPointF& pos,
      QUndoCommand* parent = 0
    );
  ~AddItems();

  virtual void undo();
  virtual void redo();

private:
  SchematicScene* scene_;
  QList<QPointF> pos_;
  QList<QGraphicsItem*> items_;

};


class AddSupportedItem: public QUndoCommand
{

public:
  AddSupportedItem(
      SchematicScene* scene,
      SchematicScene::SupportedItemType type,
      const QPointF& pos,
      QUndoCommand* parent = 0
    );
  ~AddSupportedItem();

  virtual void undo();
  virtual void redo();

  Item* item() const;

private:
  SchematicScene* scene_;
  QPointF pos_;
  Item* item_;

};


// class AddUserDefItem: public QUndoCommand
// {

// public:
//   AddUserDefItem(
//       SchematicScene* scene,
//       QByteArray md5,
//       int size,
//       std::string info,
//       const QPointF& pos,
//       QUndoCommand* parent = 0
//     );
//   ~AddUserDefItem();

//   virtual void undo();
//   virtual void redo();

//   Item* item() const;

// private:
//   SchematicScene* scene_;
//   QPointF pos_;
//   Item* item_;

// };


class AddWire: public QUndoCommand
{

public:
  AddWire(
      SchematicScene* scene,
      const QPointF& pos,
      const QLineF& wire,
      bool connectedJunctions,
      QUndoCommand* parent = 0
    );
  ~AddWire();

  virtual void undo();
  virtual void redo();

private:
  SchematicScene* scene_;
  QPointF pos_;
  Item* wire_;

};


class JoinWires: public QUndoCommand
{

public:
  JoinWires(
      Wire* w1,
      Wire* w2,
      QUndoCommand* parent = 0
    );
  ~JoinWires();

  virtual void undo();
  virtual void redo();

private:
  SchematicScene* scene_;
  Item* wire_;
  Item* w1_;
  Item* w2_;

};


class RemoveItems: public QUndoCommand
{

public:
  RemoveItems(
      SchematicScene* scene,
      QList<QGraphicsItem*> items,
      QUndoCommand* parent = 0
    );
  RemoveItems(
      SchematicScene* scene,
      QGraphicsItem* item,
      QUndoCommand* parent = 0
    );

  virtual void undo();
  virtual void redo();

private:
  SchematicScene* scene_;
  QList<QGraphicsItem*> items_;

};


class RotateItems: public QUndoCommand
{

public:
  RotateItems(
      QList<QGraphicsItem*> items,
      QUndoCommand* parent = 0
  );
  RotateItems(
      QGraphicsItem* item,
      QUndoCommand* parent = 0
  );

  virtual void undo();
  virtual void redo();

private:
  QList<QGraphicsItem*> items_;

};


class MirrorItems: public QUndoCommand
{

public:
  MirrorItems(
      QList<QGraphicsItem*> items,
      QUndoCommand* parent = 0
    );
  MirrorItems(
      QGraphicsItem* item,
      QUndoCommand* parent = 0
    );

  virtual void undo();
  virtual void redo();

private:
  QList<QGraphicsItem*> items_;

};


class BringToFrontItem: public QUndoCommand
{

public:
  BringToFrontItem(
      QGraphicsItem* item,
      QUndoCommand* parent = 0
    );

  enum { ID = 23 };

  virtual void undo();
  virtual void redo();

  inline int id() const { return ID; }
  bool mergeWith(const QUndoCommand* command);

private:
  QGraphicsItem* item_;
  qreal oldZValue_;
  qreal zValue_;

};


class SendToBackItem: public QUndoCommand
{

public:
  SendToBackItem(QGraphicsItem* item, QUndoCommand* parent = 0);

  enum { ID = 29 };

  virtual void undo();
  virtual void redo();

  inline int id() const { return ID; }
  bool mergeWith(const QUndoCommand* command);

private:
  QGraphicsItem* item_;
  qreal oldZValue_;
  qreal zValue_;

};


class MoveItems: public QUndoCommand
{

public:
  MoveItems(
    SchematicScene* scene,
    QList<QGraphicsItem*> items,
    const QPointF& offset,
    QUndoCommand* parent = 0
  );
  MoveItems(
    SchematicScene* scene,
    QGraphicsItem* item,
    const QPointF& offset,
    QUndoCommand* parent = 0
  );

  enum { ID = 7 };

  virtual void undo();
  virtual void redo();

  inline int id() const { return ID; }
  bool mergeWith(const QUndoCommand* command);

private:
  SchematicScene* scene_;
  QList<QGraphicsItem*> items_;
  QList<QPointF> oldPos_;
  QPointF offset_;

};


}


#endif // UNDOREDO_H
