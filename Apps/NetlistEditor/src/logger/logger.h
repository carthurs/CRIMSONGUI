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


#ifndef LOGGER_H
#define LOGGER_H


#include "logger/logpolicy.h"

#include <streambuf>
#include <ostream>

#include <sstream>


namespace sapecng
{


/*
 * begin: google docet
 */

template <class cT, class traits = std::char_traits<cT> >
class basic_nullbuf: public std::basic_streambuf<cT, traits>
{

  typename traits::int_type overflow(typename traits::int_type c)
  {
    return traits::not_eof(c);
  }

};

template <class cT, class traits = std::char_traits<cT> >
class basic_onullstream: public std::basic_ostream<cT, traits>
{

public:
  basic_onullstream():
    std::basic_ios<cT, traits>(&m_sbuf),
    std::basic_ostream<cT, traits>(&m_sbuf)
  {
    this->init(&m_sbuf);
  }

private:
  basic_nullbuf<cT, traits> m_sbuf;

};

typedef basic_onullstream<char> onullstream;
typedef basic_onullstream<wchar_t> wonullstream; 

/*
 * end: google docet
 */


class LogPolicy;


class Logger
{

public:
  Logger() { }
  virtual ~Logger();

  enum LogLevel {
    DEBUG = 0,
    INFO,
    WARNING,
    ERROR,
    FATAL
  };
  
  virtual std::ostream& get(LogLevel level = INFO);

  static LogLevel level() { return level_; }
  static void setLevel(LogLevel level) { level_ = level; }
  static LogPolicy* policy() { return policy_; }
  static void setPolicy(LogPolicy* policy) { policy_ = policy; }

private:
  Logger(const Logger& logger);
  Logger& operator=(const Logger& logger);

  std::ostringstream os_;
  onullstream null_;

  static LogLevel level_;
  static LogPolicy* policy_;

};


}


#endif // LOGGER_H
