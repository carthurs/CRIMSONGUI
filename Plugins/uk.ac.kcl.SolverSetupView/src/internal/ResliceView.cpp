// Blueberry
#include "ResliceView.h"
#include "VesselDrivenSlicedGeometry.h"
#include "VesselPathInteractor.h"
#include "VascularModelingNodeTypes.h"
#include "SolverSetupNodeTypes.h"
#include <HierarchyManager.h>
#include <VascularModelingNodeTypes.h>
#include <FaceData.h>

#include <vtkParametricSplineVesselPathData.h>
#include <mitkVtkResliceInterpolationProperty.h>
#include <mitkNodePredicateDataType.h>

#include <mitkSlicedGeometry3D.h>
#include <mitkImage.h>
#include <mitkRotationOperation.h>
#include <mitkInteractionConst.h>
#include <mitkImageTimeSelector.h>
#include <mitkLevelWindowProperty.h>
//#include <mitkTextAnnotation2D.h>

// Qt
#include "QmitkRenderWindow.h"
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QToolButton>

#include <ctkSliderWidget.h>
#include <ctkPopupWidget.h>
#include <usModuleRegistry.h>

#include "PCMRIUtils.h"
#include "PCMRIMappingWidget.h"

#include <berryIPartListener.h>
#include <berryIWorkbenchWindow.h>
#include <berryIWorkbenchPage.h>

//////////////////////////////////////////////////////////////////////////
// The listener for the reslice view. 
class PCMRIResliceViewWidgetListener : public berry::IPartListener {
public:
	berryObjectMacro(PCMRIResliceViewWidgetListener);

	PCMRIResliceViewWidgetListener(ResliceView* resliceView)
        : _resliceView(resliceView)
    {
    }

	~PCMRIResliceViewWidgetListener()
    {
    }

    void registerListener()
    {
        _resliceView->GetSite()->GetPage()->AddPartListener(this);
    }

    void unregisterListener()
    {
        _resliceView->GetSite()->GetPage()->RemovePartListener(this);
    }

    Events::Types GetPartEventTypes() const override
    {
        return Events::HIDDEN | Events::VISIBLE;
    }

    void PartHidden(const berry::IWorkbenchPartReference::Pointer& partRef) override
    {
        /*if (partRef->GetPart(false) == _resliceView) {
            _resliceView->_updateGeometryNodeInDataStorage();
        }*/
    }

    void PartVisible(const berry::IWorkbenchPartReference::Pointer& partRef) override
    {
      /*  if (partRef->GetPart(false) == _resliceView) {
            _resliceView->_updateGeometryNodeInDataStorage();
        }*/
    }

private:
    ResliceView* _resliceView;
};


enum ObservedObject {
    ooSNC = 0,
    ooSNC_PCMRI,
    ooRenderingManager,
	ooRenderingManagerPcmri,

    ooLast
};

class ResliceViewPrivate {
public:
    ResliceViewPrivate()
        : renderWindow(nullptr)
    {
        reinitVesselDrivenGeometryTimer.setSingleShot(true);
    }

    QHash<ObservedObject, QPair<itk::Object::Pointer, unsigned long>> observerTags;

    unsigned long vesselPathObserverTag;
    unsigned long renderingManagerInitializeObserverTag;
    QVBoxLayout* mainLayout;
    QmitkRenderWindow* renderWindow;
    QmitkRenderWindow* renderWindowPCMRI;
    QSpacerItem* spacer;
    QToolButton* resliceWidgetVisibilityButton;
    QTimer reinitVesselDrivenGeometryTimer;
	QLabel* time;
	QLabel* timeLabel;
	QScopedPointer<PCMRIResliceViewWidgetListener> resliceViewWidgetListener;
	ctkSliderWidget* timeSlider;
	QComboBox* reslicePlane;

    QHash<const mitk::DataNode*, float> savedSlicePositions;


