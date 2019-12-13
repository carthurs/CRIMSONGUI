// Blueberry
#include <berryISelectionService.h>
#include <berryISelectionProvider.h>
#include <berryIWorkbenchWindow.h>
#include <berryIWorkbenchPage.h>

#include "VesselPathPlanningView.h"
#include "VesselForestData.h"
#include "vesselPathItemModel.h"
#include "vtkParametricSplineVesselPathData.h"
#include "VesselPathInteractor.h"
#include "VesselDrivenResliceView.h"
#include "HierarchyManager.h"
#include "VesselPathOperation.h"

#include <VascularModelingNodeTypes.h>

#include <usModuleRegistry.h>

// Qt
#include <QMessageBox>
#include <QInputDialog>
#include <QShortcut>

#include <QmitkStdMultiWidget.h>
#include <QmitkStdMultiWidgetEditor.h>
#include <mitkNodePredicateDataType.h>

#include <mitkOperationEvent.h>
#include <mitkUndoController.h>
#include <mitkInteractionConst.h>

#include <itkCommand.h>

const std::string VesselPathPlanningView::VIEW_ID = "org.mitk.views.vesselpathplanningview";

VesselPathPlanningView::VesselPathPlanningView()
: _vesselPathInteractor(crimson::VesselPathInteractor::New())
, _vesselPathItemModel(new VesselPathItemModel())
{
    us::Module* vesselForestModule = us::ModuleRegistry::GetModule("VesselTree");
    _vesselPathInteractor->LoadStateMachine("VesselPathInteraction.xml", vesselForestModule);
    _vesselPathInteractor->SetEventConfig("VesselPathConfig.xml", vesselForestModule);
}

VesselPathPlanningView::~VesselPathPlanningView()
{
    editVesselPath(false);
    if (_partListener) {
        _partListener->unregisterListener();
    }
}


void VesselPathPlanningView::SetFocus()
{
//     m_Controls.buttonPerformImageProcessing->setFocus();
//     m_Controls.buttonPerformImageProcessing->setEnabled(true);
}

void VesselPathPlanningView::CreateQtPartControl(QWidget *parent)
{
    _viewWidget = parent;
    // create GUI widgets from the Qt Designer's .ui file
    _UI.setupUi(parent);
    _UI.vesselPathView->setModel(_vesselPathItemModel.get());

    connect(_UI.createTreeButton, &QAbstractButton::clicked, this, &VesselPathPlanningView::createNewVesselTree);
    connect(_UI.createPathButton, &QAbstractButton::clicked, this, [this]() { createNewVesselPath(nullptr); });
    connect(_UI.duplicatePathButton, &QAbstractButton::clicked, this, &VesselPathPlanningView::duplicateVesselPath);
    connect(_UI.editPathButton, &QAbstractButton::toggled, this, &VesselPathPlanningView::editVesselPath);
    connect(_UI.vesselPathView->verticalHeader(), &QHeaderView::sectionDoubleClicked, this, &VesselPathPlanningView::navigateToControlPoint);

    connect(_UI.addAutoButton, &QAbstractButton::clicked, this, &VesselPathPlanningView::setAddPointType);
    connect(_UI.addToStartButton, &QAbstractButton::clicked, this, &VesselPathPlanningView::setAddPointType);
    connect(_UI.addToEndButton, &QAbstractButton::clicked, this, &VesselPathPlanningView::setAddPointType);

    connect(_UI.tensionSlider, &QAbstractSlider::valueChanged, this, &VesselPathPlanningView::setSplineTension);
    connect(_UI.tensionSlider, &QAbstractSlider::sliderPressed, this, &VesselPathPlanningView::startTensionChange);
    connect(_UI.tensionSlider, &QAbstractSlider::sliderReleased, this, &VesselPathPlanningView::finishTensionChange);

    connect(_UI.createMaskedImageButton, &QAbstractButton::clicked, this, &VesselPathPlanningView::createMaskedImage);

    _partListener.reset(
        new crimson::VesselDrivenResliceViewListener([this](VesselDrivenResliceView* view) { this->setVesselDrivenResliceView(view); }, GetSite().GetPointer()));
    _partListener->registerListener();
    _updateUI();
}

