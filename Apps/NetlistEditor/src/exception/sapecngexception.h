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


#ifndef SAPECNGEXCEPTION_H
#define SAPECNGEXCEPTION_H


#include <boost/exception/all.hpp>


namespace sapecng
{

struct sapecng_exception
  : virtual std::exception, virtual boost::exception { };

struct io_error: virtual sapecng_exception { };
struct file_error: virtual sapecng_exception { };
struct stream_error: virtual sapecng_exception { };
struct read_error: virtual sapecng_exception { };
struct write_error: virtual sapecng_exception { };

struct file_read_error
  : virtual file_error, virtual read_error { };

struct file_write_error
  : virtual file_error, virtual write_error { };

struct stream_read_error
  : virtual stream_error, virtual read_error { };

struct stream_write_error
  : virtual stream_error, virtual write_error { };


}


#endif // SAPECNGEXCEPTION_H