    void _addObserver(ObservedObject type, itk::Object::Pointer object, const itk::EventObject& event, itk::Command* command)
    {
        assert(observerTags.find(type) == observerTags.end());
        command->Register();

        observerTags[type] = qMakePair(object, object->AddObserver(event, command));
    }

    void _removeObserver(ObservedObject type)
    {
        if (observerTags.contains(type)) {
            observerTags[type].first->RemoveObserver(observerTags[type].second);
            observerTags.remove(type);
        }
    }

};


const std::string ResliceView::VIEW_ID = "org.mitk.views.ResliceView";

ResliceView::ResliceView()
    : NodeDependentView(crimson::SolverSetupNodeTypes::BoundaryCondition(), true, QString("Boundary Condition"), true) 
    , d(new ResliceViewPrivate())
{
	d->resliceViewWidgetListener.reset(new PCMRIResliceViewWidgetListener(this));
}

ResliceView::~ResliceView()
{
    d->resliceViewWidgetListener->unregisterListener();

    // Disconnect time events
	mitk::SliceNavigationController* timeNavigationController = pcmriManager->GetInstance()->GetTimeNavigationController();
	timeNavigationController->Disconnect(d->renderWindowPCMRI->GetSliceNavigationController()); 

    // Remove all observers
    for (int i = 0; i < ooLast; ++i) {
        d->_removeObserver(static_cast<ObservedObject>(i));
    }

    _removeGeometryNodeFromDataStorage();
}

void ResliceView::SetFocus()
{
	mraManager->GetInstance()->SetRenderWindowFocus(d->renderWindow->GetVtkRenderWindow());
	pcmriManager->GetInstance()->SetRenderWindowFocus(d->renderWindowPCMRI->GetVtkRenderWindow());
}

