#pragma once

#include <QTimer>

#include <mitkPlanarFigure.h>
#include <mitkPointSet.h>
#include <mitkPointSetDataInteractor.h>

#include "NodeDependentView.h"
#include "ResliceViewListener.h"

#include "ui_PCMRIMappingWidget.h"
#include <AsyncTask.h>

#include <HierarchyManager.h>

#include <QmitkAbstractView.h>

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
class ResliceView;

/*!
* \brief The view responsible for the setting up the PCMRI mapping.
*/
class PCMRIMappingWidget : public QWidget
{
    Q_OBJECT

public:
    
	PCMRIMappingWidget(QWidget* parent = nullptr);
	~PCMRIMappingWidget();

	void setResliceView(ResliceView* view);

	void setCurrentNode(mitk::DataNode* node);
	void setCurrentSolidNode(mitk::DataNode* node);

	void updateSelectedFace(mitk::PlaneGeometry::Pointer plane);

	void nodeAdded(const mitk::DataNode* node);
	void nodeRemoved(const mitk::DataNode* node);

	void _connectDataStorageEvents(); //?
	void _disconnectDataStorageEvents(); //?

	static std::pair<mitk::DataNode*, mitk::DataNode*> _getSegmentationNodes(mitk::DataNode* contour);


	mitk::DataNode::Pointer getMRANode(){ return _currentMRANode.GetPointer(); };

    //////////////////////////////////////////////////////////////////////////
    // Common contour operations
private:
    enum ContourGeometyReinitType { cgrSetGeometry, cgrOffsetByGeometryCorner, cgrOffsetContourToCenter };

private slots:
    void duplicateContour();
    void interpolateContour();
	void interpolateAllContours();
	
    // Navigate to contour upon double click or undo/redo operation
    void navigateToContourByIndex(const QModelIndex& index);
    void navigateToContour(const mitk::DataNode* node);

    // Save the thumbnail in node properties and set it to the thumbnail widget
    void setNodeThumbnail(mitk::DataNode::ConstPointer node, QImage thumbnail);

    // Set the information about tool currently selected. This includes cursor and toolbar management.
    void setToolInformation_Manual(const char* name);
    void setToolInformation_Segmentation(int);
	void setToolInformation_Point(const char* name);
    void setToolInformation(std::istream& cursorStream, const char* name);
	void setPointToolInformation(std::istream& cursorStream, const char* name);
	void resetToolInformation();
	void resetPointToolInformation();

    // Highlight the contours selected in the thumbnail widget by setting the "selected" node property
    void syncContourSelection(const QItemSelection& selected, const QItemSelection& deselected);

    void cancelInteraction() { cancelInteraction(false); }
    void cancelInteraction(bool undoingPlacement);
	void cancelInteractionPoints() { cancelInteractionPoints(false); }
	void cancelInteractionPoints(bool undoingPlacement);

    void deleteSelectedContours();
    void updateCurrentContourInfo();

    void reinitAllContourGeometries(ContourGeometyReinitType reinitType = cgrOffsetByGeometryCorner);
    void reinitAllContourGeometriesByCorner() { reinitAllContourGeometries(cgrOffsetByGeometryCorner); }
	void reinitAllContourGeometriesOnVesselPathChange(); //?
   
private:
    // The data structure for storing the map from the parameter value to the contour node
    typedef std::map<float, mitk::DataNode*> ParameterMapType;

    // Update the ui elements according to the current context
    void _updateUI();

	// Add a point of a particular type.
	//template <typename PointType> void _addPoint(bool add);

    // Add a contour of a particular type and setup the contour information.
    // This also includes an attempt to convert the current contour, if any, to the requested contour type.
    template <typename ContourType> void _addContour(bool add);

    // Find the contour nodes closest to the current ResliceView position
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

	void _interpolateContourOne(mitk::DataNode* contourNodes[2], float param);

	
    //////////////////////////////////////////////////////////////////////////
    // Manual contour creation
private slots:
    void addCircle(bool);
    void addEllipse(bool);
    void addPolygon(bool);

private:
    std::unordered_map<std::string, QAbstractButton*> _contourTypeToButtonMap;


	//////////////////////////////////////////////////////////////////////////
	// Corresponding points placement
private slots:
	void addMraPoint(bool);
	void addPcmriPoint(bool);

private:
	std::unordered_map<std::string, QAbstractButton*> _pointTypeToButtonMap;

	// Add a new MRA point to the hierarchy. Sets the needed node properties
	mitk::DataNode* _addMraPointNode(mitk::PointSet::Pointer point, bool addInteractor = true);

	// Add a new PCMRI point to the hierarchy. Sets the needed node properties
	mitk::DataNode* _addPcmriPointNode(mitk::PointSet::Pointer point, bool addInteractor = true);

	// Show the information on the current point and add a data interactor 
	void _setCurrentMraPointNode(mitk::DataNode* node);
	void _setCurrentPcmriPointNode(mitk::DataNode* node);
	void _addPointInteractor(mitk::DataNode* node, mitk::BaseRenderer* renderer);

	// Convenience function for deleting current point node
	void _removeCurrentPcmriPoint();
	void _removeCurrentMraPoint();
	void _removePoint(mitk::PointSet::Pointer pointSet, mitk::PointSetDataInteractor::Pointer pointSetInteractor,
		const int index, const int slice);

	void _pointAdded(const itk::Object* obj);

	// React on the Esc button when placing corresponding points
	void _createCancelInteractionShortcutPoint();
	void _removeCancelInteractionShortcutPoint();

