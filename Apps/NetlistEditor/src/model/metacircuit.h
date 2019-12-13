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


#ifndef METACIRCUIT_H
#define METACIRCUIT_H


#include "model/circuit.h"
#include <list>
#include <map>


namespace sapecng
{



namespace detail
{


struct basic_checker
{

bool operator()(const std::pair< double, std::list<std::string> >& e) const
  { return (e.first == 0); }

};


struct degree_checker
{

bool operator()(const std::pair< int, std::list<
    std::pair< double, std::list<std::string> > > >& e) const
  { return (e.second.size() == 0); }

};


}



class metacircuit
{

public:
  typedef
  std::pair< double, std::list<std::string> >
  basic_expression;

  typedef
  std::list< basic_expression >
  degree_expression;

  typedef
  std::map< int, degree_expression >
  expression;

  static std::string as_string(const expression& e);
  void operator()(const circuit& circ);

  inline std::pair<metacircuit::expression, metacircuit::expression>
    raw() const { return raw_; }
  inline std::pair<metacircuit::expression, metacircuit::expression>
    digit() const { return digit_; }
  inline std::pair<metacircuit::expression, metacircuit::expression>
    mixed() const { return mixed_; }

private:
  void append(int deg, expression& e, const basic_expression& be);
  void simplify(std::pair<expression, expression>& p);

  void compress(expression& e);
  bool group_minus(const expression& e);
  void toggle_minus(expression& e);
  bool has_candidate(const expression& e, const std::string& c);
  void remove_candidate(expression& e, const std::string& c);
  double find_div(const expression& e);
  void normalize(expression& e, double div);

  static std::string stringify(double x);
  static std::string as_string(const degree_expression& e);
  static std::string as_string(const basic_expression& e);

  int get_sign(
      size_t vertices,
      const Graph& graph,
      std::vector< circuit::edge_descriptor > order,
      std::vector<bool> inL
    );

private:
  std::pair<metacircuit::expression, metacircuit::expression> raw_;
  std::pair<metacircuit::expression, metacircuit::expression> digit_;
  std::pair<metacircuit::expression, metacircuit::expression> mixed_;

};



}


#endif // METACIRCUIT_H
