// Qt
#include <QCheckBox>
#include <QHeaderView>

// Main include
#include "MeshExplorationView.h"

// Module includes
#include <VesselMeshingNodeTypes.h>

#include <mitkPlaneGeometryData.h>
#include <mitkNodePredicateDataType.h>

#include <boost/range/adaptor/map.hpp>

#include <gsl.h>

#include <itkCommand.h>

const std::string MeshExplorationView::VIEW_ID = "org.mitk.views.MeshExplorationView";

MeshExplorationView::MeshExplorationView()
    : NodeDependentView(crimson::VesselMeshingNodeTypes::Mesh(), true, "Mesh", true)
{
}

MeshExplorationView::~MeshExplorationView()
{
    if (currentNode()) {
        _removeClippingPlanePropertiesFromNode(currentNode());
    }

    for (const std::pair<const mitk::DataNode*, ClippingPlaneInfoTuple>& planeInfoPair : _planeWidgetToClippingPropertyMap) {
        planeInfoPair.first->GetData()->RemoveObserver(std::get<ClippingObserverTag>(planeInfoPair.second));
    }
}

void MeshExplorationView::SetFocus()
{
}

void MeshExplorationView::CreateQtPartControl(QWidget *parent)
{
    // create GUI widgets from the Qt Designer's .ui file
    _UI.setupUi(parent);

    _UI.clippingPlanesTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    _UI.clippingPlanesTableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    _UI.clippingPlanesTableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

    static_cast<QBoxLayout*>(_UI.meshNameFrame->layout())->insertWidget(0, createSelectedNodeWidget(_UI.meshNameFrame));

    connect(_UI.sliceTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(_setSliceType(int)));
    connect(_UI.updateButton, &QAbstractButton::clicked, this, &MeshExplorationView::_applyPlaneProperties);

    _findClippingPlaneNodes();
    _fillClippingPlanesTable();
}

void MeshExplorationView::_fillClippingPlanesTable()
{
    _UI.clippingPlanesTableWidget->clearContents();
    _UI.clippingPlanesTableWidget->setRowCount(0);

    if (!currentNode()) {
        return;
    }

    int rowIndex = 0;

    // First fill the standard planes
    
    // The standard plane names 
    static const std::map<std::string, std::string> standardMultiWidgetNames = {
        {"stdmulti.widget1.plane", "Axial plane"},
        {"stdmulti.widget2.plane", "Sagittal plane"},
        {"stdmulti.widget3.plane", "Coronal plane"}};

    for (const auto& namePair : standardMultiWidgetNames) {
        auto planeInfoIter = std::find_if(_planeWidgetToClippingPropertyMap.begin(), _planeWidgetToClippingPropertyMap.end(),
                                          [&namePair](const PlaneWidgetToClippingPropertyMap::value_type& v) {
                                              return v.first->GetName() == namePair.first;
                                          });

        if (planeInfoIter == _planeWidgetToClippingPropertyMap.end()) {
            continue;
        }

        _addClippinigPlaneTableRow(namePair.second, *planeInfoIter, rowIndex++);
    }

    // Then fill the rest
    for (const auto& planeInfoPair : _planeWidgetToClippingPropertyMap) {
        if (standardMultiWidgetNames.find(planeInfoPair.first->GetName()) == standardMultiWidgetNames.end()) {
            _addClippinigPlaneTableRow(planeInfoPair.first->GetName(), planeInfoPair, rowIndex++);
        }
    }

//    _UI.clippingPlanesTableWidget->sortItems(0);
}