void ResliceView::CreateQtPartControl(QWidget *parent)
{
    d->mainLayout = new QVBoxLayout(parent);

    QHBoxLayout* hLayout = new QHBoxLayout(parent);
    hLayout->addWidget(createSelectedNodeWidget(parent));

    d->resliceWidgetVisibilityButton = new QToolButton(parent);
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/vesselSeg/icons/vis_off.png"), QSize(), QIcon::Normal, QIcon::Off);
    icon.addFile(QString::fromUtf8(":/vesselSeg/icons/vis_on.png"), QSize(), QIcon::Normal, QIcon::On);
    d->resliceWidgetVisibilityButton->setIcon(icon);
    d->resliceWidgetVisibilityButton->setIconSize(QSize(24, 24));
    d->resliceWidgetVisibilityButton->setToolTip(tr("Show/hide the reslice position in 3D window."));
    d->resliceWidgetVisibilityButton->setCheckable(true);
    d->resliceWidgetVisibilityButton->setChecked(true);
    hLayout->addWidget(d->resliceWidgetVisibilityButton);

	d->reslicePlane = new QComboBox();
	d->reslicePlane->setToolTip(tr("Select the plane of the 2D PC-MRI data"));
	d->reslicePlane->addItem("Axial");
	d->reslicePlane->addItem("Coronal");
	d->reslicePlane->addItem("Sagittal");
	d->reslicePlane->setCurrentIndex(0);
	hLayout->addWidget(d->reslicePlane);
	
    d->mainLayout->addLayout(hLayout);

	hLayout = new QHBoxLayout(parent);
	//PCMRI time slider
	d->timeLabel = new QLabel(parent);
	d->timeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	d->timeLabel->setText("Time step");
	d->timeLabel->setToolTip(tr("Time step in PCMRI image"));
	d->timeLabel->setAlignment(Qt::AlignLeft);
	hLayout->addWidget(d->timeLabel);

	d->timeSlider = new ctkSliderWidget(parent);
	d->timeSlider->setDecimals(0);
	hLayout->addWidget(d->timeSlider);

	d->time = new QLabel(parent);
	d->time->setTextInteractionFlags(Qt::TextSelectableByMouse);
	d->time->setText("0");
	d->time->setMinimumWidth(d->time->fontMetrics().width("99"));
	d->time->setToolTip(tr("Time step in PCMRI image"));
	d->time->setAlignment(Qt::AlignRight);
	hLayout->addWidget(d->time);

	d->mainLayout->addLayout(hLayout);

    auto renderWindowsLayout = new  QHBoxLayout(parent);

	mraManager = mitk::RenderingManager::New();
	d->renderWindow = new QmitkRenderWindow(parent, "reslicer", nullptr, mraManager->GetInstance());
    d->renderWindow->GetRenderer()->SetDataStorage(GetDataStorage());
    d->renderWindow->GetRenderer()->SetMapperID(mitk::BaseRenderer::Standard2D);
    d->renderWindow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    renderWindowsLayout->addWidget(d->renderWindow);

	// create a specific RenderingManager for PCMRI
	pcmriManager = mitk::RenderingManager::New();
	d->renderWindowPCMRI = new QmitkRenderWindow(parent, "reslicer pcmri", nullptr, pcmriManager->GetInstance());
	d->renderWindowPCMRI->GetRenderer()->SetDataStorage(GetDataStorage());
	d->renderWindowPCMRI->GetRenderer()->SetMapperID(mitk::BaseRenderer::Standard2D);
	d->renderWindowPCMRI->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	renderWindowsLayout->addWidget(d->renderWindowPCMRI);

	
    // Connect time events
	mitk::SliceNavigationController* timeNavigationController = pcmriManager->GetInstance()->GetTimeNavigationController();
	timeNavigationController->ConnectGeometryTimeEvent(d->renderWindowPCMRI->GetSliceNavigationController(), false);
	d->renderWindowPCMRI->GetSliceNavigationController()->ConnectGeometryTimeEvent(timeNavigationController, false);

    d->mainLayout->addLayout(renderWindowsLayout, 1);
    d->mainLayout->addStretch(0);

    // Setup synchronization of renderer's stepper and the slide number
	connect(d->timeSlider, &ctkSliderWidget::valueChanged, this, &ResliceView::_setTimeSliceNumber);

    auto modifiedCommand = itk::MemberCommand<ResliceView>::New();
    modifiedCommand->SetCallbackFunction(this, &ResliceView::_syncSliderWithStepper);
    modifiedCommand->SetCallbackFunction(this, &ResliceView::_syncSliderWithStepperC);
	d->_addObserver(ooSNC_PCMRI, d->renderWindowPCMRI->GetRenderer()->GetSliceNavigationController()->GetTime(), itk::ModifiedEvent(), modifiedCommand);


    // Connect to the reinitialize command as Global Reinit replaces the renderer's geometries
    auto reinitCommand = itk::SimpleMemberCommand<ResliceView>::New();
    reinitCommand->SetCallbackFunction(this, &ResliceView::_setupRendererSlices);
    d->_addObserver(ooRenderingManager, mraManager->GetInstance(), mitk::RenderingManagerViewsInitializedEvent(), reinitCommand);

	auto reinitCommand2 = itk::SimpleMemberCommand<ResliceView>::New();
	reinitCommand2->SetCallbackFunction(this, &ResliceView::_setupRendererSlices);
	d->_addObserver(ooRenderingManagerPcmri, pcmriManager->GetInstance(), mitk::RenderingManagerViewsInitializedEvent(), reinitCommand2);

    connect(&d->reinitVesselDrivenGeometryTimer, &QTimer::timeout, this, &ResliceView::_setupRendererSlices);

	d->resliceViewWidgetListener->registerListener();

	connect(d->reslicePlane, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ResliceView::_changeReslicePlane);

    _setResliceViewEnabled(false); //not visible until a node of NodeType is selected
    initializeCurrentNode(); 
}

mitk::BaseRenderer* ResliceView::getResliceRenderer() const
{
    return d->renderWindow->GetRenderer();
}

mitk::BaseRenderer* ResliceView::getPCMRIRenderer() const
{
	return d->renderWindowPCMRI->GetRenderer();
}

