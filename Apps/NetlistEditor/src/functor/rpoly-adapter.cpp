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


#include <list>
#include <map>


extern int rpoly(double *op, int degree, double *zeror, double *zeroi);


namespace sapecng
{


std::list< std::pair< double, double > >
rpoly_adapter(const std::map< int, double >& p)
{
  std::list< std::pair< double, double > > zeros;
  int degree = p.rbegin()->first;

  // (0, 0)-centered workaround
  if(p.find(0) == p.end())
    for(int i = 0; i < degree; ++i)
      zeros.push_back(std::make_pair<double, double>(0, 0));

  double* op = new double [degree + 1];
  double* zeror = new double [degree + 1];
  double* zeroi = new double [degree + 1];

  for(int i = degree; i >= 0; --i) {
    if(p.find(i) != p.end())
      op[degree - i] = p.find(i)->second;
    else
      op[degree - i] = 0.;
  }

  int ret = rpoly(op, degree, zeror, zeroi);
  /*   if (ret == -1)  ==>  error   */

  for(int i = 0; i < ret; ++i)
    zeros.push_back(std::make_pair(zeror[i], zeroi[i]));

  delete [] zeroi;
  delete [] zeror;
  delete op;

  return zeros;
}


}
