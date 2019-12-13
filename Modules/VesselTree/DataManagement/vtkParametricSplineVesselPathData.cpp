#include "vtkParametricSplineVesselPathData.h"
#include "VesselPathOperation.h"
#include <mitkInteractionConst.h>

#include <vtkSmartPointer.h>
#include <vtkParametricSpline.h>
#include <vtkKochanekSpline.h>
#include <vtkPoints.h>
#include <vtkParametricFunctionSource.h>
#include <vtkPolyData.h>
#include <vtkMath.h>
#include <vtkPointData.h>
#include <vtkCell.h>
#include <vtkDoubleArray.h>
#include <vtkPiecewiseFunction.h>

#include <algorithm>
#include <vtkNew.h>

///////////////////////////////////////////////////////////////////
// vtkParametricSplineVesselPathData
///////////////////////////////////////////////////////////////////

namespace crimson {

static const VesselPathAbstractData::ParameterType MINIMUM_FINITE_DIFFERENCES_DELTA = 0.1; // Assuming work in millimeters, 0.1mm should be a good enough value for offset

// Subclass of a vtkKochaekSpline which provides analytic derivative.
// This allows to avoid sudden jumps of the reslice plane for tortuous vessel paths.
class vtkKochanekSplineWithDerivative : public vtkKochanekSpline {
public:
    vtkTypeMacro(vtkKochanekSplineWithDerivative, vtkKochanekSpline);

    static vtkKochanekSplineWithDerivative *New() { return new vtkKochanekSplineWithDerivative; }

