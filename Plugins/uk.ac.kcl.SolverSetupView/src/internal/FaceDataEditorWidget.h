#include "ui_FaceDataEditorWidget.h"

#include <QSortFilterProxyModel>
#include <FaceListTableModel.h>

#include <SolidDataFacePickingObserver.h>
#include <mitkDataNode.h>

namespace crimson
{
/*! \brief   A widget for editing face data (boundary conditions and materials). */
class FaceDataEditorWidget : public QWidget
{
    Q_OBJECT
public:
    FaceDataEditorWidget(QWidget* parent = nullptr);
    ~FaceDataEditorWidget();

    void setCurrentNode(mitk::DataNode* node);
    void setCurrentSolidNode(mitk::DataNode* node);
    void setCurrentVesselTreeNode(mitk::DataNode* node);

    /*!
     * \brief   Ensure that the face data is applied to all the walls present in the model. This also
     *  works if the model has been changed (e.g. a new vessel has been added)
     */
    static void applyToAllWalls(mitk::DataNode* solidNode, mitk::DataNode* faceDataNode);

private slots:
    void _syncFaceSelectionToObserver();
    void _setApplyToAllWallsFlag(bool apply);
    void _setFaceSelectionMode(bool enabled);
    void _selectFacesFromList();

private:
    void _fillFaceTable();
    void _updateUI();

    Ui::FaceDataEditorWidget _UI;

    QWidget* _currentCustomEditor = nullptr;

	SolidDataFacePickingObserver _facePickingObserver;
    us::ServiceRegistration<mitk::InteractionEventObserver> _facePickingObserverServiceRegistration;
    us::ServiceTracker<mitk::InteractionEventObserver> _pickingEventObserverServiceTracker;
    bool _allowAnyFaceSelection = false;

    FaceListTableModel _faceListModel;
    QSortFilterProxyModel _faceListProxyModel;

    mitk::DataNode* _currentNode = nullptr;
    mitk::DataNode* _currentSolidNode = nullptr;
    mitk::DataNode* _vesselTreeNode = nullptr;
};
};