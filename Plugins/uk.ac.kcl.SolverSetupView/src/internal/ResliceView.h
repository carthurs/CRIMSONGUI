#pragma once

#include "NodeDependentView.h"
#include <mitkPlaneGeometry.h>
#include <mitkCameraController.h>

namespace mitk {
    class DataNode;
}

class ResliceViewPrivate;

/*!
* \brief The view responsible for showing the reslice rendering window perpendicular to a vessel path.
*/
class ResliceView : public NodeDependentView
{
    Q_OBJECT

public:
    static const std::string VIEW_ID;

    mitk::BaseRenderer* getResliceRenderer() const;
	mitk::BaseRenderer* getPCMRIRenderer() const;
    std::vector<mitk::BaseRenderer*> getAllResliceRenderers() const;
    //void navigateTo(const mitk::Point3D& pos);
    void navigateTo(float parameterValue);
    float getCurrentParameterValue() const;
	//double getAngle() const;
    mitk::PlaneGeometry* getPlaneGeometry(float t) const;
	void updateMRARendering(mitk::PlaneGeometry::Pointer plane) const;
    void forceReinitGeometry();
	void setCurrentSolidNode(mitk::DataNode::Pointer solidNode){ currentSolidNode = solidNode; };

    ResliceView();
    ~ResliceView();

signals:
    void sliceChanged(float t);
    void geometryChanged();

public slots:
	void _setupPCMRISlices(const mitk::DataNode* node);
	void _setupMRASlice(const mitk::DataNode* node);
	void _changeReslicePlane();

private slots:
	void _setTimeSliceNumber(double slice);
    void _updateGeometryNodeInDataStorage();
    void _removeGeometryNodeFromDataStorage();
    void _setupRendererSlices();

protected:
    void currentNodeChanged(mitk::DataNode*) override;
    void currentNodeModified() override;

    void CreateQtPartControl(QWidget *parent) override;
    void SetFocus() override;

    void _setResliceViewEnabled(bool enabled);
    //bool _isCurrentVesselPathValid();

    void _syncSliderWithStepper(itk::Object* o, const itk::EventObject& e) { _syncSliderWithStepperC(o, e); }
    void _syncSliderWithStepperC(const itk::Object*, const itk::EventObject&);

    std::unique_ptr<ResliceViewPrivate> d;
	mitk::DataNode::Pointer currentPCMRINode = nullptr;
	mitk::DataNode::Pointer currentMRANode = nullptr;
	mitk::DataNode::Pointer currentSolidNode = nullptr;
	mitk::RenderingManager::Pointer pcmriManager = nullptr;
	mitk::RenderingManager::Pointer mraManager = nullptr;
	mutable mitk::PlaneGeometry* originalMRAgeometry = nullptr;

	friend class PCMRIResliceViewWidgetListener;
};