void VesselPathPlanningView::_createCancelInteractionShortcut()
{
    // Set up the shortcut for cancelling interaction if user pressed Esc
    assert(_cancelInteractionShortcut == nullptr);
    _cancelInteractionShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), _viewWidget, nullptr, nullptr, Qt::ApplicationShortcut);
    connect(_cancelInteractionShortcut, &QShortcut::activated, _UI.editPathButton, &QAbstractButton::toggle);
}

void VesselPathPlanningView::_removeCancelInteractionShortcut()
{
    delete _cancelInteractionShortcut;
    _cancelInteractionShortcut = nullptr;
}

void VesselPathPlanningView::OnSelectionChanged(berry::IWorkbenchPart::Pointer /*source*/,
    const QList<mitk::DataNode::Pointer>& nodes)
{
    this->FireNodesSelected(nodes); // Sync selection

    if (_currentVesselPathNode) {
        // Unsubscribe from old vessel path change events
        _currentVesselPathNode->GetData()->RemoveObserver(_observerTag);
        _observerTag = 0;
    }

    mitk::DataNode* pathNode = nullptr;
    mitk::DataNode* forestNode = nullptr;
    mitk::DataNode* imageNode = nullptr;

    // Find relevant nodes in the selection
    for (const mitk::DataNode::Pointer& node : nodes) {
        if (!GetDataStorage()->Exists(node)) {
            continue;
        }

        if (crimson::HierarchyManager::getInstance()->getPredicate(crimson::VascularModelingNodeTypes::VesselPath())->CheckNode(node)) {
            pathNode = node;
        }
        else if (crimson::HierarchyManager::getInstance()->getPredicate(crimson::VascularModelingNodeTypes::VesselTree())->CheckNode(node)) {
            forestNode = node;
        }
        else if (crimson::HierarchyManager::getInstance()->getPredicate(crimson::VascularModelingNodeTypes::Image())->CheckNode(node)) {
            imageNode = node;
        }
    }

    // Try and find the path among sources of selected nodes if none found so far
    if (!pathNode) {
        for (const mitk::DataNode::Pointer& node : nodes) {
            if (!GetDataStorage()->Exists(node)) {
                continue;
            }

            if ((pathNode = crimson::HierarchyManager::getInstance()->getAncestor(node, crimson::VascularModelingNodeTypes::VesselPath())) != nullptr) {
                break;
            }
        }
    }

    // Selection priority: path > forest > image - ignore the selection of lower priority nodes if the higher priority node is selected
    if (pathNode) {
        forestNode = nullptr;
        imageNode = nullptr;
    }

    if (forestNode) {
        imageNode = nullptr;
    }

    // Try to find the lower priority nodes among the sources of the higher priority ones
    if (pathNode) {
        forestNode = crimson::HierarchyManager::getInstance()->getAncestor(pathNode, crimson::VascularModelingNodeTypes::VesselTree());
    }

    if (forestNode) {
        imageNode = crimson::HierarchyManager::getInstance()->getAncestor(forestNode, crimson::VascularModelingNodeTypes::Image());
    }

    if (_currentVesselPathNode != pathNode) {
        _UI.editPathButton->setChecked(false);
    }

    // Setup the UI according to selection
    _currentImageNode = imageNode;
    _currentVesselForestNode = forestNode;
    _currentVesselPathNode = pathNode;
    _vesselPathItemModel->setVesselPath(_currentVesselPathNode ? static_cast<crimson::VesselPathAbstractData*>(_currentVesselPathNode->GetData()) : nullptr);

    setAddPointType();

    if (_currentVesselPathNode) {
        auto vesselPath = dynamic_cast<crimson::vtkParametricSplineVesselPathData*>(_currentVesselPathNode->GetData());
        if (vesselPath) {
            _UI.tensionSlider->blockSignals(true);
            _UI.tensionSlider->setValue(vesselPath->getTension() * 100);
            _UI.tensionSlider->blockSignals(false);

            // Subscribe to new vessel path change events
            auto modifiedCommand = itk::MemberCommand<VesselPathPlanningView>::New();
            modifiedCommand->SetCallbackFunction(this, &VesselPathPlanningView::_onTensionChanged);

            _observerTag = vesselPath->AddObserver(crimson::vtkParametricSplineVesselPathData::TensionChangeEvent(), modifiedCommand);
        }
    }

    selectClosestControlPoints();
    _updateUI();

    return;
}

