#include "TimeInterpolationDialog.h"
#include <IPCMRIKernel.h>

#include <ctkDoubleSpinBox.h>

#include <qwt_symbol.h>
#include <qwt_legend.h>

namespace crimson
{
	double trapz(std::vector<double> x, std::vector<double> y)
	{
		double area = 0;
		for (int i = 0; i < x.size(); i++)	
		{
			auto test = (y[i] + y[i + 1]);
			MITK_DEBUG << "Trapz sum " << test;

			int index1 = i % x.size();
			int index2 = (i + 1) % x.size();
			area += (x[1] - x[0])*(y[index1] + y[index2])*0.5;
		}
		
		return area;
	}

	TimeInterpolationDialog::TimeInterpolationDialog(PCMRIData::Pointer pcmriData, int* nControlPoints, QWidget* parent /*= nullptr*/)
		: QDialog(parent)
		, _pcmriData(pcmriData)
		, _nControlPoints(nControlPoints)
	{
		_ui.setupUi(this);

		_ui.controlPointsSpinBox->setMaximum(40);
		_ui.controlPointsSpinBox->setMinimum(2);
		_ui.controlPointsSpinBox->setSingleStep(1);
		if (*_nControlPoints > 1)
			_ui.controlPointsSpinBox->setValue(*_nControlPoints);
		else
			_ui.controlPointsSpinBox->setValue(15);

		connect(_ui.controlPointsSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &TimeInterpolationDialog::plot);

		//_flowWaveform = IPCMRIKernel::calculateFlowWaveform(_pcmriData->getMappedPCMRIvalues(), _pcmriData->getMesh(), _pcmriData->getFace());

		plot(); //do the first plot for current number of control points selected

	}

	void TimeInterpolationDialog::accept()
	{
		// Read back the values
		*_nControlPoints = _ui.controlPointsSpinBox->value();

		QDialog::accept();
	}

	void TimeInterpolationDialog::plot()
	{
		if (_plotWaveform) {
			_plotWaveform->detach();
			_plotWaveform.release();
		}

		*_nControlPoints = _ui.controlPointsSpinBox->value();

		TimeInterpolationParameters parameters = _pcmriData->getTimeInterpolationParameters();

		/// Algorithmic parameters
		double cycleDuration = 60 / (double)parameters._cardiacFrequency;
		unsigned int bsd = 3; // b-spline degree, by default is cubic
		const unsigned ndmis = 1;
		double frameRate = (double)parameters._nOriginal / cycleDuration; //taken from Alberto's code

		//original time resolution of the PCMRI image - calculate based on frame rate and trigger delay
		std::vector<double> t(parameters._nOriginal);
		std::vector<double> dataOriginal(parameters._nOriginal);
		std::vector<double> maxVelocity(parameters._nOriginal);

		for (int i = 0; i < parameters._nOriginal; i++)
		{
			t[i] = i / frameRate;
			maxVelocity[i] = -9999;
			for (int j = 0; j < _pcmriData->getMappedPCMRIvalues().shape()[0]; j++)
			{
				dataOriginal[i] += _pcmriData->getMappedPCMRIvalues()[j][i];
				if (_pcmriData->getMappedPCMRIvalues()[j][i]>maxVelocity[i])
					maxVelocity[i] = _pcmriData->getMappedPCMRIvalues()[j][i];
			}
			dataOriginal[i] /= _pcmriData->getMappedPCMRIvalues().shape()[0];
		}

		//calculate interpolated data
		std::vector<double> dataInterpolated;
		dataInterpolated = interpolate(t, dataOriginal, parameters);

		
		//plot original data
		QVector<QPointF>* samplesOriginal = new QVector<QPointF>;
		QwtPointSeriesData* myData = new QwtPointSeriesData;


		for (int i=0; i < parameters._nOriginal; i++)
			samplesOriginal->push_back(QPointF(t[i], dataOriginal[i]));

		myData->setSamples(*samplesOriginal);

		_plotWaveform = std::make_unique<QwtPlotCurve>(QString("Original flow waveform"));
		_plotWaveform->setData(myData);
		_plotWaveform->attach(_ui.waveformPlot);

		QwtSymbol *symbol = new QwtSymbol(QwtSymbol::Ellipse,
			QBrush(Qt::transparent), QPen(Qt::red, 2), QSize(8, 8));
		_plotWaveform->setSymbol(symbol);

		_plotWaveform->setStyle(QwtPlotCurve::NoCurve);

		//plot interpolated data

		QVector<QPointF>* samplesInterpolated = new QVector<QPointF>;
		QwtPointSeriesData* myDataInterpolated = new QwtPointSeriesData;

		std::vector<double> interpolated_t;
		for (double j = 0; j <= cycleDuration; j = j + 0.01){
			interpolated_t.push_back(j);
		}

		for (int i = 0; i < interpolated_t.size(); i++)
			samplesInterpolated->push_back(QPointF(interpolated_t[i], dataInterpolated[i]));

		myDataInterpolated->setSamples(*samplesInterpolated);

		_plotWaveformInterpolated = std::make_unique<QwtPlotCurve>(QString("Interpolated flow waveform"));
		_plotWaveformInterpolated->setData(myDataInterpolated);
		_plotWaveformInterpolated->attach(_ui.waveformPlot);

		QwtSymbol *symbol2 = new QwtSymbol(QwtSymbol::Cross,
			QBrush(Qt::blue), QPen(Qt::blue, 2), QSize(8, 8));
		_plotWaveformInterpolated->setSymbol(symbol2);

		_plotWaveformInterpolated->setStyle(QwtPlotCurve::NoCurve);


		_ui.waveformPlot->setCanvasBackground(Qt::white);
		QwtText titleX("Time[s]");
		titleX.setFont(QFont("Times", 10));
		_ui.waveformPlot->setAxisTitle(QwtPlot::xBottom, titleX);

		QwtText titleY("Mean velocity [mm/s]");
		titleY.setFont(QFont("Times", 10));
		_ui.waveformPlot->setAxisTitle(QwtPlot::yLeft, titleY);

		_ui.waveformPlot->setAxisAutoScale(QwtPlot::xBottom, true);
		_ui.waveformPlot->setAxisAutoScale(QwtPlot::yLeft, true);

		auto flowWaveform = IPCMRIKernel::calculateFlowWaveform(_pcmriData->getMappedPCMRIvalues(), _pcmriData->getMesh(), _pcmriData->getFace());

		double cardiacOutput = trapz(t, flowWaveform) * 60 / pow(10, 6);

		double area = _pcmriData->getMesh()->calculateArea(_pcmriData->getFace());
		MITK_DEBUG << "Face area" << area;

		/*double cardiacOutputInterpolated = trapz(interpolated_t, dataInterpolated) * 60 / pow(10, 6) * _pcmriData->getArea();*/
		double cardiacOutputInterpolated = trapz(t, dataOriginal) * 60 / pow(10, 6) * area;

		MITK_INFO << "cardiacOutput total" << cardiacOutput;
		MITK_INFO << "Approximated cardiac output " << cardiacOutputInterpolated;

		_ui.cardiacOutputValue->setPlaceholderText("Total cardiac output: " + QString::number(cardiacOutput,'g', 5) + " L/min");
		_ui.cardiacOutputValue->setReadOnly(true);

		_ui.waveformPlot->updateAxes();
		_ui.waveformPlot->insertLegend(new QwtLegend(), QwtPlot::BottomLegend);

		_ui.waveformPlot->replot();
	}

