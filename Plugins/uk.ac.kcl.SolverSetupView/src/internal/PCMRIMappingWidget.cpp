// Qt
#include <QMessageBox>
#include <QKeyEvent>
#include <QShortcut>
#include <QtCore>
#include <QIcon>

// Main include
#include "PCMRIMappingWidget.h"

#include <NodePredicateDerivation.h>
#include <NodePredicateNone.h>

// Plugin includes
#include "ResliceView.h"
#include "ThumbnailGenerator.h"
#include "ConstMemberCommand.h"
#include "ContourTypeConversion.h"
#include "PCMRIUtils.h"
#include "MapAction.h"
#include "SolverSetupView.h"
#include "TimeInterpolationDialog.h"
#include <utils/TaskStateObserver.h>

// Module includes
#include <HierarchyManager.h>
#include <AsyncTaskManager.h>
#include <AsyncTask.h>
//#include <QtPropertyStorage.h>
#include <VascularModelingNodeTypes.h>
#include <VesselMeshingNodeTypes.h>
#include <SolverSetupNodeTypes.h>

// Micro-services
#include <usModule.h>
#include <usModuleResource.h>
#include <usModuleRegistry.h>
#include <usModuleResourceStream.h>

// MITK
#include <mitkVector.h>
#include <mitkNodePredicateAnd.h>
#include <mitkNodePredicateOr.h>
#include <mitkNodePredicateProperty.h>
#include <mitkNodePredicateDataType.h>
#include <mitkNodePredicateData.h>
#include <mitkRotationOperation.h>
#include <QmitkStdMultiWidgetEditor.h>
#include <QmitkAbstractView.h>
#include <mitkProperties.h>

//Points
#include "mitkPointSet.h"
#include "mitkSinglePointDataInteractor.h"

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

#include <berryIViewRegistry.h>
#include <berryPlatformUI.h>
#include <berryIWorkbenchPage.h>

// VTK
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkWindowedSincPolyDataFilter.h>

// Solid modelling kernel
#include <ISolidModelKernel.h>
#include <DataManagement/OCCBRepData.h>
#include <FaceData.h>

// PCMRI kernel
#include <IPCMRIKernel.h>
#include <DataManagement/PCMRIData.h>

//Solver kernel
#include <IBoundaryCondition.h>

Q_DECLARE_METATYPE(const mitk::DataNode*)

using namespace crimson;

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
		node()->GetFloatProperty("mapping.parameterValue", parameterValueL);

		float parameterValueR = 0;
		static_cast<const ThumbnalListWidgetItem*>(&r)->node()->GetFloatProperty("mapping.parameterValue", parameterValueR);

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

//TODO: use another NodeType?
PCMRIMappingWidget::PCMRIMappingWidget(QWidget* parent)
	: QWidget(parent)
	, _thumbnailGenerator(new crimson::ThumbnailGenerator(crimson::HierarchyManager::getInstance()->crimson::HierarchyManager::getInstance()->getDataStorage()))
{
	// Leave only planar figures and images in the thumbnails
	_shouldBeInvisibleInThumbnailView =
		mitk::NodePredicateNot::New(mitk::NodePredicateOr::New(mitk::NodePredicateOr::New(mitk::TNodePredicateDataType<mitk::Image>::New(),
		mitk::TNodePredicateDataType<mitk::PlanarFigure>::New()),
		mitk::TNodePredicateDataType<mitk::PointSet::Pointer>::New()))
		.GetPointer();

	//_viewWidget = parent;

	// create GUI widgets from the Qt Designer's .ui file
	_UI.setupUi(this);


	//Selection of PCMRI and mesh
	_UI.meshNodeComboBox->SetDataStorage(crimson::HierarchyManager::getInstance()->crimson::HierarchyManager::getInstance()->getDataStorage());
	_UI.pcmriImageComboBox->SetDataStorage(crimson::HierarchyManager::getInstance()->crimson::HierarchyManager::getInstance()->getDataStorage());
	_UI.pcmriPhaseComboBox->SetDataStorage(crimson::HierarchyManager::getInstance()->crimson::HierarchyManager::getInstance()->getDataStorage());
	_UI.mraImageComboBox->SetDataStorage(crimson::HierarchyManager::getInstance()->crimson::HierarchyManager::getInstance()->getDataStorage());

	//Adding corresponding points to MRA and PCMRI image
	connect(_UI.PCMRIPointButton, &QAbstractButton::toggled, this, &PCMRIMappingWidget::addPcmriPoint);
	connect(_UI.MRAPointButton, &QAbstractButton::toggled, this, &PCMRIMappingWidget::addMraPoint);

	_pointTypeToButtonMap["point.MRA"] = _UI.MRAPointButton;
	_pointTypeToButtonMap["point.PCMRI"] = _UI.PCMRIPointButton;

	// Manual tools for contour creation
	connect(_UI.addCircleButton, &QAbstractButton::toggled, this, &PCMRIMappingWidget::addCircle);
	connect(_UI.addPolygonButton, &QAbstractButton::toggled, this, &PCMRIMappingWidget::addPolygon);
	connect(_UI.addEllipseButton, &QAbstractButton::toggled, this, &PCMRIMappingWidget::addEllipse);

	_contourTypeToButtonMap[mitk::PlanarCircle::GetStaticNameOfClass()] = _UI.addCircleButton;
	_contourTypeToButtonMap[mitk::PlanarEllipse::GetStaticNameOfClass()] = _UI.addEllipseButton;
	_contourTypeToButtonMap[mitk::PlanarSubdivisionPolygon::GetStaticNameOfClass()] = _UI.addPolygonButton;

	// Contour duplication and interpolation
	connect(_UI.duplicateButton, &QAbstractButton::clicked, this, &PCMRIMappingWidget::duplicateContour);
	connect(_UI.interpolateButton, &QAbstractButton::clicked, this, &PCMRIMappingWidget::interpolateContour);
	connect(_UI.interpolateAllButton, &QAbstractButton::clicked, this, &PCMRIMappingWidget::interpolateAllContours);

	// Mapping
	_mapTaskStateObserver = new crimson::TaskStateObserver(_UI.mapButton, _UI.cancelMapButton, this);
	connect(_mapTaskStateObserver, &crimson::TaskStateObserver::runTaskRequested, this, &PCMRIMappingWidget::doMap);
	connect(_mapTaskStateObserver, &crimson::TaskStateObserver::taskFinished, this, &PCMRIMappingWidget::mappingFinished);

	QShortcut* mapShortcut = new QShortcut(QKeySequence("Alt+M"), parent);
	mapShortcut->setContext(Qt::ApplicationShortcut);
	connect(mapShortcut, &QShortcut::activated, _UI.mapButton, &QAbstractButton::click);

	//Velocity calculation from PCMRI image
	connect(_UI.vencPSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PCMRIMappingWidget::editVencP);
	connect(_UI.vencSSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PCMRIMappingWidget::editVencS);
	connect(_UI.vencGSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PCMRIMappingWidget::editVencG);

	connect(_UI.rescaleSlopeSSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &PCMRIMappingWidget::editRescaleSlopeS);
	connect(_UI.rescaleSlopeSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &PCMRIMappingWidget::editRescaleSlope);
	connect(_UI.rescaleInterceptSSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PCMRIMappingWidget::editRescaleInterceptS);
	connect(_UI.rescaleInterceptSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PCMRIMappingWidget::editRescaleIntercept);
	connect(_UI.quantizationSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PCMRIMappingWidget::editQuantization);

	connect(_UI.manufacturerTabs, static_cast<void (QTabWidget::*)(int)>(&QTabWidget::currentChanged), this, &PCMRIMappingWidget::editVelocityCalculationType);

	connect(_UI.freqSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &PCMRIMappingWidget::editCardiacfrequency);
	connect(_UI.venscaleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &PCMRIMappingWidget::editVenscale);
	connect(_UI.maskCheckBox, &QCheckBox::toggled, this, &PCMRIMappingWidget::setMagnitudeMask);

	//Time interpolation settings
	connect(_UI.timeOptionsButton, &QAbstractButton::clicked, this, &PCMRIMappingWidget::editTimeInterpolationOptions);

	//Image and flow flipping
	connect(_UI.flipImageCheckBox, &QCheckBox::toggled, this, &PCMRIMappingWidget::flipPCMRIImage);
	connect(_UI.flowDirectionCheckBox, &QCheckBox::toggled, this, &PCMRIMappingWidget::flipFlow);

	// Segmentation tools for contour creation
	connect(_UI.createSegmentedButton, SIGNAL(clicked()), this,
		SLOT(createSegmented())); // Default value -> old connection style
	connect(_UI.smoothnessSlider, &QAbstractSlider::valueChanged, this, &PCMRIMappingWidget::updateContourSmoothnessFromUI);

	mitk::ToolManager* toolManager = mitk::ToolManagerProvider::GetInstance()->GetToolManager();
	assert(toolManager);

	toolManager->SetDataStorage(*(crimson::HierarchyManager::getInstance()->crimson::HierarchyManager::getInstance()->getDataStorage()));
	toolManager->InitializeTools();
	dynamic_cast<mitk::SegTool2D*>(toolManager->GetToolById(0))->SetEnable3DInterpolation(false);

	_UI.segToolsSelectionBox->SetGenerateAccelerators(true);
	_UI.segToolsSelectionBox->SetToolGUIArea(_UI.segToolsGUIArea);
	_UI.segToolsSelectionBox->SetDisplayedToolGroups(
		"Add Subtract Correction Paint Wipe 'Region Growing' Fill Erase 'Live Wire'"); // '2D Fast Marching'");
	_UI.segToolsSelectionBox->SetLayoutColumns(3);
	_UI.segToolsSelectionBox->SetEnabledMode(QmitkToolSelectionBox::EnabledWithReferenceData);
	connect(_UI.segToolsSelectionBox, &QmitkToolSelectionBox::ToolSelected, this,
		&PCMRIMappingWidget::setToolInformation_Segmentation);

	// Thumbnail related actions
	connect(_UI.contourThumbnailListWidget, &QListWidget::doubleClicked, this, &PCMRIMappingWidget::navigateToContourByIndex);

	auto deleterEventFilter = new ItemDeleterEventFilter(_UI.contourThumbnailListWidget);
	_UI.contourThumbnailListWidget->installEventFilter(deleterEventFilter);
	connect(deleterEventFilter, &ItemDeleterEventFilter::deleteRequested, this, &PCMRIMappingWidget::deleteSelectedContours);
	connect(_UI.deleteContoursButton, &QAbstractButton::clicked, this, &PCMRIMappingWidget::deleteSelectedContours);
	connect(_UI.contourThumbnailListWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this,
		&PCMRIMappingWidget::syncContourSelection);

	connect(_thumbnailGenerator.get(), &crimson::ThumbnailGenerator::thumbnailGenerated, this,
		&PCMRIMappingWidget::setNodeThumbnail);

	// Contour information control
	_UI.contourInfoGroupBox->setCollapsed(true);
	_UI.contourInfoGroupBox->setCollapsedHeight(0);
	_contourInfoUpdateTimer.setSingleShot(true);
	connect(&_contourInfoUpdateTimer, &QTimer::timeout, this, &PCMRIMappingWidget::updateCurrentContourInfo);

	_contourGeometryUpdateTimer.setSingleShot(true);
	connect(&_contourGeometryUpdateTimer, &QTimer::timeout, this,
		&PCMRIMappingWidget::reinitAllContourGeometriesOnVesselPathChange);

	// Set the objects' visibility for the thumbnail renderer
	mitk::DataStorage::SetOfObjects::ConstPointer invisibleNodes =
		crimson::HierarchyManager::getInstance()->crimson::HierarchyManager::getInstance()->getDataStorage()->GetSubset(_shouldBeInvisibleInThumbnailView);
	for (const mitk::DataNode::Pointer& node : *invisibleNodes) {
		node->SetVisibility(false, _thumbnailGenerator->getThumbnailRenderer());
	}

	// Setup the listener for the reslice view
	berry::IWorkbenchPartSite* site = berry::PlatformUI::GetWorkbench()->GetActiveWorkbenchWindow()
		->GetActivePage()->GetActivePart()->GetSite().GetPointer();
		//		->GetActivePage()->GetActivePart()->GetSite().GetPointer();
	//if (berry::PlatformUI::GetWorkbench()->GetActiveWorkbenchWindow()
	//	->GetActivePage()->FindView("org.mitk.views.SolverSetupView"))
	//{
	//	site = berry::PlatformUI::GetWorkbench()->GetActiveWorkbenchWindow()
	//		->GetActivePage()->FindView("org.mitk.views.SolverSetupView")->GetSite().GetPointer();
	//}
	//else
	//{
	//	site = berry::PlatformUI::GetWorkbench()->GetActiveWorkbenchWindow()
	//		->GetActivePage()->GetActivePart()->GetSite().GetPointer();
	//}
	_partListener.reset(new crimson::ResliceViewListener([this](ResliceView* view) { this->setResliceView(view); }, site));
	_partListener->registerListener();

 //   // Connect the data observers for all contours
	//mitk::DataStorage::SetOfObjects::ConstPointer allContourNodes = crimson::HierarchyManager::getInstance()->
	//	crimson::HierarchyManager::getInstance()->getDataStorage()->GetSubset(
 //       crimson::HierarchyManager::getInstance()->getPredicate(crimson::VascularModelingNodeTypes::Contour()));
 //   _connectAllContourObservers(allContourNodes);


    //observes when a new contour or point is added
	_connectDataStorageEvents();

	// Initialize the UI
	_updateUI();
}

