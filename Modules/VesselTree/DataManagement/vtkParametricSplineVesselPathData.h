#pragma once

#include <vtkSmartPointer.h>
#include <vtkParametricSpline.h>
#include <vtkCellLocator.h>
#include <vtkParametricFunctionSource.h>

#include "VesselPathAbstractData.h"
#include "VesselTreeExports.h"

namespace crimson {
class vtkParametricSplineVesselPathDataPrivate;

/*! \brief   Vessel path based on a vtkParametericSpline. */
class VesselTree_EXPORT vtkParametricSplineVesselPathData : public VesselPathAbstractData
{
public:
    mitkClassMacro(vtkParametricSplineVesselPathData, VesselPathAbstractData);
    itkFactorylessNewMacro(vtkParametricSplineVesselPathData);
    itkCloneMacro(vtkParametricSplineVesselPathData);
    mitkCloneMacro(vtkParametricSplineVesselPathData);

    //////////////////////////////////////////////////////////////////////////
    // VesselPathAbstractData interface implementation
    //////////////////////////////////////////////////////////////////////////
    PointType getPosition(ParameterType t) const override;

    VectorType getTangentVector(ParameterType t) const override;
    VectorType getNormalVector(ParameterType t) const override;
    ParameterType getParametricLength() const override;

    IdType controlPointsCount() const override;
    PointType getControlPoint(IdType id) const override;
    bool setControlPoint(IdType id, const PointType& pos) override;
    ParameterType getControlPointParameterValue(IdType id) const override;
    vtkSmartPointer<vtkPolyData> getPolyDataRepresentation() const override;

    bool setControlPoints(const std::vector<PointType>& controlPoints) override;
    IdType addControlPoint(const PointType& pos) override;
    IdType addControlPoint(IdType id, const PointType& pos) override;
    bool removeControlPoint(IdType id) override;

    ClosestPointQueryResult getClosestPoint(const PointType& p) const override;

    //////////////////////////////////////////////////////////////////////////

    vtkSmartPointer<vtkParametricSpline> getParametericSpline() { return _spline; }

    // Set the tension of the spline
    void setTension(mitk::ScalarType tension);
    mitk::ScalarType getTension() const;

    itkEventMacro(TensionChangeEvent, VesselPathEvent);

    void ExecuteOperation(mitk::Operation* operation) override;
private:
    vtkParametricSplineVesselPathData();
    vtkParametricSplineVesselPathData(const Self& other);
    virtual ~vtkParametricSplineVesselPathData();

    template<typename EventType>
    void _invokeControlPointEvent(IdType id);
    void _invokeModified();
    void _recomputeBBox();

    double _getVtkSplineParam(ParameterType t) const;

    vtkSmartPointer<vtkParametricSpline> _spline;

    // Parametric length - requires the 'mutable' modifier for lazy evaluation in the const functions
    mutable bool _lengthDirty;
    mutable ParameterType _length;
    vtkSmartPointer<vtkParametricFunctionSource> _splineSource;
    vtkSmartPointer<vtkCellLocator> _splineSourceCellLocator;
    mutable vtkSmartPointer<vtkPolyData> _polyDataRepresentation;
    mutable std::vector<VectorType> _propagatedNormalVectors;
    mutable int _lastPropagatedNormalIndex;
    mutable std::vector<ParameterType> _controlPointParameters;

    void _updateSplineSourceCellLocator() const;
};

}