std::vector<mitk::BaseRenderer*> ResliceView::getAllResliceRenderers() const
{
	std::vector<mitk::BaseRenderer*> renderers = { d->renderWindow->GetRenderer(), d->renderWindowPCMRI->GetRenderer() };
    return renderers;
}

void ResliceView::navigateTo(float parameterValue)
{
    mitk::SliceNavigationController* snc = d->renderWindowPCMRI->GetRenderer()->GetSliceNavigationController();
    const mitk::TimeGeometry* worldTimeGeometry = snc->GetInputWorldTimeGeometry();
    if (worldTimeGeometry == nullptr) {
        return;
    }
	snc->GetTime()->SetPos(parameterValue);
}

float ResliceView::getCurrentParameterValue() const
{
	mitk::SliceNavigationController* snc = d->renderWindowPCMRI->GetRenderer()->GetSliceNavigationController();
	const mitk::TimeGeometry* worldTimeGeometry = snc->GetInputWorldTimeGeometry();

	return getPCMRIRenderer()->GetSliceNavigationController()->GetTime()->GetPos();

}

mitk::PlaneGeometry* ResliceView::getPlaneGeometry(float t) const
{
	auto geometry = dynamic_cast<const mitk::SlicedGeometry3D*>(d->renderWindowPCMRI->GetRenderer()->GetSliceNavigationController()
		->GetInputWorldTimeGeometry()->GetGeometryForTimeStep(t).GetPointer());
	auto planeGeometry = geometry->GetPlaneGeometry(0);
	planeGeometry->SetReferenceGeometry(planeGeometry);
	return planeGeometry;
}

void ResliceView::_setResliceViewEnabled(bool enabled)
{
    d->timeSlider->setEnabled(enabled);
    d->renderWindow->setEnabled(enabled);
    d->renderWindow->setVisible(enabled);
	d->renderWindowPCMRI->setEnabled(enabled);
	d->renderWindowPCMRI->setVisible(enabled);
}
void ResliceView::currentNodeChanged(mitk::DataNode*)
{
    _setResliceViewEnabled(currentNode()); 
    if (currentNode()) {

        float resliceWindowSize = 50;

        _setupRendererSlices();
    }

}

void ResliceView::currentNodeModified()
{
	_setResliceViewEnabled(currentNode());

	d->reinitVesselDrivenGeometryTimer.start(200);
}

void ResliceView::forceReinitGeometry()
{
    if (!d->reinitVesselDrivenGeometryTimer.isActive()) {
        return; // Geometry is valid
    }

    d->reinitVesselDrivenGeometryTimer.stop();
    _setupRendererSlices();
}

//called once a new vessel path node is selected or on reinit
void ResliceView::_setupRendererSlices()
{
	if (currentNode() == nullptr) {
        return;
    }

	if (currentPCMRINode)
		_setupPCMRISlices(currentPCMRINode);

	if (currentMRANode)
		_setupMRASlice(currentMRANode);
  
}

