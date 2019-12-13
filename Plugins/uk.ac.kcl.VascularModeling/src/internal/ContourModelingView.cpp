// Qt
#include <QMessageBox>
#include <QKeyEvent>
#include <QShortcut>
#include <QtCore>
#include <QIcon>

// Main include
#include "ContourModelingView.h"

// Plugin includes
#include "VesselDrivenResliceView.h"
#include "ThumbnailGenerator.h"
#include "ConstMemberCommand.h"
#include "ContourTypeConversion.h"
#include "VascularModelingUtils.h"
#include "LoftAction.h"
#include "VascularModelingNodeTypes.h"
#include "ShowHideContoursAction.h"
#include <utils/TaskStateObserver.h>

// Module includes
#include <VesselPathAbstractData.h>
#include <HierarchyManager.h>
#include <AsyncTaskManager.h>
#include <AsyncTask.h>

// Micro-services
#include <usModule.h>
#include <usModuleResource.h>
#include <usModuleRegistry.h>
#include <usModuleResourceStream.h>

// MITK
#include <mitkVector.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateData.h>

// Planar figures
#include <mitkPlanarCircle.h>
#include <mitkPlanarEllipse.h>
#include <mitkPlanarPolygon.h>
#include <mitkPlanarSubdivisionPolygon.h>
#include <mitkPlanarFigureInteractor.h>

// Segmentation utils
#include <mitkToolManager.h>
#include <mitkToolManagerProvider.h>
#include <QmitkToolSelectionBox.h>
#include <mitkExtractSliceFilter.h>
#include <mitkVtkImageOverwrite.h>
#include <mitkImageToContourFilter.h>
#include <mitkContourModel.h>
#include <mitkContourModelUtils.h>
#include <mitkSegTool2D.h>

#include <mitkSurface.h>
#include <mitkImageSliceSelector.h>
#include <mitkShapeBasedInterpolationAlgorithm.h>

// MITK Application
#include <mitkApplicationCursor.h>
#include <mitkStatusBar.h>

// VTK
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkWindowedSincPolyDataFilter.h>

// Solid modelling kernel
#include <ISolidModelKernel.h>
#include <DataManagement/OCCBRepData.h>

Q_DECLARE_METATYPE(const mitk::DataNode*)

/*!
 * \brief   The list widget item for thumbnail list. Stores the corresponding data node as well
 *  as supports sorting by the parameter value along the vessel path.
 */
class ThumbnalListWidgetItem : public QListWidgetItem
{
public:
    static const int NodeRole = Qt::UserRole + 2;

    explicit ThumbnalListWidgetItem(const QIcon& icon, const mitk::DataNode* node)
        : QListWidgetItem(icon, "")
    {
        setData(NodeRole, QVariant::fromValue(node));
    }

    bool operator<(const QListWidgetItem& r) const override
    {
        float parameterValueL = 0;
        node()->GetFloatProperty("lofting.parameterValue", parameterValueL);

        float parameterValueR = 0;
        static_cast<const ThumbnalListWidgetItem*>(&r)->node()->GetFloatProperty("lofting.parameterValue", parameterValueR);

        return parameterValueL < parameterValueR;
    }

    const mitk::DataNode* node() const { return data(NodeRole).value<const mitk::DataNode*>(); }
};

//////////////////////////////////////////////////////////////////////////
/*!
 * \brief The event filter that reacts to the 'delete' button press in the thumbnail widget.
 */
ItemDeleterEventFilter::ItemDeleterEventFilter(QObject* parent)
    : QObject(parent)
{
}

bool ItemDeleterEventFilter::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Delete) {
            emit deleteRequested();
            return true;
        }
    }

    // standard event processing
    return QObject::eventFilter(obj, event);
}
//////////////////////////////////////////////////////////////////////////

const std::string ContourModelingView::VIEW_ID = "org.mitk.views.ContourModelingView";

ContourModelingView::ContourModelingView()
    : NodeDependentView(crimson::VascularModelingNodeTypes::VesselPath(), true, QString("Vessel path"))
    , _thumbnailGenerator(new crimson::ThumbnailGenerator(GetDataStorage()))
{
    // Leave only planar figures and images in the thumbnails
    _shouldBeInvisibleInThumbnailView =
        mitk::NodePredicateNot::New(mitk::NodePredicateOr::New(mitk::TNodePredicateDataType<mitk::Image>::New(),
                                                               mitk::TNodePredicateDataType<mitk::PlanarFigure>::New()))
            .GetPointer();
}

ContourModelingView::~ContourModelingView()
{
    cancelInteraction();
    _vesselDrivenResliceView = nullptr;
    _setSegmentationNodes(nullptr, nullptr);
    _setCurrentContourNode(nullptr);
    if (currentNode()) {
        removeLoftPreview(currentNode());
        mitk::DataStorage::SetOfObjects::ConstPointer nodes =
            crimson::VascularModelingUtils::getVesselContourNodes(currentNode());
        for (const mitk::DataNode::Pointer& node : *nodes) {
            node->SetSelected(false);
        }
        _removeContoursVisibilityPropertyObserver(currentNode());
    }
    _disconnectAllContourObservers();

    if (_partListener) {
        _partListener->unregisterListener();
    }
}

void ContourModelingView::SetFocus()
{
    //     m_Controls.buttonPerformImageProcessing->setFocus();
    //     m_Controls.buttonPerformImageProcessing->setEnabled(true);
}

void ContourModelingView::CreateQtPartControl(QWidget* parent)
{
    _viewWidget = parent;

    // create GUI widgets from the Qt Designer's .ui file
    _UI.setupUi(parent);

    // Manual tools for contour creation
    connect(_UI.addCircleButton, &QAbstractButton::toggled, this, &ContourModelingView::addCircle);
    connect(_UI.addPolygonButton, &QAbstractButton::toggled, this, &ContourModelingView::addPolygon);
    connect(_UI.addEllipseButton, &QAbstractButton::toggled, this, &ContourModelingView::addEllipse);

    _contourTypeToButtonMap[mitk::PlanarCircle::GetStaticNameOfClass()] = _UI.addCircleButton;
    _contourTypeToButtonMap[mitk::PlanarEllipse::GetStaticNameOfClass()] = _UI.addEllipseButton;
    _contourTypeToButtonMap[mitk::PlanarSubdivisionPolygon::GetStaticNameOfClass()] = _UI.addPolygonButton;

    // Contour duplication and interpolation
    connect(_UI.duplicateButton, &QAbstractButton::clicked, this, &ContourModelingView::duplicateContour);
    connect(_UI.interpolateButton, &QAbstractButton::clicked, this, &ContourModelingView::interpolateContour);

    // Lofting
    _loftTaskStateObserver = new crimson::TaskStateObserver(_UI.loftButton, _UI.cancelLoftButton, this);
    connect(_loftTaskStateObserver, &crimson::TaskStateObserver::runTaskRequested, this, &ContourModelingView::createLoft);
    connect(_loftTaskStateObserver, &crimson::TaskStateObserver::taskFinished, this, &ContourModelingView::loftingFinished);

    _loftPreviewTaskStateObserver = new crimson::TaskStateObserver(_UI.previewButton, nullptr, this);
    connect(_loftPreviewTaskStateObserver, &crimson::TaskStateObserver::runTaskRequested, this,
            &ContourModelingView::previewLoft);
    connect(_loftPreviewTaskStateObserver, &crimson::TaskStateObserver::taskFinished, this,
            &ContourModelingView::loftPreviewFinished);

    connect(_UI.loftingAlgorithmComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(setLoftingAlgorithm(int)));
    connect(_UI.contourDistanceSpingBox, SIGNAL(valueChanged(double)), this, SLOT(setInterContourDistance(double)));
    connect(_UI.seamEdgeRotationSpinBox, SIGNAL(valueChanged(int)), this, SLOT(setSeamEdgeRotation(int)));

    QShortcut* loftShortcut = new QShortcut(QKeySequence("Alt+L"), parent);
    loftShortcut->setContext(Qt::ApplicationShortcut);
    connect(loftShortcut, &QShortcut::activated, _UI.loftButton, &QAbstractButton::click);

    QShortcut* previewShortcut = new QShortcut(QKeySequence("Alt+P"), parent);
    previewShortcut->setContext(Qt::ApplicationShortcut);
    connect(previewShortcut, &QShortcut::activated, _UI.previewButton, &QAbstractButton::click);

    // Segmentation tools for contour creation
    connect(_UI.createSegmentedButton, SIGNAL(clicked()), this,
            SLOT(createSegmented())); // Default value -> old connection style
    connect(_UI.smoothnessSlider, &QAbstractSlider::valueChanged, this, &ContourModelingView::updateContourSmoothnessFromUI);

    mitk::ToolManager* toolManager = mitk::ToolManagerProvider::GetInstance()->GetToolManager();
    assert(toolManager);

    toolManager->SetDataStorage(*(this->GetDataStorage()));
    toolManager->InitializeTools();
    dynamic_cast<mitk::SegTool2D*>(toolManager->GetToolById(0))->SetEnable3DInterpolation(false);

    _UI.segToolsSelectionBox->SetGenerateAccelerators(true);
    _UI.segToolsSelectionBox->SetToolGUIArea(_UI.segToolsGUIArea);
    _UI.segToolsSelectionBox->SetDisplayedToolGroups(
        "Add Subtract Correction Paint Wipe 'Region Growing' Fill Erase 'Live Wire'"); // '2D Fast Marching'");
    _UI.segToolsSelectionBox->SetLayoutColumns(3);
    _UI.segToolsSelectionBox->SetEnabledMode(QmitkToolSelectionBox::EnabledWithReferenceData);
    connect(_UI.segToolsSelectionBox, &QmitkToolSelectionBox::ToolSelected, this,
            &ContourModelingView::setToolInformation_Segmentation);

    // Thumbnail related actions
    connect(_UI.contourThumbnailListWidget, &QListWidget::doubleClicked, this, &ContourModelingView::navigateToContourByIndex);

    auto deleterEventFilter = new ItemDeleterEventFilter(_UI.contourThumbnailListWidget);
    _UI.contourThumbnailListWidget->installEventFilter(deleterEventFilter);
    connect(deleterEventFilter, &ItemDeleterEventFilter::deleteRequested, this, &ContourModelingView::deleteSelectedContours);
    connect(_UI.deleteContoursButton, &QAbstractButton::clicked, this, &ContourModelingView::deleteSelectedContours);
    connect(_UI.contourThumbnailListWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            &ContourModelingView::syncContourSelection);

    connect(_thumbnailGenerator.get(), &crimson::ThumbnailGenerator::thumbnailGenerated, this,
            &ContourModelingView::setNodeThumbnail);

    // Contour visibility control
    connect(_UI.contourVisibilityButton, &QAbstractButton::toggled, this, &ContourModelingView::setAllContoursVisible);

    // Use inflow/outflow as wall
    connect(_UI.useInflowAsWall, &QAbstractButton::toggled, this, &ContourModelingView::updateInflowOutflowAsWallInformation);
    connect(_UI.useOutflowAsWall, &QAbstractButton::toggled, this, &ContourModelingView::updateInflowOutflowAsWallInformation);

    // Contour information control
    _UI.contourInfoGroupBox->setCollapsed(true);
    _UI.contourInfoGroupBox->setCollapsedHeight(0);
    _contourInfoUpdateTimer.setSingleShot(true);
    connect(&_contourInfoUpdateTimer, &QTimer::timeout, this, &ContourModelingView::updateCurrentContourInfo);

    _contourGeometryUpdateTimer.setSingleShot(true);
    connect(&_contourGeometryUpdateTimer, &QTimer::timeout, this,
            &ContourModelingView::reinitAllContourGeometriesOnVesselPathChange);

    // Loft preview generation control
    _loftPreviewGenerateTimer.setSingleShot(true);
    connect(&_loftPreviewGenerateTimer, &QTimer::timeout, this, &ContourModelingView::previewLoft);
    _UI.previewErrorLabel->setVisible(false);
    connect(_UI.previewVisibilityButton, &QAbstractButton::toggled, this, [this](bool on) {
        if (!currentNode()) {
            return;
        }
        mitk::DataNode::Pointer previewNode = crimson::HierarchyManager::getInstance()->getFirstDescendant(
            currentNode(), crimson::VascularModelingNodeTypes::LoftPreview());
        if (previewNode) {
            previewNode->SetVisibility(on);
            mitk::RenderingManager::GetInstance()->RequestUpdateAll();
        }
    });

    // Display of current vessel path
    static_cast<QBoxLayout*>(_UI.vesselNameFrame->layout())->insertWidget(0, createSelectedNodeWidget(_UI.vesselNameFrame));

    // Computation of vessel path through contour centers
    connect(_UI.computePathThroughCentersButton, &QAbstractButton::clicked, this,
            &ContourModelingView::computePathThroughContourCenters);

    // Set the objects' visibility for the thumbnail renderer
    mitk::DataStorage::SetOfObjects::ConstPointer invisibleNodes =
        GetDataStorage()->GetSubset(_shouldBeInvisibleInThumbnailView);
    for (const mitk::DataNode::Pointer& node : *invisibleNodes) {
        node->SetVisibility(false, _thumbnailGenerator->getThumbnailRenderer());
    }

    // Setup the listener for the reslice view
    _partListener.reset(new crimson::VesselDrivenResliceViewListener(
        [this](VesselDrivenResliceView* view) { this->setVesselDrivenResliceView(view); }, GetSite().GetPointer()));
    _partListener->registerListener();

    // Check if a vessel path is selected
    initializeCurrentNode();

    // Update the parameters in case the vessel was modified when the view was closed
    if (currentNode()) {
        _updatePlanarFigureParameters(true);
    }

    // Connect the data observers for all contours
	mitk::DataStorage::SetOfObjects::ConstPointer allContourNodes = GetDataStorage()->GetSubset(
		mitk::NodePredicateAnd::New(crimson::HierarchyManager::getInstance()->getPredicate(crimson::VascularModelingNodeTypes::Contour()),
		mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("mapping.parameterValue")).GetPointer()));
    _connectAllContourObservers(allContourNodes);

    // Initialize the UI
    _updateUI();
}

