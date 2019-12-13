#pragma once

#include "NodeDependentView.h"
#include <mitkPlaneGeometry.h>
#include <mitkCameraController.h>

namespace mitk {
    class DataNode;
}

class VesselDrivenResliceViewPrivate;

/*!
* \brief The view responsible for showing the reslice rendering window perpendicular to a vessel path.
*/
class VesselDrivenResliceView : public NodeDependentView
{
    Q_OBJECT

public:
    static const std::string VIEW_ID;

    mitk::BaseRenderer* getResliceRenderer() const;
    std::vector<mitk::BaseRenderer*> getAllResliceRenderers() const;
    void navigateTo(const mitk::Point3D& pos);
    void navigateTo(float parameterValue);
    float getCurrentParameterValue() const;
    mitk::PlaneGeometry* getPlaneGeometry(float t) const;
    void forceReinitGeometry();

    VesselDrivenResliceView();
    ~VesselDrivenResliceView();

signals:
    void sliceChanged(float t);
    void geometryChanged();

private slots:
    void _setSliceNumber(double slice);
    void _setResliceWindowSize();
    void _updateGeometryNodeInDataStorage();
    void _removeGeometryNodeFromDataStorage();
    void _setupRendererSlices();

protected:
    void currentNodeChanged(mitk::DataNode*) override;
    void currentNodeModified() override;

    void CreateQtPartControl(QWidget *parent) override;
    void SetFocus() override;

    void _setResliceViewEnabled(bool enabled);
    bool _isCurrentVesselPathValid();

    void _syncSliderWithStepper(itk::Object* o, const itk::EventObject& e) { _syncSliderWithStepperC(o, e); }
    void _syncSliderWithStepperC(const itk::Object*, const itk::EventObject&);

    std::unique_ptr<VesselDrivenResliceViewPrivate> d;

    friend class ResliceViewWidgetListener;
};