void ResliceView::_setupPCMRISlices(const mitk::DataNode* node)
{
	if (node == nullptr) {
		return;
	}

	if (node == currentPCMRINode)
	{
		return;
	}

	if (currentPCMRINode)
	{
		currentPCMRINode->SetVisibility(false, d->renderWindowPCMRI->GetRenderer());

		//set all nodes invisible

		mitk::DataStorage::SetOfObjects::ConstPointer invisibleNodes =
			crimson::HierarchyManager::getInstance()->crimson::HierarchyManager::getInstance()->getDataStorage()->GetAll();
		for (const mitk::DataNode::Pointer& node : *invisibleNodes) {
			if (crimson::HierarchyManager::getInstance()
				->getPredicate(crimson::SolverSetupNodeTypes::PCMRIPoint())
				->CheckNode(node) || crimson::HierarchyManager::getInstance()
				->getPredicate(crimson::SolverSetupNodeTypes::MRAPoint())
				->CheckNode(node))
				continue;
			else
				node->SetVisibility(false, d->renderWindowPCMRI->GetRenderer());
		}
	}
	
	currentPCMRINode = const_cast<mitk::DataNode*>(node);

	mitk::DataNode::Pointer pcmriPointNode = crimson::PCMRIUtils::getPcmriPointNode(currentPCMRINode);
	if (pcmriPointNode)
	{
		pcmriPointNode->SetVisibility(true);
	}

	float resliceWindowSize = 50;

	mitk::ScalarType paramDelta;
	mitk::Vector3D referenceImageSpacing;
	unsigned int timeSteps;

	std::tie(paramDelta, referenceImageSpacing, timeSteps) = crimson::PCMRIUtils::getResliceGeometryParameters(currentPCMRINode);

	if (currentPCMRINode.IsNotNull()) {
		//make invisible in main Qmitk render window and MRA slice window
		if (currentMRANode != currentPCMRINode)
		{
			currentPCMRINode->SetVisibility(false, d->renderWindow->GetRenderer());
		}
	
		d->renderWindow->GetRenderer()->GetCameraController()->Fit();
		//make visible in the PCMRI rendering window only
		currentPCMRINode->SetVisibility(true, d->renderWindowPCMRI->GetRenderer());
		
		//set level window properties for PAR/REC
		mitk::Image::Pointer pcmriImage = dynamic_cast<mitk::Image*>(currentPCMRINode->GetData());
		int numberOfCardiacPhases;
		if (pcmriImage->GetPropertyList()->GetIntProperty("PAR.MaxNumberOfCardiacPhases", numberOfCardiacPhases))
		{
			mitk::ImageTimeSelector::Pointer timeSelector = mitk::ImageTimeSelector::New();
			timeSelector->SetInput(dynamic_cast<mitk::Image*>(currentPCMRINode->GetData()));
			timeSelector->SetTimeNr(1);
			timeSelector->UpdateLargestPossibleRegion();
			mitk::Image::Pointer subImage = timeSelector->GetOutput();
			mitk::LevelWindowProperty::Pointer levWinProp = mitk::LevelWindowProperty::New();
			mitk::LevelWindow levelWindow;
			levelWindow.SetAuto(subImage);
			levWinProp->SetLevelWindow(levelWindow);
			currentPCMRINode->GetPropertyList()->SetProperty("levelwindow", levWinProp);
		}
	}

	mitk::ProportionalTimeGeometry::Pointer timeGeometry = mitk::ProportionalTimeGeometry::New();
	timeGeometry->Initialize(node->GetData()->GetGeometry(), timeSteps);

	// Access the saved Id here because the geometry setup will trigger Stepper's modify function to be called
	float savedSlicePos = d->savedSlicePositions.value(currentNode(), 0);

	mitk::SliceNavigationController* snc = d->renderWindowPCMRI->GetRenderer()->GetSliceNavigationController();
	unsigned int currentGeometryTime = snc->GetTime()->GetPos();

	auto timeGeom = currentPCMRINode->GetData()->GetTimeGeometry();
	auto geometry = currentPCMRINode->GetData()->GetUpdatedTimeGeometry();
	snc->SetInputWorldTimeGeometry(timeGeometry);
	if (d->reslicePlane->currentIndex() == 0)
	{
		snc->Update(mitk::SliceNavigationController::Axial, false, false, true);
		snc->SetDefaultViewDirection(mitk::SliceNavigationController::Axial);
	}
	else if (d->reslicePlane->currentIndex() == 1)
	{
		snc->Update(mitk::SliceNavigationController::Frontal, false, false, true);
		snc->SetDefaultViewDirection(mitk::SliceNavigationController::Frontal);
	}
	else if (d->reslicePlane->currentIndex() == 2)
	{
		snc->Update(mitk::SliceNavigationController::Sagittal, false, false, true);
		snc->SetDefaultViewDirection(mitk::SliceNavigationController::Sagittal);
	}
	snc->Update();
	snc->GetTime()->SetPos(currentGeometryTime);
		
	navigateTo(savedSlicePos);

	d->renderWindowPCMRI->GetRenderer()->GetCameraController()->Fit();
}