void ContourModelingView::_createCancelInteractionShortcut()
{
    // Create a QShortcut for Esc key press
    assert(_cancelInteractionShortcut == nullptr);
    _cancelInteractionShortcut =
        new QShortcut(QKeySequence(Qt::Key_Escape), _viewWidget, nullptr, nullptr, Qt::ApplicationShortcut);
    connect(_cancelInteractionShortcut, SIGNAL(activated()), this, SLOT(cancelInteraction()));
}

void ContourModelingView::_removeCancelInteractionShortcut()
{
    delete _cancelInteractionShortcut;
    _cancelInteractionShortcut = nullptr;
}

void ContourModelingView::setVesselDrivenResliceView(VesselDrivenResliceView* view)
{
    if (_vesselDrivenResliceView == view) {
        return;
    }

    if (_vesselDrivenResliceView) {
        disconnect(_vesselDrivenResliceView, &VesselDrivenResliceView::sliceChanged, this,
                   &ContourModelingView::currentSliceChanged);
        disconnect(_vesselDrivenResliceView, &VesselDrivenResliceView::geometryChanged, this,
                   &ContourModelingView::reinitAllContourGeometriesByCorner);
    }

    _vesselDrivenResliceView = view;

    if (_vesselDrivenResliceView) {
        // Connect the handlers for VesselDrivenResliceView events
        connect(_vesselDrivenResliceView, &VesselDrivenResliceView::sliceChanged, this,
                &ContourModelingView::currentSliceChanged);
        connect(_vesselDrivenResliceView, &VesselDrivenResliceView::geometryChanged, this,
                &ContourModelingView::reinitAllContourGeometriesByCorner);
    }

    updateCurrentContour();
}

void ContourModelingView::currentSliceChanged()
{
    // The user navigated to a different slice position
    cancelInteraction();
    updateCurrentContour();
}

void ContourModelingView::updateCurrentContour()
{
    mitk::DataNode* newRefImageNode = nullptr;
    mitk::DataNode* newWorkingImageNode = nullptr;
    mitk::DataNode* newContourNode = nullptr;

    if (!_vesselDrivenResliceView || !currentNode()) {
        _setSegmentationNodes(nullptr, nullptr);
        _setCurrentContourNode(nullptr);
        _updateUI();
        return;
    }

    mitk::BaseRenderer* renderer = _vesselDrivenResliceView->getResliceRenderer();

    mitk::DataStorage::SetOfObjects::ConstPointer nodes = crimson::VascularModelingUtils::getVesselContourNodes(currentNode());

    for (const mitk::DataNode::Pointer& node : *nodes) {
        // Find the contour lying in the current reslice plane

        if (node == _nodeBeingDeleted) {
            continue;
        }

        auto planarFigure = static_cast<mitk::PlanarFigure*>(node->GetData());

        const mitk::PlaneGeometry* planarFigurePlaneGeometry =
            dynamic_cast<const mitk::PlaneGeometry*>(planarFigure->GetPlaneGeometry());
        const mitk::PlaneGeometry* rendererPlaneGeometry =
            dynamic_cast<const mitk::PlaneGeometry*>(renderer->GetCurrentWorldPlaneGeometry());

        assert(planarFigurePlaneGeometry && rendererPlaneGeometry);

        double planeThickness = planarFigurePlaneGeometry->GetExtentInMM(2);
        if (!planarFigurePlaneGeometry->IsParallel(rendererPlaneGeometry) ||
            !(planarFigurePlaneGeometry->DistanceFromPlane(rendererPlaneGeometry) < planeThickness / 3.0)) {
            continue;
        }

        newContourNode = node;

        // Check if this is a segmented contour
        std::pair<mitk::DataNode*, mitk::DataNode*> refAndWorkingSegmentationNodes = _getSegmentationNodes(node);
        if (refAndWorkingSegmentationNodes.first && refAndWorkingSegmentationNodes.second) {
            // Setup the current reference image and working image nodes
            newRefImageNode = refAndWorkingSegmentationNodes.first;
            newWorkingImageNode = refAndWorkingSegmentationNodes.second;

            int smoothness;
            if (newContourNode && newContourNode->GetIntProperty("lofting.smoothness", smoothness)) {
                _UI.smoothnessSlider->blockSignals(true);
                _UI.smoothnessSlider->setValue(smoothness);
                _UI.smoothnessSlider->blockSignals(false);
            }
        }
        break;
    }

    _setCurrentContourNode(newContourNode);
    _setSegmentationNodes(newRefImageNode, newWorkingImageNode);

    _updateUI();
    _updateContourListViewSelection();
}

std::pair<mitk::DataNode*, mitk::DataNode*> ContourModelingView::_getSegmentationNodes(mitk::DataNode* contourNode)
{
    return std::make_pair<mitk::DataNode*, mitk::DataNode*>(
        crimson::HierarchyManager::getInstance()->getFirstDescendant(
            contourNode, crimson::VascularModelingNodeTypes::ContourReferenceImage()),
        crimson::HierarchyManager::getInstance()->getFirstDescendant(
            contourNode, crimson::VascularModelingNodeTypes::ContourSegmentationImage()));
}

void ContourModelingView::cancelInteraction(bool undoingPlacement)
{
    // Check if we are aligned with any of planar figures and update the ui accordingly
    mitk::ToolManager* toolManager = mitk::ToolManagerProvider::GetInstance()->GetToolManager();
    toolManager->ActivateTool(-1);

    _removeCancelInteractionShortcut();

    if (_currentManualButton) {
        resetToolInformation();
    }

    if (_currentContourNode && !undoingPlacement) {
        auto currentPlanarFigure = static_cast<mitk::PlanarFigure*>(_currentContourNode->GetData());
        if (!currentPlanarFigure->IsPlaced()) {
            // Contour has not been placed - remove the contour altogether
            _removeCurrentContour();
        } else if (!currentPlanarFigure->IsFinalized()) {
            // Interaction has started, but has not completed. Attempt to save the data by imitating interactor figure finishing
            static_cast<mitk::PlanarFigureInteractor*>(_currentContourNode->GetDataInteractor().GetPointer())->FinalizeFigure();
        }
    }
}

void ContourModelingView::currentNodeChanged(mitk::DataNode* prevPathNode)
{
    // The current vessel path has changed
    if (prevPathNode) {
        mitk::DataStorage::SetOfObjects::ConstPointer nodes =
            crimson::VascularModelingUtils::getVesselContourNodes(prevPathNode);
        for (const mitk::DataNode::Pointer& node : *nodes) {
            node->SetSelected(false);
        }

        // Update the contour geometries immediately if the request has been issued
        if (this->_contourGeometryUpdateTimer.isActive()) {
            this->_contourGeometryUpdateTimer.stop();
            reinitAllContourGeometriesOnVesselPathChange();
        }

        // Cancel a loft preview task if necessary
        crimson::AsyncTaskManager::getInstance()->cancelTask(
            crimson::VascularModelingUtils::getPreviewLoftingTaskUID(currentNode()));

        removeLoftPreview(prevPathNode);
        _removeContoursVisibilityPropertyObserver(prevPathNode);
    }

    if (currentNode()) {
        // Adapt for changes that may have happened when the view was closed (e.g. undo of an operation)
        mitk::DataStorage::SetOfObjects::ConstPointer nodes =
            crimson::VascularModelingUtils::getVesselContourNodes(currentNode());
        for (const mitk::DataNode::Pointer& node : *nodes) {
            // Re-init contour from segmentations
            mitk::DataNode* segmentationNode = _getSegmentationNodes(node).second;
            if (segmentationNode && segmentationNode->GetData()->GetMTime() > node->GetData()->GetMTime()) {
                _updateContourFromSegmentation(node);
            }

            // Update thumbnails
            mitk::BaseProperty* thumnailProperty = node->GetProperty("lofting.thumbnail");
            if (thumnailProperty == nullptr || thumnailProperty->GetMTime() < node->GetData()->GetMTime()) {
                _requestThumbnail(node.GetPointer());
            }
        }
        updateCurrentContour();

        _addContoursVisibilityPropertyObserver(currentNode());
    }

    _loftTaskStateObserver->setPrimaryObservedUID(crimson::VascularModelingUtils::getLoftingTaskUID(currentNode()));
    _loftPreviewTaskStateObserver->setPrimaryObservedUID(
        crimson::VascularModelingUtils::getPreviewLoftingTaskUID(currentNode()));

    _UI.previewVisibilityButton->setEnabled(false);
    _UI.previewErrorLabel->hide();
    _updateUI();
    _fillContourThumbnailsWidget();
}

