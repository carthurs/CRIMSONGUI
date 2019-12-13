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


#ifndef EXTENDEDMDIAREA_H
#define EXTENDEDMDIAREA_H


#include <QMdiArea>

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QtCore/QMimeData>

#include <QtCore/QList>
#include <QtCore/QFile>
#include <QtCore/QUrl>


namespace qsapecng
{



class ExtendedMdiArea: public QMdiArea
{

  Q_OBJECT

public:
  ExtendedMdiArea(QWidget* parent = 0): QMdiArea(parent)
    { setAcceptDrops(true); }

signals:
  void dropFileEvent(const QString& fileName);

protected:
  void dragEnterEvent(QDragEnterEvent* event)
  {
    if(event->mimeData()->hasUrls())
      event->accept();
  }


  void dropEvent(QDropEvent* event)
  {
    if(event->mimeData()->hasUrls()
        && event->proposedAction() == Qt::CopyAction)
    {
      QList<QUrl> urls = event->mimeData()->urls();
      foreach(QUrl url, urls) {
        QString file = url.toLocalFile();
        if(QFile::exists(file))
          emit dropFileEvent(file);
      }

      event->acceptProposedAction();
    }
  }

};



}


#endif // EXTENDEDMDIAREA_H
