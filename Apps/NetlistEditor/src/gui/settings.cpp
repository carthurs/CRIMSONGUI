/*
    SapecNG - Next Generation Symbolic Analysis Program for Electric Circuit
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


#include "config.h"
#include "gui/settings.h"

#include <QtCore/QSettings>
#include <QtCore/QDir>


namespace qsapecng
{


QPoint Settings::mwPos_ = QPoint(200, 200);
QSize Settings::mwSize_ = QSize(400, 400);
QByteArray Settings::mwState_;

QString Settings::workspace_ = QDir::homePath();

QFont Settings::appFont_;

int Settings::logLvl_ = 0;
QColor Settings::debugColor_ = QColor(Qt::black);
QColor Settings::infoColor_ = QColor(Qt::black);
QColor Settings::warningColor_ = QColor(Qt::blue);
QColor Settings::errorColor_ = QColor(Qt::red);
QColor Settings::fatalColor_ = QColor(Qt::darkRed);

QStringList Settings::recentFiles_;



void Settings::load()
{
  QSettings settings(SETTINGS_ORGANIZATION, SETTINGS_APPLICATION);

  settings.beginGroup("Application");
    appFont_ = qvariant_cast<QFont>(settings.value("font"));
    recentFiles_ = settings.value("recents").toStringList();
    workspace_ = settings.value("workspace", QDir::homePath()).toString();
  settings.endGroup();

  settings.beginGroup("MainWindow");
    mwPos_ = settings.value("pos", QPoint(200, 200)).toPoint();
    mwSize_ = settings.value("size", QSize(400, 400)).toSize();
    mwState_ = settings.value("state").toByteArray();
  settings.endGroup();

  settings.beginGroup("Log");
    logLvl_ = settings.value("logLvl").toInt();
    debugColor_ = qvariant_cast<QColor>(settings.value("debugColor", QColor(Qt::black)));
    infoColor_ = qvariant_cast<QColor>(settings.value("infoColor", QColor(Qt::black)));
    warningColor_ = qvariant_cast<QColor>(settings.value("warningColor", QColor(Qt::blue)));
    errorColor_ = qvariant_cast<QColor>(settings.value("errorColor", QColor(Qt::red)));
    fatalColor_ = qvariant_cast<QColor>(settings.value("fatalColor", QColor(Qt::darkRed)));
  settings.endGroup();
}


void Settings::save()
{
  QSettings settings(SETTINGS_ORGANIZATION, SETTINGS_APPLICATION);

  settings.beginGroup("Application");
    settings.setValue("font", appFont_);
    settings.setValue("recents", recentFiles_);
    settings.setValue("workspace", workspace_);
  settings.endGroup();

  settings.beginGroup("MainWindow");
    settings.setValue("pos", mwPos_);
    settings.setValue("size", mwSize_);
    settings.setValue("state", mwState_);
  settings.endGroup();

  settings.beginGroup("Log");
    settings.setValue("logLvl", logLvl_);
    settings.setValue("debugColor", debugColor_);
    settings.setValue("infoColor", infoColor_);
    settings.setValue("warningColor", warningColor_);
    settings.setValue("errorColor", errorColor_);
    settings.setValue("fatalColor", fatalColor_);
  settings.endGroup();
}


}