void VesselPathPlanningView::_updateUI()
{
    // Enable creating new vessel forests if a reference image is available
    _UI.createTreeButton->setEnabled(_currentImageNode != nullptr);

    _UI.createPathButton->setEnabled(_currentVesselForestNode != nullptr);
    _UI.editPathButton->setEnabled(_currentVesselPathNode != nullptr);
    _UI.duplicatePathButton->setEnabled(_currentVesselPathNode != nullptr);
    _UI.vesselPathView->setEnabled(_currentVesselPathNode != nullptr);

    _UI.tensionSlider->setEnabled(_currentVesselPathNode != nullptr &&
        dynamic_cast<crimson::vtkParametricSplineVesselPathData*>(_currentVesselPathNode->GetData()) != nullptr);

    _updateNodeName(_UI.referenceImageLineEdit, _currentImageNode);
    _updateNodeName(_UI.vesselTreeLineEdit, _currentVesselForestNode);
    _updateNodeName(_UI.vesselPathLineEdit, _currentVesselPathNode);
}

void VesselPathPlanningView::_updateNodeName(QLineEdit* lineEdit, const mitk::DataNode* node) 
{
    lineEdit->setText(node ? QString::fromStdString(node->GetName()) : "");
    lineEdit->setToolTip(node ? lineEdit->text() : lineEdit->placeholderText());
}

void VesselPathPlanningView::createNewVesselTree()
{
    assert(_currentImageNode != nullptr);

    bool ok;
    QString newTreeName = QInputDialog::getText(nullptr, tr("Enter vessel tree name"), tr("Vessel tree name"), QLineEdit::Normal, tr("New vessel tree"), &ok);

    if (!ok) {
        return;
    }
    
    auto newNode = mitk::DataNode::New();
    auto newData = crimson::VesselForestData::New();

    newNode->SetData(newData);
    newNode->SetName(newTreeName.toStdString());

    crimson::HierarchyManager::getInstance()->addNodeToHierarchy(_currentImageNode, crimson::VascularModelingNodeTypes::Image(), newNode, crimson::VascularModelingNodeTypes::VesselTree());

    _selectDataNode(newNode);
}

void VesselPathPlanningView::createNewVesselPath(mitk::DataNode* copyFrom)
{
    assert(_currentVesselForestNode != nullptr);

    bool ok;
    QString newName = tr("New vessel path");
    if (copyFrom) {
        newName = QString(copyFrom->GetName().c_str()) + " copy";
    }

    QString newPathName = QInputDialog::getText(nullptr, tr("Enter vessel path name"), tr("Vessel path name"), QLineEdit::Normal, newName, &ok);

    if (!ok) {
        return;
    }

    auto newNode = mitk::DataNode::New();
    auto newData = copyFrom ? static_cast<crimson::vtkParametricSplineVesselPathData*>(copyFrom->GetData())->Clone() : crimson::vtkParametricSplineVesselPathData::New();

    newNode->SetData(newData);
    newNode->SetName(newPathName.toStdString());
    // Assign random color
    auto randomColor = QColor::fromHsv(qrand() % 360, 255, 210);
    newNode->SetColor(randomColor.redF(), randomColor.greenF(), randomColor.blueF());

    _vesselPathNodes[newData] = newNode;

    crimson::HierarchyManager::getInstance()->addNodeToHierarchy(_currentVesselForestNode, crimson::VascularModelingNodeTypes::VesselTree(), newNode, crimson::VascularModelingNodeTypes::VesselPath());

    _selectDataNode(newNode);
}

void VesselPathPlanningView::duplicateVesselPath()
{
    createNewVesselPath(_currentVesselPathNode);
}


void VesselPathPlanningView::_selectDataNode(mitk::DataNode* newNode)
{
    this->FireNodeSelected(newNode);
    this->OnSelectionChanged(GetSite()->GetPart(), QList<mitk::DataNode::Pointer>() << newNode);
}

void VesselPathPlanningView::NodeAdded(const mitk::DataNode* constNode)
{
    auto node = const_cast<mitk::DataNode*>(constNode);

    // When a vessel forest node is added upon loading a project or importing the vessel tree - restore the path -> node map
    auto vesselPathData = dynamic_cast<crimson::VesselPathAbstractData*>(node->GetData());

    if (vesselPathData) {
        _vesselPathNodes[vesselPathData] = node;
        node->SetBoolProperty("vesselpath.editing", false);
    }
}

