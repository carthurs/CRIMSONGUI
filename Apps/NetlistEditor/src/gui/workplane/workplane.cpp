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


#include "model/metacircuit.h"
#include "functor/functor.hpp"
#include "gui/workplane/workplane.h"
#include "gui/functor/functor_traits.hpp"
#include "gui/qlogger.h"

#include <QtCore/QRegExp>
#include <QRegExpValidator>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QFrame>

#include <qwt_plot_grid.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_marker.h>
#include <qwt_scale_engine.h>
#include <qwt_picker_machine.h>

#include <map>
#include <limits>


namespace qsapecng
{


MarkableCurve::~MarkableCurve()
{
    marker_->detach();
    delete marker_;
}


void MarkableCurve::setVisible(bool on)
{
  marker_->setVisible(false);

  if(on) marker_->attach(plot());
  else marker_->detach();

  QwtPlotCurve::setVisible(on);
}


void MarkableCurve::selected()
{
  marker_->setVisible(false);
}


void MarkableCurve::appended(const QPointF& pos)
{
  if(isVisible()) {
    marker_->setVisible(true);
    moved(pos);
  }
}


void MarkableCurve::moved(const QPointF& pos)
{
  if(isVisible()) {
    std::size_t idx;
    for(idx = 0; idx < (data()->size() - 1) && sample(idx + 1).x() < pos.x(); ++idx);
    marker_->setValue(sample(idx).x(), sample(idx).y());

    QString label = "( " + QString::number(sample(idx).x()) + ", "
      + QString::number(sample(idx).y()) + " ) ["
      + QString::number(idx) + "]";
    marker_->setLabel(label);

    Qt::Alignment H =
        (sample(idx).x() >= minXValue() + ((maxXValue() - minXValue()) / 2))
      ? Qt::AlignLeft
      : Qt::AlignRight
      ;
    Qt::Alignment V =
        (sample(idx).y() >= minYValue() + ((maxYValue() - minYValue()) / 2))
      ? Qt::AlignBottom
      : Qt::AlignTop
      ;
    marker_->setLabelAlignment(H | V);

    marker_->itemChanged();
  }
}


void MarkableCurve::initialize()
{
  QwtSymbol* symbol = new QwtSymbol();
  symbol->setStyle(QwtSymbol::Ellipse);
  symbol->setSize(QSize(5, 5));

  marker_ = new QwtPlotMarker;
  marker_->setLineStyle(QwtPlotMarker::NoLine);
  marker_->setSymbol(symbol);
  marker_->setVisible(false);
}



MarkedCurve::~MarkedCurve()
{
  resetMarker();
}


void MarkedCurve::setVisible(bool on)
{
  if(!on) {
    resetMarker();
  } else {
    replotMarker();
    foreach(QwtPlotMarker* marker, markers_) {
      marker->setVisible(on);
      marker->attach(plot());
      marker->itemChanged();
    }
  }
}


void MarkedCurve::replotMarker()
{
  resetMarker();

  for(std::size_t i = 0; i < dataSize(); ++i) {
    QwtPlotMarker* marker = new QwtPlotMarker;
    marker->setLineStyle(QwtPlotMarker::NoLine);
    marker->setLabelAlignment(Qt::AlignBottom);
    marker->setValue(sample(i).x(), sample(i).y());

    QString label = "( " + QString::number(sample(i).x()) + ", "
      + QString::number(sample(i).y()) + " )";
    marker->setLabel(label);

    markers_.push_back(marker);
  }
}


void MarkedCurve::resetMarker()
{
  foreach(QwtPlotMarker* marker, markers_) {
    marker->detach();
    delete marker;
  }

  markers_.clear();
}



WorkPlane::WorkPlane(QWidget* parent)
  : QWidget(parent),
    startFreq_(0), endFreq_(0), stepFreq_(0),
    oldStartFreq_(0), oldEndFreq_(0), oldStepFreq_(0)
{
  createMainLayout();
  setupCurves();
  setDirty();

  setLayout(mainLayout_);
  setWindowTitle(tr("WorkPlane"));
  setAttribute(Qt::WA_DeleteOnClose);

  connect(data_, SIGNAL(itemActivated(QTableWidgetItem*)),
    this, SLOT(setDirty()));
}


WorkPlane::~WorkPlane()
{
  for(int i = 0; i < curves_.size(); ++i)
    delete curves_[i].first;

  curves_.clear();
  attached_.clear();
}


void WorkPlane::setData(
    const std::map<std::string, double>& values,
    const sapecng::metacircuit::expression& numerator,
    const sapecng::metacircuit::expression& denominator
  )
{
  setDirty();

  data_->clearContents();
  data_->setColumnCount(2);
  data_->setRowCount(values.size());
  data_->setSortingEnabled(false);

  int row = 0;
  for(std::map<std::string, double>::const_iterator i = values.begin();
      i != values.end(); ++i, ++row)
  {
    QLabel* left = new QLabel(QString::fromStdString(i->first));
    left->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    
    QLineEdit* right = new QLineEdit;
    right->setText(QString::number(i->second));
    QRegExp sn("[+\\-]?(?:0|[1-9]\\d*)(?:\\.\\d*)?(?:[eE][+\\-]?\\d+)?");
    QRegExpValidator* validator = new QRegExpValidator(sn, right);
    right->setValidator(validator);
    
    data_->setCellWidget(row, 0, left);
    data_->setCellWidget(row, 1, right);
  }

  data_->setSortingEnabled(true);
  data_->sortItems(0);

  QStringList labels;
  labels << "Name" << "Value";
  data_->setHorizontalHeaderLabels(labels);
  data_->setVerticalHeader(new QHeaderView(Qt::Vertical));

  num_ = numerator;
  den_ = denominator;

  std::string num = sapecng::metacircuit::as_string(numerator);
  std::string den = sapecng::metacircuit::as_string(denominator);

  std::string expr;
  expr.append(num);
  expr.append("\n");
  expr.append(std::string(
    (num.size() > den.size() ? num.size() : den.size()) * 3/2, '-'));
  expr.append("\n");
  expr.append(den);

  setToolTip(QString::fromStdString(expr));
}


void WorkPlane::setDirty()
{
  attached_.clear();
  for(int i = 0; i < curves_.size(); ++i) {
    curves_[i].first->setVisible(false);
    curves_[i].first->detach();
    curves_[i].second = false;
  }

  plot_->setTitle(tr("none"));
  plot_->setAxisTitle(QwtPlot::yLeft, tr("none"));
  plot_->setAxisTitle(QwtPlot::xBottom, tr("none"));
  plot_->updateAxes();
  plot_->replot();
}


void WorkPlane::xAxisLogScale(bool log)
{
  if(log)
    plot_->setAxisScaleEngine(QwtPlot::xBottom, new QwtLogScaleEngine);
  else
    plot_->setAxisScaleEngine(QwtPlot::xBottom, new QwtLinearScaleEngine);

  plot_->updateAxes();
  plot_->replot();
}


void WorkPlane::yAxisLogScale(bool log)
{
  if(log)
    plot_->setAxisScaleEngine(QwtPlot::yLeft, new QwtLogScaleEngine);
  else
    plot_->setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine);