	// Connect and disconnect observers for the points - to correctly update the thumbnails etc.
	void _connectPointSetObservers(mitk::DataNode* node);
	void _disconnectPointSetObservers(mitk::DataNode* node);


    //////////////////////////////////////////////////////////////////////////
    // Segmentation contour creation
private slots:
	mitk::DataNode* createSegmented(bool startNewUndoGroup = true);
    void updateContourSmoothnessFromUI();

private:
    // Set a pair of data nodes containing the reference image and the segmentation image
    void _setSegmentationNodes(mitk::DataNode* refNode, mitk::DataNode* workNode);
    //std::pair<mitk::DataNode*, mitk::DataNode*> _getSegmentationNodes(mitk::DataNode* contour);
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
	// Mapping
private slots:
	void setMeshNodeForMapping(const mitk::DataNode*);
	void setMRAImageForMapping(const mitk::DataNode*);
	void setPCMRIImageForMapping(const mitk::DataNode*);
	void setPCMRIImagePhaseForMapping(const mitk::DataNode*);
	void doMap();
	void mappingFinished(crimson::async::Task::State);
	void editTimeInterpolationOptions();
	void flipPCMRIImage();
	void flipFlow();
	void editCardiacfrequency();
	void editVelocityCalculationType();
	//Philips velocity transformation properties
	void editVencP();
	//GE velocity transformation properties
	void editVencG();
	void editVenscale();
	void setMagnitudeMask();
	//Siemens velocity transformation properties
	void editVencS();
	void editRescaleSlopeS();
	void editRescaleInterceptS();
	void editQuantization();
	//Linear velocity transformation properties
	void editRescaleSlope();
	void editRescaleIntercept();



private:
	void _setupPCMRIMappingComboBoxes();
	void _flipPCMRIImage(bool flag = false);
	mitk::TimeGeometry::Pointer _getFlippedGeometry(mitk::DataNode* node);
	

    //////////////////////////////////////////////////////////////////////////
private slots:

    // Detect the contour currently shown in the ResliceView
    void currentSliceChanged();
	void updateCurrentContour();
	void updateCurrentPcmriPoint();
	void updateCurrentMraPoint();


private:
    // Ui and main widget of this view
    Ui::PCMRIMappingWidget _UI;

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
	struct PointSetObserverTags { //?
		enum TagType {
			PointAdded = 0,
			Last
		};

		unsigned long tags[Last];
	};

    // unsigned long _nodeObserverTag = -1;
    // void _addNodeObserver();
    // void _removeNodeObserver();

    std::map<mitk::DataNode*, ContourObserverTags> _contourObserverTags;
    std::map<mitk::DataNode*, unsigned long> _segmentationObserverTags;
	std::map<mitk::DataNode*, PointSetObserverTags> _pointSetObserverTags;
    bool _ignoreContourChangeEvents = false;

    const mitk::DataNode* _nodeBeingDeleted = nullptr;

    // Current nodes
    mitk::DataNode::Pointer _currentContourNode = nullptr;
    mitk::DataNode::Pointer _currentSegmentationReferenceImageNode = nullptr;
    mitk::DataNode::Pointer _currentSegmentationWorkingImageNode = nullptr;
	mitk::DataNode::Pointer _currentPcmriPointNode = nullptr;
	mitk::DataNode::Pointer _currentMraPointNode = nullptr;
	mitk::DataNode::Pointer _currentMRANode = nullptr; //set as in vessel path planning view
	mitk::DataNode::Pointer _currentPCMRINode = nullptr; //PCMRI image - magnitude component
	mitk::DataNode::Pointer _currentPCMRIPhaseNode = nullptr; //PCMRI image - phase component
	mitk::DataNode::Pointer _currentMeshNode = nullptr;
	mitk::DataNode::Pointer _currentRootNode = nullptr;
	mitk::DataNode::Pointer _currentSolidNode = nullptr;
	mitk::DataNode::Pointer _currentNode = nullptr; //PythonBoundaryCondition //TO DO: possible abstract extension with PCMRIData being a child node of iBC

	//Time geometries of PCMRI image - original and flipped
	mitk::TimeGeometry::Pointer _pcmriGeometryOriginal = nullptr;
	mitk::TimeGeometry::Pointer _pcmriGeometryFlipped = nullptr;

	//Flag that all the contours have been generated
	bool _allContours = false;

    // To keep the thumbnails clean, filter out the data that is shown
    mitk::NodePredicateBase::Pointer _shouldBeInvisibleInThumbnailView;

    // The thumbnail generator
    std::unique_ptr<crimson::ThumbnailGenerator> _thumbnailGenerator;

    // The part listener detects if the reslice view is available
    ResliceView* _ResliceView = nullptr;
    QScopedPointer<crimson::ResliceViewListener> _partListener;

    // Interaction information
    bool _cursorSet = false;
    QShortcut* _cancelInteractionShortcut = nullptr;
	QShortcut* _cancelInteractionShortcutPoint = nullptr;
    QAbstractButton* _currentManualButton = nullptr;
	QAbstractButton* _currentPointButton = nullptr;

    // Map from parameter values to the contour nodes
    ParameterMapType _parameterMap;

    // Support for pseudo-async contour information update.
    // This is needed to avoid updating the contour info too often when user interacts with it.
    QTimer _contourInfoUpdateTimer;

    // Modification of contours geometries due to vessel path change
    std::map<mitk::DataNode*, mitk::Point3D> _newContourPositions;
    QTimer _contourGeometryUpdateTimer;

	// Mapping preview generation
	QTimer _mapPreviewGenerateTimer;

	crimson::TaskStateObserver* _mapTaskStateObserver = nullptr;

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
