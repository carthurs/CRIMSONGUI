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


#ifndef PROPERTYTEXTITEM_H
#define PROPERTYTEXTITEM_H


#include <QGraphicsTextItem>

#include "gui/qtsolutions/qtpropertyeditor/QtProperty"


namespace qsapecng
{


class PropertyTextItem: public QGraphicsTextItem
{

  Q_OBJECT

public:
  PropertyTextItem(QGraphicsItem* parent = 0)
      : QGraphicsTextItem(parent), property_(0)
    {
      setFont(QFont("Times", 6, QFont::Light));
      setTextInteractionFlags(Qt::NoTextInteraction);
      setFlags(QGraphicsItem::ItemIsMovable);
    }
  PropertyTextItem(const QString& text, QGraphicsItem* parent = 0)
      : QGraphicsTextItem(text, parent), property_(0)
    {
      setFont(QFont("Times", 6, QFont::Light));
      setTextInteractionFlags(Qt::NoTextInteraction);
      setFlags(QGraphicsItem::ItemIsMovable);
    }

  inline void setProperty(QtProperty* property) { property_ = property; }

public slots:
  void valueChanged(QtProperty* property, const QString& value)
    { if(property == property_) setPlainText(value); }

private:
  QtProperty* property_;

};


}


#endif // PROPERTYTEXTITEM_H
