#include <set>
#include <QMessageBox>
#include <ctkPopupWidget.h>
#include <QtPropertyStorage.h>

#include <usModule.h>
#include <usModuleRegistry.h>

#include <HierarchyManager.h>
#include <VesselMeshingNodeTypes.h>
#include <FaceData.h>
#include <SolidData.h>

#include "FaceDataEditorWidget.h"
#include "PCMRIMappingWidget.h"
#include "PCMRIUtils.h"

#include <SelectFacesFromListDialog.h>

namespace crimson
{
FaceDataEditorWidget::FaceDataEditorWidget(QWidget* parent)
    : QWidget(parent)
    , _pickingEventObserverServiceTracker(us::ModuleRegistry::GetModule(1)->GetModuleContext(),
        us::LDAPFilter("(name=DataNodePicker)"))
{
    _pickingEventObserverServiceTracker.Open();
    _facePickingObserverServiceRegistration = us::ModuleRegistry::GetModule(1)->GetModuleContext()->RegisterService(
        static_cast<mitk::InteractionEventObserver*>(&_facePickingObserver));
    _facePickingObserver.Disable();

    _UI.setupUi(this);

    connect(_UI.selectFacesButton, &QAbstractButton::toggled, this, &FaceDataEditorWidget::_setFaceSelectionMode);
    connect(_UI.applyToAllWallsCheckBox, &QAbstractButton::toggled, this, &FaceDataEditorWidget::_setApplyToAllWallsFlag);

    _UI.boundaryConditionPropertyBrowser->setRootIsDecorated(false);

    _faceListProxyModel.setSourceModel(&_faceListModel);
    _UI.faceListTable->setModel(&_faceListProxyModel);

    ctkPopupWidget* popup = new ctkPopupWidget(_UI.faceSelectionSettingsButton);
    QVBoxLayout* popupLayout = new QVBoxLayout(popup);

    // The popup for advanced option of allowing to select any faces regardless of 
    // what the face data requires (e.g. apply an RCR to a wall)
    auto allowAnyFacesCheckBox = new QCheckBox(popup);
    allowAnyFacesCheckBox->setText("Allow any faces to be selected. Warning: for advanced users only!");

    popupLayout->addWidget(allowAnyFacesCheckBox);

    popup->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);      // at the top left corner
    popup->setHorizontalDirection(Qt::RightToLeft);               // open outside the parent
    popup->setVerticalDirection(ctkBasePopupWidget::TopToBottom); // at the left of the spinbox sharing the top border
    // Control the animation
    popup->setAnimationEffect(ctkBasePopupWidget::FadeEffect); // could also be FadeEffect
    popup->setOrientation(Qt::Horizontal);                     // how to animate, could be Qt::Vertical or Qt::Horizontal|Qt::Vertical
    popup->setEasingCurve(QEasingCurve::OutQuart);             // how to accelerate the animation, QEasingCurve::Type
    popup->setEffectDuration(100);                             // how long in ms.
    // Control the behavior
    popup->setAutoShow(false); // automatically open when the mouse is over the spinbox
    popup->setAutoHide(true);  // automatically hide when the mouse leaves the popup or the spinbox.

    connect(_UI.faceSelectionSettingsButton, &QAbstractButton::clicked, [popup]() { popup->showPopup(); });
    connect(allowAnyFacesCheckBox, &QCheckBox::toggled, [this](bool on) { _allowAnyFaceSelection = on; });

    connect(_UI.selectFromListButton, &QAbstractButton::clicked, this, &FaceDataEditorWidget::_selectFacesFromList);

    _updateUI();
}

FaceDataEditorWidget::~FaceDataEditorWidget()
{
    setCurrentNode(nullptr);
    _setFaceSelectionMode(false);
    _facePickingObserverServiceRegistration.Unregister();
}

void FaceDataEditorWidget::_updateUI()
{
    if (_currentNode) {
        auto faceData = static_cast<FaceData*>(_currentNode->GetData());

        auto applicableFaceTypes = faceData->applicableFaceTypes();
        _UI.applyToAllWallsCheckBox->setEnabled(std::find(applicableFaceTypes.begin(), applicableFaceTypes.end(), FaceIdentifier::ftWall) !=
                                                applicableFaceTypes.end());
        _UI.selectFacesButton->setDisabled(_UI.applyToAllWallsCheckBox->isChecked());
        _UI.selectFromListButton->setDisabled(_UI.applyToAllWallsCheckBox->isChecked());
    }
}