void MeshExplorationView::_addClippinigPlaneTableRow(const std::string& clippingPlaneName, const PlaneWidgetToClippingPropertyMap::value_type &planeInfoPair, int rowIndex)
{
    auto node = planeInfoPair.first;

    auto nameItem = new QTableWidgetItem(QString::fromStdString(clippingPlaneName));
    nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);

    // A simple lambda that creates a check box centered in a layout
    auto createCenteredCheckBox = []()
    {
        QWidget *widget = new QWidget();
        QCheckBox *checkBox = new QCheckBox();
        QHBoxLayout *layout = new QHBoxLayout(widget);
        layout->addWidget(checkBox);
        layout->setAlignment(Qt::AlignCenter);
        layout->setContentsMargins(0, 0, 0, 0);
        widget->setLayout(layout);

        return checkBox;
    };

    QCheckBox * enabledWidget = createCenteredCheckBox();
    enabledWidget->setCheckState(std::get<ClippingEnabledIndex>(planeInfoPair.second) ? Qt::Checked : Qt::Unchecked);
    QCheckBox * invertedWidget = createCenteredCheckBox();
    invertedWidget->setCheckState(std::get<ClippingInvertedIndex>(planeInfoPair.second) ? Qt::Checked : Qt::Unchecked);

    _UI.clippingPlanesTableWidget->insertRow(rowIndex);
    _UI.clippingPlanesTableWidget->setItem(rowIndex, 0, nameItem);
    _UI.clippingPlanesTableWidget->setCellWidget(rowIndex, 1, enabledWidget->parentWidget());
    _UI.clippingPlanesTableWidget->setCellWidget(rowIndex, 2, invertedWidget->parentWidget());

    // Enable/disable a clipping plane
    connect(enabledWidget, &QAbstractButton::toggled, [this, node](bool checked)
    {
        ClippingPlaneInfoTuple& planeInfo = _planeWidgetToClippingPropertyMap[node];
        std::get<ClippingEnabledIndex>(planeInfo) = checked;
        if (checked) {
            currentNode()->GetPropertyList()->SetProperty(node->GetName(),
                std::get<ClippingPropertyIndex>(planeInfo));
        } else {
            currentNode()->GetPropertyList()->DeleteProperty(node->GetName());
        }
        if (_UI.autoUpdateCheckBox->isChecked()) {
            _applyPlaneProperties();
        }
    });

    // Set the clipping plane normal inversion
    connect(invertedWidget, &QAbstractButton::toggled, [this, node](bool checked)
    {
        ClippingPlaneInfoTuple& planeInfo = _planeWidgetToClippingPropertyMap[node];
        std::get<ClippingInvertedIndex>(planeInfo) = checked;
        std::get<ClippingPropertyIndex>(planeInfo)->SetNormal(std::get<ClippingPropertyIndex>(planeInfo)->GetNormal() * -1);
        if (_UI.autoUpdateCheckBox->isChecked()) {
            _applyPlaneProperties();
        }
    });
}

void MeshExplorationView::_applyPlaneProperties()
{
    if (!currentNode()) {
        return;
    }

    currentNode()->GetData()->Modified();
    mitk::RenderingManager::GetInstance()->RequestUpdateAll(mitk::RenderingManager::REQUEST_UPDATE_3DWINDOWS);
}

void MeshExplorationView::_addClippingProperty(const mitk::DataNode* clippingPlaneNode)
{
    if (_planeWidgetToClippingPropertyMap.count(clippingPlaneNode) != 0) {
        return;
    }

    auto data = static_cast<mitk::PlaneGeometryData*>(clippingPlaneNode->GetData());

    // Subscribe to clipping plane change events
    auto modifiedCommand = itk::MemberCommand<MeshExplorationView>::New();
    modifiedCommand->SetCallbackFunction(this, &MeshExplorationView::_clippingPlaneModified);
    modifiedCommand->SetCallbackFunction(this, &MeshExplorationView::_clippingPlaneModifiedC);
    unsigned long observerTag = data->AddObserver(itk::ModifiedEvent(), modifiedCommand);

    auto clippingPlaneProperty = mitk::ClippingProperty::New(data->GetPlaneGeometry()->GetOrigin(), data->GetPlaneGeometry()->GetNormal());
    clippingPlaneProperty->SetTransient(true); // Do not save the property to file
    _planeWidgetToClippingPropertyMap[clippingPlaneNode] = std::make_tuple(clippingPlaneProperty, false, false, observerTag);
}

void MeshExplorationView::_clippingPlaneModifiedC(const itk::Object* obj, const itk::EventObject&)
{
    for (const std::pair<const mitk::DataNode*, ClippingPlaneInfoTuple>& planeInfoPair : _planeWidgetToClippingPropertyMap) {
        if (planeInfoPair.first->GetData() == obj) {
            // Update the clipping plane property
            auto data = static_cast<const mitk::PlaneGeometryData*>(obj);

            const mitk::ClippingProperty::Pointer& clippingProperty = std::get<ClippingPropertyIndex>(planeInfoPair.second);
            clippingProperty->SetOrigin(data->GetPlaneGeometry()->GetOrigin());

            mitk::Vector3D normal = data->GetPlaneGeometry()->GetNormal();
            clippingProperty->SetNormal(std::get<ClippingInvertedIndex>(planeInfoPair.second) ? -normal : normal);

            if (_UI.autoUpdateCheckBox->isChecked()) {
                _applyPlaneProperties();
            }

            return;
        }
    }
}

