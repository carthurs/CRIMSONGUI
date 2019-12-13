#include "SelectFacesFromListDialog.h"

#include <SolidDataFacePickingObserver.h>

SelectFacesFromListDialog::SelectFacesFromListDialog(mitk::DataNode* vesselForestNode,
	crimson::SolidDataFacePickingObserver* observer, QWidget* parent)
    : QDialog(parent)
{
    _model.setVesselTreeNode(vesselForestNode);
    _model.setFacePickingObserver(observer);
    _proxyModel.setSourceModel(&_model);
    _ui.setupUi(this);
    _ui.facesTableView->setModel(&_proxyModel);

    _originalSelection = observer->getFaceSelectedStates();

    connect(_ui.chooseAllButton, &QAbstractButton::clicked, this, &SelectFacesFromListDialog::chooseAll);
    connect(_ui.chooseNoneButton, &QAbstractButton::clicked, this, &SelectFacesFromListDialog::chooseNone);
}

void SelectFacesFromListDialog::reject()
{
    _model.getFacePickingObserver()->setFaceSelectedStates(_originalSelection);

    QDialog::reject();
}

void SelectFacesFromListDialog::chooseAll()
{
    for (int row = 0; row < _model.rowCount(); ++row) {
        _model.setData(_model.index(row, 2), Qt::Checked, Qt::CheckStateRole);
    }
}

void SelectFacesFromListDialog::chooseNone()
{
    for (int row = 0; row < _model.rowCount(); ++row) {
        _model.setData(_model.index(row, 2), Qt::Unchecked, Qt::CheckStateRole);
    }
}
