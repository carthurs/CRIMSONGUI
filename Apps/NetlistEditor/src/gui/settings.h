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


#ifndef SETTINGS_H
#define SETTINGS_H


#include <QtCore/QPoint>
#include <QtCore/QSize>
#include <QtCore/QByteArray>

#include <QFont>
#include <QColor>


namespace qsapecng
{


class SettingsManager;


/*
 * developed as Monostate Pattern
 */
class Settings
{

friend class SettingsManager;

public:
  Settings() { }

  void load();
  void save();

  inline QPoint mwPos() const
    { return mwPos_; }

  inline QSize mwSize() const
    { return mwSize_; }

  inline QByteArray mwState() const
    { return mwState_; }

  inline QString workspace() const
    { return workspace_; }

  inline QFont appFont() const
    { return appFont_; }

  inline int logLvl() const
    { return logLvl_; }

  inline QColor debugColor() const
    { return debugColor_; }

  inline QColor infoColor() const
    { return infoColor_; }

  inline QColor warningColor() const
    { return warningColor_; }

  inline QColor errorColor() const
    { return errorColor_; }

  inline QColor fatalColor() const
    { return fatalColor_; }

  inline QStringList recentFiles() const
    { return recentFiles_; }

private:
  static QPoint mwPos_;
  static QSize mwSize_;
  static QByteArray mwState_;

  static QString workspace_;

  static QFont appFont_;

  static int logLvl_;
  static QColor debugColor_;
  static QColor infoColor_;
  static QColor warningColor_;
  static QColor errorColor_;
  static QColor fatalColor_;

  static QStringList recentFiles_;

};


class SettingsManager
{

public:
  inline void setMWPos(const QPoint& pos)
    { Settings::mwPos_ = pos; }

  inline void setMWSize(const QSize& size)
    { Settings::mwSize_ = size; }

  inline void setMWState(const QByteArray& state)
    { Settings::mwState_ = state; }

  inline void setAppFont(const QFont& font)
    { Settings::appFont_ = font; }

  inline void setWorkspace(const QString workspace)
    { Settings::workspace_ = workspace; }

  inline void setLogLvl(int lvl)
    { Settings::logLvl_= lvl; }

  inline void setDebugColor(const QColor& color)
    { Settings::debugColor_ = color; }

  inline void setInfoColor(const QColor& color)
    { Settings::infoColor_ = color; }

  inline void setWarningColor(const QColor& color)
    { Settings::warningColor_ = color; }

  inline void setErrorColor(const QColor& color)
    { Settings::errorColor_ = color; }

  inline void setFatalColor(const QColor& color)
    { Settings::fatalColor_ = color; }

  inline void setRecentFiles(const QStringList& files)
    { Settings::recentFiles_ = files; }

};


}


#endif // SETTINGS_H