void ResliceView::_setupMRASlice(const mitk::DataNode* node)
{
	if (node == nullptr) {
		return;
	}

	if (currentMRANode)
	{
		currentMRANode->SetSelected(false, d->renderWindow->GetRenderer());
		currentMRANode->SetVisibility(false, d->renderWindow->GetRenderer());
	}

	currentMRANode = const_cast<mitk::DataNode*>(node);
	
	float resliceWindowSize = 50;

	mitk::ScalarType paramDelta;
	mitk::Vector3D referenceImageSpacing;
	unsigned int timeSteps;

	//get the image from the combo box and pass it directly
	std::tie(paramDelta, referenceImageSpacing, timeSteps) = crimson::PCMRIUtils::getResliceGeometryParameters(currentMRANode);


	if (currentMRANode.IsNotNull()) {
		currentMRANode->SetBoolProperty("in plane resample extent by geometry", true, d->renderWindow->GetRenderer());
		currentMRANode->AddProperty("reslice interpolation", mitk::VtkResliceInterpolationProperty::New(VTK_RESLICE_CUBIC), d->renderWindow->GetRenderer(), true);
		if (currentMRANode != currentPCMRINode)
		{
			currentMRANode->SetSelected(false, d->renderWindowPCMRI->GetRenderer());
			currentMRANode->SetVisibility(false, d->renderWindowPCMRI->GetRenderer());
		}
		currentMRANode->SetSelected(true, d->renderWindow->GetRenderer());
		currentMRANode->SetVisibility(true, d->renderWindow->GetRenderer());
	}

	mitk::BaseGeometry::Pointer geo = node->GetData()->GetGeometry();
	d->renderWindow->GetRenderer()->SetWorldGeometry3D(geo);

	//check if there is a face previously selected
	if (currentNode() && currentSolidNode)
	{
		auto faceData = dynamic_cast<crimson::FaceData*>(currentNode()->GetData());
		auto faces = faceData->getFaces();
		if (!faces.empty())
		{

			auto selectedFace = *faces.begin();

			auto plane = crimson::PCMRIUtils::getMRAPlane(currentNode(), const_cast<mitk::DataNode*>(node), currentSolidNode, selectedFace);

			updateMRARendering(plane.GetPointer());
		}
	}

	d->renderWindow->GetRenderer()->GetCameraController()->Fit();
}

void ResliceView::_changeReslicePlane()
{

	// Access the saved Id here because the geometry setup will trigger Stepper's modify function to be called
	float savedSlicePos = d->savedSlicePositions.value(currentNode(), 0);

	mitk::SliceNavigationController* snc = d->renderWindowPCMRI->GetRenderer()->GetSliceNavigationController();
	unsigned int currentGeometryTime = snc->GetTime()->GetPos();

	if (d->reslicePlane->currentIndex() == 0)
	{
		snc->Update(mitk::SliceNavigationController::Axial, false, false, true);
		snc->SetDefaultViewDirection(mitk::SliceNavigationController::Axial);
	}
	else if (d->reslicePlane->currentIndex() == 1)
	{
		snc->Update(mitk::SliceNavigationController::Frontal, false, false, true);
		snc->SetDefaultViewDirection(mitk::SliceNavigationController::Frontal);
	}
	else if (d->reslicePlane->currentIndex() == 2)
	{
		snc->Update(mitk::SliceNavigationController::Sagittal, false, false, true);
		snc->SetDefaultViewDirection(mitk::SliceNavigationController::Sagittal);
	}
	snc->Update();
	snc->GetTime()->SetPos(currentGeometryTime);

	navigateTo(savedSlicePos);

	d->renderWindowPCMRI->GetRenderer()->GetCameraController()->Fit();
}