  plot_->updateAxes();
  plot_->replot();
}


void WorkPlane::plot(WorkPlane::F f)
{
  if(f == NOOP) {
    setDirty();
    return;
  }

  setEnabled(false);

  lastId_ = f;
  if(!attached_.isEmpty()) {
    foreach(QwtPlotCurve* curve, attached_) {
      curve->setVisible(false);
      curve->detach();
    }
  }

  switch(f)
  {
  case MAGNITUDE:
  case MAGNITUDE_RAD:
    {
      std::pair< std::vector<double>, std::vector<double> > res;
      if(!curves_[f].second) {
        QLogger::info(QObject::tr("Calculate magnitude."));
        std::map<std::string, double> values = actValues();
        res = sapecng::magnitude(
            std::make_pair(startFreq_->value(), endFreq_->value()),
            stepFreq_->value()
          )(num_, den_, values);
      }

      plot_->setTitle(functor_traits<sapecng::magnitude>::title);
      plot_->setAxisTitle(
        QwtPlot::yLeft, functor_traits<sapecng::magnitude>::yLabel);

      if(f == MAGNITUDE)
        plot_->setAxisTitle(
          QwtPlot::xBottom, functor_traits<sapecng::magnitude>::xLabel);
      else if(f == MAGNITUDE_RAD)
        plot_->setAxisTitle(QwtPlot::xBottom,
          functor_traits<sapecng::magnitude>::xLabel + tr(" (rad/s)"));

      setupCurve(res, MAGNITUDE);
      std::vector<double> rad = res.first;
      for(std::vector<double>::iterator i = rad.begin(); i < rad.end(); ++i)
        *i = 2. * 4.0 * std::atan(1.0) * *i;
      res.first = rad;
      setupCurve(res, MAGNITUDE_RAD);

      curves_[f].first->attach(plot_);
      curves_[f].first->setVisible(true);
      attached_.push_back(curves_[f].first);

      break;
    }
  case PHASE:
  case PHASE_RAD:
    {
      std::pair< std::vector<double>, std::vector<double> > res;
      if(!curves_[f].second) {
        QLogger::info(QObject::tr("Calculate phase."));
        std::map<std::string, double> values = actValues();
        res = sapecng::phase(
            std::make_pair(startFreq_->value(), endFreq_->value()),
            stepFreq_->value()
          )(num_, den_, values);
      }

      plot_->setTitle(functor_traits<sapecng::phase>::title);
      plot_->setAxisTitle(
        QwtPlot::yLeft, functor_traits<sapecng::phase>::yLabel);

      if(f == PHASE)
        plot_->setAxisTitle(
          QwtPlot::xBottom, functor_traits<sapecng::phase>::xLabel);
      else if(f == PHASE_RAD)
        plot_->setAxisTitle(QwtPlot::xBottom,
          functor_traits<sapecng::phase>::xLabel + tr(" (rad/s)"));

      setupCurve(res, PHASE);
      std::vector<double> rad = res.first;
      for(std::vector<double>::iterator i = rad.begin(); i < rad.end(); ++i)
        *i = 2. * 4.0 * std::atan(1.0) * *i;
      res.first = rad;
      setupCurve(res, PHASE_RAD);

      curves_[f].first->attach(plot_);
      curves_[f].first->setVisible(true);
      attached_.push_back(curves_[f].first);

      break;
    }
  case GAIN:
  case GAIN_RAD:
    {
      std::pair< std::vector<double>, std::vector<double> > res;
      if(!curves_[f].second) {
        QLogger::info(QObject::tr("Calculate gain."));
        std::map<std::string, double> values = actValues();
        res = sapecng::gain(
            std::make_pair(startFreq_->value(), endFreq_->value()),
            stepFreq_->value()
          )(num_, den_, values);
      }

      plot_->setTitle(functor_traits<sapecng::gain>::title);
      plot_->setAxisTitle(
        QwtPlot::yLeft, functor_traits<sapecng::gain>::yLabel);

      if(f == GAIN)
        plot_->setAxisTitle(
          QwtPlot::xBottom, functor_traits<sapecng::gain>::xLabel);
      else if(f == GAIN_RAD)
        plot_->setAxisTitle(QwtPlot::xBottom,
          functor_traits<sapecng::gain>::xLabel + tr(" (rad/s)"));

      setupCurve(res, GAIN);
      std::vector<double> rad = res.first;
      for(std::vector<double>::iterator i = rad.begin(); i < rad.end(); ++i)
        *i = 2. * 4.0 * std::atan(1.0) * *i;
      res.first = rad;
      setupCurve(res, GAIN_RAD);

      curves_[f].first->attach(plot_);
      curves_[f].first->setVisible(true);
      attached_.push_back(curves_[f].first);

      break;
    }
  case LOSS:
  case LOSS_RAD:
    {
      std::pair< std::vector<double>, std::vector<double> > res;
      if(!curves_[f].second) {
        QLogger::info(QObject::tr("Calculate loss."));
        std::map<std::string, double> values = actValues();
        res = sapecng::loss(
            std::make_pair(startFreq_->value(), endFreq_->value()),
            stepFreq_->value()
          )(num_, den_, values);
      }

      plot_->setTitle(functor_traits<sapecng::loss>::title);
      plot_->setAxisTitle(
        QwtPlot::yLeft, functor_traits<sapecng::loss>::yLabel);

      if(f == LOSS)
        plot_->setAxisTitle(
          QwtPlot::xBottom, functor_traits<sapecng::gain>::xLabel);
      else if(f == LOSS_RAD)
        plot_->setAxisTitle(QwtPlot::xBottom,
          functor_traits<sapecng::gain>::xLabel + tr(" (rad/s)"));

      setupCurve(res, LOSS);
      std::vector<double> rad = res.first;
      for(std::vector<double>::iterator i = rad.begin(); i < rad.end(); ++i)
        *i = 2. * 4.0 * std::atan(1.0) * *i;
      res.first = rad;
      setupCurve(res, LOSS_RAD);

      curves_[f].first->attach(plot_);
      curves_[f].first->setVisible(true);
      attached_.push_back(curves_[f].first);

      break;
    }
  case ZEROS:
  case POLES:
    {
      std::pair< std::vector<double>, std::vector<double> > resZ;
      if(!curves_[ZEROS].second) {
        QLogger::info(QObject::tr("Looking for zeros."));
        std::map<std::string, double> values = actValues();
        resZ = sapecng::zeros()(num_, den_, values);

        unsigned int size = resZ.first.size() > resZ.second.size() ?
          resZ.second.size() : resZ.first.size();

        for(unsigned int i = 0; i < size; ++i)
          QLogger::info(QObject::tr("Zero found near to ")
            + QString("(%1, %2)")
              .arg(QString::number(resZ.first.at(i)))
              .arg(QString::number(resZ.second.at(i))));
      }

      std::pair< std::vector<double>, std::vector<double> > resP;
      if(!curves_[POLES].second) {
        QLogger::info(QObject::tr("Looking for poles."));
        std::map<std::string, double> values = actValues();
        resP = sapecng::poles()(num_, den_, values);

        unsigned int size = resP.first.size() > resP.second.size() ?
          resP.second.size() : resP.first.size();

        for(unsigned int i = 0; i < size; ++i)
          QLogger::info(QObject::tr("Pole found near to ")
            + QString("(%1, %2)")
              .arg(QString::number(resP.first.at(i)))
              .arg(QString::number(resP.second.at(i))));
      }

      plot_->setTitle(
            functor_traits<sapecng::zeros>::title
          + "/" + functor_traits<sapecng::poles>::title
        );
      plot_->setAxisTitle(
        QwtPlot::yLeft, functor_traits<sapecng::zeros>::yLabel);
      plot_->setAxisTitle(
        QwtPlot::xBottom, functor_traits<sapecng::zeros>::xLabel);

      setupCurve(resZ, ZEROS);
      setupCurve(resP, POLES);

      curves_[ZEROS].first->attach(plot_);
      curves_[POLES].first->attach(plot_);
      curves_[ZEROS].first->setVisible(true);
      curves_[POLES].first->setVisible(true);
      attached_.push_back(curves_[ZEROS].first);
      attached_.push_back(curves_[POLES].first);

      break;
    }
  default:
    break;
  }

  plot_->setFocus(Qt::OtherFocusReason);
  plot_->updateAxes();
  plot_->replot();

  setEnabled(true);
}


