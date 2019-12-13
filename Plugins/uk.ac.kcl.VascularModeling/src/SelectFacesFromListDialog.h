#pragma once

#include <QDialog>
#include <QSortFilterProxyModel>
#include "ui_SelectFacesFromListDialog.h"
#include "SelectedFacesTableModel.h"

#include "uk_ac_kcl_VascularModeling_Export.h"

namespace crimson{
class VesselForestData;
class SolidDataFacePickingObserver;
}

namespace mitk{
class DataNode;
}

/*! \brief   Dialog for selecting faces from list. */
class VASCULARMODELING_EXPORT SelectFacesFromListDialog : public QDialog {
    Q_OBJECT
public:
	SelectFacesFromListDialog(mitk::DataNode* vesselForestNode, crimson::SolidDataFacePickingObserver* observer, QWidget* parent = nullptr);
    void reject() override;

private slots:
    void chooseAll();
    void chooseNone();

private:
    Ui::SelectFacesFromListDialog _ui;
    SelectedFacesTableModel _model;
    QSortFilterProxyModel _proxyModel;
    std::vector<bool> _originalSelection;
};