void ContourModelingView::_connectContourObservers(mitk::DataNode* node)
{
    auto updateCommand = crimson::ConstMemberCommand<ContourModelingView>::New();
    updateCommand->SetCallbackFunction(this, &ContourModelingView::_contourChanged);
    _contourObserverTags[node].tags[ContourObserverTags::Modified] =
        node->GetData()->AddObserver(itk::ModifiedEvent(), updateCommand);

    auto cancelFigurePlacementCommand = crimson::ConstMemberCommand<ContourModelingView>::New();
    cancelFigurePlacementCommand->SetCallbackFunction(this, &ContourModelingView::_figurePlacementCancelled);
    _contourObserverTags[node].tags[ContourObserverTags::FigurePlacementCancelled] =
        node->GetData()->AddObserver(mitk::CancelPlacementPlanarFigureEvent(), cancelFigurePlacementCommand);

    auto planarFigureFinishedCommand = itk::SimpleMemberCommand<ContourModelingView>::New();
    planarFigureFinishedCommand->SetCallbackFunction(this, &ContourModelingView::cancelInteraction);
    _contourObserverTags[node].tags[ContourObserverTags::FigureFinished] =
        node->GetData()->AddObserver(mitk::FinalizedPlanarFigureEvent(), planarFigureFinishedCommand);
}

void ContourModelingView::_connectSegmentationObservers(mitk::DataNode* segmentationNode)
{
    auto command = crimson::ConstMemberCommand<ContourModelingView>::New();
    command->SetCallbackFunction(this, &ContourModelingView::_updateContourFromSegmentationData);
    _segmentationObserverTags[segmentationNode] = segmentationNode->GetData()->AddObserver(itk::ModifiedEvent(), command);
}

void ContourModelingView::_disconnectContourObservers(mitk::DataNode* node)
{
    if (_contourObserverTags.find(node) != _contourObserverTags.end()) {
        for (unsigned long tag : _contourObserverTags[node].tags) {
            node->GetData()->RemoveObserver(tag);
        }
        _contourObserverTags.erase(node);
    }
}

void ContourModelingView::_disconnectSegmentationObservers(mitk::DataNode* segmentationNode)
{
    if (_segmentationObserverTags.find(segmentationNode) != _segmentationObserverTags.end()) {
        segmentationNode->GetData()->RemoveObserver(_segmentationObserverTags[segmentationNode]);
        _segmentationObserverTags.erase(segmentationNode);
    }
}

void ContourModelingView::_connectAllContourObservers(mitk::DataStorage::SetOfObjects::ConstPointer contourNodes)
{
    for (mitk::DataNode::Pointer node : *contourNodes) {
        _connectContourObservers(node);
        auto segmentationNode = _getSegmentationNodes(node).second;
        if (segmentationNode) {
            _connectSegmentationObservers(segmentationNode);
        }
    }
}

void ContourModelingView::_disconnectAllContourObservers()
{
    for (const std::pair<mitk::DataNode*, ContourObserverTags>& tagPair : _contourObserverTags) {
        for (unsigned long tag : tagPair.second.tags) {
            tagPair.first->GetData()->RemoveObserver(tag);
        }
    }
    _contourObserverTags.clear();

    for (const std::pair<mitk::DataNode*, unsigned long>& tagPair : _segmentationObserverTags) {
        tagPair.first->GetData()->RemoveObserver(tagPair.second);
    }
    _segmentationObserverTags.clear();
}

void ContourModelingView::_fillContourThumbnailsWidget()
{
    _UI.contourThumbnailListWidget->clear();

    if (currentNode()) {
        mitk::DataStorage::SetOfObjects::ConstPointer nodes =
            crimson::VascularModelingUtils::getVesselContourNodes(currentNode());
        for (const mitk::DataNode::Pointer& node : *nodes) {
            _addContoursThumbnailsWidgetItem(node);
        }
        _UI.contourThumbnailListWidget->sortItems();
        _updateContourListViewSelection();
    }
}

void ContourModelingView::currentNodeModified()
{
    if (_ignoreVesselPathModifiedEvents) {
        return;
    }

    // Make the parameter values dirty (will not work if we modify the spline when the lofting view is closed)...
    mitk::DataStorage::SetOfObjects::ConstPointer nodes = crimson::VascularModelingUtils::getVesselContourNodes(currentNode());

    if (_UI.updateContourGeometriesCheckBox->isChecked()) {
        auto vp = static_cast<crimson::VesselPathAbstractData*>(currentNode()->GetData());
        if (_newContourPositions.empty()) {
            for (const mitk::DataNode::Pointer& node : *nodes) {
                _newContourPositions[node] = crimson::VascularModelingUtils::getPlanarFigureGeometryCenter(node);
            }
        }
        for (const mitk::DataNode::Pointer& node : *nodes) {
            auto cpResult = vp->getClosestPoint(_newContourPositions[node]);
            _newContourPositions[node] = cpResult.closestPoint;
            node->SetFloatProperty("lofting.parameterValue", cpResult.t);
        }

        // Make the update lazy - to avoid to many updates during interaction
        _contourGeometryUpdateTimer.start(200);
    } else {
        for (const mitk::DataNode::Pointer& node : *nodes) {
            node->SetFloatProperty("lofting.parameterValue", -1);
        }
    }

    _updateUI();
}