PCMRIMappingWidget::~PCMRIMappingWidget()
{
	cancelInteraction();
	cancelInteractionPoints();
	_ResliceView = nullptr;
	_setSegmentationNodes(nullptr, nullptr);
	_setCurrentContourNode(nullptr);
	_setCurrentPcmriPointNode(nullptr);
	_setCurrentMraPointNode(nullptr);
	setPCMRIImageForMapping(nullptr);
	setPCMRIImagePhaseForMapping(nullptr);
	if (_currentNode) {
		if (_currentPCMRINode)
		{
			mitk::DataStorage::SetOfObjects::ConstPointer nodes =
			crimson::PCMRIUtils::getContourNodes(_currentPCMRINode);
			for (const mitk::DataNode::Pointer& node : *nodes) {
				node->SetSelected(false);
			}
		}
		//mitk::DataNode::Pointer nodePcmri =	crimson::PCMRIUtils::getPcmriPointNode(_currentPCMRINode);
		//if (nodePcmri)
		//	nodePcmri->SetSelected(false);
		//mitk::DataNode::Pointer nodeMra = crimson::PCMRIUtils::getMraPointNode(_currentNode);
		//if (nodeMra)
		//	nodeMra->SetSelected(false);
	}
	_disconnectAllContourObservers();
	_disconnectDataStorageEvents();

	if (_partListener) {
		_partListener->unregisterListener();
	}
	setCurrentNode(nullptr);
	setCurrentSolidNode(nullptr);
}

void PCMRIMappingWidget::_createCancelInteractionShortcut()
{
	// Create a QShortcut for Esc key press
	assert(_cancelInteractionShortcut == nullptr);
	_cancelInteractionShortcut =
		new QShortcut(QKeySequence(Qt::Key_Escape), this, nullptr, nullptr, Qt::ApplicationShortcut);
	connect(_cancelInteractionShortcut, SIGNAL(activated()), this, SLOT(cancelInteraction()));
}

void PCMRIMappingWidget::_removeCancelInteractionShortcut()
{
	delete _cancelInteractionShortcut;
	_cancelInteractionShortcut = nullptr;
}

void PCMRIMappingWidget::_createCancelInteractionShortcutPoint()
{
	// Create a QShortcut for Esc key press
	assert(_cancelInteractionShortcutPoint == nullptr);
	_cancelInteractionShortcutPoint =
		new QShortcut(QKeySequence(Qt::Key_Backspace), this, nullptr, nullptr, Qt::ApplicationShortcut);
	connect(_cancelInteractionShortcutPoint, SIGNAL(activated()), this, SLOT(cancelInteractionPoints()));
}

void PCMRIMappingWidget::_removeCancelInteractionShortcutPoint()
{
	delete _cancelInteractionShortcutPoint;
	_cancelInteractionShortcutPoint = nullptr;
}

void PCMRIMappingWidget::setResliceView(ResliceView* view)
{
	if (_ResliceView == view) {
		return;
	}

	if (_ResliceView) {
		QWidget::disconnect(_ResliceView, &ResliceView::sliceChanged, this,
			&PCMRIMappingWidget::currentSliceChanged);
		QWidget::disconnect(_ResliceView, &ResliceView::geometryChanged, this,
			&PCMRIMappingWidget::reinitAllContourGeometriesByCorner);
	}

	_ResliceView = view;

	if (_ResliceView) {
		// connect the handlers for ResliceView events
		connect(_ResliceView, &ResliceView::sliceChanged, this,
			&PCMRIMappingWidget::currentSliceChanged);  //this is for PCMRI slices and affects segmentation
		connect(_ResliceView, &ResliceView::geometryChanged, this,
			&PCMRIMappingWidget::reinitAllContourGeometriesByCorner);	
		_ResliceView->setCurrentSolidNode(_currentSolidNode);
		setPCMRIImageForMapping(_currentPCMRINode);
		setMRAImageForMapping(_currentMRANode);
	}

	updateCurrentContour();
	updateCurrentPcmriPoint();
	updateCurrentMraPoint();
}

void PCMRIMappingWidget::setCurrentNode(mitk::DataNode* node)
{
	if (node == _currentNode) {
		return;
	}

	mitk::DataNode* prevBCNode = _currentNode;


	// The current BC object has changed
	if (_currentNode != nullptr) {
		// Unsubscribe from old change events
        // _currentNode->GetData()->RemoveObserver(_observerTag);
        // _observerTag = 0;
       // _removeNodeObserver();
		if (_currentPCMRINode)
		{
			mitk::DataStorage::SetOfObjects::ConstPointer nodes =
				crimson::PCMRIUtils::getContourNodes(_currentPCMRINode);
			for (const mitk::DataNode::Pointer& node : *nodes) {
				node->SetSelected(false);
			}
		}
		//mitk::DataNode::Pointer nodePcmri =	crimson::PCMRIUtils::getPcmriPointNode(_currentPCMRINode);
		//if (nodePcmri)
		//		nodePcmri->SetSelected(false);
		//mitk::DataNode::Pointer nodeMra = crimson::PCMRIUtils::getMraPointNode(_currentNode);
		//if (nodeMra)
		//		nodeMra->SetSelected(false);

		QWidget::disconnect(_UI.meshNodeComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this,
			&PCMRIMappingWidget::setMeshNodeForMapping);
		QWidget::disconnect(_UI.pcmriImageComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this,
			&PCMRIMappingWidget::setPCMRIImageForMapping);
		QWidget::disconnect(_UI.pcmriPhaseComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this,
			&PCMRIMappingWidget::setPCMRIImagePhaseForMapping);
		QWidget::disconnect(_UI.mraImageComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this,
			&PCMRIMappingWidget::setMRAImageForMapping);


		_UI.meshNodeComboBox->SetPredicate(NodePredicateNone::New());
		_UI.pcmriImageComboBox->SetPredicate(NodePredicateNone::New());
		_UI.pcmriPhaseComboBox->SetPredicate(NodePredicateNone::New());
		_UI.mraImageComboBox->SetPredicate(NodePredicateNone::New());

		// Update the contour geometries immediately if the request has been issued
		if (this->_contourGeometryUpdateTimer.isActive()) {
			this->_contourGeometryUpdateTimer.stop();
			reinitAllContourGeometriesOnVesselPathChange();
		}

	}

	_currentNode = const_cast<mitk::DataNode*>(node);
	

	if (_currentNode != nullptr) {
		
		//_addNodeObserver();
		
		_setupPCMRIMappingComboBoxes();

		//check if an MRA point was previously set
		mitk::DataNode::Pointer mraNode = crimson::PCMRIUtils::getMraPointNode(_currentNode);
		if (mraNode)
			_currentMraPointNode = mraNode.GetPointer();

		_mapTaskStateObserver->setPrimaryObservedUID(crimson::PCMRIUtils::getMappingTaskUID(_currentNode));

		updateCurrentContour();
		updateCurrentMraPoint();
		updateCurrentPcmriPoint();

		auto child = crimson::HierarchyManager::getInstance()->getFirstDescendant(_currentNode, crimson::SolverSetupNodeTypes::PCMRIData(), true);
		if (child.IsNotNull())
		{
			if (_ResliceView)
				_ResliceView->navigateTo(dynamic_cast<PCMRIData*>(child->GetData())->getVisualizationParameters()._maxIndex);

		}

	}

	if (_ResliceView && _currentSolidNode)
		_ResliceView->setCurrentSolidNode(_currentSolidNode);

	_updateUI();
	_fillContourThumbnailsWidget();

}

void PCMRIMappingWidget::setCurrentSolidNode(mitk::DataNode* node)
{
	_currentSolidNode = node;
	//auto solidUID = std::string{};
	//node->GetStringProperty(crimson::HierarchyManager::nodeUIDPropertyName, solidUID);
	//_currentNode->SetStringProperty("mapping.solidnode", solidUID.c_str());
}

void PCMRIMappingWidget::currentSliceChanged()
{
	// The user navigated to a different slice position
	cancelInteraction();
	//cancelInteractionPoints();
	updateCurrentContour();
	//updateCurrentPcmriPoint();
}

void PCMRIMappingWidget::updateCurrentContour()
{
	mitk::DataNode* newRefImageNode = nullptr;
	mitk::DataNode* newWorkingImageNode = nullptr;
	mitk::DataNode* newContourNode = nullptr;

	if (!_ResliceView || !_currentNode) {
		_setSegmentationNodes(nullptr, nullptr);
		_setCurrentContourNode(nullptr);
		_updateUI();
		return;
	}

	mitk::BaseRenderer* renderer = _ResliceView->getPCMRIRenderer();

	mitk::DataStorage::SetOfObjects::ConstPointer nodes = crimson::PCMRIUtils::getContourNodes(_currentPCMRINode);

	if (!nodes->empty())
	{
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

			float timePos = _ResliceView->getCurrentParameterValue();
			float param = 0;
			node->GetFloatProperty("mapping.parameterValue", param);

			//check if planar geometries are parallel in case PCMRI image selection has changed
			if (timePos != param || !planarFigurePlaneGeometry->IsParallel(rendererPlaneGeometry)
				//||
				//!(planarFigureTimeGeometry->DistanceFromPlane(rendererTimeGeometry) < planeThickness / 3.0)
				) {
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
	}

	_setCurrentContourNode(newContourNode);

	_setSegmentationNodes(newRefImageNode, newWorkingImageNode);

	_updateUI();
	_updateContourListViewSelection();

	if(_ResliceView)
		_ResliceView->getPCMRIRenderer()->GetRenderingManager()->RequestUpdateAll();


}

void PCMRIMappingWidget::updateCurrentPcmriPoint()
{
	mitk::DataNode* newPcmriPointNode = nullptr;

	if (!_ResliceView || !_currentPCMRINode) {
		_setCurrentPcmriPointNode(nullptr);
		_updateUI();
		return;
	}

	mitk::DataNode::Pointer node = crimson::PCMRIUtils::getPcmriPointNode(_currentPCMRINode);
	if (node)
	{
		if (node != _nodeBeingDeleted)
			newPcmriPointNode = node;

	}

	_setCurrentPcmriPointNode(newPcmriPointNode);

	_updateUI();

	if(_ResliceView)
		_ResliceView->getPCMRIRenderer()->GetRenderingManager()->RequestUpdateAll();


}

//TODO: call this function when the selected solid face or selected MRA image changes
void PCMRIMappingWidget::updateCurrentMraPoint()
{
	mitk::DataNode* newMraPointNode = nullptr;

	if (!_ResliceView || !_currentNode) {
		_setCurrentMraPointNode(nullptr);
		_updateUI();
		return;
	}

	mitk::DataNode::Pointer node = crimson::PCMRIUtils::getMraPointNode(_currentNode);

	if (node)
	{
			if (node != _nodeBeingDeleted) 
				newMraPointNode = node;
			
	}
	_setCurrentMraPointNode(newMraPointNode);

	_updateUI();

	if(_ResliceView)
		_ResliceView->getResliceRenderer()->GetRenderingManager()->RequestUpdateAll();

}

std::pair<mitk::DataNode*, mitk::DataNode*> PCMRIMappingWidget::_getSegmentationNodes(mitk::DataNode* contourNode)
{
	return std::make_pair<mitk::DataNode*, mitk::DataNode*>(
		crimson::HierarchyManager::getInstance()->getFirstDescendant(
		contourNode, crimson::VascularModelingNodeTypes::ContourReferenceImage()),
		crimson::HierarchyManager::getInstance()->getFirstDescendant(
		contourNode, crimson::VascularModelingNodeTypes::ContourSegmentationImage()));
}

void PCMRIMappingWidget::cancelInteraction(bool undoingPlacement)
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
		}
		else if (!currentPlanarFigure->IsFinalized()) {
			// Interaction has started, but has not completed. Attempt to save the data by imitating interactor figure finishing
			static_cast<mitk::PlanarFigureInteractor*>(_currentContourNode->GetDataInteractor().GetPointer())->FinalizeFigure();
		}
	}
}

void PCMRIMappingWidget::cancelInteractionPoints(bool undoingPlacement)
{

	_removeCancelInteractionShortcutPoint();

	if (_currentPointButton) {
		resetPointToolInformation();
	}

	if (_currentPcmriPointNode && !undoingPlacement) {
		mitk::PointSet::Pointer currentPCMRIPoint = dynamic_cast<mitk::PointSet*>(_currentPcmriPointNode->GetData());

		if (currentPCMRIPoint->GetSize() == 0){
			// Point has not been placed - remove the point altogether
			_removeCurrentPcmriPoint();
		}
	}

	if (_currentPcmriPointNode) {
		if (_currentPcmriPointNode->GetDataInteractor()) {
			_currentPcmriPointNode->GetDataInteractor()->SetDataNode(nullptr);
		}
	}

	if (_currentMraPointNode && !undoingPlacement) {
		mitk::PointSet::Pointer currentMRAPoint = dynamic_cast<mitk::PointSet*>(_currentMraPointNode->GetData());

		if (currentMRAPoint->GetSize() == 0) {
			// Point has not been placed - remove the point altogether
			_removeCurrentMraPoint();
		}
	}

	if (_currentMraPointNode) {
		if (_currentMraPointNode->GetDataInteractor()) {
			_currentMraPointNode->GetDataInteractor()->SetDataNode(nullptr);
		}
	}
}


