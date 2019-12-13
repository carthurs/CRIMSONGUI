#pragma once

#include <QTimer>

#include <mitkPlanarFigure.h>

#include "NodeDependentView.h"
#include "VesselDrivenResliceViewListener.h"

#include "ui_ContourModelingView.h"
#include <AsyncTask.h>

namespace crimson
{
class TaskStateObserver;
}

namespace us
{
class ModuleResource;
}
namespace mitk
{
class PlanarPolygon;
}
namespace crimson
{
class ThumbnailGenerator;
}
class vtkPolyData;
class ThumbnalListWidgetItem;
class QIcon;
class QShortcut;
class VesselDrivenResliceView;

/*!
* \brief The view responsible for the setting up the vessel lofting.
*/
class ContourModelingView : public NodeDependentView
{
    Q_OBJECT

public:
    static const std::string VIEW_ID;

    ContourModelingView();
    ~ContourModelingView();

    void NodeAdded(const mitk::DataNode* node) override;
    void NodeRemoved(const mitk::DataNode* node) override;

    void setVesselDrivenResliceView(VesselDrivenResliceView* view);

    //////////////////////////////////////////////////////////////////////////
    // Common contour operations
private:
    enum ContourGeometyReinitType { cgrSetGeometry, cgrOffsetByGeometryCorner, cgrOffsetContourToCenter };

private slots:
    void duplicateContour();
    void interpolateContour();

    // Navigate to contour upon double click or undo/redo operation
    void navigateToContourByIndex(const QModelIndex& index);
    void navigateToContour(const mitk::DataNode* node);

    // Save the thumbnail in node properties and set it to the thumbnail widget
    void setNodeThumbnail(mitk::DataNode::ConstPointer node, QImage thumbnail);

    // Set the information about tool currently selected. This includes cursor and toolbar management.
    void setToolInformation_Manual(const char* name);
    void setToolInformation_Segmentation(int);
    void setToolInformation(std::istream& cursorStream, const char* name);
    void resetToolInformation();

    // Highlight the contours selected in the thumbnail widget by setting the "selected" node property
    void syncContourSelection(const QItemSelection& selected, const QItemSelection& deselected);

    void cancelInteraction() { cancelInteraction(false); }
    void cancelInteraction(bool undoingPlacement);

    void deleteSelectedContours();
    void updateCurrentContourInfo();
    void setAllContoursVisible(bool);

    void updateInflowOutflowAsWallInformation();
    void computePathThroughContourCenters();

    void reinitAllContourGeometries(ContourGeometyReinitType reinitType = cgrOffsetByGeometryCorner);
    void reinitAllContourGeometriesByCorner() { reinitAllContourGeometries(cgrOffsetByGeometryCorner); }
    void reinitAllContourGeometriesOnVesselPathChange();

private:
    // The data structure for storing the map from the parameter value to the contour node
    typedef std::map<float, mitk::DataNode*> ParameterMapType;

    // Update the ui elements according to the current context
    void _updateUI();

    // Add a contour of a particular type and setup the contour information.
    // This also includes an attempt to convert the current contour, if any, to the requested contour type.
    template <typename ContourType> void _addContour(bool add);

    // Find the contour nodes closest to the current VesselDrivenResliceView position
    // and highlight the corresponding contours in the thumbnail view
    void _updateContourListViewSelection();

    // Add a new planar figure to the hierarchy. Sets the needed node properties
    mitk::DataNode* _addPlanarFigure(mitk::PlanarFigure::Pointer figure, bool addInteractor = true);

    // Show the information on the current contour and add a data interactor if this is required
    // (e.g. it is not required for segmentation contours)
    void _setCurrentContourNode(mitk::DataNode* node);
    void _addPlanarFigureInteractor(mitk::DataNode* node);

    // Convenience function for deleting current contour node
    void _removeCurrentContour();

    // Fill all the contour thumbnails in the thumbnail widget for the current vessel path
    void _fillContourThumbnailsWidget();
    void _addContoursThumbnailsWidgetItem(const mitk::DataNode* node);
    void _contourChanged(const itk::Object* obj);
    void _figurePlacementCancelled(const itk::Object* obj);
    void _getClosestContourNodes(float parameterValue, ParameterMapType::iterator outClosestNodeIterators[2]);
    void _requestThumbnail(mitk::DataNode::ConstPointer node);
    void _requestLoftPreview();

    mitk::Point2D _getPlanarFigureCenter(const mitk::DataNode* node) const;
    void _updatePlanarFigureParameters(bool forceUpdate = false);

    ThumbnalListWidgetItem* _findThumbnailsWidgetItemByNode(const mitk::DataNode* node);
    QImage _getNodeThumbnail(const mitk::DataNode*);

    // React on the Esc button when creating or editing contours
    void _createCancelInteractionShortcut();
    void _removeCancelInteractionShortcut();

    // Connect and disconnect observers for the contours - to correctly update the thumbnails etc.
    void _connectContourObservers(mitk::DataNode*);
    void _connectSegmentationObservers(mitk::DataNode*);
    void _disconnectContourObservers(mitk::DataNode*);
    void _disconnectSegmentationObservers(mitk::DataNode*);

    void _connectAllContourObservers(mitk::DataStorage::SetOfObjects::ConstPointer nodes);
    void _disconnectAllContourObservers();

