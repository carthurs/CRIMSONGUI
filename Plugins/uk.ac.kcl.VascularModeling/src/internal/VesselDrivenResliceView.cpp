// Blueberry
#include "VesselDrivenResliceView.h"
#include "VesselDrivenSlicedGeometry.h"
#include "VesselPathInteractor.h"
#include "VascularModelingNodeTypes.h"

#include <vtkParametricSplineVesselPathData.h>
#include <mitkVtkResliceInterpolationProperty.h>
#include <mitkNodePredicateDataType.h>

#include <mitkSlicedGeometry3D.h>
#include <mitkImage.h>

// Qt
#include "QmitkRenderWindow.h"
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QToolButton>

#include <ctkSliderWidget.h>
#include <ctkPopupWidget.h>
#include <usModuleRegistry.h>

#include "ui_ReslicePropertiesWidget.h"
#include "VascularModelingUtils.h"

#include <berryIPartListener.h>
#include <berryIWorkbenchWindow.h>
#include <berryIWorkbenchPage.h>

//////////////////////////////////////////////////////////////////////////
// The listener for the reslice view. 
class ResliceViewWidgetListener : public berry::IPartListener {
public:
    berryObjectMacro(ResliceViewWidgetListener);

    ResliceViewWidgetListener(VesselDrivenResliceView* resliceView)
        : _resliceView(resliceView)
    {
    }

    ~ResliceViewWidgetListener()
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
        if (partRef->GetPart(false) == _resliceView) {
            _resliceView->_updateGeometryNodeInDataStorage();
        }
    }

    void PartVisible(const berry::IWorkbenchPartReference::Pointer& partRef) override
    {
        if (partRef->GetPart(false) == _resliceView) {
            _resliceView->_updateGeometryNodeInDataStorage();
        }
    }

private:
    VesselDrivenResliceView* _resliceView;
};


enum ObservedObject {
    ooSNC = 0,
    ooSNC_GradMag,
    ooRenderingManager,

    ooLast
};

class VesselDrivenResliceViewPrivate {
public:
    VesselDrivenResliceViewPrivate()
        : renderWindow(nullptr)
    {
        reinitVesselDrivenGeometryTimer.setSingleShot(true);
    }

    QHash<ObservedObject, QPair<itk::Object::Pointer, unsigned long>> observerTags;

    unsigned long vesselPathObserverTag;
    unsigned long renderingManagerInitializeObserverTag;
    QVBoxLayout* mainLayout;
    QmitkRenderWindow* renderWindow;
    QmitkRenderWindow* renderWindowGradMag;
    QSpacerItem* spacer;
    ctkSliderWidget* sliceNumberSlider;
    QDoubleSpinBox* resliceWindowSizeSpinBox;   
    QToolButton* resliceWidgetVisibilityButton;
    QTimer reinitVesselDrivenGeometryTimer;
    QLabel* positionInMM;
    QScopedPointer<ResliceViewWidgetListener> resliceViewWidgetListener;