void MeshExplorationView::_findClippingPlaneNodes()
{
    // Find all the data nodes containing plane geometry data
    mitk::DataStorage::SetOfObjects::ConstPointer geometryNodes =
        GetDataStorage()->GetSubset(mitk::TNodePredicateDataType<mitk::PlaneGeometryData>::New().GetPointer());

    for (const mitk::DataNode::Pointer& node : *geometryNodes) {
        _addClippingProperty(node.GetPointer());
    }
}

void MeshExplorationView::_setupClippingPlanePropertiesForCurrentNode()
{
    for (const std::pair<const mitk::DataNode*, ClippingPlaneInfoTuple>& planeInfoPair : _planeWidgetToClippingPropertyMap) {
        if (std::get<ClippingEnabledIndex>(planeInfoPair.second)) {
            currentNode()->GetPropertyList()->SetProperty(planeInfoPair.first->GetName(),
                std::get<ClippingPropertyIndex>(planeInfoPair.second));

            _applyPlaneProperties();
        }
    }
}

void MeshExplorationView::_removeClippingPlanePropertiesFromNode(mitk::DataNode* dataNode)
{
    for (const mitk::DataNode* node : _planeWidgetToClippingPropertyMap | boost::adaptors::map_keys) {
        dataNode->GetPropertyList()->DeleteProperty(node->GetName());
    }

    _applyPlaneProperties();
}

void MeshExplorationView::_setSliceType(int type)
{
    if (!currentNode()) {
        return;
    }

    enum class SliceType {
        ExtractCells = 0,   ///< Extract cells as a whole (useful for mesh exploration)
        Clip = 1    ///< Slice through the cells (useful for displaying the solution)
    };

    Expects(0 <= type && type < 2);
    switch (static_cast<SliceType>(type)) {
    case SliceType::ExtractCells:
        currentNode()->SetStringProperty("SliceType", "ExtractCells");
        currentNode()->SetBoolProperty("material.edgeVisibility", true);
        break;
    case SliceType::Clip:
        currentNode()->SetStringProperty("SliceType", "Clip");
        currentNode()->SetBoolProperty("material.edgeVisibility", false);
        break;
    }
    mitk::RenderingManager::GetInstance()->RequestUpdateAll(mitk::RenderingManager::REQUEST_UPDATE_3DWINDOWS);
}

void MeshExplorationView::currentNodeChanged(mitk::DataNode* prevNode)
{
    if (prevNode) {
        _removeClippingPlanePropertiesFromNode(prevNode);
    }

    _findClippingPlaneNodes();

    if (currentNode()) {
        _setupClippingPlanePropertiesForCurrentNode();
        _setSliceType(_UI.sliceTypeComboBox->currentIndex());
    }
    _fillClippingPlanesTable();
}

void MeshExplorationView::NodeAdded(const mitk::DataNode* node)
{
    NodeDependentView::NodeAdded(node);

    // This is mostly useful for the case where the reslice view is opened
    if (dynamic_cast<mitk::PlaneGeometryData*>(node->GetData())) {
        _addClippingProperty(node);
        _fillClippingPlanesTable();
    }
}

void MeshExplorationView::NodeChanged(const mitk::DataNode* node)
{
    NodeDependentView::NodeChanged(node);
}

void MeshExplorationView::NodeRemoved(const mitk::DataNode* node)
{
    NodeDependentView::NodeRemoved(node);

    // This is mostly useful for the case where the reslice view is closed
    auto iter = _planeWidgetToClippingPropertyMap.find(node);

    if (iter != _planeWidgetToClippingPropertyMap.end()) {
        iter->first->GetData()->RemoveObserver(std::get<ClippingObserverTag>(iter->second));
        _planeWidgetToClippingPropertyMap.erase(iter);
        _fillClippingPlanesTable();
    }
}