    void setContourVisible(mitk::DataNode* contourNode);
    void _addContoursVisibilityPropertyObserver(mitk::DataNode* node);
    void _removeContoursVisibilityPropertyObserver(mitk::DataNode* node);
    void _contoursVisibilityPropertyChanged(const itk::Object*);

    //////////////////////////////////////////////////////////////////////////
    // Manual contour creation
private slots:
    void addCircle(bool);
    void addEllipse(bool);
    void addPolygon(bool);

private:
    std::unordered_map<std::string, QAbstractButton*> _contourTypeToButtonMap;

    //////////////////////////////////////////////////////////////////////////
    // Segmentation contour creation
private slots:
    void createSegmented(bool startNewUndoGroup = true);
    void updateContourSmoothnessFromUI();

private:
    // Set a pair of data nodes containing the reference image and the segmentation image
    void _setSegmentationNodes(mitk::DataNode* refNode, mitk::DataNode* workNode);
    std::pair<mitk::DataNode*, mitk::DataNode*> _getSegmentationNodes(mitk::DataNode* contour);
    std::pair<mitk::DataNode::Pointer, mitk::DataNode::Pointer>
    _createSegmentationNodes(mitk::Image* image, const mitk::PlaneGeometry* sliceGeometry, mitk::PlanarFigure* figureToFill);

    // Fill the relevant pixels in the segmentation slice based on a contour
    void _fillPlanarFigureInSlice(mitk::Image* segmentationImage, mitk::PlanarFigure* figure);

    // Extract and smooth the contour defined by the segmented slice
    void _updateContourFromSegmentation(mitk::DataNode* contourNode);
    void _updateContourFromSegmentationData(const itk::Object*);

    float _getContourSmoothnessFromNode(mitk::DataNode*);
    void _createSmoothedContourFromVtkPolyData(vtkPolyData* polyData, mitk::PlanarPolygon* contour, float smoothness);

    //////////////////////////////////////////////////////////////////////////
    // Lofting
private slots:
    void createLoft();
    void previewLoft();
    void removeLoftPreview(mitk::DataNode* vesselNode);
    void setLoftingAlgorithm(int);
    void setSeamEdgeRotation(int);
    void setInterContourDistance(double);
    void loftingFinished(crimson::async::Task::State);
    void loftPreviewFinished(crimson::async::Task::State);

    // Detect the contour currently shown in the VesselDrivenResliceView
    void currentSliceChanged();
    void updateCurrentContour();

private:
    void CreateQtPartControl(QWidget* parent) override;
    void SetFocus() override;

    // Reimplemented from VesselDependentView
    void currentNodeChanged(mitk::DataNode*) override;
    void currentNodeModified() override;

private:
    // Ui and main widget of this view
    Ui::ContourModelingWidget _UI;
    QWidget* _viewWidget = nullptr;

    // The events that need to be observed for correct behavior
    struct ContourObserverTags {
        enum TagType {
            Modified = 0,
            FigureFinished,
            // FigurePlaced,
            FigurePlacementCancelled,

            Last
        };

        unsigned long tags[Last];
    };

    std::map<mitk::DataNode*, ContourObserverTags> _contourObserverTags;
    std::map<mitk::DataNode*, unsigned long> _segmentationObserverTags;
    bool _ignoreContourChangeEvents = false;
    bool _ignoreVesselPathModifiedEvents = false;
    unsigned long _contoursVisibilityPropertyObserverTag = 0;

    const mitk::DataNode* _nodeBeingDeleted = nullptr;

    // Current nodes
    mitk::DataNode::Pointer _currentContourNode = nullptr;
    mitk::DataNode::Pointer _currentSegmentationReferenceImageNode = nullptr;
    mitk::DataNode::Pointer _currentSegmentationWorkingImageNode = nullptr;

    // To keep the thumbnails clean, filter out the data that is shown
    mitk::NodePredicateBase::Pointer _shouldBeInvisibleInThumbnailView;

    // The thumbnail generator
    std::unique_ptr<crimson::ThumbnailGenerator> _thumbnailGenerator;

    // The part listener detects if the reslice view is available
    VesselDrivenResliceView* _vesselDrivenResliceView = nullptr;
    QScopedPointer<crimson::VesselDrivenResliceViewListener> _partListener;

    // Interaction information
    bool _cursorSet = false;
    QShortcut* _cancelInteractionShortcut = nullptr;
    QAbstractButton* _currentManualButton = nullptr;

    // Map from parameter values to the contour nodes
    ParameterMapType _parameterMap;

    // Support for pseudo-async contour information update.
    // This is needed to avoid updating the contour info too often when user interacts with it.
    QTimer _contourInfoUpdateTimer;

    // Modification of contours geometries due to vessel path change
    std::map<mitk::DataNode*, mitk::Point3D> _newContourPositions;
    QTimer _contourGeometryUpdateTimer;

    // Loft preview generation
    QTimer _loftPreviewGenerateTimer;

    crimson::TaskStateObserver* _loftTaskStateObserver = nullptr;
    crimson::TaskStateObserver* _loftPreviewTaskStateObserver = nullptr;
};

//////////////////////////////////////////////////////////////////////////
// Internal
//////////////////////////////////////////////////////////////////////////
class ItemDeleterEventFilter : public QObject
{
    Q_OBJECT
public:
    ItemDeleterEventFilter(QObject* parent = nullptr);

    bool eventFilter(QObject* obj, QEvent* event) override;

signals:
    void deleteRequested();
};
