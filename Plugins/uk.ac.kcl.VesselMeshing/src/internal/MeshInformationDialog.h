#pragma once

#include <QDialog>
#include "ui_MeshInformationDialog.h"

#include <mitkDataNode.h>

#include <qwt_plot_histogram.h>

#include <memory>

/*! \brief   Dialog for showing the mesh information (number of entities and aspect ratio histogram). */
class MeshInformationDialog : public QDialog {
    Q_OBJECT
public:
    MeshInformationDialog(QWidget* parent = nullptr);

    void setMeshNode(mitk::DataNode* meshNode);
    
private:
    Ui::MeshInformationDialog _ui;
    std::unique_ptr<QwtPlotHistogram> _plotHistogram;
};