void WorkPlane::plot(int f)
{
  plot((F) f);
}


void WorkPlane::redraw()
{
  setDirty();
  plot(lastId_);
}


std::map<std::string, double> WorkPlane::actValues() const
{
  std::map<std::string, double> values;

  for(int i = 0; i < data_->rowCount(); ++i)
    values[qobject_cast<const QLabel*>(
        data_->cellWidget(i, 0))->text().toStdString()] =
      qobject_cast<const QLineEdit*>(
	data_->cellWidget(i, 1))->text().toDouble();

  return values;
}


void WorkPlane::createMainLayout()
{
  data_ = new QTableWidget;
  data_->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
  data_->horizontalHeader()->setStretchLastSection(true);

  startFreq_ = new QDoubleSpinBox;
  startFreq_->setMaximum(std::numeric_limits<double>::max());
  startFreq_->setDecimals(5);
  startFreq_->setValue(0.00001);
  oldStartFreq_ = 0.01;
  endFreq_ = new QDoubleSpinBox;
  endFreq_->setMaximum(std::numeric_limits<double>::max());
  endFreq_->setDecimals(5);
  endFreq_->setValue(99.);
  oldEndFreq_ = 99.;
  stepFreq_ = new QDoubleSpinBox;
  stepFreq_->setMaximum(std::numeric_limits<double>::max());
  stepFreq_->setDecimals(5);
  stepFreq_->setValue(0.001);
  oldStepFreq_ = 0.001;

  QFormLayout* freqLayout = new QFormLayout;
  freqLayout->addRow(tr("Start (Hz):"), startFreq_);
  freqLayout->addRow(tr("End (Hz):"), endFreq_);
  freqLayout->addRow(tr("Step (Hz):"), stepFreq_);
  freqLayout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
  QGroupBox* freqBox = new QGroupBox(tr("Frequency box"));
  freqBox->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
  freqBox->setLayout(freqLayout);

  QHBoxLayout* replotLayout = new QHBoxLayout;
  QPushButton* replot = new QPushButton(tr("Replot"));
  replotLayout->addStretch();
  replotLayout->addWidget(replot);
  replotLayout->addStretch();
  connect(replot, SIGNAL(clicked(bool)), this, SLOT(redraw()));

  dataLayout_ = new QVBoxLayout;
  dataLayout_->addWidget(data_);
  dataLayout_->addWidget(freqBox);
  dataLayout_->addStretch(1);
  dataLayout_->addLayout(replotLayout);
  dataLayout_->addStretch(2);

  plot_ = new QwtPlot_ContextMenu(this);
  plot_->setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine);
  plot_->setAxisScaleEngine(QwtPlot::xBottom, new QwtLogScaleEngine);
  plot_->setAutoReplot(true);

  tracker_ = new QwtPlotPicker(plot_->canvas());
  tracker_->setStateMachine(new QwtPickerDragRectMachine());
  tracker_->setRubberBand(QwtPicker::VLineRubberBand);
  tracker_->setTrackerMode(QwtPicker::AlwaysOff);

  grid_ = new QwtPlotGrid;
  grid_->setPen(QPen(Qt::gray, 0., Qt::DotLine));
  grid_->attach(plot_);
  plot_->replot();

  centralLayout_ = new QVBoxLayout;
  centralLayout_->addWidget(plot_);

  QFrame* separator;

  mainLayout_ = new QHBoxLayout;
  mainLayout_->addLayout(dataLayout_);
  separator = new QFrame;
  separator->setFrameStyle(QFrame::VLine | QFrame::Sunken);
  separator->setMidLineWidth(2);
  mainLayout_->addWidget(separator);
  mainLayout_->addLayout(centralLayout_);
  mainLayout_->setStretchFactor(dataLayout_, 1);
  mainLayout_->setStretchFactor(centralLayout_, 2);
}