    // Compute the derivative of the spline
    // This is a copy of the vtkKochanekSpline's Evaluate() function, except for the last line
    // where the basis functions are replaced with their derivatives, e.g. t^3 -> 3 * t^2
    double EvaluateDerivative(double t)
    {
        double *intervals;
        double *coefficients;

        // check to see if we need to recompute the spline
        if (this->ComputeTime < this->GetMTime())
        {
            this->Compute();
        }

        // make sure we have at least 2 points
        int size = this->PiecewiseFunction->GetSize();
        if (size < 2)
        {
            return 0.0;
        }

        intervals = this->Intervals;
        coefficients = this->Coefficients;

        if (this->Closed)
        {
            size = size + 1;
        }

        // clamp the function at both ends
        if (t < intervals[0])
        {
            t = intervals[0];
        }
        if (t > intervals[size - 1])
        {
            t = intervals[size - 1];
        }

        // find pointer to cubic spline coefficient
        int index = this->FindIndex(size, t);

        // calculate offset within interval
        t = (t - intervals[index]) / (intervals[index + 1] - intervals[index]);

        // evaluate derivative
        return 3 * t * t * *(coefficients + index * 4 + 3) + 2 * t * *(coefficients + index * 4 + 2) + *(coefficients + index * 4 + 1);
    }

};


// Evaluate the derivative of a 3D spline using the vtkKochanekSplineWithDerivative analytic derivative computation
void EvaluateDerivative(double out[3], vtkParametricSpline* spline, double t)
{
    double range[2];
    spline->GetXSpline()->GetParametricRange(range);

    t = std::min(range[1] - 1e-6, std::max(range[0] + 1e-6, t * (range[1] - range[0])));

    out[0] = static_cast<vtkKochanekSplineWithDerivative*>(spline->GetXSpline())->EvaluateDerivative(t);
    out[1] = static_cast<vtkKochanekSplineWithDerivative*>(spline->GetYSpline())->EvaluateDerivative(t);
    out[2] = static_cast<vtkKochanekSplineWithDerivative*>(spline->GetZSpline())->EvaluateDerivative(t);
}

vtkParametricSplineVesselPathData::vtkParametricSplineVesselPathData()
    : _spline(vtkSmartPointer<vtkParametricSpline>::New())
    , _lengthDirty(false)
    , _length(0)
    , _splineSource(vtkSmartPointer<vtkParametricFunctionSource>::New())
    , _splineSourceCellLocator(vtkSmartPointer<vtkCellLocator>::New())
    , _lastPropagatedNormalIndex(-1)
{
    _spline->SetXSpline(vtkSmartPointer<vtkKochanekSplineWithDerivative>::New());
    _spline->SetYSpline(vtkSmartPointer<vtkKochanekSplineWithDerivative>::New());
    _spline->SetZSpline(vtkSmartPointer<vtkKochanekSplineWithDerivative>::New());
}

vtkParametricSplineVesselPathData::vtkParametricSplineVesselPathData(const Self& other)
    : VesselPathAbstractData(other)
    , _spline(vtkSmartPointer<vtkParametricSpline>::New())
    , _lengthDirty(true) // For spline source and point locator to be rebuilt
    , _length(other._length) 
    , _splineSource(vtkSmartPointer<vtkParametricFunctionSource>::New())
    , _splineSourceCellLocator(vtkSmartPointer<vtkCellLocator>::New())
    , _lastPropagatedNormalIndex(-1)
{
    _spline->SetXSpline(vtkSmartPointer<vtkKochanekSplineWithDerivative>::New());
    _spline->SetYSpline(vtkSmartPointer<vtkKochanekSplineWithDerivative>::New());
    _spline->SetZSpline(vtkSmartPointer<vtkKochanekSplineWithDerivative>::New());

    auto points = vtkSmartPointer<vtkPoints>::New();
    points->DeepCopy(other._spline->GetPoints());
    _spline->SetPoints(points);

    setTension(other.getTension());
}

vtkParametricSplineVesselPathData::~vtkParametricSplineVesselPathData()
{
}

auto vtkParametricSplineVesselPathData::getPosition(ParameterType t) const -> PointType
{
    PointType output;

    double vtkSplineParam = _getVtkSplineParam(t);
    _spline->Evaluate(&vtkSplineParam, &output[0], nullptr);
    return output;
}

auto vtkParametricSplineVesselPathData::getTangentVector(ParameterType t) const -> VectorType
{
    if (controlPointsCount() < 2) {
        assert(false);
        MITK_WARN << "Attempting to get tangent vector for a spline with less than two control points.";
        return VectorType(0.0);
    }

    double tangent[3];
    EvaluateDerivative(tangent, _spline, _getVtkSplineParam(t));

    VectorType result;
    mitk::FillVector3D(result, tangent[0], tangent[1], tangent[2]);
    result.Normalize();
    return result;
}

auto vtkParametricSplineVesselPathData::getNormalVector(ParameterType t) const -> VectorType
{
    if (controlPointsCount() < 2) {
        assert(false);
        MITK_WARN << "Attempting to get normal vector for a spline with less than two control points.";
        return VectorType(0.0);
    }

    if (_lastPropagatedNormalIndex == -1) {
        getParametricLength(); // Update splince source if necessary
        _propagatedNormalVectors.resize(_splineSource->GetOutput()->GetNumberOfPoints());
                                             
        // Stable reference frame computation, using Bloomenthal's algorithm
        // http://www.unchainedgeometry.com/jbloom/pdf/ref-frames.pdf
        // Allows to avoid sudden changes in direction and problems at straight segments

        // Compute first reference frame
        int replaceComp;
        VectorType tangent = getTangentVector(0);
        VectorType tmpRotVector(0.0);

        if (fabs(tangent[2]) > 0.0001) {
            tmpRotVector[1] = 1;
            replaceComp = 2;
        }
        else if (fabs(tangent[1]) > 0.0001)  {
            tmpRotVector[0] = 1;
            replaceComp = 1;
        }
        else {
            tmpRotVector[2] = 1;
            replaceComp = 0;
        }

        mitk::ScalarType dotProduct = 0;

        for (int j = 0; j < 3; j++)
            dotProduct += (tangent[j] * tmpRotVector[j]);

        tmpRotVector[replaceComp] = -dotProduct / tangent[replaceComp];

        tmpRotVector.Normalize();
        _propagatedNormalVectors[0] = tmpRotVector;
        _lastPropagatedNormalIndex = 0;
    }

    // Compute the normal vectors lazily - only up to the requested point
    int requiredPropagationIndex = (int)ceil(std::min(1.0, t / getParametricLength()) * (_propagatedNormalVectors.size() - 1));

    if (requiredPropagationIndex == 0) {
        return _propagatedNormalVectors[0];
    }

    itk::Matrix<VectorType::ComponentType> rotMatrix;

    VectorType prevTangent = getTangentVector(_lastPropagatedNormalIndex / (_propagatedNormalVectors.size() - 1.0) * getParametricLength());
    for (int i = _lastPropagatedNormalIndex + 1; i <= requiredPropagationIndex; ++i) {
        ParameterType curT = i / (_propagatedNormalVectors.size() - 1.0) * getParametricLength();
        VectorType curTangent = getTangentVector(curT);

        VectorType axis = itk::CrossProduct(prevTangent, curTangent);
        double norm = axis.GetNorm();
        if (norm < 1e-6f) {
            // No rotation needed
            _propagatedNormalVectors[i] = _propagatedNormalVectors[i - 1];
            continue;
        }
        axis /= norm;

        // See http://www.unchainedgeometry.com/jbloom/pdf/ref-frames.pdf
        double sqx = axis[0] * axis[0];
        double sqy = axis[1] * axis[1];
        double sqz = axis[2] * axis[2];
        double cos_ = curTangent * prevTangent;
        double cos1 = 1 - cos_;
        double xycos1 = axis[0] * axis[1] * cos1;
        double yzcos1 = axis[1] * axis[2] * cos1;
        double zxcos1 = axis[2] * axis[0] * cos1;
        double sin_ = sqrt(1.0 - cos_ * cos_);
        double xsin = axis[0] * sin_;
        double ysin = axis[1] * sin_;
        double zsin = axis[2] * sin_;

        rotMatrix(0, 0) = sqx + (1 - sqx) * cos_;
        rotMatrix(1, 0) = xycos1 + zsin;
        rotMatrix(2, 0) = zxcos1 - ysin;

        rotMatrix(0, 1) = xycos1 - zsin;
        rotMatrix(1, 1) = sqy + (1 - sqy) * cos_;
        rotMatrix(2, 1) = yzcos1 + xsin;

        rotMatrix(0, 2) = zxcos1 + ysin;
        rotMatrix(1, 2) = yzcos1 - xsin;
        rotMatrix(2, 2) = sqz + (1 - sqz) * cos_;

        _propagatedNormalVectors[i] = rotMatrix * _propagatedNormalVectors[i - 1];
        _lastPropagatedNormalIndex++;

        prevTangent = curTangent;
    }

    // Do LERP. Not likely that SLERP is required
    VectorType prevN = _propagatedNormalVectors[requiredPropagationIndex - 1];
    VectorType nextN = _propagatedNormalVectors[requiredPropagationIndex];

    VectorType::ComponentType blendParam = (t / getParametricLength()) * _propagatedNormalVectors.size();
    blendParam = blendParam - (int)blendParam;
    VectorType normal = (1 - blendParam) * prevN + blendParam * nextN;

    // Orthogonalize tangent/normal pair
    VectorType tangent = getTangentVector(t);

    normal = itk::CrossProduct(itk::CrossProduct(tangent, normal), tangent);

    normal.Normalize();
    return normal;

#if 0
    // Compute normal using central differences as the cross product of two tangent vectors
    ParameterType finiteDifferencesParameterDelta = std::min(getParametricLength() * 0.01, MINIMUM_FINITE_DIFFERENCES_DELTA);
    ParameterType prevT = std::max(0.0, t - finiteDifferencesParameterDelta);
    ParameterType nextT = std::min(t + finiteDifferencesParameterDelta, getParametricLength());

    VectorType prevTangent = getTangentVector(prevT);
    VectorType nextTangent = getTangentVector(nextT);

    VectorType normal = itk::CrossProduct(prevTangent, nextTangent);
    auto length = normal.GetNorm();

    if (length < 1e-8) {
        // Tangents are parallel, fall back on simple fixed-vector cross product
        VectorType tangent = getTangentVector(t);
        VectorType binormal(0.0);
        binormal[std::min_element(&tangent[0], &tangent[0] + 3) - &tangent[0]] = 1; 

        normal = itk::CrossProduct(tangent, binormal);
        length = normal.GetNorm();
    }

    return normal / length;
#endif
}

auto vtkParametricSplineVesselPathData::getParametricLength() const -> ParameterType
{
    if (_lengthDirty) {
        if (controlPointsCount() < 2) {
            _length = 0;
        }
        else {
            // TODO: optimize if necessary

            int resolution = controlPointsCount() * 2;
            ParameterType prevLength = 0;
            ParameterType currLength = 0;

            // Iteratively compute the spline length
            // In addition compute the correct arc-length stored in the "parameters" data array

            auto parameterArray = vtkSmartPointer<vtkDoubleArray>::New();
            parameterArray->SetName("parameters");
            _splineSource->SetParametricFunction(_spline);
            do {
                prevLength = currLength;

                _splineSource->SetUResolution(resolution);
                _splineSource->Update();
                auto points = _splineSource->GetOutput()->GetPoints();
                assert(points->GetNumberOfPoints() > 0);

                parameterArray->SetNumberOfTuples(_splineSource->GetOutput()->GetNumberOfPoints());
                parameterArray->SetTuple1(0, 0);

                currLength = 0;
                double prevPoint[3];
                points->GetPoint(0, prevPoint); // This is due to GetPoint() returning a pointer to an internal array that is replaced upon each call to GetPoint()
                for (vtkIdType i = 1; i < points->GetNumberOfPoints(); ++i) {
                    currLength += sqrt(vtkMath::Distance2BetweenPoints(prevPoint, points->GetPoint(i)));
                    parameterArray->SetTuple1(i, currLength);
                    memcpy(prevPoint, points->GetPoint(i), sizeof(prevPoint));
                }

                resolution *= 4;
            } while (fabs(prevLength - currLength) > 0.0001 * currLength);

            _splineSource->GetOutput()->GetPointData()->AddArray(parameterArray);

            _length = currLength;
        }

        _lengthDirty = false;
    }

    return _length;
}

void vtkParametricSplineVesselPathData::_updateSplineSourceCellLocator() const
{
    if (_lengthDirty) {
        getParametricLength(); // Update the spline source
    }

    // Update locator if necessary
    if (_splineSourceCellLocator->GetMTime() < _splineSource->GetMTime()) {
        _splineSourceCellLocator->SetDataSet(_splineSource->GetOutput());
        _splineSourceCellLocator->BuildLocator();
        _splineSourceCellLocator->Modified();

        _controlPointParameters.clear();
        auto parameterArray = static_cast<vtkDoubleArray*>(_splineSource->GetOutput()->GetPointData()->GetArray("parameters"));

        // Assign a segment index to each point of the splineSource polyData
        auto segmentIndexArray = vtkSmartPointer<vtkIntArray>::New();
        segmentIndexArray->Allocate(_splineSource->GetOutput()->GetNumberOfPoints());

        vtkIdType startOfSegmentPointId = 0;
        for (IdType segmentId = 1; segmentId < controlPointsCount(); ++segmentId) {
            double startParameter = parameterArray->GetValue(startOfSegmentPointId == 0 ? startOfSegmentPointId : startOfSegmentPointId - 1);
            double endParameter = parameterArray->GetValue(startOfSegmentPointId + 1 == parameterArray->GetNumberOfTuples() ? startOfSegmentPointId : startOfSegmentPointId + 1);

            double distToStart = getPosition(startParameter).EuclideanDistanceTo(getControlPoint(segmentId - 1));
            double distToEnd = getPosition(endParameter).EuclideanDistanceTo(getControlPoint(segmentId - 1));

            while (distToStart > 1e-6 && distToEnd > 1e-6 && fabs(distToEnd - distToStart) > 1e-8) {
                if (distToStart > distToEnd) {
                    startParameter = (startParameter + endParameter) / 2;
                    distToStart = getPosition(startParameter).EuclideanDistanceTo(getControlPoint(segmentId - 1));
                }
                else {
                    endParameter = (startParameter + endParameter) / 2;
                    distToEnd = getPosition(endParameter).EuclideanDistanceTo(getControlPoint(segmentId - 1));
                }
            }

            _controlPointParameters.push_back(distToStart < distToEnd ? startParameter : endParameter);

            double pos[3];
            _spline->GetPoints()->GetPoint(segmentId, pos);

            double closestPoint[3];
            vtkIdType cellId;
            int subId;
            double dist2;
            _splineSourceCellLocator->FindClosestPoint(pos, closestPoint, cellId, subId, dist2);

            vtkIdType endOfSegmentPointId = _splineSource->GetOutput()->GetCell(cellId)->GetPointId(subId);

            for (; startOfSegmentPointId <= endOfSegmentPointId; ++startOfSegmentPointId) {
                segmentIndexArray->SetValue(startOfSegmentPointId, segmentId - 1);
            }
        }
        _controlPointParameters.push_back(getParametricLength());

        _splineSource->GetOutput()->GetPointData()->SetScalars(segmentIndexArray);
    }
}

auto vtkParametricSplineVesselPathData::getControlPointParameterValue(IdType id) const -> ParameterType
{
    if (controlPointsCount() < 2) {
        return 0;
    }

    _updateSplineSourceCellLocator();

    return _controlPointParameters[id];
}

vtkSmartPointer<vtkPolyData> vtkParametricSplineVesselPathData::getPolyDataRepresentation() const
{
    if (controlPointsCount() < 2) {
        return nullptr;
    }    

    getParametricLength();
    _polyDataRepresentation = _splineSource->GetOutput();

    return _polyDataRepresentation;
}

auto vtkParametricSplineVesselPathData::getClosestPoint(const PointType& p) const -> ClosestPointQueryResult
{
    ClosestPointQueryResult result;
    result.resultType = ClosestPointQueryResult::CLOSEST_POINT_NONE;

    if (controlPointsCount() == 0) {
        return result;
    }

    if (controlPointsCount() == 1) {
        result.resultType = ClosestPointQueryResult::CLOSEST_POINT_ENDPOINT;
        result.controlPointId = 0;
        result.closestPoint = getControlPoint(0);
        return result;
    }

    _updateSplineSourceCellLocator();

    double pos[3];
    mitk::itk2vtk(p, pos);
    double closestPoint[3];
    vtkIdType cellId;
    int subId;
    double dist2;
    _splineSourceCellLocator->FindClosestPoint(pos, closestPoint, cellId, subId, dist2);

    vtkIdType pointId = _splineSource->GetOutput()->GetCell(cellId)->GetPointId(subId);
    mitk::vtk2itk(closestPoint, result.closestPoint);

    if (result.closestPoint.SquaredEuclideanDistanceTo(getControlPoint(0)) < 1e-8) {
        result.resultType = ClosestPointQueryResult::CLOSEST_POINT_ENDPOINT;
        result.controlPointId = 0;
        result.t = 0;
    }
    else if (result.closestPoint.SquaredEuclideanDistanceTo(getControlPoint(controlPointsCount() - 1)) < 1e-8) {
        result.resultType = ClosestPointQueryResult::CLOSEST_POINT_ENDPOINT;
        result.controlPointId = controlPointsCount() - 1;
        result.t = getParametricLength();
    }
    else {
        // Get the segment Id
        vtkIntArray* prevControlPointIndexArray = static_cast<vtkIntArray*>(_splineSource->GetOutput()->GetPointData()->GetScalars());

        result.resultType = ClosestPointQueryResult::CLOSEST_POINT_CURVE;
        result.controlPointId = prevControlPointIndexArray->GetValue(pointId);

        vtkCell* cell = _splineSource->GetOutput()->GetCell(cellId);

        double points[2][3];
        _splineSource->GetOutput()->GetPoint(cell->GetPointId(subId), points[0]);
        _splineSource->GetOutput()->GetPoint(cell->GetPointId(subId + 1), points[1]);
        double d[2] = { sqrt(vtkMath::Distance2BetweenPoints(closestPoint, points[0])), sqrt(vtkMath::Distance2BetweenPoints(closestPoint, points[1])) };
        double interpParam = d[0] / (d[0] + d[1]);

        auto parameterArray = static_cast<vtkDoubleArray*>(_splineSource->GetOutput()->GetPointData()->GetArray("parameters"));
        ParameterType t = parameterArray->GetTuple1(subId) * (1.0 - interpParam) + parameterArray->GetTuple1(subId + 1) * interpParam;

        result.t = t;
    }

    return result;
}

auto vtkParametricSplineVesselPathData::addControlPoint(const PointType& pos) -> IdType
{
    vtkSmartPointer<vtkPoints> points = _spline->GetPoints();
    if (!points) {
        points = vtkSmartPointer<vtkPoints>::New();
        _spline->SetPoints(points);
    }

    IdType rc = points->InsertNextPoint(pos[0], pos[1], pos[2]);
    _invokeControlPointEvent<ControlPointInsertEvent>(controlPointsCount() - 1);
    _invokeModified();

    return rc;
}

auto vtkParametricSplineVesselPathData::addControlPoint(IdType id, const PointType& pos) -> IdType
{
    vtkSmartPointer<vtkPoints> points = _spline->GetPoints();
    if (!points) {
        points = vtkSmartPointer<vtkPoints>::New();
        _spline->SetPoints(points);
    }

    if (id >= controlPointsCount()) {
        return addControlPoint(pos);
    }


    IdType nItemsToMove = controlPointsCount() - id;

    // Add a dummy point
    points->InsertNextPoint(0, 0, 0);

    // Move the control points information. Unfortunately vtkPoints and vtkDataArray do not provide such interface
    vtkDataArray* data = _spline->GetPoints()->GetData();
    memmove(data->GetVoidPointer((id + 1) * data->GetNumberOfComponents()),
        data->GetVoidPointer((id)* data->GetNumberOfComponents()),
        data->GetDataTypeSize() * data->GetNumberOfComponents() * nItemsToMove);

    // Set the new point values
    points->SetPoint(id, pos[0], pos[1], pos[2]);

    _invokeControlPointEvent<ControlPointInsertEvent>(id);
    _invokeModified();

    return id;
}

auto vtkParametricSplineVesselPathData::controlPointsCount() const -> IdType
{
    return !_spline->GetPoints() ? 0 : _spline->GetPoints()->GetNumberOfPoints();
}

auto vtkParametricSplineVesselPathData::getControlPoint(IdType id) const -> PointType
{
    if (!_spline->GetPoints() || id >= controlPointsCount()) {
        MITK_ERROR << "Attempting to get a non-existing control point";
        return PointType();
    }

    double pos[3];
    _spline->GetPoints()->GetPoint(id, pos);
    return PointType(pos);
}

bool vtkParametricSplineVesselPathData::setControlPoint(IdType id, const PointType& pos)
{
    if (!_spline->GetPoints() || id >= controlPointsCount()) {
        MITK_ERROR << "Attempting to set a non-existing control point";
        return false;
    }

    _spline->GetPoints()->SetPoint(id, pos[0], pos[1], pos[2]);
    _invokeControlPointEvent<ControlPointModifiedEvent>(id);
    _invokeModified();
    return true;
}

bool vtkParametricSplineVesselPathData::setControlPoints(const std::vector<PointType>& controlPoints)
{
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

    for (auto it = controlPoints.begin(); it != controlPoints.end(); ++it) {
        points->InsertNextPoint((*it)[0], (*it)[1], (*it)[2]);
    }

    _spline->SetPoints(points);
    _invokeModified();
    this->InvokeEvent(AllControlPointsSetEvent());
    return true;
}

bool vtkParametricSplineVesselPathData::removeControlPoint(IdType id)
{
    if (!_spline->GetPoints() || id >= controlPointsCount()) {
        return false;
    }

    _spline->GetPoints()->GetData()->RemoveTuple(id);
    _invokeControlPointEvent<ControlPointRemoveEvent>(id);
    _invokeModified();

    return true;
}

void vtkParametricSplineVesselPathData::_recomputeBBox()
{
    mitk::BoundingBox::Pointer newBBox = mitk::BoundingBox::New();

    for (IdType i = 0; i < controlPointsCount(); ++i) {
        newBBox->ConsiderPoint(getControlPoint(i));
    }

    this->GetTimeGeometry()->GetGeometryForTimeStep(0)->SetBounds(newBBox->GetBounds());
    this->GetTimeGeometry()->Update();
}

void vtkParametricSplineVesselPathData::_invokeModified()
{
    _lengthDirty = true;
    _lastPropagatedNormalIndex = -1;
    _recomputeBBox();

    _spline->Modified();
    this->Modified();
}

template<typename EventType>
void vtkParametricSplineVesselPathData::_invokeControlPointEvent(IdType id)
{
    EventType e;
    e.SetControlPointId(id);
    this->InvokeEvent(e);
}

auto vtkParametricSplineVesselPathData::_getVtkSplineParam(ParameterType t) const -> ParameterType
{
    // Compute the vtk's parameter value given the arc-length

    getParametricLength(); // Force the "parameters" array computation

    auto parameterArray = static_cast<vtkDoubleArray*>(_splineSource->GetOutput()->GetPointData()->GetArray("parameters"));
    int i = 0;
    if (parameterArray) {
        i = std::lower_bound(parameterArray->GetPointer(0), parameterArray->GetPointer(0) + parameterArray->GetNumberOfTuples(), t) - parameterArray->GetPointer(0);
    }

    if (i == 0) {
        return 0;
    }

    if (i == parameterArray->GetNumberOfTuples()) {
        return 1;
    }

    double vtkSplineParams[2] = { (i - 1.0) / (parameterArray->GetNumberOfTuples() - 1.0), i / (parameterArray->GetNumberOfTuples() - 1.0) };
    double interpValue = (t - parameterArray->GetTuple1(i - 1)) / (parameterArray->GetTuple1(i) - parameterArray->GetTuple1(i - 1));
    return (1.0 - interpValue) * vtkSplineParams[0] + interpValue * vtkSplineParams[1];
}

void vtkParametricSplineVesselPathData::setTension(mitk::ScalarType tension)
{
    static_cast<vtkKochanekSpline*>(_spline->GetXSpline())->SetDefaultTension(tension);
    static_cast<vtkKochanekSpline*>(_spline->GetYSpline())->SetDefaultTension(tension);
    static_cast<vtkKochanekSpline*>(_spline->GetZSpline())->SetDefaultTension(tension);

    this->InvokeEvent(TensionChangeEvent());
    _invokeModified();
}

mitk::ScalarType vtkParametricSplineVesselPathData::getTension() const
{
    return static_cast<vtkKochanekSpline*>(_spline->GetXSpline())->GetDefaultTension();
}

void vtkParametricSplineVesselPathData::ExecuteOperation(mitk::Operation* operation)
{
    auto op = dynamic_cast<VesselPathOperation*>(operation);
    if (!op) {
        return;
    }

    if (op->GetOperationType() == mitk::OpSCALE) { //changes spline tension
        this->setTension(op->GetTension());
    }
    else {
        VesselPathAbstractData::ExecuteOperation(op);
    }
}

}