void PCMRIMappingWidget::_connectContourObservers(mitk::DataNode* node)
{
	auto updateCommand = crimson::ConstMemberCommand<PCMRIMappingWidget>::New();
	updateCommand->SetCallbackFunction(this, &PCMRIMappingWidget::_contourChanged);
	_contourObserverTags[node].tags[ContourObserverTags::Modified] =
		node->GetData()->AddObserver(itk::ModifiedEvent(), updateCommand);

	auto cancelFigurePlacementCommand = crimson::ConstMemberCommand<PCMRIMappingWidget>::New();
	cancelFigurePlacementCommand->SetCallbackFunction(this, &PCMRIMappingWidget::_figurePlacementCancelled);
	_contourObserverTags[node].tags[ContourObserverTags::FigurePlacementCancelled] =
		node->GetData()->AddObserver(mitk::CancelPlacementPlanarFigureEvent(), cancelFigurePlacementCommand);

	auto planarFigureFinishedCommand = itk::SimpleMemberCommand<PCMRIMappingWidget>::New();
	planarFigureFinishedCommand->SetCallbackFunction(this, &PCMRIMappingWidget::cancelInteraction);
	_contourObserverTags[node].tags[ContourObserverTags::FigureFinished] =
		node->GetData()->AddObserver(mitk::FinalizedPlanarFigureEvent(), planarFigureFinishedCommand);
}

void PCMRIMappingWidget::_connectSegmentationObservers(mitk::DataNode* segmentationNode)
{
	auto command = crimson::ConstMemberCommand<PCMRIMappingWidget>::New();
	command->SetCallbackFunction(this, &PCMRIMappingWidget::_updateContourFromSegmentationData);
	_segmentationObserverTags[segmentationNode] = segmentationNode->GetData()->AddObserver(itk::ModifiedEvent(), command);
}


void PCMRIMappingWidget::_disconnectContourObservers(mitk::DataNode* node)
{
	if (_contourObserverTags.find(node) != _contourObserverTags.end()) {
		for (unsigned long tag : _contourObserverTags[node].tags) {
			node->GetData()->RemoveObserver(tag);
		}
		_contourObserverTags.erase(node);
	}
}

void PCMRIMappingWidget::_disconnectSegmentationObservers(mitk::DataNode* segmentationNode)
{
	if (_segmentationObserverTags.find(segmentationNode) != _segmentationObserverTags.end()) {
		segmentationNode->GetData()->RemoveObserver(_segmentationObserverTags[segmentationNode]);
		_segmentationObserverTags.erase(segmentationNode);
	}
}


void PCMRIMappingWidget::_connectAllContourObservers(mitk::DataStorage::SetOfObjects::ConstPointer contourNodes)
{
	for (mitk::DataNode::Pointer node : *contourNodes) {
		_connectContourObservers(node);
		auto segmentationNode = _getSegmentationNodes(node).second;
		if (segmentationNode) {
			_connectSegmentationObservers(segmentationNode);
		}
	}
}

void PCMRIMappingWidget::_disconnectAllContourObservers()
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

void PCMRIMappingWidget::_fillContourThumbnailsWidget()
{
	_UI.contourThumbnailListWidget->clear();

	if (_currentPCMRINode) {
		mitk::DataStorage::SetOfObjects::ConstPointer nodes =
			crimson::PCMRIUtils::getContourNodes(_currentPCMRINode);
		for (const mitk::DataNode::Pointer& node : *nodes) {
			_addContoursThumbnailsWidgetItem(node);
		}
		_UI.contourThumbnailListWidget->sortItems();
		_updateContourListViewSelection();
	}
}

void PCMRIMappingWidget::reinitAllContourGeometriesOnVesselPathChange()
{
	_newContourPositions.clear();
	if (_ResliceView) {
		_ResliceView->forceReinitGeometry();
	}
	reinitAllContourGeometries(cgrSetGeometry);
	//mitk::RenderingManager::GetInstance()->RequestUpdateAll();
	if(_ResliceView)
		_ResliceView->getPCMRIRenderer()->GetRenderingManager()->RequestUpdateAll();

}

void PCMRIMappingWidget::_updateUI()
{
	bool contourModelingToolsEnabled = _currentPCMRINode && _ResliceView && _UI.pcmriImageComboBox->GetSelectedNode();
	_UI.contourEditingGroup->setEnabled(contourModelingToolsEnabled);

	bool objectSelectionEnabled = _currentPCMRINode;

	if (contourModelingToolsEnabled) {
		_UI.segToolsGUIArea->setEnabled(_currentSegmentationReferenceImageNode != nullptr);
		_UI.segToolsSelectionBox->setEnabled(_currentSegmentationReferenceImageNode != nullptr);
		_UI.createSegmentedButton->setEnabled(_currentSegmentationReferenceImageNode == nullptr);
		_UI.smoothnessSlider->setEnabled(_currentSegmentationReferenceImageNode != nullptr);
	}

	_UI.correspondingPointsGroup->setEnabled(_ResliceView && _UI.pcmriImageComboBox->GetSelectedNode()&&_UI.mraImageComboBox->GetSelectedNode());
	_UI.objectSelectionGroup->setEnabled(objectSelectionEnabled);

	_mapTaskStateObserver->setEnabled(_UI.contourThumbnailListWidget->count() > 0 && _UI.pcmriImageComboBox->GetSelectedNode()
		&& _UI.pcmriPhaseComboBox->GetSelectedNode() && _UI.freqSpinBox->value() > 0
		&& _UI.mraImageComboBox->GetSelectedNode() && _UI.meshNodeComboBox->GetSelectedNode() && _currentPcmriPointNode && _currentMraPointNode);
	_UI.deleteContoursButton->setEnabled(_UI.contourThumbnailListWidget->selectedItems().size() > 0);

	if (_currentNode){
		crimson::FaceData* fData = static_cast<crimson::FaceData*>(_currentNode->GetData());
		_UI.mapButton->setEnabled(!(fData->getFaces().empty()) && _UI.contourThumbnailListWidget->count() > 0 && _UI.pcmriImageComboBox->GetSelectedNode()
			&& _UI.pcmriPhaseComboBox->GetSelectedNode() && _UI.freqSpinBox->value() > 0
			&& _UI.mraImageComboBox->GetSelectedNode() && _UI.meshNodeComboBox->GetSelectedNode() && _currentPcmriPointNode && _currentMraPointNode);
		
		_UI.flipImageCheckBox->setEnabled(_UI.pcmriImageComboBox->GetSelectedNode());
		_UI.gaussianDoubleSpinBox->setEnabled(_UI.pcmriPhaseComboBox->GetSelectedNode());

		auto child = crimson::HierarchyManager::getInstance()->getFirstDescendant(_currentNode, crimson::SolverSetupNodeTypes::PCMRIData(), true);
		_UI.timeOptionsButton->setEnabled(child.IsNotNull());
		_UI.flowDirectionCheckBox->setEnabled(child.IsNotNull());
	}
	else
	{
		_UI.mapButton->setEnabled(false);
		_UI.timeOptionsButton->setEnabled(false);
		_UI.flowDirectionCheckBox->setEnabled(false);
	}


}



void PCMRIMappingWidget::_updateContourListViewSelection()
{
	// In the thumbnail view, select the contours closest to the reslice plane,
	// or the current contour if one lies in the current reslice plane
	_UI.contourThumbnailListWidget->clearSelection();

	if (_currentPCMRINode && _ResliceView != nullptr) {
		ParameterMapType::iterator closestNodeIters[2];
		_getClosestContourNodes(_ResliceView->getCurrentParameterValue(), closestNodeIters);

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

void PCMRIMappingWidget::_getClosestContourNodes(float parameterValue, ParameterMapType::iterator outClosestNodeIterators[2])
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

void PCMRIMappingWidget::syncContourSelection(const QItemSelection& selected, const QItemSelection& deselected)
{
	if (_currentContourNode)
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

		//mitk::RenderingManager::GetInstance()->RequestUpdateAll();
		if (_ResliceView)
			_ResliceView->getPCMRIRenderer()->GetRenderingManager()->RequestUpdateAll();
	}

}

// Add the contour on the current geometry
//
template <typename ContourType>
void PCMRIMappingWidget::_addContour(bool add)
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
	}
	else {
		cancelInteraction();
	}

	_updateUI();
}

void PCMRIMappingWidget::addCircle(bool add) { _addContour<mitk::PlanarCircle>(add); }

void PCMRIMappingWidget::addEllipse(bool add) { _addContour<mitk::PlanarEllipse>(add); }

void PCMRIMappingWidget::addPolygon(bool add)
{
	_addContour<mitk::PlanarSubdivisionPolygon>(add);
	if (add) {
		// Allow adding new control points to the subdivision polygon
		_currentContourNode->SetBoolProperty("planarfigure.isextendable", true);
	}
}

void PCMRIMappingWidget::addPcmriPoint(bool add)
{
	if (add) {
		mitk::PointSet::Pointer newPoint;

		bool pointRemoved = false;
		if (_currentPcmriPointNode != nullptr) {

			// Remove the old point and continue with replacing
			_removeCurrentPcmriPoint();
			pointRemoved = true;
		}

		bool emptyPointCreated = false;
		if (newPoint.IsNull()) {
			emptyPointCreated = true;
			newPoint = mitk::PointSet::New();
			newPoint->SetGeometry(_ResliceView->getPCMRIRenderer()->GetCurrentWorldPlaneGeometry()->Clone());
		}

		_setCurrentPcmriPointNode(_addPcmriPointNode(newPoint.GetPointer()));
		setToolInformation_Point("point.PCMRI");

	}
	else {
		cancelInteractionPoints();
	}

	_updateUI();
}


void PCMRIMappingWidget::addMraPoint(bool add)
{
	if (add) {
		mitk::PointSet::Pointer newPoint;

		bool pointRemoved = false;
		if (_currentMraPointNode != nullptr) {
			// Remove the old point and continue with replacing
			_removeCurrentMraPoint();
			pointRemoved = true;
		}

		bool emptyPointCreated = false;
		if (newPoint.IsNull()) {
			emptyPointCreated = true;
			newPoint = mitk::PointSet::New();
		}
		_setCurrentMraPointNode(_addMraPointNode(newPoint.GetPointer()));
		setToolInformation_Point("point.MRA");

	}
	else {
		cancelInteractionPoints();
	}

	_updateUI();
}

mitk::DataNode* PCMRIMappingWidget::_addPlanarFigure(mitk::PlanarFigure::Pointer figure, bool addInteractor)
{
	// Add the data node for a new planar figure
	auto newPlanarFigureNode = mitk::DataNode::New();
	newPlanarFigureNode->SetData(figure);

	// Set up the planar figure geometry
	assert(_ResliceView);
	mitk::BaseRenderer* renderer = _ResliceView->getPCMRIRenderer();
	const mitk::PlaneGeometry* geometry = dynamic_cast<const mitk::PlaneGeometry*>(renderer->GetCurrentWorldPlaneGeometry());
	auto planeGeometry = const_cast<mitk::PlaneGeometry*>(geometry);

	figure->SetPlaneGeometry(planeGeometry);

	// Set up the node properties
	crimson::PCMRIUtils::setDefaultContourNodeProperties(newPlanarFigureNode, addInteractor);

	newPlanarFigureNode->SetIntProperty("lofting.smoothness", _UI.smoothnessSlider->value());
	if (addInteractor) {
		newPlanarFigureNode->SetBoolProperty("lofting.interactiveContour", true);
	}

	// Finally, add the new node to hierarchy
	crimson::HierarchyManager::getInstance()->addNodeToHierarchy(
		_currentPCMRINode, crimson::VascularModelingNodeTypes::Image(), newPlanarFigureNode,
		crimson::VascularModelingNodeTypes::Contour());

	// And assign the parameter value
	crimson::PCMRIUtils::assignPlanarFigureParameter(_currentPCMRINode, newPlanarFigureNode, _ResliceView->getPCMRIRenderer());

	newPlanarFigureNode->SetVisibility(true);

	return newPlanarFigureNode;
}

mitk::DataNode* PCMRIMappingWidget::_addPcmriPointNode(mitk::PointSet::Pointer point, bool addInteractor)
{
	// Add the data node for a new planar point
	auto newPointNode = mitk::DataNode::New();
	newPointNode->SetData(point);
	//newPointNode->SetBoolProperty("hidden object", true);

	assert(_ResliceView);
	float color[3] = { 1.0f, 0.0f, 0.0f };
	// Set up the node properties
	crimson::PCMRIUtils::setDefaultPointNodeProperties(newPointNode, addInteractor, color, _ResliceView->getPCMRIRenderer());

	newPointNode->SetName("point.PCMRI");
	if (addInteractor) {
		newPointNode->SetBoolProperty("mapping.interactivePoint", true);
	}
	newPointNode->SetBoolProperty("mapping.pcmriPoint", true);


	// Finally, add the new node to hierarchy
	crimson::HierarchyManager::getInstance()->addNodeToHierarchy(
		_currentPCMRINode, crimson::VascularModelingNodeTypes::Image(), newPointNode,
		crimson::SolverSetupNodeTypes::PCMRIPoint());

	if (newPointNode != nullptr)
		_addPointInteractor(newPointNode, _ResliceView->getPCMRIRenderer());

	return newPointNode;
}

