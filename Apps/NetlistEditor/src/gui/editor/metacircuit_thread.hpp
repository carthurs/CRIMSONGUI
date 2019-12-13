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


#ifndef METACIRCUIT_THREAD_H
#define METACIRCUIT_THREAD_H


#include "model/metacircuit.h"
#include "parser/parser.h"

#include <QtCore/QMutexLocker>
#include <QtCore/QThread>
#include <QtCore/QMutex>


namespace qsapecng
{


class MetaCircuit_Thread: public QThread
{

  Q_OBJECT

public:
  MetaCircuit_Thread(QObject* parent = 0): QThread(parent)
    { circuit_ = new sapecng::circuit; }
  ~MetaCircuit_Thread() { delete circuit_; }

  inline void apply(sapecng::abstract_parser& parser)
    {
      QMutexLocker locker(&mutex_);
      sapecng::abstract_builder* builder =
        new sapecng::circuit_builder(*circuit_);
      parser.parse(*builder);
      delete builder;
    }

  inline sapecng::metacircuit::expression raw_numerator()
    { QMutexLocker locker(&mutex_); return meta_.raw().first; }
  inline sapecng::metacircuit::expression raw_denominator()
    { QMutexLocker locker(&mutex_); return meta_.raw().second; }
  inline sapecng::metacircuit::expression digit_numerator()
    { QMutexLocker locker(&mutex_); return meta_.digit().first; }
  inline sapecng::metacircuit::expression digit_denominator()
    { QMutexLocker locker(&mutex_); return meta_.digit().second; }
  inline sapecng::metacircuit::expression mixed_numerator()
    { QMutexLocker locker(&mutex_); return meta_.mixed().first; }
  inline sapecng::metacircuit::expression mixed_denominator()
    { QMutexLocker locker(&mutex_); return meta_.mixed().second; }

protected:
  void run()
  {
    QMutexLocker locker(&mutex_);
    meta_(*circuit_);
  }

private:
  sapecng::metacircuit meta_;
  sapecng::circuit* circuit_;
  QMutex mutex_;

};


}


#endif // METACIRCUIT_THREAD_H
