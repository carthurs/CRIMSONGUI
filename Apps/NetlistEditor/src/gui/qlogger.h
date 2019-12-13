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


#ifndef QLOGGER_H
#define QLOGGER_H


#include "logger/logpolicy.h"
#include "logger/logger.h"

#include <QtCore/QObject>
#include <QtCore/QRegExp>
#include <QColor>


typedef sapecng::LogPolicy SapecNGLogger;
typedef sapecng::LogPolicy SapecNGLogPolicy;


namespace qsapecng
{


class QLogPolicy: public QObject, public SapecNGLogPolicy
{

  Q_OBJECT

public:
  QLogPolicy(QObject* parent = 0)
    : QObject(parent), SapecNGLogPolicy() { }
  ~QLogPolicy() { }

  void operator()(const std::string& msg)
  {
    QString qmsg = QString::fromStdString(msg);
    if(qmsg.contains(QRegExp("^\\[DEBUG\\]\\s*")) && debugColor_.isValid())
      emit textColorChanged(debugColor_);
    if(qmsg.contains(QRegExp("^\\[INFO\\]\\s*")) && infoColor_.isValid())
      emit textColorChanged(infoColor_);
    if(qmsg.contains(QRegExp("^\\[WARNING\\]\\s*")) && warningColor_.isValid())
      emit textColorChanged(warningColor_);
    if(qmsg.contains(QRegExp("^\\[ERROR\\]\\s*")) && errorColor_.isValid())
      emit textColorChanged(errorColor_);
    if(qmsg.contains(QRegExp("^\\[FATAL\\]\\s*")) && fatalColor_.isValid())
      emit textColorChanged(fatalColor_);
    emit log(qmsg);
  }

  inline QColor debugColor() const
    { return debugColor_; }
  inline void setDebugColor(QColor color)
    { debugColor_ = color; }

  inline QColor infoColor() const
    { return infoColor_; }
  inline void setInfoColor(QColor color)
    { infoColor_ = color; }

  inline QColor warningColor() const
    { return warningColor_; }
  inline void setWarningColor(QColor color)
    { warningColor_ = color; }

  inline QColor errorColor() const
    { return errorColor_; }
  inline void setErrorColor(QColor color)
    { errorColor_ = color; }

  inline QColor fatalColor() const
    { return fatalColor_; }
  inline void setFatalColor(QColor color)
    { fatalColor_ = color; }

signals:
  void log(const QString& msg);
  void textColorChanged(const QColor& color);

private:
  QColor debugColor_;
  QColor infoColor_;
  QColor warningColor_;
  QColor errorColor_;
  QColor fatalColor_;

};


class QLogger
{

public:
  static void setLevel(sapecng::Logger::LogLevel lvl)
    { sapecng::Logger::setLevel(lvl); }

  static void setPolicy(QLogPolicy* policy)
    { sapecng::Logger::setPolicy(policy); }

  static void debug(const QString& msg)
    {
      sapecng::Logger().get(sapecng::Logger::DEBUG)
        << (QObject::tr("[DEBUG] ") + msg).toStdString();
    }

  static void info(const QString& msg)
    {
		std::string message = (QObject::tr("[INFO] ") + msg).toStdString();
      sapecng::Logger().get(sapecng::Logger::INFO)
        << message;
    }

  static void warning(const QString& msg)
    {
      sapecng::Logger().get(sapecng::Logger::WARNING)
        << (QObject::tr("[WARNING] ") + msg).toStdString();
    }

  static void error(const QString& msg)
    {
      sapecng::Logger().get(sapecng::Logger::ERROR)
        << (QObject::tr("[ERROR] ") + msg).toStdString();
    }

  static void fatal(const QString& msg)
    {
      sapecng::Logger().get(sapecng::Logger::FATAL)
        << (QObject::tr("[FATAL] ") + msg).toStdString();
    }

};


}


#endif // QLOGGER_H