mitk::DataNode* PCMRIMappingWidget::_addMraPointNode(mitk::PointSet::Pointer point, bool addInteractor)
{
	// Add the data node for a new planar point
	auto newPointNode = mitk::DataNode::New();
	newPointNode->SetData(point);
	//newPointNode->SetBoolProperty("hidden object", true);

	assert(_ResliceView);
	// Set up the node properties
	float color[3] = { 0.5f, 1.0f, 0.5f };
	crimson::PCMRIUtils::setDefaultPointNodeProperties(newPointNode, addInteractor, color, _ResliceView->getResliceRenderer());

	newPointNode->SetName("point.MRA");
	if (addInteractor) {
		newPointNode->SetBoolProperty("mapping.interactivePoint", true);
	}
	newPointNode->SetBoolProperty("mapping.mraPoint", true);


	// Finally, add the new node to hierarchy
	crimson::HierarchyManager::getInstance()->addNodeToHierarchy(
		_currentNode, crimson::SolverSetupNodeTypes::BoundaryCondition(), newPointNode,
		crimson::SolverSetupNodeTypes::MRAPoint());

	if (newPointNode != nullptr)
		_addPointInteractor(newPointNode, _ResliceView->getResliceRenderer());

	return newPointNode;
}

void PCMRIMappingWidget::_connectDataStorageEvents()
{
	crimson::HierarchyManager::getInstance()->getDataStorage()->AddNodeEvent.AddListener(
		mitk::MessageDelegate1<PCMRIMappingWidget, const mitk::DataNode*>(this, &PCMRIMappingWidget::nodeAdded));

	crimson::HierarchyManager::getInstance()->getDataStorage()->RemoveNodeEvent.AddListener(
		mitk::MessageDelegate1<PCMRIMappingWidget, const mitk::DataNode*>(this, &PCMRIMappingWidget::nodeRemoved));
}

void PCMRIMappingWidget::_disconnectDataStorageEvents()
{
	crimson::HierarchyManager::getInstance()->getDataStorage()->AddNodeEvent.RemoveListener(
		mitk::MessageDelegate1<PCMRIMappingWidget, const mitk::DataNode*>(this, &PCMRIMappingWidget::nodeAdded));

	crimson::HierarchyManager::getInstance()->getDataStorage()->RemoveNodeEvent.RemoveListener(
		mitk::MessageDelegate1<PCMRIMappingWidget, const mitk::DataNode*>(this, &PCMRIMappingWidget::nodeRemoved));
}

//encapsulate this to work when called from SolverSetupView
void PCMRIMappingWidget::nodeAdded(const mitk::DataNode* node)
{
	//NodeDependentView::NodeAdded(node);

	// Old scenes - fix the visibility issues
	if (_ResliceView) {
		if (crimson::HierarchyManager::getInstance()
			->getPredicate(crimson::VascularModelingNodeTypes::ContourSegmentationImage())
			->CheckNode(node)) {
			for (mitk::BaseRenderer* rend : _ResliceView->getAllResliceRenderers()) {
				const_cast<mitk::DataNode*>(node)->SetVisibility(false, rend);
			}
		}
	}

	// Set the visibility for the thumbnail renderer
	if (_shouldBeInvisibleInThumbnailView->CheckNode(node)) {
		const_cast<mitk::DataNode*>(node)->SetVisibility(false, _thumbnailGenerator->getThumbnailRenderer());
	}

	auto hierarchyManager = crimson::HierarchyManager::getInstance();
	if (hierarchyManager->getAncestor(node, crimson::VascularModelingNodeTypes::Image(),true) == _currentPCMRINode) {
		if (hierarchyManager->getPredicate(crimson::VascularModelingNodeTypes::Contour())->CheckNode(node)) {
			// New contour has been added - add to thumbnail view and connect the observers
			_addContoursThumbnailsWidgetItem(node);
			_connectContourObservers(const_cast<mitk::DataNode*>(node));
			_UI.contourThumbnailListWidget->sortItems();
			// In case the addition is the undo of contour deletion - navigate to the contour
			navigateToContour(node);
			updateCurrentContour();
		}
		
		else if (hierarchyManager->getPredicate(crimson::SolverSetupNodeTypes::PCMRIPoint())->CheckNode(node)) {
			//_connectPointSetObservers(const_cast<mitk::DataNode*>(node));
			updateCurrentPcmriPoint();
		}
		else
		{
			updateCurrentContour();
		}
	}
	if (hierarchyManager->getAncestor(node, crimson::VascularModelingNodeTypes::Image()) == _currentPCMRINode) 
	{
		 if (hierarchyManager->getPredicate(crimson::VascularModelingNodeTypes::ContourSegmentationImage())->CheckNode(node))
		 {
			_connectSegmentationObservers(const_cast<mitk::DataNode*>(node));
			updateCurrentContour();
		 }
		 else
		 {
			 updateCurrentContour();
		 }
	}
	if (hierarchyManager->getAncestor(node, crimson::SolverSetupNodeTypes::BoundaryCondition()) == _currentNode) {

		if (hierarchyManager->getPredicate(crimson::SolverSetupNodeTypes::MRAPoint())->CheckNode(node)) {
			updateCurrentMraPoint();
		}
	}
	
}