void VesselPathPlanningView::NodeRemoved(const mitk::DataNode* node)
{
    // Check if a data node is a vessel path. If so, remove it from the vessel forest
    for (auto kv : _vesselPathNodes) {
        if (kv.second == node) {
            // Remove the vessel path from the forest
            auto sources = GetDataStorage()->GetSources(node, mitk::TNodePredicateDataType<crimson::VesselForestData>::New());
            if (sources->size() > 0) {
                auto vesselForestData = dynamic_cast<crimson::VesselForestData*>((*sources)[0]->GetData());
                assert(vesselForestData);
                Q_UNUSED(vesselForestData);

                _vesselPathNodes.erase(kv.first);
            }

            break;
        }
    }

    if (node == _currentVesselPathNode) {
        // Unsubscribe from old vessel path change events
        _currentVesselPathNode->GetData()->RemoveObserver(_observerTag);
        _observerTag = 0;

        _currentVesselPathNode = nullptr;
        this->OnSelectionChanged(GetSite()->GetPart(), QList<mitk::DataNode::Pointer>() << _currentVesselForestNode);
    }
    else if (node == _currentVesselForestNode) {
        _currentVesselPathNode = nullptr;
        this->OnSelectionChanged(GetSite()->GetPart(), QList<mitk::DataNode::Pointer>() << _currentImageNode);
    }
    else if (node == _currentImageNode) {
        this->OnSelectionChanged(GetSite()->GetPart(), QList<mitk::DataNode::Pointer>());
    }
}

void VesselPathPlanningView::NodeChanged(const mitk::DataNode* node)
{
    if (node == _currentImageNode) {
        _updateNodeName(_UI.referenceImageLineEdit, node);
    }

    if (node == _currentVesselForestNode) {
        _updateNodeName(_UI.vesselTreeLineEdit, node);
    }

    if (node == _currentVesselPathNode) {
        _updateNodeName(_UI.vesselPathLineEdit, node);
    }
}

