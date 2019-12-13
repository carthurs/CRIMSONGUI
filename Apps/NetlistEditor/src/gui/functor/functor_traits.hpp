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


#ifndef FUNCTOR_TRAITS_H
#define FUNCTOR_TRAITS_H


#include "functor/functor.hpp"

#include <qwt_plot_curve.h>
#include <qwt_symbol.h>


namespace qsapecng
{



template < class F >
struct functor_traits { };



template < >
struct functor_traits< sapecng::magnitude >
{
  static QString title;
  static QString xLabel;
  static QString yLabel;
  static QwtPlotCurve::CurveStyle style;
  static QwtPlotCurve::CurveAttribute attribute;
  static QwtSymbol::Style symbol;
  static int ssize;
  static QPen pen;
};
QString functor_traits<sapecng::magnitude>::title = QObject::tr("Magnitude");
QString functor_traits<sapecng::magnitude>::xLabel = QObject::tr("Frequency");
QString functor_traits<sapecng::magnitude>::yLabel = QObject::tr("Value");
QwtPlotCurve::CurveStyle
  functor_traits<sapecng::magnitude>::style = QwtPlotCurve::Lines;
QwtPlotCurve::CurveAttribute
  functor_traits<sapecng::magnitude>::attribute = QwtPlotCurve::Fitted;
QwtSymbol::Style
  functor_traits<sapecng::magnitude>::symbol = QwtSymbol::NoSymbol;
int functor_traits<sapecng::magnitude>::ssize = 0;
QPen functor_traits<sapecng::magnitude>::pen =
  QPen(Qt::SolidPattern, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);



template < >
struct functor_traits< sapecng::phase >
{
  static QString title;
  static QString xLabel;
  static QString yLabel;
  static QwtPlotCurve::CurveStyle style;
  static QwtPlotCurve::CurveAttribute attribute;
  static QwtSymbol::Style symbol;
  static int ssize;
  static QPen pen;
};
QString functor_traits<sapecng::phase>::title = QObject::tr("Phase");
QString functor_traits<sapecng::phase>::xLabel = QObject::tr("Frequency");
QString functor_traits<sapecng::phase>::yLabel = QObject::tr("Value");
QwtPlotCurve::CurveStyle
  functor_traits<sapecng::phase>::style = QwtPlotCurve::Lines;
QwtPlotCurve::CurveAttribute
  functor_traits<sapecng::phase>::attribute = QwtPlotCurve::Fitted;
QwtSymbol::Style
  functor_traits<sapecng::phase>::symbol = QwtSymbol::NoSymbol;
int functor_traits<sapecng::phase>::ssize = 0;
QPen functor_traits<sapecng::phase>::pen =
  QPen(Qt::SolidPattern, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);



template < >
struct functor_traits< sapecng::gain >
{
  static QString title;
  static QString xLabel;
  static QString yLabel;
  static QwtPlotCurve::CurveStyle style;
  static QwtPlotCurve::CurveAttribute attribute;
  static QwtSymbol::Style symbol;
  static int ssize;
  static QPen pen;
};
QString functor_traits<sapecng::gain>::title = QObject::tr("Gain");
QString functor_traits<sapecng::gain>::xLabel = QObject::tr("Frequency");
QString functor_traits<sapecng::gain>::yLabel = QObject::tr("dB");
QwtPlotCurve::CurveStyle
  functor_traits<sapecng::gain>::style = QwtPlotCurve::Lines;
QwtPlotCurve::CurveAttribute
  functor_traits<sapecng::gain>::attribute = QwtPlotCurve::Fitted;
QwtSymbol::Style
  functor_traits<sapecng::gain>::symbol = QwtSymbol::NoSymbol;
int functor_traits<sapecng::gain>::ssize = 0;
QPen functor_traits<sapecng::gain>::pen =
  QPen(Qt::SolidPattern, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);



template < >
struct functor_traits< sapecng::loss >
{
  static QString title;
  static QString xLabel;
  static QString yLabel;
  static QwtPlotCurve::CurveStyle style;
  static QwtPlotCurve::CurveAttribute attribute;
  static QwtSymbol::Style symbol;
  static int ssize;
  static QPen pen;
};
QString functor_traits<sapecng::loss>::title = QObject::tr("Loss");
QString functor_traits<sapecng::loss>::xLabel = QObject::tr("Frequency");
QString functor_traits<sapecng::loss>::yLabel = QObject::tr("dB");
QwtPlotCurve::CurveStyle
  functor_traits<sapecng::loss>::style = QwtPlotCurve::Lines;
QwtPlotCurve::CurveAttribute
  functor_traits<sapecng::loss>::attribute = QwtPlotCurve::Fitted;
QwtSymbol::Style
  functor_traits<sapecng::loss>::symbol = QwtSymbol::NoSymbol;
int functor_traits<sapecng::loss>::ssize = 0;
QPen functor_traits<sapecng::loss>::pen =
  QPen(Qt::SolidPattern, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);



template < >
struct functor_traits< sapecng::zeros >
{
  static QString title;
  static QString xLabel;
  static QString yLabel;
  static QwtPlotCurve::CurveStyle style;
  static QwtPlotCurve::CurveAttribute attribute;
  static QwtSymbol::Style symbol;
  static int ssize;
  static QPen pen;
};
QString functor_traits<sapecng::zeros>::title = QObject::tr("Zeros");
QString functor_traits<sapecng::zeros>::xLabel = QObject::tr("Real part");
QString functor_traits<sapecng::zeros>::yLabel = QObject::tr("Img part");
QwtPlotCurve::CurveStyle
  functor_traits<sapecng::zeros>::style = QwtPlotCurve::NoCurve;
QwtPlotCurve::CurveAttribute
  functor_traits<sapecng::zeros>::attribute = QwtPlotCurve::Fitted;
QwtSymbol::Style
  functor_traits<sapecng::zeros>::symbol = QwtSymbol::Ellipse;
int functor_traits<sapecng::zeros>::ssize = 7;
QPen functor_traits<sapecng::zeros>::pen = QPen();



template < >
struct functor_traits< sapecng::poles >
{
  static QString title;
  static QString xLabel;
  static QString yLabel;
  static QwtPlotCurve::CurveStyle style;
  static QwtPlotCurve::CurveAttribute attribute;
  static QwtSymbol::Style symbol;
  static int ssize;
  static QPen pen;
};
QString functor_traits<sapecng::poles>::title = QObject::tr("Poles");
QString functor_traits<sapecng::poles>::xLabel = QObject::tr("Real part");
QString functor_traits<sapecng::poles>::yLabel = QObject::tr("Img part");
QwtPlotCurve::CurveStyle
  functor_traits<sapecng::poles>::style = QwtPlotCurve::NoCurve;
QwtPlotCurve::CurveAttribute
  functor_traits<sapecng::poles>::attribute = QwtPlotCurve::Fitted;
QwtSymbol::Style
  functor_traits<sapecng::poles>::symbol = QwtSymbol::XCross;
int functor_traits<sapecng::poles>::ssize = 7;
QPen functor_traits<sapecng::poles>::pen = QPen();



}


#endif // FUNCTOR_TRAITS_H