void WorkPlane::setupCurves()
{
  curves_ = QVector< QPair< QwtPlotCurve*, bool > >(NOOP);
  QwtSymbol* symbol;

  symbol = new QwtSymbol();
  symbol->setStyle(functor_traits<sapecng::magnitude>::symbol);
  symbol->setSize(functor_traits<sapecng::magnitude>::ssize);
  curves_[MAGNITUDE].first = new MarkableCurve;
  curves_[MAGNITUDE].first->setPen(functor_traits<sapecng::magnitude>::pen);
  curves_[MAGNITUDE].first->setStyle(functor_traits<sapecng::magnitude>::style);
  curves_[MAGNITUDE].first->setSymbol(symbol);
  curves_[MAGNITUDE].first->setCurveAttribute(
    functor_traits<sapecng::magnitude>::attribute);
  curves_[MAGNITUDE].second = false;

  connect(tracker_, SIGNAL(selected(const QPointF&)),
    (MarkableCurve*) curves_[MAGNITUDE].first, SLOT(selected()));
  connect(tracker_, SIGNAL(appended(const QPointF&)),
    (MarkableCurve*) curves_[MAGNITUDE].first,
    SLOT(appended(const QPointF&)));
  connect(tracker_, SIGNAL(moved(const QPointF&)),
    (MarkableCurve*) curves_[MAGNITUDE].first,
    SLOT(moved(const QPointF&)));

  symbol = new QwtSymbol();
  symbol->setStyle(functor_traits<sapecng::magnitude>::symbol);
  symbol->setSize(functor_traits<sapecng::magnitude>::ssize);
  curves_[MAGNITUDE_RAD].first = new MarkableCurve;
  curves_[MAGNITUDE_RAD].first->setPen(functor_traits<sapecng::magnitude>::pen);
  curves_[MAGNITUDE_RAD].first->setStyle(functor_traits<sapecng::magnitude>::style);
  curves_[MAGNITUDE_RAD].first->setSymbol(symbol);
  curves_[MAGNITUDE_RAD].first->setCurveAttribute(
    functor_traits<sapecng::magnitude>::attribute);
  curves_[MAGNITUDE_RAD].second = false;

  connect(tracker_, SIGNAL(selected(const QPointF&)),
    (MarkableCurve*) curves_[MAGNITUDE_RAD].first, SLOT(selected()));
  connect(tracker_, SIGNAL(appended(const QPointF&)),
    (MarkableCurve*) curves_[MAGNITUDE_RAD].first,
    SLOT(appended(const QPointF&)));
  connect(tracker_, SIGNAL(moved(const QPointF&)),
    (MarkableCurve*) curves_[MAGNITUDE_RAD].first,
    SLOT(moved(const QPointF&)));

  symbol = new QwtSymbol();
  symbol->setStyle(functor_traits<sapecng::phase>::symbol);
  symbol->setSize(functor_traits<sapecng::phase>::ssize);
  curves_[PHASE].first = new MarkableCurve;
  curves_[PHASE].first->setPen(functor_traits<sapecng::phase>::pen);
  curves_[PHASE].first->setStyle(functor_traits<sapecng::phase>::style);
  curves_[PHASE].first->setSymbol(symbol);
  curves_[PHASE].first->setCurveAttribute(
    functor_traits<sapecng::phase>::attribute);
  curves_[PHASE].second = false;

  connect(tracker_, SIGNAL(selected(const QPointF&)),
    (MarkableCurve*) curves_[PHASE].first, SLOT(selected()));
  connect(tracker_, SIGNAL(appended(const QPointF&)),
    (MarkableCurve*) curves_[PHASE].first,
    SLOT(appended(const QPointF&)));
  connect(tracker_, SIGNAL(moved(const QPointF&)),
    (MarkableCurve*) curves_[PHASE].first, SLOT(moved(const QPointF&)));

  symbol = new QwtSymbol();
  symbol->setStyle(functor_traits<sapecng::phase>::symbol);
  symbol->setSize(functor_traits<sapecng::phase>::ssize);
  curves_[PHASE_RAD].first = new MarkableCurve;
  curves_[PHASE_RAD].first->setPen(functor_traits<sapecng::phase>::pen);
  curves_[PHASE_RAD].first->setStyle(functor_traits<sapecng::phase>::style);
  curves_[PHASE_RAD].first->setSymbol(symbol);
  curves_[PHASE_RAD].first->setCurveAttribute(
    functor_traits<sapecng::phase>::attribute);
  curves_[PHASE_RAD].second = false;

  connect(tracker_, SIGNAL(selected(const QPointF&)),
    (MarkableCurve*) curves_[PHASE_RAD].first, SLOT(selected()));
  connect(tracker_, SIGNAL(appended(const QPointF&)),
    (MarkableCurve*) curves_[PHASE_RAD].first,
    SLOT(appended(const QPointF&)));
  connect(tracker_, SIGNAL(moved(const QPointF&)),
    (MarkableCurve*) curves_[PHASE_RAD].first,
    SLOT(moved(const QPointF&)));

  symbol = new QwtSymbol();
  symbol->setStyle(functor_traits<sapecng::gain>::symbol);
  symbol->setSize(functor_traits<sapecng::gain>::ssize);
  curves_[GAIN].first = new MarkableCurve;
  curves_[GAIN].first->setPen(functor_traits<sapecng::gain>::pen);
  curves_[GAIN].first->setStyle(functor_traits<sapecng::gain>::style);
  curves_[GAIN].first->setSymbol(symbol);
  curves_[GAIN].first->setCurveAttribute(
    functor_traits<sapecng::gain>::attribute);
  curves_[GAIN].second = false;

  connect(tracker_, SIGNAL(selected(const QPointF&)),
    (MarkableCurve*) curves_[GAIN].first, SLOT(selected()));
  connect(tracker_, SIGNAL(appended(const QPointF&)),
    (MarkableCurve*) curves_[GAIN].first,
    SLOT(appended(const QPointF&)));
  connect(tracker_, SIGNAL(moved(const QPointF&)),
    (MarkableCurve*) curves_[GAIN].first, SLOT(moved(const QPointF&)));

  symbol = new QwtSymbol();
  symbol->setStyle(functor_traits<sapecng::gain>::symbol);
  symbol->setSize(functor_traits<sapecng::gain>::ssize);
  curves_[GAIN_RAD].first = new MarkableCurve;
  curves_[GAIN_RAD].first->setPen(functor_traits<sapecng::gain>::pen);
  curves_[GAIN_RAD].first->setStyle(functor_traits<sapecng::gain>::style);
  curves_[GAIN_RAD].first->setSymbol(symbol);
  curves_[GAIN_RAD].first->setCurveAttribute(
    functor_traits<sapecng::gain>::attribute);
  curves_[GAIN_RAD].second = false;

  connect(tracker_, SIGNAL(selected(const QPointF&)),
    (MarkableCurve*) curves_[GAIN_RAD].first, SLOT(selected()));
  connect(tracker_, SIGNAL(appended(const QPointF&)),
    (MarkableCurve*) curves_[GAIN_RAD].first,
    SLOT(appended(const QPointF&)));
  connect(tracker_, SIGNAL(moved(const QPointF&)),
    (MarkableCurve*) curves_[GAIN_RAD].first,
    SLOT(moved(const QPointF&)));

  symbol = new QwtSymbol();
  symbol->setStyle(functor_traits<sapecng::loss>::symbol);
  symbol->setSize(functor_traits<sapecng::loss>::ssize);
  curves_[LOSS].first = new MarkableCurve;
  curves_[LOSS].first->setPen(functor_traits<sapecng::loss>::pen);
  curves_[LOSS].first->setStyle(functor_traits<sapecng::loss>::style);
  curves_[LOSS].first->setSymbol(symbol);
  curves_[LOSS].first->setCurveAttribute(
    functor_traits<sapecng::loss>::attribute);
  curves_[LOSS].second = false;

  connect(tracker_, SIGNAL(selected(const QPointF&)),
    (MarkableCurve*) curves_[LOSS].first, SLOT(selected()));
  connect(tracker_, SIGNAL(appended(const QPointF&)),
    (MarkableCurve*) curves_[LOSS].first, SLOT(appended(const QPointF&)));
  connect(tracker_, SIGNAL(moved(const QPointF&)),
    (MarkableCurve*) curves_[LOSS].first, SLOT(moved(const QPointF&)));

  symbol = new QwtSymbol();
  symbol->setStyle(functor_traits<sapecng::loss>::symbol);
  symbol->setSize(functor_traits<sapecng::loss>::ssize);
  curves_[LOSS_RAD].first = new MarkableCurve;
  curves_[LOSS_RAD].first->setPen(functor_traits<sapecng::loss>::pen);
  curves_[LOSS_RAD].first->setStyle(functor_traits<sapecng::loss>::style);
  curves_[LOSS_RAD].first->setSymbol(symbol);
  curves_[LOSS_RAD].first->setCurveAttribute(
    functor_traits<sapecng::loss>::attribute);
  curves_[LOSS_RAD].second = false;

  connect(tracker_, SIGNAL(selected(const QPointF&)),
    (MarkableCurve*) curves_[LOSS_RAD].first, SLOT(selected()));
  connect(tracker_, SIGNAL(appended(const QPointF&)),
    (MarkableCurve*) curves_[LOSS_RAD].first,
    SLOT(appended(const QPointF&)));
  connect(tracker_, SIGNAL(moved(const QPointF&)),
    (MarkableCurve*) curves_[LOSS_RAD].first,
    SLOT(moved(const QPointF&)));

  symbol = new QwtSymbol();
  symbol->setStyle(functor_traits<sapecng::zeros>::symbol);
  symbol->setSize(functor_traits<sapecng::zeros>::ssize);
  curves_[ZEROS].first = new MarkedCurve;
  curves_[ZEROS].first->setPen(functor_traits<sapecng::zeros>::pen);
  curves_[ZEROS].first->setStyle(functor_traits<sapecng::zeros>::style);
  curves_[ZEROS].first->setSymbol(symbol);
  curves_[ZEROS].first->setCurveAttribute(
    functor_traits<sapecng::zeros>::attribute);
  curves_[ZEROS].second = false;

  symbol = new QwtSymbol();
  symbol->setStyle(functor_traits<sapecng::poles>::symbol);
  symbol->setSize(functor_traits<sapecng::poles>::ssize);
  curves_[POLES].first = new MarkedCurve;
  curves_[POLES].first->setPen(functor_traits<sapecng::poles>::pen);
  curves_[POLES].first->setStyle(functor_traits<sapecng::poles>::style);
  curves_[POLES].first->setSymbol(symbol);
  curves_[POLES].first->setCurveAttribute(
    functor_traits<sapecng::poles>::attribute);
  curves_[POLES].second = false;

  for(int i = 0; i < NOOP; ++i) {
    curves_[i].first->attach(plot_);
    curves_[i].first->setVisible(false);
  }

  lastId_ = NOOP;
}


void WorkPlane::setupCurve(
    std::pair< std::vector<double>, std::vector<double> > data,
    WorkPlane::F f
  )
{
  if(!curves_[f].second) {
    curves_[f].second = true;

	QVector<QPointF> seriesData;

	size_t size = std::min(data.first.size(), data.second.size());
	seriesData.resize(size);
	for (size_t i = 0; i < size; ++i) {
		seriesData[i] = QPointF(data.first[i], data.second[i]);
	}

	curves_[f].first->setData(new QwtPointSeriesData(seriesData));
  }
}


}
