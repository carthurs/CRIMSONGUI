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


#ifndef FUNCTOR_H
#define FUNCTOR_H


#include "model/metacircuit.h"

#include <complex>
#include <map>


namespace sapecng
{



struct functor
{

public:
  std::pair< std::vector<double>, std::vector<double> >
  operator()
    (
      const metacircuit::expression& numerator,
      const metacircuit::expression& denominator,
      std::map< std::string, double > values
    )
  {
    std::map< int, double > real_num = synthesis(numerator, values);
    std::map< int, double > real_den = synthesis(denominator, values);
    return op(real_num, real_den);
  }

protected:
  virtual std::pair< std::vector<double>, std::vector<double> >
    op(std::map< int, double > num, std::map< int, double > den) = 0;

private:
  std::map< int, double >
  synthesis(
      const metacircuit::expression& expr,
      std::map< std::string, double > values
    )
  {
    std::map< int, double > syn;

    for(metacircuit::expression::const_iterator
      eit = expr.begin(); eit != expr.end(); ++eit)
    {
      double value = 0.;
      for(metacircuit::degree_expression::const_iterator dit =
        eit->second.begin(); dit != eit->second.end(); ++dit)
      {
        double sub_value = dit->first;
        for(std::list<std::string>::const_iterator
            it = dit->second.begin(); it != dit->second.end(); ++it)
          sub_value *= values[*it]; // 0-default drives from standard C++

        value += sub_value;
      }
      syn[eit->first] += value;
    }

    return syn;
  }

};



struct frequency_range_functor: public functor
{

public:
  frequency_range_functor(std::pair< double, double > range, double step)
    : range_(range), step_(step) { }

protected:
  virtual double apply( std::complex<double> v ) = 0;

private:
  std::pair< std::vector<double>, std::vector<double> >
    op(std::map< int, double > num, std::map< int, double > den)
  {
    std::pair< std::vector<double>, std::vector<double> > plot;
    if(step_ > 0 && range_.first <= range_.second)
    {
      for(double cnt = range_.first; cnt < range_.second; cnt += step_)
      {
        std::complex<double> cnum = compute(num, cnt);
        std::complex<double> cden = compute(den, cnt);

        plot.first.push_back(cnt);
        plot.second.push_back( apply(cnum / cden) );
      }
    }

    return plot;
  }

  std::complex<double>
  compute(
      std::map< int, double > expr,
      double step
    )
  {
    std::complex<double> cvalue(0., 0.);
    for(std::map< int, double >::const_iterator it = expr.begin();
      it != expr.end(); ++it)
    {
      bool real = (it->first % 2 == 0);
      double value =
          std::pow(-1., ((it->first - (real ? 0 : 1)) / 2))
        * std::pow(2. * 4.0 * std::atan(1.0) * step, it->first)
        * it->second;

      if(real)
        cvalue += std::complex<double>(value, 0.);
      else
        cvalue += std::complex<double>(0., value);
    }

    return cvalue;
  }

private:
  std::pair< double, double > range_;
  double step_;

};



struct magnitude: public frequency_range_functor
{

public:
  magnitude(std::pair< double, double > range, double step)
    : frequency_range_functor(range, step) { }

private:
  double apply( std::complex<double> v ) { return std::abs(v); }

};



struct phase: public frequency_range_functor
{

public:
  phase(std::pair< double, double > range, double step)
    : frequency_range_functor(range, step) { }

private:
  double apply( std::complex<double> v ) { return std::arg(v); }

};



struct gain: public frequency_range_functor
{

public:
  gain(std::pair< double, double > range, double step)
    : frequency_range_functor(range, step) { }

private:
  double apply( std::complex<double> v )
    { return (20. * std::log10( std::abs(v) )); }

};



struct loss: public frequency_range_functor
{

public:
  loss(std::pair< double, double > range, double step)
    : frequency_range_functor(range, step) { }

private:
  double apply( std::complex<double> v )
    { return (20. * std::log10((1. / std::abs(v) ))); }

};



extern
std::list< std::pair< double, double > >
rpoly_adapter(const std::map< int, double >& p);

struct zeros: public functor
{

private:
  std::pair< std::vector<double>, std::vector<double> >
    op(std::map< int, double > num, std::map< int, double > den)
  {
    std::pair< std::vector<double>, std::vector<double> > ret;

    std::list< std::pair< double, double > > pts = rpoly_adapter(num);
    for(std::list< std::pair< double, double > >::const_iterator
        i = pts.begin(); i != pts.end(); ++i)
    {
      ret.first.push_back(i->first);
      ret.second.push_back(i->second);
    }

    return ret;
  }

};

struct poles: public functor
{

private:
  std::pair< std::vector<double>, std::vector<double> >
    op(std::map< int, double > num, std::map< int, double > den)
  {
    std::pair< std::vector<double>, std::vector<double> > ret;

    std::list< std::pair< double, double > > pts = rpoly_adapter(den);
    for(std::list< std::pair< double, double > >::const_iterator
        i = pts.begin(); i != pts.end(); ++i)
    {
      ret.first.push_back(i->first);
      ret.second.push_back(i->second);
    }

    return ret;
  }

};



}


#endif // FUNCTOR_H