void FaceDataEditorWidget::setCurrentNode(mitk::DataNode* node)
{
    if (node == _currentNode) {
        return;
    }

    _UI.selectFacesButton->setChecked(false);

    if (_currentCustomEditor) {
        _currentCustomEditor->setVisible(false);
        _UI.editBoundaryConditionFrame->layout()->removeWidget(_currentCustomEditor);
        _currentCustomEditor->setParent(nullptr);
        _currentCustomEditor = nullptr;
    }

    _currentNode = node;

    _UI.boundaryConditionPropertyBrowser->clear();
    _UI.faceSelectionFrame->setVisible(true);

    if (_currentNode) {
        bool applyFlag = false;
        _currentNode->GetBoolProperty("bc.applyToAllWalls", applyFlag);
        _UI.applyToAllWallsCheckBox->setChecked(applyFlag);

        auto faceData = static_cast<FaceData*>(_currentNode->GetData());
        _UI.boundaryConditionPropertyBrowser->setResizeMode(QtTreePropertyBrowser::ResizeToContents);
        _UI.boundaryConditionPropertyBrowser->setFactoryForManager(faceData->getPropertyStorage()->getPropertyManager(),
                                                                   new QtVariantEditorFactory(_UI.boundaryConditionPropertyBrowser));
        _UI.boundaryConditionPropertyBrowser->addProperty(faceData->getPropertyStorage()->getTopProperty());
        _UI.boundaryConditionPropertyBrowser->setResizeMode(QtTreePropertyBrowser::Interactive);
        faceData->Modified(); // Assume modification when exposing properties to the user
        // ^^^^^^^^^^^^ Replace with
        // connect(bc->getPropertyStorage()->getPropertyManager(), &QtVariantPropertyManager::valueChanged, [bc]() {
        // bc->Modified(); })

		//if PCMRI create the PCMRI widget
		if ((gsl::to_string(faceData->getName()) == "Prescribed velocities (PC-MRI)" )|| (gsl::to_string(faceData->getName()) == "PCMRI"))
		{
			_currentCustomEditor = new PCMRIMappingWidget(_UI.editBoundaryConditionFrame);
			dynamic_cast<PCMRIMappingWidget*>(_currentCustomEditor)->setCurrentSolidNode(_currentSolidNode);
			dynamic_cast<PCMRIMappingWidget*>(_currentCustomEditor)->setCurrentNode(_currentNode);
			auto solidUID = std::string{};
			_currentSolidNode->GetStringProperty(crimson::HierarchyManager::nodeUIDPropertyName, solidUID);
			_currentNode->SetStringProperty("mapping.solidnode", solidUID.c_str());
			//auto faceData = static_cast<FaceData*>(_currentNode->GetData());
			
		}
		else
		{
			// Try to create a custom editor widget if not PCMRI
			_currentCustomEditor = faceData->createCustomEditorWidget();
		}

		if (_currentCustomEditor) {
			// Integrate the custom editor to the UI
			_currentCustomEditor->setParent(_UI.editBoundaryConditionFrame);
			_UI.editBoundaryConditionFrame->layout()->addWidget(_currentCustomEditor);
			_currentCustomEditor->setVisible(true);
		}
        _UI.faceSelectionFrame->setVisible(!faceData->applicableFaceTypes().empty());
    }

    _fillFaceTable();
    _updateUI();
}

void FaceDataEditorWidget::setCurrentSolidNode(mitk::DataNode* node)
{
    _currentSolidNode = node;
    _updateUI();
}

void FaceDataEditorWidget::setCurrentVesselTreeNode(mitk::DataNode* node)
{
    _vesselTreeNode = node;
    _faceListModel.setVesselTreeNode(node);
    _updateUI();
}

