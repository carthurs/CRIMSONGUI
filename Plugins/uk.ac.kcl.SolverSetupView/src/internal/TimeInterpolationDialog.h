#pragma once

#include <QDialog>
#include "ui_TimeInterpolationDialog.h"
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_series_data.h>

#include <PCMRIData.h>

#include <memory>

class OverrideStatusButtonHandler;

namespace crimson
{

	/*! \brief   Dialog for setting the time interpolation options. */
	class TimeInterpolationDialog : public QDialog
	{
		Q_OBJECT
	public:

		/*!
		 * \brief   Constructor.
		 *
		 * \param   params                  List of parameters to edit.
		 * \param   editingGlobal           true to show that global meshing parameters are edited.
		 * \param   defaultLocalParameters  The default values for parameters.
		 * \param   parent                  (Optional) If non-null, the parent.
		 */
		TimeInterpolationDialog(crimson::PCMRIData::Pointer pcmriData, int* nControlPoints, QWidget* parent = nullptr);

		void accept() override;

		private slots:


	private:
		Ui::TimeInterpolationDialog _ui;
		crimson::PCMRIData::Pointer _pcmriData;
		int* _nControlPoints;
		std::unique_ptr<QwtPlotCurve> _plotWaveform;
		std::unique_ptr<QwtPlotCurve> _plotWaveformInterpolated;
		void plot();
		std::vector<double> interpolate(std::vector<double> t, std::vector<double> dataOriginal, TimeInterpolationParameters parameters);

	};
}