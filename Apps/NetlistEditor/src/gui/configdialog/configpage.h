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


#ifndef CONFIGPAGE_H
#define CONFIGPAGE_H


#include <QWidget>


class QComboBox;
class QFontDialog;
class QPushButton;

class QSignalMapper;


namespace qsapecng
{


class ConfigPage: public QWidget
{

  Q_OBJECT

public:
  ConfigPage(QWidget* parent = 0)
    : QWidget(parent) { setWindowTitle("[*]"); }

  void setPageModified(bool modified)
    { setWindowModified(modified); }
  bool isPageModified()
    { return isWindowModified(); }

  virtual void apply() = 0;

};


class GeneralPage: public ConfigPage
{

  Q_OBJECT

public:
  GeneralPage(QWidget* parent = 0);

  void apply();

private slots:
  void changeColor(int id);
  void levelChanged();

private:
  QComboBox* logLevel_;

  QSignalMapper* colorMapper_;
  QPushButton* debugColor_;
  QPushButton* infoColor_;
  QPushButton* warningColor_;
  QPushButton* errorColor_;
  QPushButton* fatalColor_;

};


class FontPage: public ConfigPage
{

  Q_OBJECT

public:
  FontPage(QWidget* parent = 0);

  void apply();

private slots:
  void currentFontChanged();

private:
  QFontDialog* fontPage_;

};


}


#endif // CONFIGDIALOG_H