void VesselPathPlanningView::editVesselPath(bool enable)
{
    if (enable) {
        assert(_currentVesselPathNode);
        _currentVesselPathNode->SetVisibility(true);
        _vesselPathInteractor->SetDataNode(_currentVesselPathNode);
        _currentVesselPathNode->SetBoolProperty("vesselpath.editing", true);
        _createCancelInteractionShortcut();
    }
    else {
        if (_vesselPathInteractor->GetDataNode()) {
            _vesselPathInteractor->GetDataNode()->SetBoolProperty("vesselpath.editing", false);
            _vesselPathInteractor->SetDataNode(nullptr);
        }
        _removeCancelInteractionShortcut();
    }

    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void VesselPathPlanningView::navigateToControlPoint(int id)
{
    if (!_vesselDrivenResliceView) {
        return;
    }
    auto vessel = static_cast<crimson::VesselPathAbstractData*>(_currentVesselPathNode->GetData());
    _vesselDrivenResliceView->navigateTo(vessel->getControlPointParameterValue(id));

    QmitkStdMultiWidgetEditor* qSMWE = dynamic_cast<QmitkStdMultiWidgetEditor*>(GetRenderWindowPart());
    if (qSMWE) {
        qSMWE->GetStdMultiWidget()->MoveCrossToPosition(vessel->getControlPoint(id));
    }
}

void VesselPathPlanningView::setAddPointType()
{
    crimson::VesselPathInteractor::AddPointType type = crimson::VesselPathInteractor::aptNone;

    if (_UI.addAutoButton->isChecked()) {
        type = crimson::VesselPathInteractor::aptAuto;
    }
    else if (_UI.addToStartButton->isChecked()) {
        type = crimson::VesselPathInteractor::aptStart;
    }
    else if (_UI.addToEndButton->isChecked()) {
        type = crimson::VesselPathInteractor::aptEnd;
    }

    if (_currentVesselPathNode) {
        _currentVesselPathNode->SetIntProperty("vesselpath.addPointType", static_cast<int>(type));
    }
}

void VesselPathPlanningView::startTensionChange()
{
    // start/finishTensionChange function pair allow to avoid an undo item to be created during slider dragging
    _tensionSliderDragStarted = true;
    _startTension = static_cast<crimson::vtkParametricSplineVesselPathData*>(_currentVesselPathNode->GetData())->getTension();
}

void VesselPathPlanningView::finishTensionChange()
{
    auto data = static_cast<crimson::vtkParametricSplineVesselPathData*>(_currentVesselPathNode->GetData());

    mitk::OperationEvent::IncCurrObjectEventId();
    //mitk::OperationEvent::ExecuteIncrement();

    auto doOp = new crimson::VesselPathOperation(mitk::OpSCALE, crimson::VesselPathOperation::PointType(), 0, data->getTension());
    auto undoOp = new crimson::VesselPathOperation(mitk::OpSCALE, crimson::VesselPathOperation::PointType(), 0, _startTension);
    auto operationEvent = new mitk::OperationEvent(data, doOp, undoOp, "Set spline tension");
    mitk::UndoController::GetCurrentUndoModel()->SetOperationEvent(operationEvent);

    _tensionSliderDragStarted = false;
}

void VesselPathPlanningView::setSplineTension(int value)
{
    auto data = static_cast<crimson::vtkParametricSplineVesselPathData*>(_currentVesselPathNode->GetData());
    float tension = value / 100.0;
    float prevTension = data->getTension();

    data->setTension(tension);
    if (!_tensionSliderDragStarted) {
        _startTension = prevTension;
        finishTensionChange();
    }
    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void VesselPathPlanningView::_onTensionChanged(itk::Object *caller, const itk::EventObject &event)
{
    auto tensionChangeEvt = dynamic_cast<const crimson::vtkParametricSplineVesselPathData::TensionChangeEvent*>(&event);
    if (tensionChangeEvt) {
        _UI.tensionSlider->blockSignals(true);
        _UI.tensionSlider->setValue(static_cast<crimson::vtkParametricSplineVesselPathData*>(caller)->getTension() * 100);
        _UI.tensionSlider->blockSignals(false);
    }
}

void VesselPathPlanningView::setVesselDrivenResliceView(VesselDrivenResliceView* view)
{
    if (_vesselDrivenResliceView == view) {
        return;
    }

    if (_vesselDrivenResliceView) {
        disconnect(_vesselDrivenResliceView, &VesselDrivenResliceView::sliceChanged, this, &VesselPathPlanningView::selectClosestControlPoints);
        disconnect(_vesselDrivenResliceView, &VesselDrivenResliceView::geometryChanged, this, &VesselPathPlanningView::selectClosestControlPoints);
    }

    _vesselDrivenResliceView = view;

    if (_vesselDrivenResliceView) {
        connect(_vesselDrivenResliceView, &VesselDrivenResliceView::sliceChanged, this, &VesselPathPlanningView::selectClosestControlPoints);
        connect(_vesselDrivenResliceView, &VesselDrivenResliceView::geometryChanged, this, &VesselPathPlanningView::selectClosestControlPoints);
    }

    selectClosestControlPoints();
}

void VesselPathPlanningView::selectClosestControlPoints()
{
    // Select the control points closest to the current position of the reslice window

    _UI.vesselPathView->clearSelection();

    if (_currentVesselPathNode && _vesselDrivenResliceView) {
        // Find closest points 
        auto vesselPathData = dynamic_cast<crimson::VesselPathAbstractData*>(_currentVesselPathNode->GetData());

        int firstId = -1, lastId = -1;

        float parameterValue = _vesselDrivenResliceView->getCurrentParameterValue();

        for (size_t i = 0; i < vesselPathData->controlPointsCount(); ++i) {
            if (fabsf(parameterValue - vesselPathData->getControlPointParameterValue(i)) < 1e-4) {
                // The reslice is at a control point
                firstId = lastId = i;
                break;
            }

            if (parameterValue < vesselPathData->getControlPointParameterValue(i)) {
                // The reslice is between control points
                firstId = std::max(0, static_cast<int>(i) - 1);
                lastId = i;
                break;
            }
        }

        if (firstId < 0) {
            firstId = lastId = vesselPathData->controlPointsCount() - 1;
        }

        QItemSelection selectedItems;
        selectedItems.select(_UI.vesselPathView->model()->index(firstId, 0, QModelIndex()), _UI.vesselPathView->model()->index(lastId, _UI.vesselPathView->model()->columnCount() - 1, QModelIndex()));
        _UI.vesselPathView->selectionModel()->select(selectedItems, QItemSelectionModel::ClearAndSelect);
        _UI.vesselPathView->scrollTo(_UI.vesselPathView->model()->index(firstId, 0, QModelIndex()));
        _UI.vesselPathView->scrollTo(_UI.vesselPathView->model()->index(lastId, 0, QModelIndex()));
    }
}




//////////////////////////////////////////////////////////////////////////
// TODO: MASKED IMAGE CREATION
//////////////////////////////////////////////////////////////////////////

#include "ui_createMaskedImageDialog.h"

#include <mitkSurfaceToImageFilter.h>
#include <mitkImageStatisticsHolder.h>
#include <mitkImagePixelWriteAccessor.h>
#include <mitkImagePixelReadAccessor.h>
#include <mitkImageTimeSelector.h>
#include <SolidData.h>

struct IndexComparator {
    bool operator()(const itk::Index<3>&l, const itk::Index<3>&r) {
        return l[0] != r[0] ? l[0] < r[0] : (l[1] != r[1] ? l[1] < r[1] : l[2] < r[2]);
    }
};

typedef std::set < itk::Index<3>, IndexComparator > FrontType;

template<typename TPixel>
void dilate(mitk::ImagePixelWriteAccessor<TPixel, 3>& writeAccess, 
    mitk::ImagePixelReadAccessor<TPixel, 3>& originalImage,
    itk::Index<3> index, 
    FrontType& currFront, 
    FrontType& nextFront, mitk::ScalarType bkValue, bool offsetting)
{
    TPixel sum = 0;
    int n = 0;
    bool needWrite = fabs(writeAccess.GetPixelByIndex(index) - bkValue) < 1e-6;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            for (int z = -1; z <= 1; ++z) {
                if (x == 0 && y == 0 && z == 0) {
                    continue;
                }
                itk::Index<3> offs = index;
                offs[0] += x;
                offs[1] += y;
                offs[2] += z;

                bool out = false;
                for (int i = 0; i < 3; ++i) {
                    if (offs[i] < 0 || offs[i] >= writeAccess.GetDimension(i)) {
                        out = true;
                        break;
                    }
                }
                if (out) {
                    continue;
                }


                if (fabs(writeAccess.GetPixelByIndex(offs) - bkValue) < 1e-6) {
                    if (currFront.find(offs) == currFront.end()) {
                        nextFront.insert(offs);
                    }
                }
                else if (needWrite/* && currFront.find(offs) == currFront.end()*/) {
                    sum += writeAccess.GetPixelByIndex(offs);
                    n++;
                }
            }
        }
    }
//     for (int i = 0; i < 3; ++i) {
//         for (int dir = -1; dir <= 1; dir += 2) {
//             itk::Index<3> offs = index;
//             offs[i] += dir;
//             if (offs[i] >= 0 && offs[i] < writeAccess.GetDimension(i)) {
//                 if (fabs(writeAccess.GetPixelByIndex(offs) - bkValue) < 1e-6) {
//                     if (currFront.find(offs) == currFront.end()) {
//                         nextFront.insert(offs);
//                     }
//                 }
//                 else if (needWrite && currFront.find(offs) == currFront.end()) {
//                     sum += writeAccess.GetPixelByIndex(offs);
//                     n++;
//                 }
//             }
//         }
//     }

    if (needWrite && n > 0) {
        if (offsetting) {
            writeAccess.SetPixelByIndex(index, originalImage.GetPixelByIndex(index));
        }
        else {
            writeAccess.SetPixelByIndex(index, sum / n);
        }
    }
}

