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


#ifndef ITEM_H
#define ITEM_H


#include <QGraphicsItem>
#include <QPen>


class QGraphicsSceneMouseEvent;


namespace qsapecng
{


class SchematicScene;


class Item: public QGraphicsItem
{

public:
  Item(QGraphicsItem* parent = 0, SchematicScene* scene = 0);
  virtual ~Item() { }

  enum { Type = UserType + 1 };
  enum { OpacityFactor = 3 };

  enum ItemType {
      Std = Type,
      Wire,
      Label
    };

  virtual inline ItemType itemType() const { return Std; }
  virtual inline int type() const { return Type; }

  virtual void mirror();
  virtual void rotate();
  virtual void rotateBack();

  QHash<QString, QString> getProperties() const;

  bool mirrored() const;
  bool rotated() const;
  uint angle() const;

  SchematicScene* schematicScene() const;

protected:
  virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event);
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);

protected:
  qreal penWidth() const;
  QPen defaultPen_;
  QPen itemPen_;

private:
  bool mirrored_;
  uint angle_;

};


}


#endif // ITEM_H