void ContourModelingView::reinitAllContourGeometriesOnVesselPathChange()
{
    _newContourPositions.clear();
    if (_vesselDrivenResliceView) {
        _vesselDrivenResliceView->forceReinitGeometry();
    }
    reinitAllContourGeometries(cgrSetGeometry);
    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void ContourModelingView::_updateUI()
{
    bool contourModelingToolsEnabled =
        currentNode() && _vesselDrivenResliceView &&
        static_cast<crimson::VesselPathAbstractData*>(currentNode()->GetData())->controlPointsCount() > 0;
    _UI.contourEditingGroup->setEnabled(contourModelingToolsEnabled);

    if (contourModelingToolsEnabled) {
        _UI.segToolsGUIArea->setEnabled(_currentSegmentationReferenceImageNode != nullptr);
        _UI.segToolsSelectionBox->setEnabled(_currentSegmentationReferenceImageNode != nullptr);
        _UI.createSegmentedButton->setEnabled(_currentSegmentationReferenceImageNode == nullptr);
        _UI.smoothnessSlider->setEnabled(_currentSegmentationReferenceImageNode != nullptr);
    }

    _loftTaskStateObserver->setEnabled(_UI.contourThumbnailListWidget->count() > 0);
    _loftPreviewTaskStateObserver->setEnabled(_UI.contourThumbnailListWidget->count() > 0);
    _UI.deleteContoursButton->setEnabled(_UI.contourThumbnailListWidget->selectedItems().size() > 0);
    _UI.contourVisibilityButton->setEnabled(_UI.contourThumbnailListWidget->count() > 0);
    _UI.computePathThroughCentersButton->setEnabled(_UI.contourThumbnailListWidget->count() > 1);

    if (currentNode()) {
        // Update UI using the data node properties
        _contoursVisibilityPropertyChanged(nullptr);

        int loftingAlgorithm = crimson::ISolidModelKernel::laAppSurf;
        currentNode()->GetIntProperty("lofting.loftingAlgorithm", loftingAlgorithm);
        _UI.loftingAlgorithmComboBox->blockSignals(true);
        _UI.loftingAlgorithmComboBox->setCurrentIndex(loftingAlgorithm);
        _UI.loftingAlgorithmComboBox->blockSignals(false);

        int seamEdgeRotation = 0;
        currentNode()->GetIntProperty("lofting.seamEdgeRotation", seamEdgeRotation);
        _UI.seamEdgeRotationSpinBox->blockSignals(true);
        _UI.seamEdgeRotationSpinBox->setValue(seamEdgeRotation);
        _UI.seamEdgeRotationSpinBox->blockSignals(false);

        float interContourDistance = 5.0;
        currentNode()->GetFloatProperty("lofting.interContourDistance", interContourDistance);
        _UI.contourDistanceSpingBox->blockSignals(true);
        _UI.contourDistanceSpingBox->setValue(interContourDistance);
        _UI.contourDistanceSpingBox->blockSignals(false);

        disconnect(_UI.useInflowAsWall, &QAbstractButton::toggled, this,
                   &ContourModelingView::updateInflowOutflowAsWallInformation);
        disconnect(_UI.useOutflowAsWall, &QAbstractButton::toggled, this,
                   &ContourModelingView::updateInflowOutflowAsWallInformation);

        bool useInflowAsWall = false;
        currentNode()->GetBoolProperty("lofting.useInflowAsWall", useInflowAsWall);
        _UI.useInflowAsWall->setChecked(useInflowAsWall);

        bool useOutflowAsWall = false;
        currentNode()->GetBoolProperty("lofting.useOutflowAsWall", useOutflowAsWall);
        _UI.useOutflowAsWall->setChecked(useOutflowAsWall);

        connect(_UI.useInflowAsWall, &QAbstractButton::toggled, this,
                &ContourModelingView::updateInflowOutflowAsWallInformation);
        connect(_UI.useOutflowAsWall, &QAbstractButton::toggled, this,
                &ContourModelingView::updateInflowOutflowAsWallInformation);
    }
}

void ContourModelingView::_addContoursVisibilityPropertyObserver(mitk::DataNode* node)
{
    // The lofting.contoursVisible property may be changed from context menu - UI needs to be updated accordingly
    mitk::BaseProperty* property = node->GetPropertyList()->GetProperty("lofting.contoursVisible");
    if (!property) {
        node->SetBoolProperty("lofting.contoursVisible", true);
        property = node->GetPropertyList()->GetProperty("lofting.contoursVisible");
    }
    auto propertyChangedCommand = crimson::ConstMemberCommand<ContourModelingView>::New();
    propertyChangedCommand->SetCallbackFunction(this, &ContourModelingView::_contoursVisibilityPropertyChanged);
    _contoursVisibilityPropertyObserverTag = property->AddObserver(itk::ModifiedEvent(), propertyChangedCommand);
}

void ContourModelingView::_removeContoursVisibilityPropertyObserver(mitk::DataNode* node)
{
    mitk::BaseProperty* property = node->GetPropertyList()->GetProperty("lofting.contoursVisible");
    property->RemoveObserver(_contoursVisibilityPropertyObserverTag);
    _contoursVisibilityPropertyObserverTag = 0;
}

void ContourModelingView::_contoursVisibilityPropertyChanged(const itk::Object*)
{
    bool contoursVisible = true;
    currentNode()->GetBoolProperty("lofting.contoursVisible", contoursVisible);
    _UI.contourVisibilityButton->blockSignals(true);
    _UI.contourVisibilityButton->setChecked(contoursVisible);
    _UI.contourVisibilityButton->blockSignals(false);
}

void ContourModelingView::_updateContourListViewSelection()
{
    // In the thumbnail view, select the contours closest to the reslice plane,
    // or the current contour if one lies in the current reslice plane
    _UI.contourThumbnailListWidget->clearSelection();

    if (currentNode() && _vesselDrivenResliceView) {
        ParameterMapType::iterator closestNodeIters[2];
        _getClosestContourNodes(_vesselDrivenResliceView->getCurrentParameterValue(), closestNodeIters);

        for (const ParameterMapType::iterator& iter : closestNodeIters) {
            if (iter != _parameterMap.end()) {
                ThumbnalListWidgetItem* item = _findThumbnailsWidgetItemByNode(iter->second);
                if (item) {
                    item->setSelected(true);
                    _UI.contourThumbnailListWidget->scrollToItem(item);
                }
            }
        }
    }

    _updateUI();
}

void ContourModelingView::_getClosestContourNodes(float parameterValue, ParameterMapType::iterator outClosestNodeIterators[2])
{
    _updatePlanarFigureParameters();

    outClosestNodeIterators[0] = outClosestNodeIterators[1] = _parameterMap.end();
    if (_currentContourNode) {
        // If there's a contour lying in the current reslice plane, return it as the only result
        outClosestNodeIterators[0] =
            std::find_if(_parameterMap.begin(), _parameterMap.end(),
                         [this](ParameterMapType::value_type kv) { return kv.second == _currentContourNode; });
        assert(outClosestNodeIterators[0] != _parameterMap.end());
        return;
    }

    // Binary search by parameter value
    auto lowerBound = _parameterMap.lower_bound(parameterValue);

    if (lowerBound == _parameterMap.begin()) {
        outClosestNodeIterators[0] = lowerBound;
        return;
    }

    outClosestNodeIterators[1] = lowerBound;
    outClosestNodeIterators[0] = --lowerBound;
}

void ContourModelingView::syncContourSelection(const QItemSelection& selected, const QItemSelection& deselected)
{
    // Sync the selection property of the data node with the thumbnail view
    // This renders the contours selected in the thumbnail view in red allowing easy identification
    for (const QModelIndex& index : selected.indexes()) {
        const mitk::DataNode* node =
            static_cast<ThumbnalListWidgetItem*>(_UI.contourThumbnailListWidget->item(index.row()))->node();
        const_cast<mitk::DataNode*>(node)->SetSelected(true);
    }

    for (const QModelIndex& index : deselected.indexes()) {
        const mitk::DataNode* node =
            static_cast<ThumbnalListWidgetItem*>(_UI.contourThumbnailListWidget->item(index.row()))->node();
        const_cast<mitk::DataNode*>(node)->SetSelected(false);
    }

    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

// Add the contour on the current geometry
//
template <typename ContourType>
void ContourModelingView::_addContour(bool add)
{
    if (add) {
        typename ContourType::Pointer newFigure;

        bool contourRemoved = false;
        if (_currentContourNode != nullptr) {
            // If there's already a contour in the current reslice plane, try to perform the conversion
            auto oldFigure = static_cast<mitk::PlanarFigure*>(_currentContourNode->GetData());
            newFigure = crimson::tryConvertContourType<ContourType>(oldFigure);

            if (newFigure == oldFigure) {
                setToolInformation_Manual(ContourType::GetStaticNameOfClass());
                if (oldFigure->IsPlaced()) {
                    resetToolInformation(); // Conversion to the same type attempted. If the figure is already placed - uncheck
                                            // the button
                }
                return;
            }

            // Remove the old contour and continue with replacing
            _removeCurrentContour();
            contourRemoved = true;
        }

        bool emptyFigureCreated = false;
        if (newFigure.IsNull()) {
            emptyFigureCreated = true;
            newFigure = ContourType::New();
        }

        // Group the removal of the old contour and adding a new one into one undo group
        crimson::HierarchyManager::getInstance()->setStartNewUndoGroup(!contourRemoved);
        _setCurrentContourNode(_addPlanarFigure(newFigure.GetPointer()));
        crimson::HierarchyManager::getInstance()->setStartNewUndoGroup(true);
        setToolInformation_Manual(ContourType::GetStaticNameOfClass());

        // If a conversion was performed, a complete figure has been created and interaction doesn't need to be started
        // immediately
        if (!emptyFigureCreated) {
            cancelInteraction();
        }
    } else {
        cancelInteraction();
    }

    _updateUI();
}

void ContourModelingView::addCircle(bool add) { _addContour<mitk::PlanarCircle>(add); }

void ContourModelingView::addEllipse(bool add) { _addContour<mitk::PlanarEllipse>(add); }

void ContourModelingView::addPolygon(bool add)
{
    _addContour<mitk::PlanarSubdivisionPolygon>(add);
    if (add) {
        // Allow adding new control points to the subdivision polygon
        _currentContourNode->SetBoolProperty("planarfigure.isextendable", true);
    }
}

mitk::DataNode* ContourModelingView::_addPlanarFigure(mitk::PlanarFigure::Pointer figure, bool addInteractor)
{
    // Add the data node for a new planar figure
    auto newPlanarFigureNode = mitk::DataNode::New();
    newPlanarFigureNode->SetData(figure);

    // Set up the planar figure geometry
    assert(_vesselDrivenResliceView);
    mitk::BaseRenderer* renderer = _vesselDrivenResliceView->getResliceRenderer();
    const mitk::PlaneGeometry* geometry = dynamic_cast<const mitk::PlaneGeometry*>(renderer->GetCurrentWorldPlaneGeometry());
    auto planeGeometry = const_cast<mitk::PlaneGeometry*>(geometry);

    figure->SetPlaneGeometry(planeGeometry);

    // Set up the node properties
    crimson::VascularModelingUtils::setDefaultContourNodeProperties(newPlanarFigureNode, addInteractor);

    newPlanarFigureNode->SetIntProperty("lofting.smoothness", _UI.smoothnessSlider->value());
    if (addInteractor) {
        newPlanarFigureNode->SetBoolProperty("lofting.interactiveContour", true);
    }

    // Finally, add the new node to hierarchy
    crimson::HierarchyManager::getInstance()->addNodeToHierarchy(
        currentNode(), crimson::VascularModelingNodeTypes::VesselPath(), newPlanarFigureNode,
        crimson::VascularModelingNodeTypes::Contour());

    // And assign the parameter value
    crimson::VascularModelingUtils::assignPlanarFigureParameter(currentNode(), newPlanarFigureNode);
    setContourVisible(newPlanarFigureNode);

    return newPlanarFigureNode;
}

void ContourModelingView::NodeAdded(const mitk::DataNode* node)
{
    NodeDependentView::NodeAdded(node);

	/*if (!currentNode())
		return;*/

    // Old scenes - fix the visibility issues
    if (_vesselDrivenResliceView) {
        if (crimson::HierarchyManager::getInstance()
                ->getPredicate(crimson::VascularModelingNodeTypes::ContourSegmentationImage())
                ->CheckNode(node)) {
            for (mitk::BaseRenderer* rend : _vesselDrivenResliceView->getAllResliceRenderers()) {
                const_cast<mitk::DataNode*>(node)->SetVisibility(false, rend);
            }
        }
    }

    // Set the visibility for the thumbnail renderer
    if (_shouldBeInvisibleInThumbnailView->CheckNode(node)) {
        const_cast<mitk::DataNode*>(node)->SetVisibility(false, _thumbnailGenerator->getThumbnailRenderer());
    }

    auto hierarchyManager = crimson::HierarchyManager::getInstance();
    if (hierarchyManager->getAncestor(node, crimson::VascularModelingNodeTypes::VesselPath()) == currentNode()) {
        if (hierarchyManager->getPredicate(crimson::VascularModelingNodeTypes::Contour())->CheckNode(node)) {
            // New contour has been added - add to thumbnail view and connect the observers
            _addContoursThumbnailsWidgetItem(node);
            _connectContourObservers(const_cast<mitk::DataNode*>(node));
            _UI.contourThumbnailListWidget->sortItems();
            // In case the addition is the undo of contour deletion - navigate to the contour
            navigateToContour(node);
            _requestLoftPreview();
        } else if (hierarchyManager->getPredicate(crimson::VascularModelingNodeTypes::ContourSegmentationImage())
                       ->CheckNode(node)) {
            _connectSegmentationObservers(const_cast<mitk::DataNode*>(node));
        }
    }

    updateCurrentContour();
}

void ContourModelingView::_addContoursThumbnailsWidgetItem(const mitk::DataNode* node)
{
    // Update the parameters to ensure correct sorting
    _updatePlanarFigureParameters();

    QImage iconImage = _getNodeThumbnail(node);

    if (iconImage.isNull()) {
        // Request thumbnail creation
        _requestThumbnail(node);

        // Set a placeholder as the icon
        iconImage = QImage(64, 64, QImage::Format_RGB888);
        iconImage.fill(Qt::gray);
    }

    QIcon icon(QPixmap::fromImage(iconImage));
    _UI.contourThumbnailListWidget->addItem(new ThumbnalListWidgetItem(icon, node));
}

void ContourModelingView::navigateToContourByIndex(const QModelIndex& index)
{
    auto node = static_cast<ThumbnalListWidgetItem*>(_UI.contourThumbnailListWidget->item(index.row()))->node();
    navigateToContour(node);
}

void ContourModelingView::navigateToContour(const mitk::DataNode* node)
{
    if (!_vesselDrivenResliceView) {
        return;
    }

    if (_currentContourNode == node) {
        return;
    }

    for (auto iter = _parameterMap.begin(); iter != _parameterMap.end(); ++iter) {
        if (iter->second == node) {
            _vesselDrivenResliceView->navigateTo(iter->first);
            break;
        }
    }
}

void ContourModelingView::NodeRemoved(const mitk::DataNode* node)
{
    NodeDependentView::NodeRemoved(node);

    _nodeBeingDeleted = node;

    if (node == _currentContourNode) {
        _setCurrentContourNode(nullptr);
        _setSegmentationNodes(nullptr, nullptr);
    }

    auto hierarchyManager = crimson::HierarchyManager::getInstance();
    if (hierarchyManager->getAncestor(node, crimson::VascularModelingNodeTypes::VesselPath()) == currentNode()) {
        if (hierarchyManager->getPredicate(crimson::VascularModelingNodeTypes::Contour())->CheckNode(node)) {
            // Remove from parameter map if contour belongs to the current vessel
            for (auto iter = _parameterMap.begin(); iter != _parameterMap.end(); ++iter) {
                if (iter->second == node) {
                    _parameterMap.erase(iter);
                    break;
                }
            }
            _disconnectContourObservers(const_cast<mitk::DataNode*>(node));
            delete _findThumbnailsWidgetItemByNode(node);
            _requestLoftPreview();
        } else if (hierarchyManager->getPredicate(crimson::VascularModelingNodeTypes::ContourSegmentationImage())
                       ->CheckNode(node)) {
            _disconnectSegmentationObservers(const_cast<mitk::DataNode*>(node));
        }
    }

    // Cancel the thumbnail generation request if it has been issued
    _thumbnailGenerator->cancelThumbnailRequest(node);

    updateCurrentContour();

    _nodeBeingDeleted = nullptr;
}

void ContourModelingView::_addPlanarFigureInteractor(mitk::DataNode* node)
{
    bool isInteractiveContour = false;
    if (node->GetBoolProperty("lofting.interactiveContour", isInteractiveContour) && isInteractiveContour) {
        node->SetVisibility(true);
        auto figureInteractor = mitk::PlanarFigureInteractor::New();
        us::Module* planarFigureModule = us::ModuleRegistry::GetModule("MitkPlanarFigure");
        figureInteractor->LoadStateMachine("PlanarFigureInteraction.xml", planarFigureModule);
        figureInteractor->SetEventConfig("PlanarFigureConfig.xml", planarFigureModule);
        figureInteractor->SetDataNode(node);
    }
}

mitk::Point2D ContourModelingView::_getPlanarFigureCenter(const mitk::DataNode* figureNode) const
{
    auto planarFigure = static_cast<mitk::PlanarFigure*>(figureNode->GetData());
    Expects(planarFigure->GetPolyLinesSize() > 0);

    mitk::PlanarFigure::PolyLineType polyLine = planarFigure->GetPolyLine(0);

    // Simple center of mass computation
    mitk::Point2D center(0);
    mitk::ScalarType factor = 1.0 / polyLine.size();
    for (const mitk::Point2D& polyLineElement : polyLine) {
        center += polyLineElement.GetVectorFromOrigin() * factor;
    }

    return center;
}

void ContourModelingView::_updatePlanarFigureParameters(bool forceUpdate)
{
    _parameterMap = crimson::VascularModelingUtils::updatePlanarFigureParameters(currentNode(), _nodeBeingDeleted, forceUpdate);
}

void ContourModelingView::computePathThroughContourCenters()
{
    if (QMessageBox::warning(nullptr, "Undo not available for this operation", "Computing the path through contour centers "
                                                                               "will replace the current path. This operation "
                                                                               "cannot be undone.\n\n Do you wish to continue?",
                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No) {
        return;
    }

    // Setup the new vessel paths whose control points are the centers of contours

    std::vector<mitk::DataNode*> nodes = crimson::VascularModelingUtils::getVesselContourNodesSortedByParameter(currentNode());
    std::vector<mitk::Point3D> newControlPoints;

    auto vp = static_cast<crimson::VesselPathAbstractData*>(currentNode()->GetData());
    for (mitk::DataNode* node : nodes) {
        mitk::Point3D center3D;
        static_cast<mitk::PlanarFigure*>(node->GetData())->GetPlaneGeometry()->Map(_getPlanarFigureCenter(node), center3D);
        newControlPoints.push_back(center3D);
    }

    _ignoreVesselPathModifiedEvents = true;
    vp->setControlPoints(newControlPoints);
    _ignoreVesselPathModifiedEvents = false;

    // Reinitialize the plane geometries of the contours
    _vesselDrivenResliceView->forceReinitGeometry();
    int i = 0;
    for (mitk::DataNode* node : nodes) {
        node->SetFloatProperty("lofting.parameterValue", vp->getClosestPoint(vp->getControlPoint(i++)).t);
    }
    reinitAllContourGeometries(cgrOffsetContourToCenter);
}

void ContourModelingView::createLoft() { LoftAction().Run(currentNode()); }

void ContourModelingView::previewLoft()
{
    if (crimson::AsyncTaskManager::getInstance()->getTaskState(
            crimson::VascularModelingUtils::getPreviewLoftingTaskUID(currentNode())) != crimson::async::Task::State_Finished) {
        // Postpone if task is already running
        crimson::AsyncTaskManager::getInstance()->cancelTask(
            crimson::VascularModelingUtils::getPreviewLoftingTaskUID(currentNode()));
        _requestLoftPreview();
        return;
    }

    LoftAction(true, _UI.contourDistanceSpingBox->value()).Run(currentNode());
}

void ContourModelingView::setLoftingAlgorithm(int index)
{
    if (currentNode()) {
        currentNode()->SetIntProperty("lofting.loftingAlgorithm", index);
        _requestLoftPreview();
    }
}

void ContourModelingView::setSeamEdgeRotation(int angle)
{
    if (currentNode()) {
        currentNode()->SetIntProperty("lofting.seamEdgeRotation", angle);
        _requestLoftPreview();
    }
}

void ContourModelingView::loftingFinished(crimson::async::Task::State state)
{
    if (state == crimson::async::Task::State_Finished) {
        removeLoftPreview(currentNode());
    }
}

void ContourModelingView::loftPreviewFinished(crimson::async::Task::State state)
{
    if (state == crimson::async::Task::State_Finished) {
        mitk::DataNode* solidNode = crimson::HierarchyManager::getInstance()->getFirstDescendant(
            currentNode(), crimson::VascularModelingNodeTypes::Loft());
        if (solidNode) {
            solidNode->SetVisibility(false);
            mitk::RenderingManager::GetInstance()->RequestUpdateAll();
        }
        _UI.previewErrorLabel->hide();
        _UI.previewVisibilityButton->setEnabled(true);
        _UI.previewVisibilityButton->setChecked(true);
    } else {
        mitk::DataNode* previewNode = crimson::HierarchyManager::getInstance()->getFirstDescendant(
            currentNode(), crimson::VascularModelingNodeTypes::LoftPreview());
        if (previewNode) {
            previewNode->SetVisibility(false);
            mitk::RenderingManager::GetInstance()->RequestUpdateAll();
        }
        _UI.previewVisibilityButton->setEnabled(false);
        _UI.previewErrorLabel->show();
    }
}

std::pair<mitk::DataNode::Pointer, mitk::DataNode::Pointer>
ContourModelingView::_createSegmentationNodes(mitk::Image* image, const mitk::PlaneGeometry* sliceGeometry,
                                              mitk::PlanarFigure* figureToFill)
{
    // Make sure that for reslicing and overwriting the same alogrithm is used. We can specify the mode of the vtk reslicer
    vtkSmartPointer<mitkVtkImageOverwrite> reslice = vtkSmartPointer<mitkVtkImageOverwrite>::New();
    // set to false to extract a slice
    reslice->SetOverwriteMode(false);
    reslice->Modified();

    // use ExtractSliceFilter with our specific vtkImageReslice for overwriting and extracting
    mitk::ExtractSliceFilter::Pointer extractor = mitk::ExtractSliceFilter::New(reslice);

    extractor->SetInput(image);
    extractor->SetTimeStep(0);
    extractor->SetWorldGeometry(sliceGeometry);
    extractor->SetVtkOutputRequest(false);
    extractor->SetResliceTransformByGeometry(image->GetTimeGeometry()->GetGeometryForTimeStep(0));
    extractor->SetInPlaneResampleExtentByGeometry(true);
    extractor->SetInterpolationMode(mitk::ExtractSliceFilter::RESLICE_CUBIC);

    extractor->Modified();
    extractor->Update();

    mitk::Image::Pointer slice = extractor->GetOutput();
    mitk::Vector3D spacing = slice->GetGeometry()->GetSpacing();
    spacing[2] = 0.001;
    slice->SetSpacing(spacing);
    mitk::DataNode::Pointer sliceNode = mitk::DataNode::New();
    sliceNode->SetBoolProperty("hidden object", true);
    sliceNode->SetName("slice");
    sliceNode->SetVisibility(false);
    sliceNode->SetData(slice);
    sliceNode->SetBoolProperty("lofting.contour_reference_image", true);

    mitk::Tool* firstTool = mitk::ToolManagerProvider::GetInstance()->GetToolManager()->GetToolById(0);

    float color[3] = {1, 0, 0};
    mitk::DataNode::Pointer emptySegmentation =
        firstTool->CreateEmptySegmentationNode(slice, "segmented slice", mitk::Color(color));
    emptySegmentation->SetBoolProperty("hidden object", true);
    emptySegmentation->SetVisibility(false);
    for (mitk::BaseRenderer* rend : _vesselDrivenResliceView->getAllResliceRenderers()) {
        emptySegmentation->SetVisibility(true, rend);
    }
    emptySegmentation->SetBoolProperty("lofting.contour_segmentation_image", true);

    if (figureToFill != nullptr) {
        // Attempt conversion
        auto segmentationImage = static_cast<mitk::Image*>(emptySegmentation->GetData());
        _fillPlanarFigureInSlice(segmentationImage, figureToFill);
    }

    return std::make_pair(sliceNode, emptySegmentation);
}

void ContourModelingView::createSegmented(bool startNewUndoGroup)
{
    cancelInteraction();
    assert(_currentSegmentationWorkingImageNode == nullptr);

    mitk::DataStorage::SetOfObjects::ConstPointer imageNodes =
        GetDataStorage()->GetSources(currentNode(), mitk::TNodePredicateDataType<mitk::Image>::New(), false);
    if (imageNodes->size() == 0) {
        return;
    }

    auto contour = mitk::PlanarPolygon::New();
    contour->PlaceFigure(mitk::Point2D());
    contour->SetFinalized(true);

    assert(_vesselDrivenResliceView);

    mitk::BaseRenderer* renderer = _vesselDrivenResliceView->getResliceRenderer();

    const mitk::PlaneGeometry* geometry = dynamic_cast<const mitk::PlaneGeometry*>(renderer->GetCurrentWorldPlaneGeometry());
    contour->SetPlaneGeometry(const_cast<mitk::PlaneGeometry*>(geometry));

    std::pair<mitk::DataNode::Pointer, mitk::DataNode::Pointer> segmentationImagesPair = _createSegmentationNodes(
        static_cast<mitk::Image*>((*imageNodes)[0]->GetData()), geometry,
        _currentContourNode == nullptr ? nullptr : static_cast<mitk::PlanarFigure*>(_currentContourNode->GetData()));

    bool contourRemoved = false;
    if (_currentContourNode != nullptr) {
        _removeCurrentContour();
        contourRemoved = true;
    }

    // Clump deletion & creation of new node into one undo group
    crimson::HierarchyManager::getInstance()->setStartNewUndoGroup(!contourRemoved && startNewUndoGroup);
    mitk::DataNode* contourNode = _addPlanarFigure(contour.GetPointer(), false);
    contourNode->SetBoolProperty("planarfigure.drawcontrolpoints", false);

    crimson::HierarchyManager::getInstance()->setStartNewUndoGroup(false);
    crimson::HierarchyManager::getInstance()->addNodeToHierarchy(contourNode, crimson::VascularModelingNodeTypes::Contour(),
                                                                 segmentationImagesPair.first,
                                                                 crimson::VascularModelingNodeTypes::ContourReferenceImage());
    crimson::HierarchyManager::getInstance()->addNodeToHierarchy(
        contourNode, crimson::VascularModelingNodeTypes::Contour(), segmentationImagesPair.second,
        crimson::VascularModelingNodeTypes::ContourSegmentationImage());
    crimson::HierarchyManager::getInstance()->setStartNewUndoGroup(true);

    _setCurrentContourNode(contourNode);
    _setSegmentationNodes(segmentationImagesPair.first, segmentationImagesPair.second);

    _updateContourFromSegmentation(contourNode);
    _updateUI();
}

void ContourModelingView::_removeCurrentContour()
{
    if (!_currentContourNode || _currentContourNode == _nodeBeingDeleted) {
        return;
    }

    GetDataStorage()->Remove(_currentContourNode);
}

void ContourModelingView::_setSegmentationNodes(mitk::DataNode* refNode, mitk::DataNode* workNode)
{
    if (_currentSegmentationReferenceImageNode == refNode && _currentSegmentationWorkingImageNode == workNode) {
        return;
    }

    if (_vesselDrivenResliceView && _currentSegmentationWorkingImageNode) {
        for (mitk::BaseRenderer* rend : _vesselDrivenResliceView->getAllResliceRenderers()) {
            _currentSegmentationWorkingImageNode->SetVisibility(false, rend);
        }
    }

    _currentSegmentationReferenceImageNode = refNode;
    _currentSegmentationWorkingImageNode = workNode;

    if (_vesselDrivenResliceView && _currentSegmentationWorkingImageNode) {
        for (mitk::BaseRenderer* rend : _vesselDrivenResliceView->getAllResliceRenderers()) {
            _currentSegmentationWorkingImageNode->SetVisibility(true, rend);
        }
    }

    mitk::ToolManagerProvider::GetInstance()->GetToolManager()->SetReferenceData(_currentSegmentationReferenceImageNode);
    mitk::ToolManagerProvider::GetInstance()->GetToolManager()->SetWorkingData(_currentSegmentationWorkingImageNode);
}

void ContourModelingView::_setCurrentContourNode(mitk::DataNode* node)
{
    if (_currentContourNode == node) {
        return;
    }

    if (_currentContourNode) {
        cancelInteraction();
        if (_currentContourNode->GetDataInteractor()) {
            _currentContourNode->GetDataInteractor()->SetDataNode(nullptr);
        }
    }

    _currentContourNode = node;

    if (_currentContourNode) {
        _addPlanarFigureInteractor(_currentContourNode);

        auto planarFigure = static_cast<mitk::PlanarFigure*>(_currentContourNode->GetData());
        if (!planarFigure->IsFinalized()) {
            _contourTypeToButtonMap[planarFigure->GetNameOfClass()]->setChecked(true);
        }
    }
    _updateContourListViewSelection();
    updateCurrentContourInfo();
}

void ContourModelingView::updateContourSmoothnessFromUI()
{
    _currentContourNode->SetIntProperty("lofting.smoothness", _UI.smoothnessSlider->value());
    _updateContourFromSegmentation(_currentContourNode);
}

void ContourModelingView::_updateContourFromSegmentationData(const itk::Object* segmentationData)
{
    mitk::BaseData* data = const_cast<mitk::BaseData*>(static_cast<const mitk::BaseData*>(segmentationData));

    mitk::DataNode* workingImageNode = GetDataStorage()->GetNode(mitk::NodePredicateData::New(data));
    assert(workingImageNode);
    mitk::DataNode* contourNode =
        crimson::HierarchyManager::getInstance()->getAncestor(workingImageNode, crimson::VascularModelingNodeTypes::Contour());
    assert(contourNode);
    _updateContourFromSegmentation(contourNode);

    mitk::DataNode* vesselNode =
        crimson::HierarchyManager::getInstance()->getAncestor(contourNode, crimson::VascularModelingNodeTypes::VesselPath());
    assert(vesselNode);

    if (vesselNode == currentNode()) {
        // If the change happened due to undo/redo - navigate to the contour
        // Has no effect if the changes happened to the current contour
        navigateToContour(contourNode);
    }
}

void ContourModelingView::_updateContourFromSegmentation(mitk::DataNode* contourNode)
{
    // Compute the planarfigure representation for the segmented image
    
    mitk::DataNode* workingImageNode = _getSegmentationNodes(contourNode).second;
    assert(workingImageNode);

    mitk::PlanarPolygon* contour = static_cast<mitk::PlanarPolygon*>(contourNode->GetData());
    contour->SetClosed(true);

    while (contour->GetNumberOfControlPoints() > contour->GetMinimumNumberOfControlPoints()) {
        contour->RemoveLastControlPoint();
    }
    for (unsigned int i = 0; i < contour->GetNumberOfControlPoints(); ++i) {
        contour->SetControlPoint(i, mitk::Point2D(0.0));
    }

    auto image = static_cast<mitk::Image*>(workingImageNode->GetData());

    auto sliceSelector = mitk::ImageSliceSelector::New();
    sliceSelector->SetInput(image);
    sliceSelector->SetSliceNr(0);
    sliceSelector->SetTimeNr(0);
    sliceSelector->SetChannelNr(0);
    sliceSelector->Update();
    mitk::Image::Pointer slice = sliceSelector->GetOutput();

    mitk::ImageToContourFilter::Pointer contourExtractor = mitk::ImageToContourFilter::New();
    contourExtractor->SetInput(slice);
    contourExtractor->Update();
    mitk::Surface::Pointer contourSurf = contourExtractor->GetOutput();

    if (contourSurf->GetVtkPolyData()->GetNumberOfCells() == 0 ||
        contourSurf->GetVtkPolyData()->GetCell(0)->GetPointIds()->GetNumberOfIds() <
            contour->GetMinimumNumberOfControlPoints()) {
        return;
    }

    _createSmoothedContourFromVtkPolyData(contourSurf->GetVtkPolyData(), contour, _getContourSmoothnessFromNode(contourNode));

    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

float ContourModelingView::_getContourSmoothnessFromNode(mitk::DataNode* contourNode)
{
    int smoothness = 0;
    contourNode->GetIntProperty("lofting.smoothness", smoothness);
    return (float)smoothness / _UI.smoothnessSlider->maximum();
}

void ContourModelingView::_createSmoothedContourFromVtkPolyData(vtkPolyData* polyData, mitk::PlanarPolygon* contour,
                                                                float smoothness)
{
    vtkSmartPointer<vtkPolyData> contourPolyData;

    int maxNIter = 1000;
    int nIter = smoothness * maxNIter;

    if (nIter != 0) {
        vtkNew<vtkWindowedSincPolyDataFilter> smoothingFilter;
        smoothingFilter->SetInputData(polyData);
        smoothingFilter->NormalizeCoordinatesOn();
        smoothingFilter->FeatureEdgeSmoothingOff();
        smoothingFilter->BoundarySmoothingOn();
        smoothingFilter->SetFeatureAngle(180); // No "features" - everything should be smoothed
        smoothingFilter->SetEdgeAngle(180);

        smoothingFilter->SetNumberOfIterations(nIter + 1);
        smoothingFilter->SetPassBand(0.1);
        smoothingFilter->Update();

        contourPolyData = smoothingFilter->GetOutput();
    } else {
        contourPolyData = polyData;
    }

    vtkCell* cell = contourPolyData->GetCell(0);

    for (int i = 0; i < cell->GetPointIds()->GetNumberOfIds() - 1; ++i) {
        mitk::Point3D p;
        mitk::vtk2itk(cell->GetPoints()->GetPoint(cell->GetPointId(i)), p);

        mitk::Point2D p2d;
        contour->GetPlaneGeometry()->Map(p, p2d);
        contour->SetControlPoint(i, p2d, true);
    }
}

void ContourModelingView::setToolInformation_Segmentation(int id)
{
    if (id >= 0) {
        mitk::ToolManager* toolManager = mitk::ToolManagerProvider::GetInstance()->GetToolManager();
        us::ModuleResourceStream cursor(toolManager->GetToolById(id)->GetCursorIconResource(), std::ios::binary);
        setToolInformation(cursor, toolManager->GetToolById(id)->GetName());
    } else {
        resetToolInformation();
    }
}

void ContourModelingView::setToolInformation_Manual(const char* name)
{
    static std::unordered_map<const char*, const char*> typeNameToHumanReadableNameMap = {
        {mitk::PlanarCircle::GetStaticNameOfClass(), "Circle"},
        {mitk::PlanarEllipse::GetStaticNameOfClass(), "Ellipse"},
        {mitk::PlanarSubdivisionPolygon::GetStaticNameOfClass(), "Polygon"},
    };

    auto humanReadableName = typeNameToHumanReadableNameMap[name];

    // Set up the cursor
    QPixmap pix(QString(":/contourModeling/cursors/") + QString::fromLatin1(humanReadableName) + "_cursor.png");
    QBuffer buf;
    pix.save(&buf, "PNG");
    std::string data(buf.data().constData(), buf.data().length());
    std::istringstream cursor(data, std::ios_base::binary | std::ios_base::in);

    setToolInformation(cursor, humanReadableName);
    _currentManualButton = _contourTypeToButtonMap[name];
}

void ContourModelingView::resetToolInformation()
{
    _removeCancelInteractionShortcut();
    if (_currentManualButton) {
        _currentManualButton->blockSignals(true);
        _currentManualButton->setChecked(false);
        _currentManualButton->blockSignals(false);
        _currentManualButton = nullptr;
    }

    if (_cursorSet) {
        mitk::ApplicationCursor::GetInstance()->PopCursor();
        mitk::StatusBar::GetInstance()->DisplayText("");
        _cursorSet = false;
    }
}

void ContourModelingView::setToolInformation(std::istream& cursorStream, const char* name)
{
    // Remove previously set mouse cursor
    resetToolInformation();

    _createCancelInteractionShortcut();

    // Set up status bar text
    std::string text = std::string("Active Tool: \"") + name + "\"";
    mitk::StatusBar::GetInstance()->DisplayText(text.c_str());

    // Set the cursor
    mitk::ApplicationCursor::GetInstance()->PushCursor(cursorStream, 0, 0);
    _cursorSet = true;
}

void ContourModelingView::setNodeThumbnail(mitk::DataNode::ConstPointer node, QImage thumbnail)
{
    // Save the thumbnail in property list as a png in base-64
    QBuffer buffer;
    thumbnail.save(&buffer, "PNG");
    node->GetPropertyList()->SetProperty("lofting.thumbnail",
                                         mitk::StringProperty::New(QString(buffer.data().toBase64()).toStdString()));

    ThumbnalListWidgetItem* item = _findThumbnailsWidgetItemByNode(node);
    if (item) {
        item->setIcon(QPixmap::fromImage(thumbnail));
    }
}

ThumbnalListWidgetItem* ContourModelingView::_findThumbnailsWidgetItemByNode(const mitk::DataNode* node)
{
    for (int i = 0; i < _UI.contourThumbnailListWidget->count(); ++i) {
        auto item = static_cast<ThumbnalListWidgetItem*>(_UI.contourThumbnailListWidget->item(i));
        if (item->node() == node) {
            return item;
        }
    }

    return nullptr;
}

QImage ContourModelingView::_getNodeThumbnail(const mitk::DataNode* node)
{
    QImage iconImage;
    auto thumbnailProp = dynamic_cast<mitk::StringProperty*>(node->GetProperty("lofting.thumbnail"));
    if (!thumbnailProp /* || thumbnailProp->GetMTime() < node->GetData()->GetMTime()*/) { // Verify consistency which may fail
                                                                                          // due to undo/redo
        return iconImage;
    }
    QByteArray byteArray = QByteArray::fromBase64(QByteArray(thumbnailProp->GetValue()));
    QBuffer buf(&byteArray);

    iconImage.load(&buf, "PNG");
    return iconImage;
}

void ContourModelingView::reinitAllContourGeometries(ContourGeometyReinitType reinitType)
{
    if (!currentNode()) {
        return;
    }

    // Adjust all contour geometries. The adjustment strategy depends on the type of event
    // which triggered the reinitialization

    mitk::DataStorage::SetOfObjects::ConstPointer contours =
        crimson::VascularModelingUtils::getVesselContourNodes(currentNode());

    _updatePlanarFigureParameters();

    _ignoreContourChangeEvents = true;
    for (const mitk::DataNode::Pointer& contourNode : *contours) {
        auto figure = static_cast<mitk::PlanarFigure*>(contourNode->GetData());

        float t;
        contourNode->GetFloatProperty("lofting.parameterValue", t);

        const mitk::PlaneGeometry* geometry =
            dynamic_cast<const mitk::PlaneGeometry*>(_vesselDrivenResliceView->getPlaneGeometry(t));
        auto planeGeometry = const_cast<mitk::PlaneGeometry*>(geometry);

        switch (reinitType) {
        case cgrSetGeometry:
            // The vessel path has changed - simply set the geometry
            figure->SetPlaneGeometry(planeGeometry);
            break;
        case cgrOffsetByGeometryCorner: {
            // The reslice size has changed. 
            // Maintain the absolute position of the contour by moving the control points accordingly
            mitk::Point2D oldOriginInNewCoordinates;
            planeGeometry->Map(figure->GetPlaneGeometry()->GetOrigin(), oldOriginInNewCoordinates);
            mitk::Vector2D offset = oldOriginInNewCoordinates.GetVectorFromOrigin();

            figure->SetPlaneGeometry(planeGeometry);
            // Apply offset vector in case the reslice window size has been changed
            for (unsigned int i = 0; i < figure->GetNumberOfControlPoints(); ++i) {
                figure->mitk::PlanarFigure::SetControlPoint(i, figure->GetControlPoint(i) + offset);
            }
        } break;
        case cgrOffsetContourToCenter: {
            // User requested to change the vessel path to pass through contour centers
            mitk::Point2D oldCenter = _getPlanarFigureCenter(contourNode);
            figure->SetPlaneGeometry(planeGeometry);
            mitk::Point3D geometryCenter3D = crimson::VascularModelingUtils::getPlanarFigureGeometryCenter(contourNode);
            mitk::Point2D newCenter;
            planeGeometry->Map(geometryCenter3D, newCenter);
            mitk::Vector2D offset = newCenter - oldCenter;

            for (unsigned int i = 0; i < figure->GetNumberOfControlPoints(); ++i) {
                figure->mitk::PlanarFigure::SetControlPoint(i, figure->GetControlPoint(i) + offset);
            }
        } break;
        }

        // Re-create segmentation images from the updated contours
        std::pair<mitk::DataNode*, mitk::DataNode*> oldSegmentationNodes = _getSegmentationNodes(contourNode);
        mitk::DataStorage::SetOfObjects::ConstPointer imageNodes =
            GetDataStorage()->GetSources(currentNode(), mitk::TNodePredicateDataType<mitk::Image>::New(), false);
        if (oldSegmentationNodes.first != nullptr && imageNodes->size() > 0) {
            std::pair<mitk::DataNode::Pointer, mitk::DataNode::Pointer> newSegmentationNodes =
                _createSegmentationNodes(static_cast<mitk::Image*>((*imageNodes)[0]->GetData()), planeGeometry, figure);

            static_cast<mitk::Image*>(oldSegmentationNodes.first->GetData())
                ->SetGeometry(static_cast<mitk::Image*>(newSegmentationNodes.first->GetData())->GetGeometry()->Clone());
            static_cast<mitk::Image*>(oldSegmentationNodes.second->GetData())
                ->SetGeometry(static_cast<mitk::Image*>(newSegmentationNodes.second->GetData())->GetGeometry()->Clone());
        }

        // Create new thumbnails
        _requestThumbnail(contourNode.GetPointer());
    }
    _requestLoftPreview();
    _ignoreContourChangeEvents = false;
}

void ContourModelingView::_contourChanged(const itk::Object* contourData)
{
    if (_ignoreContourChangeEvents) {
        return;
    }

    mitk::BaseData* data = const_cast<mitk::BaseData*>(static_cast<const mitk::BaseData*>(contourData));
    mitk::DataNode* node = GetDataStorage()->GetNode(mitk::NodePredicateData::New(data));
    if (node) {
        _requestThumbnail(node);
        _requestLoftPreview();

        mitk::DataNode* vesselNode =
            crimson::HierarchyManager::getInstance()->getAncestor(node, crimson::VascularModelingNodeTypes::VesselPath());
        assert(vesselNode);

        if (vesselNode == currentNode()) {
            // Navigate to contour in case the change came from undo/redo
            navigateToContour(node);
        }

        if (node == _currentContourNode) {
            _contourInfoUpdateTimer.start(250);
        }
    }
}

void ContourModelingView::_figurePlacementCancelled(const itk::Object* contourData)
{
    // Undo has reached the placement event - set the state back to "adding a new contour"
    cancelInteraction(true);

    if (!_currentContourNode || _currentContourNode->GetData() != contourData) {
        return;
    }

    _contourTypeToButtonMap[contourData->GetNameOfClass()]->setChecked(true);
}

void ContourModelingView::duplicateContour()
{
    QList<QListWidgetItem*> selection = _UI.contourThumbnailListWidget->selectedItems();
    for (QListWidgetItem* item : selection) {
        const mitk::DataNode* node = static_cast<ThumbnalListWidgetItem*>(item)->node();
        if (node == _currentContourNode) {
            selection.removeOne(item);
            break;
        }
    }

    bool contourRemoved = false;
    if (_currentContourNode) {
        int rc =
            QMessageBox::question(nullptr, "Replace current contour?", "Are you sure you want to replace the current contour?",
                                  QMessageBox::Yes, QMessageBox::No);
        if (rc != QMessageBox::Yes) {
            return;
        }
        _removeCurrentContour(); // Might have side effect of changing the selection
        contourRemoved = true;
    }

    if (selection.size() == 0) {
        selection = _UI.contourThumbnailListWidget->selectedItems();
    }

    if (selection.size() == 0) {
        QMessageBox::information(nullptr, "No items selected", "Please select the contour to duplicate.", QMessageBox::Ok);
        return;
    }

    _updatePlanarFigureParameters();

    const mitk::DataNode* closestNode = nullptr;
    float minDistance = std::numeric_limits<float>::max();
    float curParameter = _vesselDrivenResliceView->getCurrentParameterValue();
    for (QListWidgetItem* item : selection) {
        const mitk::DataNode* node = static_cast<ThumbnalListWidgetItem*>(item)->node();
        float t;
        node->GetFloatProperty("lofting.parameterValue", t);
        float distance = fabs(curParameter - t);
        if (distance < minDistance) {
            minDistance = distance;
            closestNode = node;
        }
    }

    bool isExtendable = false;
    closestNode->GetBoolProperty("planarfigure.isextendable", isExtendable);

    auto clonedFigure = static_cast<mitk::PlanarFigure*>(closestNode->GetData())->Clone();

    bool isSegmented = GetDataStorage()->GetDerivations(closestNode)->size() != 0;

    crimson::HierarchyManager::getInstance()->setStartNewUndoGroup(!contourRemoved);
    mitk::DataNode* dataNode = _addPlanarFigure(clonedFigure, !isSegmented);
    if (isExtendable) {
        dataNode->SetBoolProperty("planarfigure.isextendable", true);
    }
    crimson::HierarchyManager::getInstance()->setStartNewUndoGroup(true);

    if (isSegmented) {
        createSegmented();
    }
}

void ContourModelingView::interpolateContour()
{
    QList<QListWidgetItem*> selection = _UI.contourThumbnailListWidget->selectedItems();
    for (QListWidgetItem* item : selection) {
        const mitk::DataNode* node = static_cast<ThumbnalListWidgetItem*>(item)->node();
        if (node == _currentContourNode) {
            selection.removeOne(item);
            break;
        }
    }

    bool contourRemoved = false;
    if (_currentContourNode) {
        int rc =
            QMessageBox::question(nullptr, "Replace current contour?", "Are you sure you want to replace the current contour?",
                                  QMessageBox::Yes, QMessageBox::No);
        if (rc != QMessageBox::Yes) {
            return;
        }
        _removeCurrentContour(); // Might have side effect of changing the selection
        contourRemoved = true;
    }

    if (selection.size() < 3) {
        selection = _UI.contourThumbnailListWidget->selectedItems();
    }

    if (selection.size() == 1) {
        duplicateContour();
        return;
    }

    if (selection.size() != 2) {
        QMessageBox::information(nullptr, "Select contours", "Please select two contours to interpolate.", QMessageBox::Ok);
        return;
    }

    _updatePlanarFigureParameters();

    const mitk::DataNode* contourNodes[2];
    float parameterValues[2];
    mitk::Image::Pointer contourSegmentedImages[2];

    const mitk::PlaneGeometry* geometry = _vesselDrivenResliceView->getResliceRenderer()->GetCurrentWorldPlaneGeometry();

    const int nInterpolationSlices = 100;
    const unsigned int dimensions[] = {static_cast<unsigned int>(geometry->GetBounds()[1]),
                                       static_cast<unsigned int>(geometry->GetBounds()[3])};

    const mitk::ScalarType spacing[] = {geometry->GetSpacing()[0], geometry->GetSpacing()[1], geometry->GetSpacing()[2]};

    mitk::Point3D origin;
    origin[0] = -spacing[0] * dimensions[0] / 2.0;
    origin[1] = -spacing[1] * dimensions[1] / 2.0;
    origin[2] = -spacing[0] / 2.0;

    // Step 1: get the segmentation images for figures
    mitk::PixelType pixelType(mitk::MakeScalarPixelType<unsigned char>());

    for (int i = 0; i < 2; ++i) {
        contourNodes[i] = static_cast<ThumbnalListWidgetItem*>(selection[i])->node();
        contourNodes[i]->GetFloatProperty("lofting.parameterValue", parameterValues[i]);
        auto planarFigure = static_cast<mitk::PlanarFigure*>(contourNodes[i]->GetData())->Clone();

        contourSegmentedImages[i] = mitk::Image::New();
        contourSegmentedImages[i]->Initialize(pixelType, 2, dimensions);
        contourSegmentedImages[i]->SetSpacing(spacing);
        origin[2] = -spacing[2] / 2.0 + (i == 0 ? 0 : nInterpolationSlices * spacing[2]);
        contourSegmentedImages[i]->SetOrigin(origin);

        {
            mitk::ImageWriteAccessor writeAccess(contourSegmentedImages[i]);
            memset(writeAccess.GetData(), 0, dimensions[0] * dimensions[1] * sizeof(unsigned char));
        }

        planarFigure->SetPlaneGeometry(
            static_cast<mitk::SlicedGeometry3D*>(contourSegmentedImages[i]->GetGeometry())->GetPlaneGeometry(0));

        _fillPlanarFigureInSlice(contourSegmentedImages[i], planarFigure);
    }

    // Step 2: interpolate
    mitk::Image::Pointer outImg;

    outImg = mitk::Image::New();
    outImg->Initialize(pixelType, 2, dimensions);
    outImg->SetSpacing(spacing);
    origin[2] = -spacing[2] / 2.0 + (nInterpolationSlices / 2) * spacing[2];
    outImg->SetOrigin(origin);

    float currentParameterValue = _vesselDrivenResliceView->getCurrentParameterValue();
    float interpolationFactor = (currentParameterValue - parameterValues[0]) / (parameterValues[1] - parameterValues[0]);

    auto interpolationAlgo = mitk::ShapeBasedInterpolationAlgorithm::New();
    interpolationAlgo->Interpolate(contourSegmentedImages[0].GetPointer(), 0, contourSegmentedImages[1].GetPointer(),
                                   nInterpolationSlices - 1, (int)((nInterpolationSlices - 1) * interpolationFactor + 0.5), 2,
                                   outImg, 0, nullptr);

    // Step 3: retrieve the contour from interpolated slice

    //////////////////////////////////////////////////////////////////////////
    // BEGIN DUPLICATION
    auto contour = mitk::PlanarPolygon::New();
    contour->SetClosed(true);
    contour->PlaceFigure(mitk::Point2D());
    contour->SetPlaneGeometry(static_cast<mitk::SlicedGeometry3D*>(outImg->GetGeometry())->GetPlaneGeometry(0));

    mitk::ImageToContourFilter::Pointer contourExtractor = mitk::ImageToContourFilter::New();
    contourExtractor->SetInput(outImg);
    contourExtractor->Update();

    mitk::Surface::Pointer contourSurf = contourExtractor->GetOutput();

    if (contourSurf->GetVtkPolyData()->GetNumberOfCells() == 0 ||
        contourSurf->GetVtkPolyData()->GetCell(0)->GetPointIds()->GetNumberOfIds() <
            contour->GetMinimumNumberOfControlPoints()) {
        return;
    }

    _createSmoothedContourFromVtkPolyData(contourSurf->GetVtkPolyData(), contour,
                                          (float)_UI.smoothnessSlider->value() / _UI.smoothnessSlider->maximum());
    createSegmented(!contourRemoved);

    contour->SetPlaneGeometry(static_cast<mitk::PlanarFigure*>(_currentContourNode->GetData())->GetPlaneGeometry()->Clone());
    _fillPlanarFigureInSlice(static_cast<mitk::Image*>(_currentSegmentationWorkingImageNode->GetData()), contour);

    // END DUPLICATION
    //////////////////////////////////////////////////////////////////////////
}

void ContourModelingView::_fillPlanarFigureInSlice(mitk::Image* segmentationImage, mitk::PlanarFigure* figure)
{
    // Setup the segmentation image from a contour of differnt type 
    // This acts as a conversion from manual contours to segmented ones

    if (figure->GetPolyLinesSize() == 0) {
        return;
    }

    mitk::PlanarFigure::PolyLineType polyLine = figure->GetPolyLine(0);

    auto contourModel = mitk::ContourModel::New();
    for (const mitk::Point2D& polyLineElement : polyLine) {
        mitk::Point3D mappedPoint;
        figure->GetPlaneGeometry()->Map(polyLineElement, mappedPoint);
        contourModel->AddVertex(mappedPoint);
    }

    mitk::ContourModel::Pointer projectedContour =
        mitk::ContourModelUtils::ProjectContourTo2DSlice(segmentationImage, contourModel, true, false);
    mitk::ContourModelUtils::FillContourInSlice(projectedContour, 0, segmentationImage, nullptr, 1);
}

void ContourModelingView::deleteSelectedContours()
{
    QList<QListWidgetItem*> selection = _UI.contourThumbnailListWidget->selectedItems();

    if (selection.size() == 0) {
        return;
    }

    if (QMessageBox::question(_UI.contourThumbnailListWidget, "Delete contours?",
                              "Are you sure you want to delete the selected contours?", QMessageBox::Yes,
                              QMessageBox::No) == QMessageBox::No) {
        return;
    }

    for (int i = 0; i < selection.size(); ++i) {
        crimson::HierarchyManager::getInstance()->setStartNewUndoGroup(i == 0);
        QListWidgetItem* item = selection[i];
        const mitk::DataNode* nodeToDelete = static_cast<ThumbnalListWidgetItem*>(item)->node();
        GetDataStorage()->Remove(nodeToDelete);
    }
    crimson::HierarchyManager::getInstance()->setStartNewUndoGroup(true);
}

void ContourModelingView::setContourVisible(mitk::DataNode* contourNode)
{
    mitk::DataNode::Pointer vesselNode =
        crimson::HierarchyManager::getInstance()->getAncestor(contourNode, crimson::VascularModelingNodeTypes::VesselPath());

    if (!vesselNode) {
        return;
    }

    bool visible = true;
    vesselNode->GetBoolProperty("lofting.contoursVisible", visible);

    ShowHideContoursAction::setContourVisibility(contourNode, visible);
}

void ContourModelingView::setAllContoursVisible(bool visible)
{
    if (!currentNode()) {
        return;
    }

    ShowHideContoursAction::setAllVesselContoursVisibility(currentNode(), visible);
}

void ContourModelingView::updateCurrentContourInfo()
{
    // Update the "current contour information" (e.g. area or radii)

    if (!_currentContourNode) {
        _UI.contourInfoTextBrowser->setText("");
        return;
    }

    auto planarFigure = static_cast<mitk::PlanarFigure*>(_currentContourNode->GetData());

    if (!planarFigure->IsFinalized()) {
        _UI.contourInfoTextBrowser->setText("");
        return;
    }

    QString text;
    for (unsigned int i = 0; i < planarFigure->GetNumberOfFeatures(); ++i) {
        text.append(QString("%1: %2%3")
                        .arg(planarFigure->GetFeatureName(i))
                        .arg(planarFigure->GetQuantity(i))
                        .arg(planarFigure->GetFeatureUnit(i)));
        if (i != planarFigure->GetNumberOfFeatures() - 1) {
            text.append("\n");
        }
    }
    _UI.contourInfoTextBrowser->setText(text);
    QSizeF docSize = _UI.contourInfoTextBrowser->document()->size();
    _UI.contourInfoTextBrowser->setFixedHeight(docSize.height());
}

void ContourModelingView::_requestThumbnail(mitk::DataNode::ConstPointer node)
{
    mitk::SliceNavigationController* tnc = mitk::RenderingManager::GetInstance()->GetTimeNavigationController();
    mitk::TimePointType time = tnc->GetInputWorldTimeGeometry()->TimeStepToTimePoint(tnc->GetTime()->GetPos());

    _thumbnailGenerator->requestThumbnail(node, time);
}

void ContourModelingView::_requestLoftPreview()
{
    if (_UI.livePreviewCheckBox->isChecked()) {
        _loftPreviewGenerateTimer.start(300);
    }
}

static void updateAsWallInformation(crimson::VesselForestData* vesselForest,
                                    const crimson::VesselForestData::VesselPathUIDType& pathUID, bool setAsWall, bool inflow,
                                    mitk::DataNode* solidModelNode)
{
    auto uidPair =
        std::make_pair(pathUID, inflow ? crimson::VesselForestData::InflowUID : crimson::VesselForestData::OutflowUID);

    if (setAsWall) {
        vesselForest->setFilletSizeInfo(uidPair, 0);
    } else {
        vesselForest->removeFilletSizeInfo(uidPair);
    }

    if (solidModelNode) {
        auto solidModelData = static_cast<crimson::OCCBRepData*>(solidModelNode->GetData());
        auto faceIndex = inflow ? solidModelData->inflowFaceId() : solidModelData->outflowFaceId();
        auto faceIdOpt = solidModelData->getFaceIdentifierMap().getFaceIdentifierForModelFace(faceIndex);
        if (faceIdOpt) {
            auto newFaceId = faceIdOpt->get();
            newFaceId.faceType = setAsWall ? crimson::FaceIdentifier::ftWall : (inflow ? crimson::FaceIdentifier::ftCapInflow
                                                                                       : crimson::FaceIdentifier::ftCapOutflow);
            solidModelData->getFaceIdentifierMap().setFaceIdentifierForModelFace(faceIndex, newFaceId);
        }
    }
}

void ContourModelingView::updateInflowOutflowAsWallInformation()
{
    if (!currentNode()) {
        return;
    }

    auto vesselForestNode =
        crimson::HierarchyManager::getInstance()->getAncestor(currentNode(), crimson::VascularModelingNodeTypes::VesselTree());

    if (!vesselForestNode) {
        return;
    }

    auto vesselForest = static_cast<crimson::VesselForestData*>(vesselForestNode->GetData());
    auto vessel = static_cast<crimson::VesselPathAbstractData*>(currentNode()->GetData());

    auto solidModelNode = crimson::HierarchyManager::getInstance()->getFirstDescendant(
        currentNode(), crimson::VascularModelingNodeTypes::Solid());

    // Add or remove filleting information for vessel if necessary
    updateAsWallInformation(vesselForest, vessel->getVesselUID(), _UI.useInflowAsWall->isChecked(), true, solidModelNode);
    updateAsWallInformation(vesselForest, vessel->getVesselUID(), _UI.useOutflowAsWall->isChecked(), false, solidModelNode);

    currentNode()->SetBoolProperty("lofting.useInflowAsWall", _UI.useInflowAsWall->isChecked());
    currentNode()->SetBoolProperty("lofting.useOutflowAsWall", _UI.useOutflowAsWall->isChecked());
}

void ContourModelingView::removeLoftPreview(mitk::DataNode* vesselNode)
{
    mitk::DataNode::Pointer previewNode = crimson::HierarchyManager::getInstance()->getFirstDescendant(
        vesselNode, crimson::VascularModelingNodeTypes::LoftPreview());
    if (previewNode) {
        GetDataStorage()->Remove(previewNode);
    }
}

void ContourModelingView::setInterContourDistance(double value)
{
    currentNode()->SetFloatProperty("lofting.interContourDistance", value);
    _requestLoftPreview();
}