	std::vector<double> TimeInterpolationDialog::interpolate(std::vector<double> t, std::vector<double> dataOriginal, TimeInterpolationParameters parameters)
	{
		bool debug = false;
		bool parallel_on = false;
		int threads_to_use = 2;

		/// Algorithmic parameters
		double cycleDuration = 60 / (double)parameters._cardiacFrequency;
		unsigned int bsd = 3; // b-spline degree, by default is cubic
		const unsigned ndmis = 1;

		//original time resolution of the PCMRI image
		std::vector< IPCMRIKernel::BsplineGrid1DType::PointType > coordinates; // in this case, 1D
		//timepoints after interpolation
		std::vector< IPCMRIKernel::BsplineGrid1DType::PointType > interpolated_t;

		for (int i = 0; i < parameters._nOriginal; i++){
			coordinates.push_back(t[i]);			
		}

		for (double j = 0; j <= cycleDuration; j = j + 0.01){
			interpolated_t.push_back(j);
		}
		
		std::vector<double> dataInterpolated(interpolated_t.size());

		std::vector<IPCMRIKernel::BsplineGrid1DType::CoefficientType> values; // in this case, velocity
		for (int i = 0; i < parameters._nOriginal; i++){
			values.push_back(dataOriginal[i]);
		}

		double bounds[2 * ndmis]; // bounds of the domain, time in this case
		bounds[0] = t[0];
		bounds[1] = t[parameters._nOriginal - 1];
		IPCMRIKernel::BsplineGrid1DType::PointType grid_spacing;
		double bspline_spacing_1D = (bounds[1] - bounds[0]) / (*_nControlPoints - 1);

		grid_spacing[0] = bspline_spacing_1D; // for example!

		IPCMRIKernel::BsplineGrid1DType::Pointer control_points(new IPCMRIKernel::BsplineGrid1DType(bounds, bsd, grid_spacing, 0)); /// the control point grid does not need border. This grid covers the whole ROI and there is no control_points division.
		IPCMRIKernel::BsplineGrid1DType::IndexType cyclicDimensions;
		cyclicDimensions[0] = 1; // first dimension is cyclic
		control_points->SetCyclicDimensions(cyclicDimensions);
		control_points->SetCyclicBounds(0, t[0], cycleDuration);
		control_points->SetDebug(debug);
		if (debug) std::cout << "\t\tUsing " << threads_to_use << " threads." << std::endl;
		control_points->SetParallel(parallel_on, threads_to_use);
		control_points->UpdateCyclicBehaviour();

		double lambda = 0; // no regularization

		fit_cyclicBspline1D<IPCMRIKernel::BsplineGrid1DType>(coordinates, values, lambda, control_points);

		/// interpolate ---------------------------------------

		std::vector<IPCMRIKernel::BsplineGrid1DType::CoefficientType>  interpolated_values;
		interpolated_values = control_points->evaluate(interpolated_t);

		for (int i = 0; i < interpolated_values.size(); i++)
			dataInterpolated[i]=interpolated_values[i][0];

		return dataInterpolated;

	}
}