template<typename TPixel>
bool extrapolate(const mitk::Image::Pointer& image, const mitk::Image::Pointer& orgImage, mitk::ScalarType bkValue, int offset, bool expandOnly)
{
    try {
        mitk::ImagePixelWriteAccessor<TPixel, 3> writeAccess(image);
        mitk::ImagePixelReadAccessor<TPixel, 3> readAccess(orgImage);

        itk::Index<3> index;
        FrontType fullVoxels;
        for (index[2] = 0; index[2] < writeAccess.GetDimension(2); ++index[2]) {
            for (index[1] = 0; index[1] < writeAccess.GetDimension(1); ++index[1]) {
                for (index[0] = 0; index[0] < writeAccess.GetDimension(0); ++index[0]) {
                    if (fabs(writeAccess.GetPixelByIndex(index) - bkValue) > 1e-6) {
                        fullVoxels.insert(index);
                    }
                }
            }
        }

        FrontType currFront = fullVoxels;


        int iter = 0;
        while (!currFront.empty()) {
            FrontType newFront;
            for (auto idx : currFront) {
                dilate(writeAccess, readAccess, idx, currFront, newFront, bkValue, iter < offset);
            }
            if (++iter == offset && expandOnly) {
                break;
            }
            currFront = newFront;
        }

        return true;
    }
    catch (...) {
        return false;
    }
}

