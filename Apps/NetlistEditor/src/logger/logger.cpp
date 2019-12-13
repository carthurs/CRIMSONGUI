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


#include "logger/logger.h"
#include "logger/logpolicy.h"


namespace sapecng
{


Logger::LogLevel Logger::level_ = INFO;
LogPolicy* Logger::policy_ = 0;


Logger::~Logger()
{
  if(policy_)
    (*policy_)(os_.str());
}


std::ostream& Logger::get(LogLevel level)
{
  if(level < level_)
    return null_;
  else
    return os_;
}


}