    QHash<const mitk::DataNode*, mitk::Point3D> savedSlicePositions;


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


const std::string VesselDrivenResliceView::VIEW_ID = "org.mitk.views.VesselDrivenResliceView";

VesselDrivenResliceView::VesselDrivenResliceView()
    : NodeDependentView(crimson::VascularModelingNodeTypes::VesselPath(), true, QString("Vessel path"), true)
    , d(new VesselDrivenResliceViewPrivate())
{
    d->resliceViewWidgetListener.reset(new ResliceViewWidgetListener(this));
}

VesselDrivenResliceView::~VesselDrivenResliceView()
{
    d->resliceViewWidgetListener->unregisterListener();

    // Disconnect time events
    mitk::SliceNavigationController* timeNavigationController = mitk::RenderingManager::GetInstance()->GetTimeNavigationController();
    timeNavigationController->Disconnect(d->renderWindow->GetSliceNavigationController());
    timeNavigationController->Disconnect(d->renderWindowGradMag->GetSliceNavigationController());

    // Remove all observers
    for (int i = 0; i < ooLast; ++i) {
        d->_removeObserver(static_cast<ObservedObject>(i));
    }

    _removeGeometryNodeFromDataStorage();
}

void VesselDrivenResliceView::SetFocus()
{
	mitk::RenderingManager::GetInstance()->SetRenderWindowFocus(d->renderWindow->GetVtkRenderWindow());
    //d->renderWindow->GetRenderer()->SetFocused(true);
}

void VesselDrivenResliceView::CreateQtPartControl(QWidget *parent)
{
    d->mainLayout = new QVBoxLayout(parent);

    QHBoxLayout* hLayout = new QHBoxLayout(parent);
    hLayout->addWidget(createSelectedNodeWidget(parent));

    QToolButton* reslicePropertiesButton = new QToolButton(parent);
    reslicePropertiesButton->setIcon(QIcon(":/vesselSeg/icons/settings.png"));
    reslicePropertiesButton->setIconSize(QSize(24, 24));
    reslicePropertiesButton->setToolTip(tr("Modify the reslice window properties."));
    hLayout->addWidget(reslicePropertiesButton);

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
    connect(d->resliceWidgetVisibilityButton, &QAbstractButton::clicked, this, &VesselDrivenResliceView::_updateGeometryNodeInDataStorage);

    d->mainLayout->addLayout(hLayout);

    ctkPopupWidget* popup = new ctkPopupWidget(reslicePropertiesButton);
    QVBoxLayout* popupLayout = new QVBoxLayout(popup);

    auto reslicePropertiesWidget = new QWidget(popup);
    Ui::ReslicePropertiesWidget ui;
    ui.setupUi(reslicePropertiesWidget);

    popupLayout->addWidget(reslicePropertiesWidget);

    popup->setAlignment(Qt::AlignHCenter | Qt::AlignBottom); // at the top left corner
    popup->setHorizontalDirection(Qt::RightToLeft); // open outside the parent
    popup->setVerticalDirection(ctkBasePopupWidget::TopToBottom); // at the left of the spinbox sharing the top border
    // Control the animation
    popup->setAnimationEffect(ctkBasePopupWidget::FadeEffect); // could also be FadeEffect
    popup->setOrientation(Qt::Horizontal); // how to animate, could be Qt::Vertical or Qt::Horizontal|Qt::Vertical
    popup->setEasingCurve(QEasingCurve::OutQuart); // how to accelerate the animation, QEasingCurve::Type
    popup->setEffectDuration(100); // how long in ms.
    // Control the behavior
    popup->setAutoShow(false); // automatically open when the mouse is over the spinbox
    popup->setAutoHide(true); // automatically hide when the mouse leaves the popup or the spinbox.

    d->resliceWindowSizeSpinBox = ui.resliceWindowSizeSpinBox;

    connect(reslicePropertiesButton, SIGNAL(clicked()), popup, SLOT(showPopup()));
    connect(ui.resliceWindowSizeSpinBox, &QAbstractSpinBox::editingFinished, this, &VesselDrivenResliceView::_setResliceWindowSize);

    hLayout = new QHBoxLayout(parent);
    d->sliceNumberSlider = new ctkSliderWidget(parent);
    d->sliceNumberSlider->setDecimals(0);
    hLayout->addWidget(d->sliceNumberSlider);

    d->positionInMM = new QLabel(parent);
    d->positionInMM->setTextInteractionFlags(Qt::TextSelectableByMouse);
    d->positionInMM->setText("0.00 mm");
    d->positionInMM->setMinimumWidth(d->positionInMM->fontMetrics().width("9999.99 mm"));
    d->positionInMM->setToolTip(tr("Distance from the beginning of vessel path to current position"));
    d->positionInMM->setAlignment(Qt::AlignRight);
    
    hLayout->addWidget(d->positionInMM);
    
    d->mainLayout->addLayout(hLayout);


    auto renderWindowsLayout = new  QHBoxLayout(parent);

    d->renderWindow = new QmitkRenderWindow(parent, "reslicer", nullptr, mitk::RenderingManager::GetInstance());
    d->renderWindow->GetRenderer()->SetDataStorage(GetDataStorage());
    d->renderWindow->GetRenderer()->SetMapperID(mitk::BaseRenderer::Standard2D);
    d->renderWindow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    renderWindowsLayout->addWidget(d->renderWindow);


    d->renderWindowGradMag = new QmitkRenderWindow(parent, "reslicer grad mag", nullptr, mitk::RenderingManager::GetInstance());
    d->renderWindowGradMag->GetRenderer()->SetDataStorage(GetDataStorage());
    d->renderWindowGradMag->GetRenderer()->SetMapperID(mitk::BaseRenderer::Standard2D);
    d->renderWindowGradMag->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    renderWindowsLayout->addWidget(d->renderWindowGradMag);

    // Connect time events
    mitk::SliceNavigationController* timeNavigationController = mitk::RenderingManager::GetInstance()->GetTimeNavigationController();
    timeNavigationController->ConnectGeometryTimeEvent(d->renderWindow->GetSliceNavigationController(), false);
    timeNavigationController->ConnectGeometryTimeEvent(d->renderWindowGradMag->GetSliceNavigationController(), false);
    d->renderWindow->GetSliceNavigationController()->ConnectGeometryTimeEvent(timeNavigationController, false);
    d->renderWindowGradMag->GetSliceNavigationController()->ConnectGeometryTimeEvent(timeNavigationController, false);


    d->mainLayout->addLayout(renderWindowsLayout, 1);
    d->mainLayout->addStretch(0);

    // Setup synchronization of renderer's stepper and the slide number
    connect(d->sliceNumberSlider, &ctkSliderWidget::valueChanged, this, &VesselDrivenResliceView::_setSliceNumber);

    auto modifiedCommand = itk::MemberCommand<VesselDrivenResliceView>::New();
    modifiedCommand->SetCallbackFunction(this, &VesselDrivenResliceView::_syncSliderWithStepper);
    modifiedCommand->SetCallbackFunction(this, &VesselDrivenResliceView::_syncSliderWithStepperC);
    d->_addObserver(ooSNC, d->renderWindow->GetRenderer()->GetSliceNavigationController()->GetSlice(), itk::ModifiedEvent(), modifiedCommand);


    modifiedCommand = itk::MemberCommand<VesselDrivenResliceView>::New();
    modifiedCommand->SetCallbackFunction(this, &VesselDrivenResliceView::_syncSliderWithStepper);
    modifiedCommand->SetCallbackFunction(this, &VesselDrivenResliceView::_syncSliderWithStepperC);
    d->_addObserver(ooSNC_GradMag, d->renderWindowGradMag->GetRenderer()->GetSliceNavigationController()->GetSlice(), itk::ModifiedEvent(), modifiedCommand);


    // Connect to the reinitialize command as Global Reinit replaces the renderer's geometries
    auto reinitCommand = itk::SimpleMemberCommand<VesselDrivenResliceView>::New();
    reinitCommand->SetCallbackFunction(this, &VesselDrivenResliceView::_setupRendererSlices);
    d->_addObserver(ooRenderingManager, mitk::RenderingManager::GetInstance(), mitk::RenderingManagerViewsInitializedEvent(), reinitCommand);

    connect(&d->reinitVesselDrivenGeometryTimer, &QTimer::timeout, this, &VesselDrivenResliceView::_setupRendererSlices);

    d->resliceViewWidgetListener->registerListener();

    _setResliceViewEnabled(false);
    initializeCurrentNode();
    _updateGeometryNodeInDataStorage();
}

mitk::BaseRenderer* VesselDrivenResliceView::getResliceRenderer() const
{
    return d->renderWindow->GetRenderer();
}

std::vector<mitk::BaseRenderer*> VesselDrivenResliceView::getAllResliceRenderers() const
{
    std::vector<mitk::BaseRenderer*> renderers = { d->renderWindow->GetRenderer(), d->renderWindowGradMag->GetRenderer() };
    return renderers;
}

void VesselDrivenResliceView::navigateTo(const mitk::Point3D& pos)
{
    mitk::SliceNavigationController* snc = d->renderWindow->GetRenderer()->GetSliceNavigationController();
    const mitk::TimeGeometry* worldTimeGeometry = snc->GetInputWorldTimeGeometry();
    if (worldTimeGeometry == nullptr) {
        return;
    }
    auto vesselDrivenGeometry = static_cast<const crimson::VesselDrivenSlicedGeometry*>(worldTimeGeometry->GetGeometryForTimeStep(0).GetPointer());
    int sliceNo = vesselDrivenGeometry->findSliceByPoint(pos);
    snc->GetSlice()->SetPos(sliceNo);
    //d->renderWindowGradMag->GetRenderer()->GetSliceNavigationController()->GetSlice()->SetPos(sliceNo);
}

void VesselDrivenResliceView::navigateTo(float parameterValue)
{
    mitk::SliceNavigationController* snc = d->renderWindow->GetRenderer()->GetSliceNavigationController();
    const mitk::TimeGeometry* worldTimeGeometry = snc->GetInputWorldTimeGeometry();
    if (worldTimeGeometry == nullptr) {
        return;
    }
    auto vesselDrivenGeometry = static_cast<const crimson::VesselDrivenSlicedGeometry*>(worldTimeGeometry->GetGeometryForTimeStep(0).GetPointer());
    int sliceNo = vesselDrivenGeometry->getSliceNumberByParameterValue(parameterValue);
    snc->GetSlice()->SetPos(sliceNo);
    //d->renderWindowGradMag->GetRenderer()->GetSliceNavigationController()->GetSlice()->SetPos(sliceNo);
}

float VesselDrivenResliceView::getCurrentParameterValue() const
{
	auto geometry = dynamic_cast<const crimson::VesselDrivenSlicedGeometry*>(getResliceRenderer()->GetCurrentWorldGeometry());
    return geometry == nullptr ? 0 : geometry->getParameterValueBySliceNumber(getResliceRenderer()->GetSliceNavigationController()->GetSlice()->GetPos());
}

mitk::PlaneGeometry* VesselDrivenResliceView::getPlaneGeometry(float t) const
{
	auto geometry = dynamic_cast<const crimson::VesselDrivenSlicedGeometry*>(getResliceRenderer()->GetCurrentWorldGeometry());
    return geometry->GetPlaneGeometry(geometry->getSliceNumberByParameterValue(t));
}

void VesselDrivenResliceView::_setResliceViewEnabled(bool enabled)
{
    d->sliceNumberSlider->setEnabled(enabled);
    d->renderWindow->setEnabled(enabled);
    d->renderWindow->setVisible(enabled);
    d->renderWindowGradMag->setEnabled(enabled);
    d->renderWindowGradMag->setVisible(enabled);
}

void VesselDrivenResliceView::currentNodeChanged(mitk::DataNode*)
{
    _setResliceViewEnabled(_isCurrentVesselPathValid());
    if (currentNode()) {
        // As the current vessel path is always perpendicular to the view, increase the line width for better visibility
        currentNode()->SetIntProperty("vesselpath.line_width", 4, d->renderWindow->GetRenderer());
        currentNode()->SetIntProperty("vesselpath.selected_line_width", 4, d->renderWindow->GetRenderer());
        currentNode()->SetIntProperty("vesselpath.editing_line_width", 6, d->renderWindow->GetRenderer());
        currentNode()->SetSelected(true, d->renderWindow->GetRenderer());
        currentNode()->SetIntProperty("vesselpath.line_width", 4, d->renderWindowGradMag->GetRenderer());
        currentNode()->SetIntProperty("vesselpath.selected_line_width", 4, d->renderWindowGradMag->GetRenderer());
        currentNode()->SetIntProperty("vesselpath.editing_line_width", 6, d->renderWindowGradMag->GetRenderer());
        currentNode()->SetSelected(true, d->renderWindowGradMag->GetRenderer());

        float resliceWindowSize = 50;
        currentNode()->GetFloatProperty("reslice.windowSize", resliceWindowSize);
        d->resliceWindowSizeSpinBox->blockSignals(true);
        d->resliceWindowSizeSpinBox->setValue(resliceWindowSize);
        d->resliceWindowSizeSpinBox->blockSignals(false);

        _setupRendererSlices();
    }
    else {
        d->positionInMM->setText("0.00 mm");
    }
    _updateGeometryNodeInDataStorage();
}

void VesselDrivenResliceView::currentNodeModified()
{
    _setResliceViewEnabled(_isCurrentVesselPathValid());
    auto vesselPathInteractor = dynamic_cast<crimson::VesselPathInteractor*>(currentNode()->GetDataInteractor().GetPointer());
    if (vesselPathInteractor &&
        vesselPathInteractor->getMovingControlPoint() != -1 &&
        (d->renderWindow->GetRenderer() == vesselPathInteractor->lastEventSender() ||
        d->renderWindowGradMag->GetRenderer() == vesselPathInteractor->lastEventSender())) {
        auto vesselPath = static_cast<crimson::VesselPathAbstractData*>(currentNode()->GetData());
        // Do not update the geometry when moving the point in the reslice window
        // But ensure that after the move we reset the geometry on the control point being moved
        d->savedSlicePositions[currentNode()] = vesselPath->getControlPoint(vesselPathInteractor->getMovingControlPoint());
        return;
    }

    d->reinitVesselDrivenGeometryTimer.start(200);
    _updateGeometryNodeInDataStorage();
}

void VesselDrivenResliceView::forceReinitGeometry()
{
    if (!d->reinitVesselDrivenGeometryTimer.isActive()) {
        return; // Geometry is valid
    }

    d->reinitVesselDrivenGeometryTimer.stop();
    _setupRendererSlices();
}

void VesselDrivenResliceView::_setupRendererSlices()
{
    if (!_isCurrentVesselPathValid()) {
        return;
    }

    auto vesselPath = static_cast<crimson::VesselPathAbstractData*>(currentNode()->GetData());

    if (!vesselPath || vesselPath->controlPointsCount() == 0) {
        return;
    }

    float resliceWindowSize = 50;
    currentNode()->GetFloatProperty("reslice.windowSize", resliceWindowSize);

    mitk::ScalarType paramDelta;
    mitk::Vector3D referenceImageSpacing;
    unsigned int timeSteps;

    std::tie(paramDelta, referenceImageSpacing, timeSteps) = crimson::VascularModelingUtils::getResliceGeometryParameters(currentNode());
//    mitk::TimeBounds timeBounds;
//    timeBounds[0] = 0;
//    timeBounds[1] = 0;

    mitk::DataNode::Pointer imageNode = crimson::HierarchyManager::getInstance()->getAncestor(currentNode(), crimson::VascularModelingNodeTypes::Image());

    if (imageNode.IsNotNull()) {
        imageNode->SetBoolProperty("in plane resample extent by geometry", true, d->renderWindow->GetRenderer());
        imageNode->AddProperty("reslice interpolation", mitk::VtkResliceInterpolationProperty::New(VTK_RESLICE_CUBIC), d->renderWindow->GetRenderer(), true);
        imageNode->SetBoolProperty("show gradient magnitude", true, d->renderWindowGradMag->GetRenderer());
    }

    auto vesselDrivenGeometry = crimson::VesselDrivenSlicedGeometry::New();
    vesselDrivenGeometry->InitializedVesselDrivenSlicedGeometry(vesselPath, paramDelta, referenceImageSpacing, resliceWindowSize);
    
    // TODO: check on time-resolved image data
    // vesselDrivenGeometry->SetTimeBounds(timeBounds);

    mitk::ProportionalTimeGeometry::Pointer timeGeometry = mitk::ProportionalTimeGeometry::New();
    timeGeometry->Initialize(vesselDrivenGeometry, timeSteps);

    // Access the saved Id here because the geometry setup will trigger Stepper's modify function to be called
    mitk::Point3D savedSlicePos = d->savedSlicePositions.value(currentNode(), vesselPath->getPosition(0));

    mitk::SliceNavigationController* snc = d->renderWindow->GetRenderer()->GetSliceNavigationController();
    unsigned int currentGeometryTime = snc->GetTime()->GetPos();

    snc->SetInputWorldTimeGeometry(timeGeometry);
    snc->SetViewDirection(mitk::SliceNavigationController::Original);
    snc->Update();
    snc->GetTime()->SetPos(currentGeometryTime);

    snc = d->renderWindowGradMag->GetRenderer()->GetSliceNavigationController();
    snc->SetInputWorldTimeGeometry(timeGeometry);
    snc->SetViewDirection(mitk::SliceNavigationController::Original);
    snc->Update();
    snc->GetTime()->SetPos(currentGeometryTime);

    navigateTo(savedSlicePos);

    d->renderWindow->GetRenderer()->GetCameraController()->Fit();
	d->renderWindowGradMag->GetRenderer()->GetCameraController()->Fit();
}

void VesselDrivenResliceView::_setSliceNumber(double slice)
{
    unsigned int sliceNumber = static_cast<unsigned int>(slice);
    d->renderWindow->GetRenderer()->GetSliceNavigationController()->GetSlice()->SetPos(sliceNumber);
    d->renderWindowGradMag->GetRenderer()->GetSliceNavigationController()->GetSlice()->SetPos(sliceNumber);
    d->positionInMM->setText(QString("%1 mm").arg(getCurrentParameterValue(), 6, 'f', 2));
}

void VesselDrivenResliceView::_setResliceWindowSize()
{
    if (currentNode()) {
        currentNode()->SetFloatProperty("reslice.windowSize", d->resliceWindowSizeSpinBox->value());
        _setupRendererSlices();
        emit geometryChanged();
    }
}

void VesselDrivenResliceView::_syncSliderWithStepperC(const itk::Object* o, const itk::EventObject&)
{
    static bool updating;
    if (updating) {
        return;
    }
    updating = true;

    // const_cast due to lack of itkGetConstMacro() in the mitk::Stepper
    auto stepper = const_cast<mitk::Stepper*>(static_cast<const mitk::Stepper*>(o));


    // TODO: check from which render window does this signal come from
    d->sliceNumberSlider->setRange(0, stepper->GetSteps() - 1);
    d->sliceNumberSlider->setValue(stepper->GetPos());

    auto timeGeometry = d->renderWindow->GetRenderer()->GetSliceNavigationController()->GetInputWorldTimeGeometry();
    if (timeGeometry) {
        // Avoid saving slice ID's if they come from geometry replacement upon global reinit
        auto vesselDrivenGeometry =
            dynamic_cast<const crimson::VesselDrivenSlicedGeometry*>(timeGeometry->GetGeometryForTimeStep(0).GetPointer());
        if (vesselDrivenGeometry) {
            d->savedSlicePositions[currentNode()] = vesselDrivenGeometry->getSliceCenter(stepper->GetPos());

            emit sliceChanged(vesselDrivenGeometry->getParameterValueBySliceNumber(stepper->GetPos()));
        }
    }

    updating = false;
}

void VesselDrivenResliceView::_updateGeometryNodeInDataStorage()
{
    mitk::DataNode* resliceWidgetNode = d->renderWindow->GetRenderer()->GetCurrentWorldPlaneGeometryNode();
    resliceWidgetNode->SetVisibility(_isCurrentVesselPathValid() && d->resliceWidgetVisibilityButton->isChecked() && 
        this->GetSite() && this->GetSite()->GetPage() && this->GetSite()->GetPage()->IsPartVisible(berry::IWorkbenchPart::Pointer(this)));
    if (!GetDataStorage()->Exists(resliceWidgetNode)) {
        resliceWidgetNode->SetBoolProperty("helper object", true);
        resliceWidgetNode->SetName("Vessel reslice plane");
        resliceWidgetNode->SetIntProperty("Crosshair.Gap Size", 4);
        resliceWidgetNode->SetBoolProperty("Crosshair.Ignore", true);
        GetDataStorage()->Add(resliceWidgetNode);
    }
    mitk::RenderingManager::GetInstance()->RequestUpdateAll(mitk::RenderingManager::REQUEST_UPDATE_3DWINDOWS);
}

void VesselDrivenResliceView::_removeGeometryNodeFromDataStorage()
{
    mitk::DataNode* resliceWidgetNode = d->renderWindow->GetRenderer()->GetCurrentWorldPlaneGeometryNode();
    if (GetDataStorage()->Exists(resliceWidgetNode)) {
        GetDataStorage()->Remove(resliceWidgetNode);
    }
}


bool VesselDrivenResliceView::_isCurrentVesselPathValid()
{
    return currentNode() != nullptr && static_cast<crimson::VesselPathAbstractData*>(currentNode()->GetData())->controlPointsCount() >= 2;
}