void PCMRIMappingWidget::_addContoursThumbnailsWidgetItem(const mitk::DataNode* node)
{
	// Update the parameters to ensure correct sorting
	if (_ResliceView)
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

void PCMRIMappingWidget::navigateToContourByIndex(const QModelIndex& index)
{
	auto node = static_cast<ThumbnalListWidgetItem*>(_UI.contourThumbnailListWidget->item(index.row()))->node();
	navigateToContour(node);
}

void PCMRIMappingWidget::navigateToContour(const mitk::DataNode* node)
{
	if (!_ResliceView) {
		return;
	}

	if (_currentContourNode == node) {
		return;
	}

	for (auto iter = _parameterMap.begin(); iter != _parameterMap.end(); ++iter) {
		if (iter->second == node) {
			_ResliceView->navigateTo(iter->first);
			break;
		}
	}
}

void PCMRIMappingWidget::nodeRemoved(const mitk::DataNode* node)
{

	_nodeBeingDeleted = node;


	if (node == _currentPCMRINode) {
		_setCurrentContourNode(nullptr);
		_setSegmentationNodes(nullptr, nullptr);
		setPCMRIImageForMapping(nullptr);
	}

	if (node == _currentPCMRIPhaseNode) {
		setPCMRIImagePhaseForMapping(nullptr);
	}

	if (node == _currentPcmriPointNode) {
		if (_currentPcmriPointNode == nullptr)
			return;
		_setCurrentPcmriPointNode(nullptr);
	}

	if (node == _currentMraPointNode) {
		if (_currentMraPointNode == nullptr)
			return;
		_setCurrentMraPointNode(nullptr);
	}

	auto hierarchyManager = crimson::HierarchyManager::getInstance();
	if (hierarchyManager->getAncestor(node, crimson::VascularModelingNodeTypes::Image(), true) == _currentPCMRINode) {
		if (hierarchyManager->getPredicate(crimson::VascularModelingNodeTypes::Contour())->CheckNode(node)) {
			// Remove from parameter map if contour belongs to the current PC-MRI image
			for (auto iter = _parameterMap.begin(); iter != _parameterMap.end(); ++iter) {
				if (iter->second == node) {
					_parameterMap.erase(iter);
					break;
				}
			}
			_disconnectContourObservers(const_cast<mitk::DataNode*>(node));
			delete _findThumbnailsWidgetItemByNode(node);
			//_requestLoftPreview();

			// Cancel the thumbnail generation request if it has been issued
			_thumbnailGenerator->cancelThumbnailRequest(node);

			updateCurrentContour();
		}
		else if (hierarchyManager->getPredicate(crimson::VascularModelingNodeTypes::ContourSegmentationImage())
			->CheckNode(node)) {
			_disconnectSegmentationObservers(const_cast<mitk::DataNode*>(node));

			// Cancel the thumbnail generation request if it has been issued
			_thumbnailGenerator->cancelThumbnailRequest(node);

			updateCurrentContour();
		}
		else if (hierarchyManager->getPredicate(crimson::SolverSetupNodeTypes::PCMRIPoint())->CheckNode(node)) {
			updateCurrentPcmriPoint();
		}
	}
	if (hierarchyManager->getAncestor(node, crimson::SolverSetupNodeTypes::BoundaryCondition()) == _currentNode) {

		if (hierarchyManager->getPredicate(crimson::SolverSetupNodeTypes::MRAPoint())->CheckNode(node)) {
			updateCurrentMraPoint();
		}
	}
	if (hierarchyManager->getPredicate(crimson::VesselMeshingNodeTypes::Mesh())->CheckNode(node))
	{
		auto childPCMRI = hierarchyManager->getFirstDescendant(_currentNode, crimson::SolverSetupNodeTypes::PCMRIData(), true);
		if (childPCMRI)
		{
			if (QMessageBox::question(nullptr, "Delete mesh?",
				"Deleting this mesh will also remove the results of the PC-MRI mapping associated with the mesh. Are you sure you want to do this?"
				, QMessageBox::Yes,
				QMessageBox::No) == QMessageBox::Yes) {
				crimson::HierarchyManager::getInstance()->getDataStorage()->Remove(childPCMRI);
			}
			else
			{
				crimson::HierarchyManager::getInstance()->getDataStorage()->Add(_currentMeshNode, _currentSolidNode);
			}
		}
	}

	_nodeBeingDeleted = nullptr;
}

void PCMRIMappingWidget::_addPlanarFigureInteractor(mitk::DataNode* node)
{
	bool isInteractiveContour = false;
	if (node->GetBoolProperty("lofting.interactiveContour", isInteractiveContour) && isInteractiveContour) {
		//node->SetVisibility(false);
		//node->SetVisibility(true, _ResliceView->getPCMRIRenderer());
		auto figureInteractor = mitk::PlanarFigureInteractor::New();
		us::Module* planarFigureModule = us::ModuleRegistry::GetModule("MitkPlanarFigure");
		figureInteractor->LoadStateMachine("PlanarFigureInteraction.xml", planarFigureModule);
		figureInteractor->SetEventConfig("PlanarFigureConfig.xml", planarFigureModule);
		figureInteractor->SetDataNode(node);
	}
}

void PCMRIMappingWidget::_addPointInteractor(mitk::DataNode* node, mitk::BaseRenderer* renderer)
{
	bool isInteractivePoint = false;
	if (node->GetBoolProperty("mapping.interactivePoint", isInteractivePoint) && isInteractivePoint) {
		node->SetSelected(true, renderer);
		node->SetVisibility(true, renderer);
		renderer->RequestUpdate();
		auto pointInteractor = mitk::SinglePointDataInteractor::New();
		us::Module* solverModule = us::ModuleRegistry::GetModule("SolverSetupService");
		//us::Module* pointSetModule = us::ModuleRegistry::GetModule("MitkPointSet");
		pointInteractor->LoadStateMachine("PointSet.xml");
		pointInteractor->SetEventConfig("PointSetConfigCRIMSON.xml", solverModule);
		pointInteractor->SetDataNode(node);

	}
}

mitk::Point2D PCMRIMappingWidget::_getPlanarFigureCenter(const mitk::DataNode* figureNode) const
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

void PCMRIMappingWidget::_updatePlanarFigureParameters(bool forceUpdate)
{
	_parameterMap = crimson::PCMRIUtils::updatePlanarFigureParameters(_currentPCMRINode, _nodeBeingDeleted, _ResliceView->getPCMRIRenderer(), forceUpdate);
}

void PCMRIMappingWidget::doMap()
{
	int nSlices;
	if (!_currentPCMRINode->GetData()->GetPropertyList()->GetIntProperty("PAR.MaxNumberOfCardiacPhases", nSlices))
		//if this field does not exist, it's a DICOM file
		nSlices = dynamic_cast<mitk::Image*>(_currentPCMRINode->GetData())->GetDimensions()[3];
	
	std::vector<mitk::DataNode*> contourNodesAll = crimson::PCMRIUtils::getContourNodesSortedByParameter(_currentPCMRINode);
	if (contourNodesAll.size() < nSlices)
		interpolateAllContours();

	double profileSmoothness = _UI.gaussianDoubleSpinBox->value();
	int intSmoothness;
	if (_currentNode->GetIntProperty("mapping.smoothness", intSmoothness))
	{
		mitk::BaseProperty::Pointer doubleProp = mitk::DoubleProperty::New(profileSmoothness);
		_currentNode->ReplaceProperty("mapping.smoothness", doubleProp);
	}
	else
		_currentNode->SetDoubleProperty("mapping.smoothness", profileSmoothness);

	MapAction().Run(_currentNode);
}

void PCMRIMappingWidget::mappingFinished(crimson::async::Task::State state)
{
	if (state == crimson::async::Task::State_Finished) {
		mitk::DataNode::Pointer PCMRIDataNode = crimson::HierarchyManager::getInstance()->getFirstDescendant(
					_currentNode, crimson::SolverSetupNodeTypes::PCMRIData());

		auto uid = std::string{};
		PCMRIDataNode->GetStringProperty(crimson::HierarchyManager::nodeUIDPropertyName, uid);
		dynamic_cast<IBoundaryCondition*>(_currentNode->GetData())->setDataUID(uid);


		PCMRIDataNode->SetVisibility(_ResliceView->getPCMRIRenderer(), false);

		if (_ResliceView)
			_ResliceView->navigateTo(dynamic_cast<PCMRIData*>(PCMRIDataNode->GetData())->getVisualizationParameters()._maxIndex);

		_currentSolidNode->SetOpacity(0.3);
	}
	_updateUI();
}


std::pair<mitk::DataNode::Pointer, mitk::DataNode::Pointer>
PCMRIMappingWidget::_createSegmentationNodes(mitk::Image* image, const mitk::PlaneGeometry* sliceGeometry,
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
	extractor->SetTimeStep(_ResliceView->getPCMRIRenderer()->GetTimeStep()); //TODO: pass parameter here in case of interpolation
	extractor->SetWorldGeometry(sliceGeometry);
	extractor->SetVtkOutputRequest(false);
	//extractor->SetResliceTransformByGeometry(image->GetTimeGeometry()->GetGeometryForTimeStep(_ResliceView->getPCMRIRenderer()->GetTimeStep()));
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

	float color[3] = { 1, 0, 0 };
	mitk::DataNode::Pointer emptySegmentation =
		firstTool->CreateEmptySegmentationNode(slice, "segmented slice", mitk::Color(color));
	emptySegmentation->SetBoolProperty("hidden object", true);
	emptySegmentation->SetVisibility(false);
	emptySegmentation->SetVisibility(true, _ResliceView->getPCMRIRenderer());
	emptySegmentation->SetBoolProperty("lofting.contour_segmentation_image", true);

	if (figureToFill != nullptr) {
		// Attempt conversion
		auto segmentationImage = static_cast<mitk::Image*>(emptySegmentation->GetData());
		_fillPlanarFigureInSlice(segmentationImage, figureToFill);
	}

	return std::make_pair(sliceNode, emptySegmentation);
}

mitk::DataNode* PCMRIMappingWidget::createSegmented(bool startNewUndoGroup)
{
	cancelInteraction();
	assert(_currentSegmentationWorkingImageNode == nullptr);

	if (!_currentPCMRINode) {
		return nullptr;
	}

	auto contour = mitk::PlanarPolygon::New();
	contour->PlaceFigure(mitk::Point2D());
	contour->SetFinalized(true);

	assert(_ResliceView);

	mitk::BaseRenderer* renderer = _ResliceView->getPCMRIRenderer();

	const mitk::PlaneGeometry* geometry = _ResliceView->getPlaneGeometry(_ResliceView->getCurrentParameterValue());
	contour->SetPlaneGeometry(const_cast<mitk::PlaneGeometry*>(geometry));

		std::pair<mitk::DataNode::Pointer, mitk::DataNode::Pointer> segmentationImagesPair = _createSegmentationNodes(
		dynamic_cast<mitk::Image*>(_currentPCMRINode->GetData()), geometry,
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

	return contourNode;
}

void PCMRIMappingWidget::_removeCurrentContour()
{
	if (!_currentContourNode || _currentContourNode == _nodeBeingDeleted) {
		return;
	}

	crimson::HierarchyManager::getInstance()->getDataStorage()->Remove(_currentContourNode);
}

void PCMRIMappingWidget::_removeCurrentPcmriPoint()
{
	if (!_currentPcmriPointNode || _currentPcmriPointNode == _nodeBeingDeleted) {
		return;
	}

	crimson::HierarchyManager::getInstance()->getDataStorage()->Remove(_currentPcmriPointNode);
}

void PCMRIMappingWidget::_removeCurrentMraPoint()
{
	if (!_currentMraPointNode || _currentMraPointNode == _nodeBeingDeleted) {
		return;
	}

	crimson::HierarchyManager::getInstance()->getDataStorage()->Remove(_currentMraPointNode);
}


void PCMRIMappingWidget::_setSegmentationNodes(mitk::DataNode* refNode, mitk::DataNode* workNode)
{
	if (_currentSegmentationReferenceImageNode == refNode && _currentSegmentationWorkingImageNode == workNode) {
		return;
	}

	if (_ResliceView && _currentSegmentationWorkingImageNode) {
		for (mitk::BaseRenderer* rend : _ResliceView->getAllResliceRenderers()) {
			_currentSegmentationWorkingImageNode->SetVisibility(false, rend);
		}
	}

	_currentSegmentationReferenceImageNode = refNode;
	_currentSegmentationWorkingImageNode = workNode;

	if (_ResliceView && _currentSegmentationWorkingImageNode) {
		_currentSegmentationWorkingImageNode->SetVisibility(true, _ResliceView->getPCMRIRenderer());
	}

	mitk::ToolManagerProvider::GetInstance()->GetToolManager()->SetReferenceData(_currentSegmentationReferenceImageNode);
	mitk::ToolManagerProvider::GetInstance()->GetToolManager()->SetWorkingData(_currentSegmentationWorkingImageNode);
}

void PCMRIMappingWidget::_setCurrentContourNode(mitk::DataNode* node)
{
	if (_currentContourNode == node) {
		return;
	}

	if (_currentContourNode) {
		cancelInteraction();
		//no idea why this has to be hard-coded when a separate rendering manager is used, but it has to be
		_currentContourNode->SetVisibility(false);
		if (_ResliceView)
			_currentContourNode->SetVisibility(false, _ResliceView->getPCMRIRenderer());
		if (_currentContourNode->GetDataInteractor()) {
			_currentContourNode->GetDataInteractor()->SetDataNode(nullptr);
		}
	}

	_currentContourNode = node;

	if (_currentContourNode) {
		if (_ResliceView)
			_currentContourNode->SetVisibility(true, _ResliceView->getPCMRIRenderer());
		_addPlanarFigureInteractor(_currentContourNode);

		auto planarFigure = static_cast<mitk::PlanarFigure*>(_currentContourNode->GetData());
		if (!planarFigure->IsFinalized()) {
			_contourTypeToButtonMap[planarFigure->GetNameOfClass()]->setChecked(true);
		}
	}
	_updateContourListViewSelection();
	updateCurrentContourInfo();
}

void PCMRIMappingWidget::_setCurrentPcmriPointNode(mitk::DataNode* node)
{
	if (_currentPcmriPointNode == node) {
		//if the node is not NULL but doesn't have an interactor
		if (_currentPcmriPointNode && _ResliceView)
		{
				_currentPcmriPointNode->SetVisibility(true, _ResliceView->getPCMRIRenderer());
		}

		return;
	}

	//we're switching point sets (probably because  BC changed as well)
	if (_currentPcmriPointNode != nullptr) {
		if (node != nullptr)
			cancelInteractionPoints();
		//no idea why this has to be hard-coded when a separate rendering manager is used, but it has to be
		_currentPcmriPointNode->SetVisibility(false);
		if (_ResliceView)
			_currentPcmriPointNode->SetVisibility(false, _ResliceView->getPCMRIRenderer());
		if (_currentPcmriPointNode->GetDataInteractor()) {
			_currentPcmriPointNode->GetDataInteractor()->SetDataNode(nullptr);
		}
	}

	_currentPcmriPointNode = node;

	if (_currentPcmriPointNode != nullptr && _ResliceView) {
		_currentPcmriPointNode->SetVisibility(true, _ResliceView->getPCMRIRenderer());
	}
}

void PCMRIMappingWidget::_setCurrentMraPointNode(mitk::DataNode* node)
{
	if (_currentMraPointNode == node) {
		//if the node is not NULL but doesn't have an interactor
		if (_currentMraPointNode && _ResliceView)
		{			
			_currentMraPointNode->SetVisibility(true, _ResliceView->getResliceRenderer());	
			
		}
		return;
	}

	if (_currentMraPointNode != nullptr) {
		if (node != nullptr)
			cancelInteractionPoints();
		//no idea why this has to be hard-coded when a separate rendering manager is used, but it has to be
		_currentMraPointNode->SetVisibility(false);
		if (_ResliceView)
			_currentMraPointNode->SetVisibility(false, _ResliceView->getResliceRenderer());
		if (_currentMraPointNode->GetDataInteractor()) {
			_currentMraPointNode->GetDataInteractor()->SetDataNode(nullptr);
		}
	}

	_currentMraPointNode = node;
	if (_currentMraPointNode != nullptr && _ResliceView) {
		_currentMraPointNode->SetVisibility(true, _ResliceView->getResliceRenderer());
	}

}

void PCMRIMappingWidget::updateContourSmoothnessFromUI()
{
	_currentContourNode->SetIntProperty("lofting.smoothness", _UI.smoothnessSlider->value());
	_updateContourFromSegmentation(_currentContourNode);
}

void PCMRIMappingWidget::_updateContourFromSegmentationData(const itk::Object* segmentationData)
{
	mitk::BaseData* data = const_cast<mitk::BaseData*>(static_cast<const mitk::BaseData*>(segmentationData));

	mitk::DataNode* workingImageNode = crimson::HierarchyManager::getInstance()->getDataStorage()->GetNode(mitk::NodePredicateData::New(data));
	assert(workingImageNode);
	mitk::DataNode* contourNode =
		crimson::HierarchyManager::getInstance()->getAncestor(workingImageNode, crimson::VascularModelingNodeTypes::Contour());
	assert(contourNode);
	_updateContourFromSegmentation(contourNode);

	mitk::DataNode* pcmriImageNode =
		crimson::HierarchyManager::getInstance()->getAncestor(contourNode, crimson::VascularModelingNodeTypes::Image(),true);
	assert(pcmriImageNode);

	if (pcmriImageNode == _currentPCMRINode) {
		// If the change happened due to undo/redo - navigate to the contour
		// Has no effect if the changes happened to the current contour
		navigateToContour(contourNode);
	}
}

void PCMRIMappingWidget::_updateContourFromSegmentation(mitk::DataNode* contourNode)
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

	if (_ResliceView)
		_ResliceView->getPCMRIRenderer()->GetRenderingManager()->RequestUpdateAll();
}

float PCMRIMappingWidget::_getContourSmoothnessFromNode(mitk::DataNode* contourNode)
{
	int smoothness = 0;
	contourNode->GetIntProperty("lofting.smoothness", smoothness);
	return (float)smoothness / _UI.smoothnessSlider->maximum();
}

void PCMRIMappingWidget::_createSmoothedContourFromVtkPolyData(vtkPolyData* polyData, mitk::PlanarPolygon* contour,
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
	}
	else {
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

void PCMRIMappingWidget::setToolInformation_Segmentation(int id)
{
	if (id >= 0) {
		mitk::ToolManager* toolManager = mitk::ToolManagerProvider::GetInstance()->GetToolManager();
		us::ModuleResourceStream cursor(toolManager->GetToolById(id)->GetCursorIconResource(), std::ios::binary);
		setToolInformation(cursor, toolManager->GetToolById(id)->GetName());
	}
	else {
		resetToolInformation();
	}
}

void PCMRIMappingWidget::setToolInformation_Manual(const char* name)
{
	static std::unordered_map<const char*, const char*> typeNameToHumanReadableNameMap = {
		{ mitk::PlanarCircle::GetStaticNameOfClass(), "Circle" },
		{ mitk::PlanarEllipse::GetStaticNameOfClass(), "Ellipse" },
		{ mitk::PlanarSubdivisionPolygon::GetStaticNameOfClass(), "Polygon" },
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

void PCMRIMappingWidget::setToolInformation_Point(const char* name)
{
	// Set up the cursor - hard coded for corresponding points
	QPixmap pix(QString(":/contourModeling/cursors/Point_cursor.png"));
	QBuffer buf;
	pix.save(&buf, "PNG");
	std::string data(buf.data().constData(), buf.data().length());
	std::istringstream cursor(data, std::ios_base::binary | std::ios_base::in);

	setToolInformation(cursor, name);
	_currentPointButton = _pointTypeToButtonMap[name];
}

void PCMRIMappingWidget::resetToolInformation()
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

void PCMRIMappingWidget::resetPointToolInformation()
{
	_removeCancelInteractionShortcutPoint();
	if (_currentPointButton) {
		_currentPointButton->blockSignals(true);
		_currentPointButton->setChecked(false);
		_currentPointButton->blockSignals(false);
		_currentPointButton = nullptr;
	}

	if (_cursorSet) {
		mitk::ApplicationCursor::GetInstance()->PopCursor();
		mitk::StatusBar::GetInstance()->DisplayText("");
		_cursorSet = false;
	}
}

void PCMRIMappingWidget::setToolInformation(std::istream& cursorStream, const char* name)
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

void PCMRIMappingWidget::setPointToolInformation(std::istream& cursorStream, const char* name)
{
	// Remove previously set mouse cursor
	resetPointToolInformation();

	_createCancelInteractionShortcutPoint();

	// Set up status bar text
	std::string text = std::string("Active Tool: \"") + name + "\"";
	mitk::StatusBar::GetInstance()->DisplayText(text.c_str());

	// Set the cursor
	mitk::ApplicationCursor::GetInstance()->PushCursor(cursorStream, 0, 0);
	_cursorSet = true;
}

void PCMRIMappingWidget::setNodeThumbnail(mitk::DataNode::ConstPointer node, QImage thumbnail)
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

ThumbnalListWidgetItem* PCMRIMappingWidget::_findThumbnailsWidgetItemByNode(const mitk::DataNode* node)
{
	for (int i = 0; i < _UI.contourThumbnailListWidget->count(); ++i) {
		auto item = static_cast<ThumbnalListWidgetItem*>(_UI.contourThumbnailListWidget->item(i));
		if (item->node() == node) {
			return item;
		}
	}

	return nullptr;
}

QImage PCMRIMappingWidget::_getNodeThumbnail(const mitk::DataNode* node)
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

void PCMRIMappingWidget::reinitAllContourGeometries(ContourGeometyReinitType reinitType)
{
	if (!_currentPCMRINode) {
		return;
	}

	// Adjust all contour geometries. The adjustment strategy depends on the type of event
	// which triggered the reinitialization

	mitk::DataStorage::SetOfObjects::ConstPointer contours =
		crimson::PCMRIUtils::getContourNodes(_currentPCMRINode);

	_updatePlanarFigureParameters();

	_ignoreContourChangeEvents = true;
	for (const mitk::DataNode::Pointer& contourNode : *contours) {
		auto figure = static_cast<mitk::PlanarFigure*>(contourNode->GetData());

		float t;
		contourNode->GetFloatProperty("mapping.parameterValue", t);

		const mitk::PlaneGeometry* geometry =
			dynamic_cast<const mitk::PlaneGeometry*>(_ResliceView->getPlaneGeometry(t));
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
		}

		// Re-create segmentation images from the updated contours
		std::pair<mitk::DataNode*, mitk::DataNode*> oldSegmentationNodes = _getSegmentationNodes(contourNode);

		if (oldSegmentationNodes.first != nullptr && _currentPCMRINode) {
			std::pair<mitk::DataNode::Pointer, mitk::DataNode::Pointer> newSegmentationNodes =
				_createSegmentationNodes(static_cast<mitk::Image*>(_currentPCMRINode->GetData()), planeGeometry, figure);

			static_cast<mitk::Image*>(oldSegmentationNodes.first->GetData())
				->SetGeometry(static_cast<mitk::Image*>(newSegmentationNodes.first->GetData())->GetGeometry()->Clone());
			static_cast<mitk::Image*>(oldSegmentationNodes.second->GetData())
				->SetGeometry(static_cast<mitk::Image*>(newSegmentationNodes.second->GetData())->GetGeometry()->Clone());
		}

		// Create new thumbnails
		_requestThumbnail(contourNode.GetPointer());
	}
	_ignoreContourChangeEvents = false;
}

void PCMRIMappingWidget::_contourChanged(const itk::Object* contourData)
{
	if (_ignoreContourChangeEvents) {
		return;
	}

	mitk::BaseData* data = const_cast<mitk::BaseData*>(static_cast<const mitk::BaseData*>(contourData));
	mitk::DataNode* node = crimson::HierarchyManager::getInstance()->getDataStorage()->GetNode(mitk::NodePredicateData::New(data));

	if (node) {
		_requestThumbnail(node);

		mitk::DataNode* pcmriNode =
			crimson::HierarchyManager::getInstance()->getAncestor(node, crimson::VascularModelingNodeTypes::Image(), true);
		assert(pcmriNode);

		if (pcmriNode == _currentPCMRINode) {
			// Navigate to contour in case the change came from undo/redo
			navigateToContour(node);
		}

		if (node == _currentContourNode) {
			_contourInfoUpdateTimer.start(250);
		}
	}
}

void PCMRIMappingWidget::_figurePlacementCancelled(const itk::Object* contourData)
{
	// Undo has reached the placement event - set the state back to "adding a new contour"
	cancelInteraction(true);

	if (!_currentContourNode || _currentContourNode->GetData() != contourData) {
		return;
	}

	_contourTypeToButtonMap[contourData->GetNameOfClass()]->setChecked(true);
}

void PCMRIMappingWidget::duplicateContour()
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
	float curParameter = _ResliceView->getCurrentParameterValue();
	for (QListWidgetItem* item : selection) {
		const mitk::DataNode* node = static_cast<ThumbnalListWidgetItem*>(item)->node();
		float t;
		node->GetFloatProperty("mapping.parameterValue", t);
		float distance = fabs(curParameter - t);
		if (distance < minDistance) {
			minDistance = distance;
			closestNode = node;
		}
	}

	bool isExtendable = false;
	closestNode->GetBoolProperty("planarfigure.isextendable", isExtendable);

	auto clonedFigure = static_cast<mitk::PlanarFigure*>(closestNode->GetData())->Clone();

	bool isSegmented = crimson::HierarchyManager::getInstance()->getDataStorage()->GetDerivations(closestNode)->size() != 0;

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

void PCMRIMappingWidget::interpolateContour()
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

	const mitk::PlaneGeometry* geometry = _ResliceView->getPCMRIRenderer()->GetCurrentWorldPlaneGeometry();

	const int nInterpolationSlices = 100;
	const unsigned int dimensions[] = { static_cast<unsigned int>(geometry->GetBounds()[1]),
		static_cast<unsigned int>(geometry->GetBounds()[3]) };

	const mitk::ScalarType spacing[] = { geometry->GetSpacing()[0], geometry->GetSpacing()[1], geometry->GetSpacing()[2] };

	mitk::Point3D origin;
	origin[0] = -spacing[0] * dimensions[0] / 2.0;
	origin[1] = -spacing[1] * dimensions[1] / 2.0;
	origin[2] = -spacing[0] / 2.0;

	// Step 1: get the segmentation images for figures
	mitk::PixelType pixelType(mitk::MakeScalarPixelType<unsigned char>());

	for (int i = 0; i < 2; ++i) {
		contourNodes[i] = static_cast<ThumbnalListWidgetItem*>(selection[i])->node();
		contourNodes[i]->GetFloatProperty("mapping.parameterValue", parameterValues[i]);
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

	float currentParameterValue = _ResliceView->getCurrentParameterValue();
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

void PCMRIMappingWidget::_interpolateContourOne(mitk::DataNode* contourNodes[2], float param)
{
	_ResliceView->navigateTo(param);

	//check if a contour with this parameter already exists (if we passed it in the function) and remove it before proceeding
	bool contourRemoved = false;


	if (_currentContourNode) {
		_removeCurrentContour(); // Might have side effect of changing the selection
		contourRemoved = true;
	}

	_updatePlanarFigureParameters();

	float parameterValues[2];
	mitk::Image::Pointer contourSegmentedImages[2];

	const mitk::PlaneGeometry* geometry = _ResliceView->getPCMRIRenderer()->GetCurrentWorldPlaneGeometry();

	const int nInterpolationSlices = 100;
	const unsigned int dimensions[] = { static_cast<unsigned int>(geometry->GetBounds()[1]),
		static_cast<unsigned int>(geometry->GetBounds()[3]) };

	const mitk::ScalarType spacing[] = { geometry->GetSpacing()[0], geometry->GetSpacing()[1], geometry->GetSpacing()[2] };

	mitk::Point3D origin;
	origin[0] = -spacing[0] * dimensions[0] / 2.0;
	origin[1] = -spacing[1] * dimensions[1] / 2.0;
	origin[2] = -spacing[0] / 2.0;

	// Step 1: get the segmentation images for figures
	mitk::PixelType pixelType(mitk::MakeScalarPixelType<unsigned char>());

	for (int i = 0; i < 2; ++i) {
		contourNodes[i]->GetFloatProperty("mapping.parameterValue", parameterValues[i]);
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

	int nSlices;
	if (!_currentPCMRINode->GetData()->GetPropertyList()->GetIntProperty("PAR.MaxNumberOfCardiacPhases", nSlices))
		//if this field does not exist, it's a DICOM file
		nSlices = dynamic_cast<mitk::Image*>(_currentPCMRINode->GetData())->GetDimensions()[3];
	if (parameterValues[1] == 0)
		parameterValues[1] = nSlices;
	if (parameterValues[0] == nSlices - 1)
		parameterValues[0] = -1;

	float currentParameterValue = param;
	float interpolationFactor = abs((currentParameterValue - parameterValues[0]) / (parameterValues[1] - parameterValues[0]));

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

	auto newNode = createSegmented(!contourRemoved);
	newNode->SetBoolProperty("mapping.interpolated", true);
	//newNode->SetFloatProperty("mapping.parameterValue", param);

	contour->SetPlaneGeometry(static_cast<mitk::PlanarFigure*>(_currentContourNode->GetData())->GetPlaneGeometry()->Clone());
	_fillPlanarFigureInSlice(static_cast<mitk::Image*>(_currentSegmentationWorkingImageNode->GetData()), contour);

	// END DUPLICATION
	//////////////////////////////////////////////////////////////////////////
}

void PCMRIMappingWidget::interpolateAllContours()
{
	int nSlices;
	if (!_currentPCMRINode->GetData()->GetPropertyList()->GetIntProperty("PAR.MaxNumberOfCardiacPhases", nSlices))
		//if this field does not exist, it's a DICOM file
		nSlices = dynamic_cast<mitk::Image*>(_currentPCMRINode->GetData())->GetDimensions()[3];

	std::vector<mitk::DataNode*> contourNodesAll = crimson::PCMRIUtils::getContourNodesSortedByParameter(_currentPCMRINode);

	if (contourNodesAll.size() < 2) {
		QMessageBox::information(nullptr, "Create contours", "Please first create two contours to interpolate.", QMessageBox::Ok);
		return;
	}

	//check if we have contours in each slice

	if (contourNodesAll.size() < nSlices)
	{
		//if some slices are missing contours, generate them by interpolation and mark this in property list of their node
		//_allContours = false;

		int parameter = 0;
		for (std::vector<mitk::DataNode*>::size_type it = 0; it < contourNodesAll.size(); ++it)
		{
			auto pf = static_cast<mitk::PlanarFigure*>(static_cast<mitk::DataNode*>(contourNodesAll.at(it))->GetData());
			float p;
			static_cast<mitk::DataNode*>(contourNodesAll.at(it))->GetFloatProperty("mapping.parameterValue", p);
			if (p != parameter)
			{
				mitk::DataNode* contourNodeLeft;
				mitk::DataNode* contourNodeRight;

				//contour missing at this time slice
				if (parameter == 0)
				{
					//first few slices are missing, have to fill them up
					contourNodeLeft = contourNodesAll.back();
					contourNodeRight = contourNodesAll.at(it);
				}
				else
				{
					//normal insertion
					contourNodeLeft = contourNodesAll.at(it-1);
					contourNodeRight = contourNodesAll.at(it);
				}

				mitk::DataNode* contourNodes[2];
				contourNodes[0] = contourNodeLeft;
				contourNodes[1] = contourNodeRight;

				_interpolateContourOne(contourNodes, parameter);
			}
			parameter++;
			contourNodesAll = crimson::PCMRIUtils::getContourNodesSortedByParameter(_currentPCMRINode);
		}

		//check if we reached the end but not defined all contours

		while (parameter != nSlices)
		{
			mitk::DataNode* contourNodeLeft = contourNodesAll.back();
			mitk::DataNode* contourNodeRight = contourNodesAll.front();
			mitk::DataNode* contourNodes[2];
			contourNodes[0] = contourNodeLeft;
			contourNodes[1] = contourNodeRight;
			_interpolateContourOne(contourNodes, parameter);
			contourNodesAll = crimson::PCMRIUtils::getContourNodesSortedByParameter(_currentPCMRINode);
			parameter++;
		}

	}

	else
		//loop through all previously interpolated slices and interpolate them again
	{
		//_allContours = true;
		for (std::vector<mitk::DataNode*>::iterator it = contourNodesAll.begin(); it != contourNodesAll.end(); ++it)
		{
			bool interpolated = false;
			if ((*it)->GetBoolProperty("mapping.interpolated", interpolated))
			{
				mitk::DataNode* contourNodeLeft = nullptr;
				mitk::DataNode* contourNodeRight = nullptr;
				//find left non-interpolated node
				for (std::vector<mitk::DataNode*>::iterator jt = it; jt >= contourNodesAll.begin(); --jt)
				{
					if (!(*jt)->GetBoolProperty("mapping.interpolated", interpolated))
					{
						contourNodeLeft = *jt;
						break;
					}
				}
				//go from the end to find left value
				if (contourNodeLeft == nullptr)
				{
					for (std::vector<mitk::DataNode*>::iterator jt = contourNodesAll.end()-1; jt != it; --jt)
					{
						if (!(*jt-1)->GetBoolProperty("mapping.interpolated", interpolated))
						{
							contourNodeLeft = *jt;
							break;
						}
					}
				}
				//find right non-interpolated node
				for (std::vector<mitk::DataNode*>::iterator jt = it + 1; jt != contourNodesAll.end(); ++jt)
				{
					if (!(*jt)->GetBoolProperty("mapping.interpolated", interpolated))
					{
						contourNodeRight = *jt;
						break;
					}
				}
				//go from the beginning to find right value
				if (contourNodeRight == nullptr)
				{
					for (std::vector<mitk::DataNode*>::iterator jt = contourNodesAll.begin(); jt != it; ++jt)
					{
						if (!(*jt)->GetBoolProperty("mapping.interpolated", interpolated))
						{
							contourNodeRight = *jt;
							break;
						}
					}
				}

				mitk::DataNode* contourNodes[2];
				contourNodes[0] = contourNodeLeft;
				contourNodes[1] = contourNodeRight;
				float paramLeft, paramRight = 0;
				contourNodeLeft->GetFloatProperty("mapping.parameterValue", paramLeft);
				contourNodeRight->GetFloatProperty("mapping.parameterValue", paramRight);
				float parameter = 0;
				(*it)->GetFloatProperty("mapping.parameterValue", parameter);
				_interpolateContourOne(contourNodes, parameter);
			}
			else
				continue;

		}		
	}
}

void PCMRIMappingWidget::_fillPlanarFigureInSlice(mitk::Image* segmentationImage, mitk::PlanarFigure* figure)
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

void PCMRIMappingWidget::deleteSelectedContours()
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
		crimson::HierarchyManager::getInstance()->getDataStorage()->Remove(nodeToDelete);
	}
	crimson::HierarchyManager::getInstance()->setStartNewUndoGroup(true);
}


void PCMRIMappingWidget::updateCurrentContourInfo()
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

void PCMRIMappingWidget::_requestThumbnail(mitk::DataNode::ConstPointer node)
{
	mitk::SliceNavigationController* tnc;
	if (_ResliceView)
		tnc = _ResliceView->getPCMRIRenderer()->GetRenderingManager()->GetTimeNavigationController();
	else
		tnc = mitk::RenderingManager::GetInstance()->GetTimeNavigationController();
	mitk::TimePointType time = tnc->GetInputWorldTimeGeometry()->TimeStepToTimePoint(tnc->GetTime()->GetPos());

	_thumbnailGenerator->requestThumbnail(node, _currentPCMRINode.GetPointer(), time);
}



void PCMRIMappingWidget::setMeshNodeForMapping(const mitk::DataNode* node)
{
	_updateUI();
	if (!node) {
		return;
	}
	_currentMeshNode = const_cast<mitk::DataNode*>(node);
	auto meshUID = std::string{};
	node->GetStringProperty(crimson::HierarchyManager::nodeUIDPropertyName, meshUID);
	_currentNode->SetStringProperty("mapping.meshnode", meshUID.c_str());

}

void PCMRIMappingWidget::setMRAImageForMapping(const mitk::DataNode* node)
{
	_updateUI();
	if (!node) {
		return;
	}
	_currentMRANode = const_cast<mitk::DataNode*>(node);

	if (_currentMRANode)
	{
		if (_ResliceView)
			_ResliceView->_setupMRASlice(_currentMRANode);
		
		//store MRA UID as a property of the BC node for persistence
		auto mraUID = std::string{};
		node->GetStringProperty(crimson::HierarchyManager::nodeUIDPropertyName, mraUID);
		_currentNode->SetStringProperty("mapping.mranode", mraUID.c_str());
	}

}

void PCMRIMappingWidget::setPCMRIImageForMapping(const mitk::DataNode* node)
{
	_updateUI();
	if (!node) {
		return;
	}

	if (_currentPCMRINode != nullptr)
		_disconnectAllContourObservers();

	_currentPCMRINode = const_cast<mitk::DataNode*>(node);

	if (_currentPCMRINode)
	{
		if (_ResliceView)
			_ResliceView->_setupPCMRISlices(_currentPCMRINode);

		auto pcmriUID = std::string{};
		node->GetStringProperty(crimson::HierarchyManager::nodeUIDPropertyName, pcmriUID);
		_currentNode->SetStringProperty("mapping.pcmrinode", pcmriUID.c_str());

		auto test = dynamic_cast<mitk::Image*>(_currentPCMRINode->GetData())->GetTimeGeometry();
		auto test2 = dynamic_cast<mitk::Image*>(_currentPCMRINode->GetData())->GetTimeGeometry()->Clone();
		auto test3 = _getFlippedGeometry(_currentPCMRINode);

		_pcmriGeometryOriginal = dynamic_cast<mitk::Image*>(_currentPCMRINode->GetData())->GetTimeGeometry()->Clone();
		_pcmriGeometryFlipped = _getFlippedGeometry(_currentPCMRINode);

		bool flagImage = false;
		_currentPCMRINode->GetBoolProperty("mapping.imageFlipped", flagImage);
		if (flagImage)
		{
			dynamic_cast<mitk::Image*>(_currentPCMRINode->GetData())->SetTimeGeometry(_pcmriGeometryFlipped);
			if (_currentPCMRIPhaseNode)
				dynamic_cast<mitk::Image*>(_currentPCMRIPhaseNode->GetData())->SetTimeGeometry(_pcmriGeometryFlipped);
		}
		else
		{
			dynamic_cast<mitk::Image*>(_currentPCMRINode->GetData())->SetTimeGeometry(_pcmriGeometryOriginal);
			if (_currentPCMRIPhaseNode)
				dynamic_cast<mitk::Image*>(_currentPCMRIPhaseNode->GetData())->SetTimeGeometry(_pcmriGeometryOriginal);
		}
		//we want to keep the previous state of the flip image checkbox
		bool flagMenu = _UI.flipImageCheckBox->isChecked();
		if (flagImage != flagMenu)
			_UI.flipImageCheckBox->setChecked(flagImage);

		// Adapt for changes that may have happened when the view was closed (e.g. undo of an operation)
		mitk::DataStorage::SetOfObjects::ConstPointer nodes =
			crimson::PCMRIUtils::getContourNodes(_currentPCMRINode);
		// Connect the data observers for contours
		_connectAllContourObservers(nodes);
		if (!nodes->empty())
		{

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
		}

		//check if a PCMRI point was previously set
		mitk::DataNode::Pointer pcmriNode = crimson::PCMRIUtils::getPcmriPointNode(_currentPCMRINode);
		if (pcmriNode)
		{
			_setCurrentPcmriPointNode(pcmriNode);
			auto test = pcmriNode.GetPointer();
		}
		

		//it seems that all the objects in the storage are by default set as visible in a new renderer window

		auto page = berry::PlatformUI::GetWorkbench()->GetActiveWorkbenchWindow()->GetActivePage();
		mitk::IRenderWindowPart* renderPart = dynamic_cast<mitk::IRenderWindowPart*>(page->GetActiveEditor().GetPointer());
		QmitkStdMultiWidgetEditor* linkedRenderWindowPart = dynamic_cast<QmitkStdMultiWidgetEditor*>(renderPart);
		if (linkedRenderWindowPart != NULL)
			linkedRenderWindowPart->EnableSlicingPlanes(true);

		

		mitk::BaseData::Pointer basedata = _currentPCMRINode->GetData();
		if (basedata.IsNotNull() && basedata->GetTimeGeometry()->IsValid())
		{
			mitk::RenderingManager::GetInstance()->InitializeViews(basedata->GetTimeGeometry(), mitk::RenderingManager::REQUEST_UPDATE_ALL, true);
			mitk::RenderingManager::GetInstance()->RequestUpdateAll();
		}

	}


}

void PCMRIMappingWidget::setPCMRIImagePhaseForMapping(const mitk::DataNode* node)
{
	_updateUI();
	if (!node) {
		return;
	}
	
	_currentPCMRIPhaseNode = const_cast<mitk::DataNode*>(node);

	if (_currentPCMRIPhaseNode)
	{
		auto pcmriUID = std::string{};
		node->GetStringProperty(crimson::HierarchyManager::nodeUIDPropertyName, pcmriUID);
		_currentNode->SetStringProperty("mapping.pcmrinodephase", pcmriUID.c_str());

		if (_currentPCMRIPhaseNode != _currentPCMRINode)
		{
			bool flagImage = false;
			_currentPCMRINode->GetBoolProperty("mapping.imageFlipped", flagImage);
			if (flagImage)
				dynamic_cast<mitk::Image*>(_currentPCMRIPhaseNode->GetData())->SetTimeGeometry(_pcmriGeometryFlipped);
			else
				dynamic_cast<mitk::Image*>(_currentPCMRIPhaseNode->GetData())->SetTimeGeometry(_pcmriGeometryOriginal);
		}

		if (_currentPCMRIPhaseNode != _currentPCMRINode)
			_currentPCMRIPhaseNode->SetVisibility(false);	

		int freq = 0;
		if (!_currentPCMRIPhaseNode->GetData()->GetPropertyList()->GetIntProperty("PAR.CardiacFrequency", freq))
		{
			_currentPCMRIPhaseNode->GetIntProperty("mapping.cardiacfrequency", freq);
		}
		_UI.freqSpinBox->setValue(freq);
		editCardiacfrequency();

		float venc = 0;
		int intVenc = 0;		
		
		if (_currentPCMRIPhaseNode->GetData()->GetPropertyList()->GetFloatProperty("PAR.PhaseEncodingVelocity", venc))
		{
			_UI.vencPSpinBox->setValue((int)venc);
			editVencP();
		}
		else if (_currentPCMRIPhaseNode->GetIntProperty("mapping.vencP", intVenc))
		{
			_UI.vencPSpinBox->setValue(intVenc);
			editVencP();
		}

		intVenc = 0;
		_currentPCMRIPhaseNode->GetIntProperty("mapping.vencG", intVenc);
		_UI.vencGSpinBox->setValue(intVenc);
		editVencG();

		intVenc = 0;
		_currentPCMRIPhaseNode->GetIntProperty("mapping.vencS", intVenc);
		_UI.vencSSpinBox->setValue(intVenc);
		editVencS();

		double rescaleSlope = 0;
		_currentPCMRIPhaseNode->GetDoubleProperty("mapping.rescaleSlopeS", rescaleSlope);
		_UI.rescaleSlopeSSpinBox->setValue(rescaleSlope);
		editRescaleSlopeS();

		rescaleSlope = 0;
		_currentPCMRIPhaseNode->GetDoubleProperty("mapping.rescaleSlope", rescaleSlope);
		_UI.rescaleSlopeSpinBox->setValue(rescaleSlope);
		editRescaleSlope();

		int rescaleIntercept = 0;
		_currentPCMRIPhaseNode->GetIntProperty("mapping.rescaleIntercept", rescaleIntercept);
		_UI.rescaleInterceptSpinBox->setValue(rescaleIntercept);
		editRescaleIntercept();

		rescaleIntercept = 0;
		_currentPCMRIPhaseNode->GetIntProperty("mapping.rescaleInterceptS", rescaleIntercept);
		_UI.rescaleInterceptSSpinBox->setValue(rescaleIntercept);
		editRescaleInterceptS();

		int quantization = 0;
		_currentPCMRIPhaseNode->GetIntProperty("mapping.quantization", quantization);
		_UI.quantizationSpinBox->setValue(quantization);
		editQuantization();
		
		double venscale = 0;
		_currentPCMRIPhaseNode->GetDoubleProperty("mapping.venscale", venscale);
		_UI.venscaleSpinBox->setValue(venscale);
		editVenscale();

		bool mask = 0;
		_currentPCMRIPhaseNode->GetBoolProperty("mapping.magnitudemask", mask);
		_UI.maskCheckBox->setChecked(mask);
		setMagnitudeMask();

		std::string manufacturer;
		_currentPCMRIPhaseNode->GetStringProperty("mapping.velocityCalculationType", manufacturer);
		if (manufacturer == "Philips")
			_UI.manufacturerTabs->setCurrentIndex(0);
		else if (manufacturer == "GE")
			_UI.manufacturerTabs->setCurrentIndex(1);
		else if (manufacturer == "Siemens")
			_UI.manufacturerTabs->setCurrentIndex(2);
		else if (manufacturer == "Linear")
			_UI.manufacturerTabs->setCurrentIndex(3);
		else
			editVelocityCalculationType();
	}	
}

void PCMRIMappingWidget::_setupPCMRIMappingComboBoxes()
{

	//disconnect possible existing connections
	QWidget::disconnect(_UI.pcmriImageComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this,
		&PCMRIMappingWidget::setPCMRIImageForMapping);
	QWidget::disconnect(_UI.pcmriPhaseComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this,
		&PCMRIMappingWidget::setPCMRIImagePhaseForMapping);
	QWidget::disconnect(_UI.mraImageComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this,
		&PCMRIMappingWidget::setMRAImageForMapping);
	QWidget::disconnect(_UI.meshNodeComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this,
		&PCMRIMappingWidget::setMeshNodeForMapping);


	auto meshPredicate = HierarchyManager::getInstance()->getPredicate(VesselMeshingNodeTypes::Mesh()); 

	_UI.meshNodeComboBox->SetPredicate(meshPredicate);

	auto meshUID = std::string{};
	_currentNode->GetStringProperty("mapping.meshnode", meshUID);
	mitk::DataNode* nodeToSelect = HierarchyManager::getInstance()->findNodeByUID(meshUID);
	if (nodeToSelect) {
		_UI.meshNodeComboBox->SetSelectedNode(nodeToSelect);
	}
	else if (_UI.meshNodeComboBox->count() > 0)
		_UI.meshNodeComboBox->SetSelectedNode(_UI.meshNodeComboBox->GetNode(0));

	setMeshNodeForMapping(_UI.meshNodeComboBox->GetSelectedNode());


	_UI.pcmriImageComboBox->SetPredicate(mitk::NodePredicateAnd::New(
		HierarchyManager::getInstance()->getPredicate(VascularModelingNodeTypes::Image()),
		mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("hidden object")).GetPointer())); //make only visible images selectable

	auto pcmriUID = std::string{};
	_currentNode->GetStringProperty("mapping.pcmrinode", pcmriUID);
	mitk::DataNode* nodeToSelectPcmri = HierarchyManager::getInstance()->findNodeByUID(pcmriUID);
	if (nodeToSelectPcmri) {
		_UI.pcmriImageComboBox->SetSelectedNode(nodeToSelectPcmri);
	}
	else if (_UI.pcmriImageComboBox->count() > 0)
		_UI.pcmriImageComboBox->SetSelectedNode(_UI.pcmriImageComboBox->GetNode(0));
	setPCMRIImageForMapping(_UI.pcmriImageComboBox->GetSelectedNode());


	_UI.pcmriPhaseComboBox->SetPredicate(mitk::NodePredicateAnd::New(
		HierarchyManager::getInstance()->getPredicate(VascularModelingNodeTypes::Image()),
		mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("hidden object")).GetPointer())); //make only visible images selectable

	auto pcmriphaseUID = std::string{};
	_currentNode->GetStringProperty("mapping.pcmrinodephase", pcmriphaseUID);
	mitk::DataNode* nodeToSelectPcmriPhase = HierarchyManager::getInstance()->findNodeByUID(pcmriphaseUID);
	if (nodeToSelectPcmriPhase) {
		_UI.pcmriPhaseComboBox->SetSelectedNode(nodeToSelectPcmriPhase);
	}
	else if (_UI.pcmriPhaseComboBox->count() > 0)
		_UI.pcmriPhaseComboBox->SetSelectedNode(_UI.pcmriPhaseComboBox->GetNode(0));
	setPCMRIImagePhaseForMapping(_UI.pcmriPhaseComboBox->GetSelectedNode());


	_UI.mraImageComboBox->SetPredicate(mitk::NodePredicateAnd::New(
		HierarchyManager::getInstance()->getPredicate(VascularModelingNodeTypes::Image()),
		mitk::NodePredicateNot::New(mitk::NodePredicateProperty::New("hidden object")).GetPointer())); //make only visible images selectable

	auto hm = crimson::HierarchyManager::getInstance();
	auto imageParentNode = hm->getAncestor(_currentNode, crimson::VascularModelingNodeTypes::Image());
	if (imageParentNode.IsNotNull())
		_UI.mraImageComboBox->SetSelectedNode(imageParentNode);
	else{
		auto mraUID = std::string{};
		_currentNode->GetStringProperty("mapping.mranode", mraUID);
		mitk::DataNode* nodeToSelectMra = HierarchyManager::getInstance()->findNodeByUID(mraUID);
		if (nodeToSelectMra) {
			_UI.mraImageComboBox->SetSelectedNode(nodeToSelectMra);
		}
		else if (_UI.mraImageComboBox->count() > 0)
			_UI.mraImageComboBox->SetSelectedNode(_UI.mraImageComboBox->GetNode(0));
	}
	setMRAImageForMapping(_UI.mraImageComboBox->GetSelectedNode());


	connect(_UI.meshNodeComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this, &PCMRIMappingWidget::setMeshNodeForMapping);
	connect(_UI.pcmriImageComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this, &PCMRIMappingWidget::setPCMRIImageForMapping);
	connect(_UI.pcmriPhaseComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this, &PCMRIMappingWidget::setPCMRIImagePhaseForMapping);
	connect(_UI.mraImageComboBox, &QmitkDataStorageComboBox::OnSelectionChanged, this, &PCMRIMappingWidget::setMRAImageForMapping);

}

void PCMRIMappingWidget::updateSelectedFace(mitk::PlaneGeometry::Pointer plane)
{
	if (plane && _ResliceView){
		_ResliceView->updateMRARendering(plane.GetPointer());
		_updateUI();
	}
	else
		return;
}

void PCMRIMappingWidget::editTimeInterpolationOptions()
{
	auto hm = crimson::HierarchyManager::getInstance();
	auto pcmriNode = hm->getFirstDescendant(_currentNode, crimson::SolverSetupNodeTypes::PCMRIData(), true);
		
	if (pcmriNode)
	{
		auto pcmriData = dynamic_cast<PCMRIData*>(pcmriNode->GetData());
		int nControlPoints = dynamic_cast<PCMRIData*>(pcmriNode->GetData())->getNControlPoints();
		
		if (!pcmriData->getMesh())
		{
			MeshData::Pointer mesh = dynamic_cast<MeshData*>(crimson::HierarchyManager::getInstance()->
				findNodeByUID(pcmriData->getMeshNodeUID())->GetData());
			pcmriData->setMesh(mesh.GetPointer());
		}

		TimeInterpolationDialog dlg(pcmriData, &nControlPoints);

		if (dlg.exec() == QDialog::Accepted) {
			dynamic_cast<PCMRIData*>(pcmriNode->GetData())->setNControlPoints(nControlPoints);
		}
	}

}

void PCMRIMappingWidget::flipFlow()
{
	auto child = crimson::HierarchyManager::getInstance()->getFirstDescendant(_currentNode, crimson::SolverSetupNodeTypes::PCMRIData(), true);
	if (child.IsNotNull())
	{
		auto childPCMRI = dynamic_cast<PCMRIData*>(child->GetData());
		if (!childPCMRI->getMesh())
		{
			MeshData::Pointer mesh = dynamic_cast<MeshData*>(crimson::HierarchyManager::getInstance()->
				findNodeByUID(childPCMRI->getMeshNodeUID())->GetData());
			childPCMRI->setMesh(mesh.GetPointer());
		}
		childPCMRI->flipFlow();
	}

	mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void PCMRIMappingWidget::flipPCMRIImage()
{
	if (_currentPCMRINode)
	{
		bool flagImage = false;
		_currentPCMRINode->GetBoolProperty("mapping.imageFlipped", flagImage);
		//we want to keep the previous state
		bool flagMenu = _UI.flipImageCheckBox->isChecked();
		if (flagImage != flagMenu)
			_flipPCMRIImage(flagMenu);
	}
}

mitk::TimeGeometry::Pointer PCMRIMappingWidget::_getFlippedGeometry(mitk::DataNode* node)
{
	mitk::Vector3D yAxis;
	yAxis[0] = 0;
	yAxis[1] = 1;
	yAxis[2] = 0;

	mitk::Vector3D zAxis;
	zAxis[0] = 0;
	zAxis[1] = 1;
	zAxis[2] = 1;

	mitk::TimeGeometry::Pointer timeGeometry;

	mitk::Image::Pointer pcmriImage = dynamic_cast<mitk::Image*>(_currentPCMRINode->GetData());
	mitk::RotationOperation* op = new mitk::RotationOperation(mitk::OpROTATE, pcmriImage->GetGeometry()->GetCenter(), pcmriImage->GetGeometry()->GetAxisVector(0), 180);
	mitk::RotationOperation* op2 = new mitk::RotationOperation(mitk::OpROTATE, pcmriImage->GetGeometry()->GetCenter(), pcmriImage->GetGeometry()->GetAxisVector(2), 180);
	
	if (dynamic_cast<mitk::Image*>(node->GetData()))
	{
		timeGeometry = dynamic_cast<mitk::Image*>(node->GetData())->GetTimeGeometry()->Clone();
	
	}
	else if (dynamic_cast<mitk::PlanarFigure*>(node->GetData()))
	{
		timeGeometry = dynamic_cast<mitk::PlanarFigure*>(node->GetData())->GetTimeGeometry()->Clone();
	}
	else if (dynamic_cast<mitk::PointSet*>(node->GetData()))
	{
		timeGeometry = dynamic_cast<mitk::PointSet*>(node->GetData())->GetTimeGeometry()->Clone();
	}
	timeGeometry->ExecuteOperation(op);
	timeGeometry->ExecuteOperation(op2);

	delete op;
	delete op2;

	return timeGeometry;
}

void PCMRIMappingWidget::_flipPCMRIImage(bool flag)
{
	if (_currentPCMRINode)
	{
		mitk::Image::Pointer pcmriImage = dynamic_cast<mitk::Image*>(_currentPCMRINode->GetData());
		mitk::Image::Pointer pcmriImagePhase = dynamic_cast<mitk::Image*>(_currentPCMRIPhaseNode->GetData());

		mitk::DataStorage::SetOfObjects::ConstPointer nodes =
			crimson::PCMRIUtils::getContourNodes(_currentPCMRINode);
		mitk::DataNode::Pointer nodePcmri = crimson::PCMRIUtils::getPcmriPointNode(_currentPCMRINode);

		if (flag)
		{
			pcmriImage->SetTimeGeometry(_pcmriGeometryFlipped);
			if (pcmriImagePhase != pcmriImage)
				pcmriImagePhase->SetTimeGeometry(_pcmriGeometryFlipped);

		}
		else
		{
			pcmriImage->SetTimeGeometry(_pcmriGeometryOriginal);
			if (pcmriImagePhase != pcmriImage)
				pcmriImagePhase->SetTimeGeometry(_pcmriGeometryOriginal);

		}
		
		pcmriImage->Update();
		_currentPCMRINode->Modified();

		pcmriImagePhase->Update();
		_currentPCMRIPhaseNode->Modified();

		//rotate the pcmri landmark
		if (nodePcmri)
		{
			auto point = dynamic_cast<mitk::PointSet*>(nodePcmri->GetData());
			auto timeGeometryPoint = _getFlippedGeometry(nodePcmri);
			point->SetClonedTimeGeometry(timeGeometryPoint.GetPointer());
			nodePcmri->Update();
		}

		if (_ResliceView)
			_ResliceView->getPCMRIRenderer()->GetRenderingManager()->RequestUpdateAll();

		mitk::RenderingManager::GetInstance()->RequestUpdateAll();
		mitk::RenderingManager::GetInstance()->RequestUpdateAll(mitk::RenderingManager::REQUEST_UPDATE_3DWINDOWS);

		//set the flag for persistance
		_currentPCMRINode->SetBoolProperty("mapping.imageFlipped", flag);
		_currentPCMRIPhaseNode->SetBoolProperty("mapping.imageFlipped", flag);

	}
}

void PCMRIMappingWidget::editCardiacfrequency()
{
	int freq = 0;
	freq = _UI.freqSpinBox->value();
	_currentNode->SetIntProperty("mapping.cardiacfrequency", freq);
	_currentPCMRIPhaseNode->SetIntProperty("mapping.cardiacfrequency", freq);
	_updateUI();
}

void PCMRIMappingWidget::editVencP()
{
	float venc = 0;
	venc = _UI.vencPSpinBox->value();
	_currentNode->SetIntProperty("mapping.vencP", (int)venc);
	_currentPCMRIPhaseNode->SetIntProperty("mapping.vencP", (int)venc);
	_updateUI();
}

void PCMRIMappingWidget::editVencG()
{
	float value = 0;
	value = _UI.vencGSpinBox->value();
	_currentNode->SetIntProperty("mapping.vencG", (int)value);
	_currentPCMRIPhaseNode->SetIntProperty("mapping.vencG", (int)value);
	_updateUI();
}

void PCMRIMappingWidget::editVenscale()
{
	double value = 0;
	value = _UI.venscaleSpinBox->value();
	_currentNode->SetDoubleProperty("mapping.venscale", value);
	_currentPCMRIPhaseNode->SetDoubleProperty("mapping.venscale", value);
	_updateUI();
}

void PCMRIMappingWidget::setMagnitudeMask()
{
	bool mask = 0;
	mask = _UI.maskCheckBox->isChecked();
	_currentNode->SetBoolProperty("mapping.magnitudemask", mask);
	_currentPCMRIPhaseNode->SetBoolProperty("mapping.magnitudemask", mask);
	_updateUI();
}

void PCMRIMappingWidget::editVencS()
{
	float value = 0;
	value = _UI.vencSSpinBox->value();
	_currentNode->SetIntProperty("mapping.vencS", (int)value);
	_currentPCMRIPhaseNode->SetIntProperty("mapping.vencS", (int)value);
	_updateUI();
}

void PCMRIMappingWidget::editRescaleSlopeS()
{
	double value = 0;
	value = _UI.rescaleSlopeSSpinBox->value();
	_currentNode->SetDoubleProperty("mapping.rescaleSlopeS", value);
	_currentPCMRIPhaseNode->SetDoubleProperty("mapping.rescaleSlopeS", value);
	_updateUI();
}

void PCMRIMappingWidget::editRescaleInterceptS()
{
	double value = 0;
	value = _UI.rescaleInterceptSSpinBox->value();
	_currentNode->SetDoubleProperty("mapping.rescaleInterceptS", value);
	_currentPCMRIPhaseNode->SetDoubleProperty("mapping.rescaleInterceptS", value);
	_updateUI();
}

void PCMRIMappingWidget::editQuantization()
{
	float value = 0;
	value = _UI.quantizationSpinBox->value();
	_currentNode->SetIntProperty("mapping.quantization", (int)value);
	_currentPCMRIPhaseNode->SetIntProperty("mapping.quantization", (int)value);
	_updateUI();
}

void PCMRIMappingWidget::editRescaleSlope()
{
	double value = 0;
	value = _UI.rescaleSlopeSpinBox->value();
	_currentNode->SetDoubleProperty("mapping.rescaleSlope", value);
	_currentPCMRIPhaseNode->SetDoubleProperty("mapping.rescaleSlope", value);
	_updateUI();
}

void PCMRIMappingWidget::editRescaleIntercept()
{
	double value = 0;
	value = _UI.rescaleInterceptSpinBox->value();
	_currentNode->SetDoubleProperty("mapping.rescaleIntercept", value);
	_currentPCMRIPhaseNode->SetDoubleProperty("mapping.rescaleIntercept", value);
	_updateUI();
}

void PCMRIMappingWidget::editVelocityCalculationType()
{
	MITK_INFO << _UI.manufacturerTabs->currentIndex();
	switch (_UI.manufacturerTabs->currentIndex())
	{
		//Linear transformation
		case 0:
		{
			_currentPCMRIPhaseNode->GetPropertyList()->SetStringProperty("mapping.velocityCalculationType", "Linear");
			break;
		}
		//Philips
		case 1:
		{
			_currentPCMRIPhaseNode->SetStringProperty("mapping.velocityCalculationType", "Philips");
			break;
		}
		//GE
		case 2:
		{
			_currentPCMRIPhaseNode->SetStringProperty("mapping.velocityCalculationType", "GE");
			break;
		}
		//Siemens
		case 3:
		{
			_currentPCMRIPhaseNode->SetStringProperty("mapping.velocityMapping", "Siemens");
			break;
		}
	}
}