void FaceDataEditorWidget::_setFaceSelectionMode(bool enabled)
{
    if (enabled && !_currentSolidNode) {
        QMessageBox::warning(nullptr, "Blended solid not found", "Please create a blended solid to assign the boundary condition");
        _UI.selectFacesButton->setChecked(false);
        return;
    }

    if (_pickingEventObserverServiceTracker.GetService()) {
        if (enabled) {
            _pickingEventObserverServiceTracker.GetService()->Disable();
            _facePickingObserver.Enable();
        } else {
            _pickingEventObserverServiceTracker.GetService()->Enable();
            _facePickingObserver.Disable();
        }
    }

    if (!_currentSolidNode || !_currentNode) {
        return;
    }

    _currentSolidNode->SetVisibility(true);
    mitk::DataNode::Pointer meshNode = HierarchyManager::getInstance()->getFirstDescendant(_currentSolidNode, VesselMeshingNodeTypes::Mesh());
    if (meshNode.IsNotNull()) {
        meshNode->SetVisibility(false);
    }

    FaceData* faceData = static_cast<FaceData*>(_currentNode->GetData());
    if (!enabled) {
        // Save the selected faces in face data data
        _facePickingObserver.setSelectionChangedObserver(std::function<void(void)>());
        _facePickingObserver.clearSelection();
        _facePickingObserver.SetDataNode(nullptr);
    } else {
        _facePickingObserver.SetDataNode(_currentSolidNode);
        _facePickingObserver.clearSelection();
        // Sync selection from face data to observer
        for (const FaceIdentifier& id : faceData->getFaces()) {
            _facePickingObserver.selectFace(id, true);
        }
        _facePickingObserver.setSelectionChangedObserver(std::bind(&FaceDataEditorWidget::_syncFaceSelectionToObserver, this));
    }

    if (_allowAnyFaceSelection) {
        _facePickingObserver.resetSelectableTypes();
    } else {
        auto applicableFaceTypes = faceData->applicableFaceTypes();
        _facePickingObserver.setSelectableTypes(std::set<FaceIdentifier::FaceType>{applicableFaceTypes.begin(), applicableFaceTypes.end()});
    }
    mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void FaceDataEditorWidget::_selectFacesFromList()
{
    _UI.selectFacesButton->setChecked(false);
    _setFaceSelectionMode(true);
    SelectFacesFromListDialog(_vesselTreeNode, &_facePickingObserver).exec();
    _setFaceSelectionMode(false);
}

void FaceDataEditorWidget::_fillFaceTable()
{
    if (_currentNode) {
        auto faces = static_cast<FaceData*>(_currentNode->GetData())->getFaces();
        _faceListModel.setFaces(std::set<FaceIdentifier>{faces.begin(), faces.end()});
    } else {
        _faceListModel.setFaces(std::set<FaceIdentifier>());
    }
}

void FaceDataEditorWidget::_setApplyToAllWallsFlag(bool apply)
{
    bool applyFlag = false;
    _currentNode->GetBoolProperty("bc.applyToAllWalls", applyFlag);

    if (applyFlag == apply) {
        return;
    }

    _currentNode->SetBoolProperty("bc.applyToAllWalls", apply);
    if (apply) {
        _UI.selectFacesButton->setChecked(false);
        applyToAllWalls(_currentSolidNode, _currentNode);
    }
    _fillFaceTable();
    _updateUI();
}

void FaceDataEditorWidget::applyToAllWalls(mitk::DataNode* solidNode, mitk::DataNode* faceDataNode)
{
    if (!solidNode || !faceDataNode) {
        return;
    }

	SolidData* brepData = static_cast<SolidData*>(solidNode->GetData());
    auto faceData = static_cast<FaceData*>(faceDataNode->GetData());

    // Save the selected faces in face data
    std::set<FaceIdentifier> wallFaces;

    for (int i = 0; i < brepData->getFaceIdentifierMap().getNumberOfFaceIdentifiers(); ++i) {
        if (brepData->getFaceIdentifierMap().getFaceIdentifier(i).faceType == FaceIdentifier::ftWall) {
            wallFaces.insert(brepData->getFaceIdentifierMap().getFaceIdentifier(i));
        }
    }
    faceData->setFaces(wallFaces);
}

void FaceDataEditorWidget::_syncFaceSelectionToObserver()
{
    if (!_currentSolidNode || !_currentNode) {
        return;
    }

	SolidData* brepData = static_cast<SolidData*>(_currentSolidNode->GetData());
    auto faceData = static_cast<FaceData*>(_currentNode->GetData());

    std::set<FaceIdentifier> selectedFaces;

    for (size_t i = 0; i < _facePickingObserver.getFaceSelectedStates().size(); ++i) {
        if (_facePickingObserver.isFaceSelected(i)) {
            selectedFaces.insert(brepData->getFaceIdentifierMap().getFaceIdentifier(i));
        }
    }
	//TODO: wrap this in a function and connect to a call via slot?
	//if PCMRIMappingWidget exists, update its ResliceView and selected face geometry
	if ((gsl::to_string(faceData->getName()) == "Prescribed velocities (PC-MRI)") || (gsl::to_string(faceData->getName()) == "PCMRI"))
	{
		if (selectedFaces.size()>1)
			QMessageBox::warning(nullptr, "More than one face selected", "Please select only one face for patient-specific velocity profile");
		else if (selectedFaces.size()==0)
			dynamic_cast<PCMRIMappingWidget*>(_currentCustomEditor)->updateSelectedFace(nullptr);
		else
		{ 
			mitk::DataNode::Pointer node = dynamic_cast<PCMRIMappingWidget*>(_currentCustomEditor)->getMRANode();
			auto plane = PCMRIUtils::getMRAPlane(_currentNode, node, _currentSolidNode, *selectedFaces.begin());
			dynamic_cast<PCMRIMappingWidget*>(_currentCustomEditor)->updateSelectedFace(plane.GetPointer());
		}
			
	}
    faceData->setFaces(selectedFaces);
    _fillFaceTable();
}
}
