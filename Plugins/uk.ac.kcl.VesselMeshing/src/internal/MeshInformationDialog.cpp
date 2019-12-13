#include "MeshInformationDialog.h"
#include <MeshData.h>

MeshInformationDialog::MeshInformationDialog(QWidget* parent)
    : QDialog(parent)
{
    _ui.setupUi(this);
}

void MeshInformationDialog::setMeshNode(mitk::DataNode* meshNode)
{
    crimson::MeshData* data = static_cast<crimson::MeshData*>(meshNode->GetData());

    setWindowTitle(QString::fromStdString("Mesh information for " + meshNode->GetName()));

    _ui.basicPropertiesTable->setItem(0, 0, new QTableWidgetItem(QString::number(data->getNNodes())));
    _ui.basicPropertiesTable->setItem(1, 0, new QTableWidgetItem(QString::number(data->getNEdges())));
    _ui.basicPropertiesTable->setItem(2, 0, new QTableWidgetItem(QString::number(data->getNFaces())));
    _ui.basicPropertiesTable->setItem(3, 0, new QTableWidgetItem(QString::number(data->getNElements())));

    _ui.basicPropertiesTable->setFixedHeight(_ui.basicPropertiesTable->verticalHeader()->length() + _ui.basicPropertiesTable->contentsMargins().top() + _ui.basicPropertiesTable->contentsMargins().bottom());
    _ui.basicPropertiesTable->resizeColumnsToContents();

    std::vector<double> aspectRatios = data->getElementAspectRatios();

    if (_plotHistogram) {
        _plotHistogram->detach();
        _plotHistogram.release();
    }

    if (aspectRatios.size() > 0) {
        // Compute the aspect ratio histogram
        double max = *std::max_element(aspectRatios.begin(), aspectRatios.end());

        double bucketSize = max / 100;
        QVector<QwtIntervalSample> histogram;
        histogram.resize((size_t)ceil(max / bucketSize));

        std::for_each(aspectRatios.begin(), aspectRatios.end(), [&histogram, bucketSize](double v) { histogram[(int)floor(v / bucketSize)].value++; });

        int index = 0;
        std::for_each(histogram.begin(), histogram.end(), [&index, bucketSize](QwtIntervalSample& sample) { sample.interval.setInterval(index * bucketSize, (index + 1) * bucketSize); ++index; });

        _plotHistogram = std::make_unique<QwtPlotHistogram>(QString("Aspect ratio histogram"));
        _plotHistogram->setSamples(histogram);
        _plotHistogram->attach(_ui.aspectRatioHistogram);

        QLinearGradient brushGradient(0, 0, 1, 0);
        brushGradient.setColorAt(0, Qt::blue);
        brushGradient.setColorAt(1, Qt::darkBlue);
        brushGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
        _plotHistogram->setBrush(QBrush(brushGradient));
        _ui.aspectRatioHistogram->updateAxes();
    }
    _ui.aspectRatioHistogram->replot();
}

