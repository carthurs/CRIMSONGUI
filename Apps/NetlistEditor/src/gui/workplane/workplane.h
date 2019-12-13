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


#ifndef WORKPLANE_H
#define WORKPLANE_H


#include "model/metacircuit.h"

#include <QWidget>
#include <QMenu>
#include <QContextMenuEvent>

#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QPair>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

#include <string>
#include <map>


class QVBoxLayout;
class QHBoxLayout;
class QDoubleSpinBox;
class QTableWidget;

class QwtPlotGrid;
class QwtPlotPicker;
class QwtPlotMarker;


namespace qsapecng
{


class QwtPlot_ContextMenu: public QwtPlot
{

public:
  QwtPlot_ContextMenu(QWidget* parent = 0): QwtPlot(parent), contextMenu_(0) { }
  QwtPlot_ContextMenu(const QwtText& title, QWidget* parent = 0)
    : QwtPlot(title, parent), contextMenu_(0) { }

  inline void setContextMenu(QMenu* menu) { contextMenu_ = menu; }
  inline QMenu* contextMenu() const { return contextMenu_; }

  void contextMenuEvent(QContextMenuEvent* event)
    { if(contextMenu_) contextMenu_->exec(event->globalPos()); }

private:
  QMenu* contextMenu_;

};



typedef
  std::pair< std::vector<double>, std::vector<double> >
  (*functor)
    (
      const sapecng::metacircuit::expression& numerator,
      const sapecng::metacircuit::expression& denominator,
      std::map< std::string, double > values
    )
  ;

class MarkableCurve: public QObject, public QwtPlotCurve
{

  Q_OBJECT

public:
  MarkableCurve(): QwtPlotCurve() { initialize(); }
  ~MarkableCurve();

  void setVisible(bool on);

public slots:
  void selected();
  void appended(const QPointF& pos);
  void moved(const QPointF& pos);

private:
  void initialize();

private:
  QwtPlotMarker* marker_;

};



class MarkedCurve: public QwtPlotCurve
{

public:
  MarkedCurve(): QwtPlotCurve() { }
  ~MarkedCurve();

  void setVisible(bool on);
  void replotMarker();

private:
  void resetMarker();

private:
  QVector<QwtPlotMarker*> markers_;

};



class WorkPlane: public QWidget
{

  Q_OBJECT

public:
  enum F
  {
    MAGNITUDE,
    MAGNITUDE_RAD,
    PHASE,
    PHASE_RAD,
    GAIN,
    GAIN_RAD,
    LOSS,
    LOSS_RAD,
    ZEROS,
    POLES,
    NOOP
  };

public:
  WorkPlane(QWidget* parent = 0);
  ~WorkPlane();

  void setData(const std::map<std::string, double>& values,
      const sapecng::metacircuit::expression& numerator,
      const sapecng::metacircuit::expression& denominator
    );

  inline void setContextMenu(QMenu* menu) { plot_->setContextMenu(menu); }
  inline QMenu* contextMenu() const { return plot_->contextMenu(); }

  const QwtPlot& const_plot() const { return *plot_; }

public slots:
  void setDirty();
  void xAxisLogScale(bool log = true);
  void yAxisLogScale(bool log = false);
  void plot(WorkPlane::F f);
  void plot(int f);
  void redraw();

private:
  std::map<std::string, double> actValues() const;
  void createMainLayout();
  void setupCurves();
  void setupCurve(
      std::pair< std::vector<double>, std::vector<double> > data,
      WorkPlane::F f
    );

private:
  sapecng::metacircuit::expression num_;
  sapecng::metacircuit::expression den_;

  QwtPlotGrid* grid_;
  QwtPlot_ContextMenu* plot_;
  QVector< QwtPlotCurve* > attached_;
  QVector< QPair< QwtPlotCurve*, bool > > curves_;
  QwtPlotPicker* tracker_;
  F lastId_;

  QDoubleSpinBox* startFreq_;
  QDoubleSpinBox* endFreq_;
  QDoubleSpinBox* stepFreq_;
  double oldStartFreq_;
  double oldEndFreq_;
  double oldStepFreq_;

  QTableWidget* data_;

  QVBoxLayout* dataLayout_;
  QVBoxLayout* centralLayout_;
  QHBoxLayout* mainLayout_;

};


}


#endif // WORKPLANE_H