void VesselPathPlanningView::createMaskedImage()
{
    QDialog dlg;
    Ui::CreateMaskedImageDialog ui;
    ui.setupUi(&dlg);
    if (dlg.exec() == QDialog::Rejected) {
        return;
    }

    if (!_currentImageNode) {
        MITK_ERROR << "No image selected!";
        return;
    }

    mitk::Image::Pointer image = static_cast<mitk::Image*>(_currentImageNode->GetData());

    mitk::DataNode* surfaceNode = crimson::HierarchyManager::getInstance()->getFirstDescendant(_currentImageNode, crimson::VascularModelingNodeTypes::Blend());
    if (!surfaceNode) {
        MITK_ERROR << "No blended surface found!";
        return;
    }

	mitk::Surface* surfaceData = static_cast<crimson::SolidData*>(surfaceNode->GetData())->getSurfaceRepresentation();

    if (image->GetDimension() == 4) {
        mitk::ImageTimeSelector::Pointer timeStepExtractor = mitk::ImageTimeSelector::New();
        int step = mitk::RenderingManager::GetInstance()->GetTimeNavigationController()->GetTime()->GetPos();
        timeStepExtractor->SetTimeNr(step);
        timeStepExtractor->SetInput(image);
        timeStepExtractor->Update();
        image = timeStepExtractor->GetOutput();
        static_cast<mitk::ProportionalTimeGeometry*>(image->GetTimeGeometry())->SetFirstTimePoint(0);
    }


    mitk::SurfaceToImageFilter::Pointer maskMaker = mitk::SurfaceToImageFilter::New();
    maskMaker->SetImage(image);
    maskMaker->SetInput(surfaceData);
    maskMaker->SetMakeOutputBinary(false);
    mitk::ScalarType bkValue = ui.extrapolateCheckBox->isChecked() ? image->GetStatistics()->GetScalarValueMin() - 1 : ui.bkValueSpinBox->value();
    maskMaker->SetBackgroundValue(bkValue);
    maskMaker->Update();

    mitk::Image::Pointer maskedImageData = maskMaker->GetOutput();

    bool success = false;
    success = success || extrapolate<unsigned char>(maskedImageData, image, bkValue, ui.offsetSpinBox->value(), !ui.extrapolateCheckBox->isChecked());
    success = success || extrapolate<char>(maskedImageData, image, bkValue, ui.offsetSpinBox->value(), !ui.extrapolateCheckBox->isChecked());
    success = success || extrapolate<unsigned int>(maskedImageData, image, bkValue, ui.offsetSpinBox->value(), !ui.extrapolateCheckBox->isChecked());
    success = success || extrapolate<int>(maskedImageData, image, bkValue, ui.offsetSpinBox->value(), !ui.extrapolateCheckBox->isChecked());
    success = success || extrapolate<float>(maskedImageData, image, bkValue, ui.offsetSpinBox->value(), !ui.extrapolateCheckBox->isChecked());
    success = success || extrapolate<double>(maskedImageData, image, bkValue, ui.offsetSpinBox->value(), !ui.extrapolateCheckBox->isChecked());

    mitk::DataNode::Pointer newNode = mitk::DataNode::New();
    newNode->SetName(_currentImageNode->GetName() + " masked");
    newNode->SetData(maskedImageData);
    GetDataStorage()->Add(newNode, _currentImageNode);
}
