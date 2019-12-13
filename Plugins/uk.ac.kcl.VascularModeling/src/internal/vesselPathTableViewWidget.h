#pragma once
#include <QTableView>

/*!
* \brief A QTableView for showing VesselPathItemModel with support for removing selected points.
*/
class VesselPathTableViewWidget : public QTableView {
    Q_OBJECT
public:
    VesselPathTableViewWidget(QWidget* parent = nullptr);
    
    void keyPressEvent(QKeyEvent *event) override;
    void setModel(QAbstractItemModel* model) override;

public slots:
    void removeSelectedPoints();
};