void ResliceView::_setTimeSliceNumber(double slice)
{
	unsigned int sliceNumber = static_cast<unsigned int>(slice);
	d->renderWindowPCMRI->GetRenderer()->GetSliceNavigationController()->GetTime()->SetPos(sliceNumber);
	d->time->setText(QString("%1").arg(getCurrentParameterValue(), 6, 'f', 2));
}

void ResliceView::_syncSliderWithStepperC(const itk::Object* o, const itk::EventObject&)
{
    static bool updating;
    if (updating) {
        return;
    }
    updating = true;

    // const_cast due to lack of itkGetConstMacro() in the mitk::Stepper
    auto stepper = const_cast<mitk::Stepper*>(static_cast<const mitk::Stepper*>(o));

	int numberOfCardiacPhases = 0;
	if (currentPCMRINode)
	{
		mitk::Image::Pointer pcmriImage = dynamic_cast<mitk::Image*>(currentPCMRINode->GetData());
		if (pcmriImage->GetPropertyList()->GetIntProperty("PAR.MaxNumberOfCardiacPhases", numberOfCardiacPhases))
		{
			stepper->SetRange(1, numberOfCardiacPhases);
			stepper->SetSteps(numberOfCardiacPhases);
		}
		else
		{
			stepper->SetRange(1, pcmriImage->GetDimensions()[3]);
			stepper->SetSteps(pcmriImage->GetDimensions()[3]);
		}
	}

	d->timeSlider->setRange(0, stepper->GetSteps() - 1);
	d->timeSlider->setValue(stepper->GetPos());

    auto timeGeometry = d->renderWindowPCMRI->GetRenderer()->GetSliceNavigationController()->GetInputWorldTimeGeometry();
    if (timeGeometry) {
        // Avoid saving slice ID's if they come from geometry replacement upon global reinit
        auto spatialGeometry = timeGeometry->GetGeometryForTimeStep(stepper->GetPos()).GetPointer();
		if (spatialGeometry) {
			d->savedSlicePositions[currentNode()] = getCurrentParameterValue();

			emit sliceChanged(stepper->GetPos());
        }
    }

    updating = false;
}

void ResliceView::_updateGeometryNodeInDataStorage()
{
    mitk::DataNode* resliceWidgetNode = d->renderWindow->GetRenderer()->GetCurrentWorldPlaneGeometryNode();
    resliceWidgetNode->SetVisibility(currentNode() && d->resliceWidgetVisibilityButton->isChecked() && 
        this->GetSite() && this->GetSite()->GetPage() && this->GetSite()->GetPage()->IsPartVisible(berry::IWorkbenchPart::Pointer(this)));
    if (!GetDataStorage()->Exists(resliceWidgetNode)) {
        resliceWidgetNode->SetBoolProperty("helper object", true);
        resliceWidgetNode->SetName("MRA Vessel reslice plane");
        resliceWidgetNode->SetIntProperty("Crosshair.Gap Size", 4);
        resliceWidgetNode->SetBoolProperty("Crosshair.Ignore", true);
        GetDataStorage()->Add(resliceWidgetNode);
    }
    mitk::RenderingManager::GetInstance()->RequestUpdateAll(mitk::RenderingManager::REQUEST_UPDATE_3DWINDOWS);
}

void ResliceView::_removeGeometryNodeFromDataStorage()
{
    mitk::DataNode* resliceWidgetNode = d->renderWindow->GetRenderer()->GetCurrentWorldPlaneGeometryNode();
    if (GetDataStorage()->Exists(resliceWidgetNode)) {
        GetDataStorage()->Remove(resliceWidgetNode);
    }
}

//set up the rendering of MRA based on the selected face
void ResliceView::updateMRARendering(mitk::PlaneGeometry::Pointer plane) const
{
	
	originalMRAgeometry = plane.GetPointer();

	d->renderWindow->GetRenderer()->SetWorldGeometry3D(plane);
	d->renderWindow->GetRenderer()->GetCameraController()->Fit();
	d->renderWindow->GetRenderer()->ForceImmediateUpdate();

}
