#pragma once

#include <mitkSlicedGeometry3D.h>
#include <VesselPathAbstractData.h>
#include <mitkSliceNavigationController.h>

namespace crimson {

class VesselDrivenSlicedGeometry : public mitk::SlicedGeometry3D {
public:
    mitkClassMacro(VesselDrivenSlicedGeometry, SlicedGeometry3D);
    itkNewMacro(Self);

    virtual itk::LightObject::Pointer InternalClone() const override
    {
        Self::Pointer newGeometry = new VesselDrivenSlicedGeometry(*this);
        newGeometry->UnRegister();
        return newGeometry.GetPointer();
    }


    void InitializedVesselDrivenSlicedGeometry(crimson::VesselPathAbstractData::Pointer vesselPath, crimson::VesselPathAbstractData::ParameterType parametricDelta, const mitk::Vector3D& referenceImageSpacing, mitk::ScalarType resliceWindowSize)
    {
        Superclass::Initialize();
        SetEvenlySpaced(false);
        _parametricDelta = parametricDelta;
        _vesselPath = vesselPath;
        _referenceImageSpacing = referenceImageSpacing;
        _resliceWindowSize = resliceWindowSize;
        _reinitializeGeometry();
    }

    mitk::PlaneGeometry* GetPlaneGeometry(int s) const override
    {
        if (!_vesselPath) {
            return nullptr;
        }

        if (!IsValidSlice(s)) {
            return nullptr;
        }

        if (m_PlaneGeometries[s]) {
            return m_PlaneGeometries[s];
        }

        auto planeGeometry = mitk::PlaneGeometry::New();

        crimson::VesselPathAbstractData::PointType pos;
        crimson::VesselPathAbstractData::VectorType tangent;
        crimson::VesselPathAbstractData::VectorType normal;
        crimson::VesselPathAbstractData::VectorType binormal;

        mitk::FillVector3D(pos, 0, 0, 0);
        mitk::FillVector3D(normal, 0, 0, 1);
        mitk::FillVector3D(normal, 0, 1, 0);
        mitk::FillVector3D(binormal, 1, 0, 0);

        if (_vesselPath->controlPointsCount() >= 2) {
            // Can compute the full frame of reference
            crimson::VesselPathAbstractData::ParameterType t = getParameterValueBySliceNumber(s);
            pos = _vesselPath->getPosition(t);

            tangent = _vesselPath->getTangentVector(t);
            normal = _vesselPath->getNormalVector(t);
            binormal = itk::CrossProduct(tangent, normal);
        }
        else if (_vesselPath->controlPointsCount() == 1) {
            // Can only compute position
            pos = _vesselPath->getPosition(0);
        }
        else {
            // No control points - fall back to defaults
        }

        // TODO: check if this works with rotated volumes
        mitk::Vector3D spacing;
        spacing[0] = CalculateSpacing(_referenceImageSpacing, normal);
        spacing[1] = CalculateSpacing(_referenceImageSpacing, binormal);
        spacing[2] = CalculateSpacing(_referenceImageSpacing, tangent);

        mitk::Vector2D resliceSizeInUnits;
        resliceSizeInUnits[0] = (int)(_resliceWindowSize / spacing[0]);
        resliceSizeInUnits[1] = (int)(_resliceWindowSize / spacing[1]);
        spacing[0] = _resliceWindowSize / resliceSizeInUnits[0];
        spacing[1] = _resliceWindowSize / resliceSizeInUnits[1];

        planeGeometry->InitializeStandardPlane(normal, binormal);
        planeGeometry->SetOrigin(pos - normal * _resliceWindowSize / 2 - binormal * _resliceWindowSize / 2);

        mitk::ScalarType bounds[6] = { 0, resliceSizeInUnits[0], 0, resliceSizeInUnits[1], -0.001, 0.001 };
        planeGeometry->SetBounds(bounds);
        planeGeometry->SetReferenceGeometry(planeGeometry); // Set reference geometry to the plane geometry itself so that the image mapper and geometry mapper work correctly
        m_PlaneGeometries[s] = planeGeometry;

        planeGeometry->SetSpacing(spacing);

        // Update the bounds of the whole 3D geometry
        mitk::BoundingBox::Pointer bbox = this->GetBoundingBox()->Clone();
        for (int i = 0; i < 8; ++i) {
            bbox->ConsiderPoint(planeGeometry->GetCornerPoint(i));
        }
        // TODO: duh!!
        const_cast<VesselDrivenSlicedGeometry*>(this)->SetBounds(bbox->GetBounds());

        return m_PlaneGeometries[s];
    }

    int findSliceByPoint(const mitk::Point3D& p) const
    {
        if (_vesselPath->controlPointsCount() < 2) {
            return 0;
        }

        float minDistance = std::numeric_limits<float>::max();
        int closestSliceId = 0;
        for (unsigned int i = 0; i < GetSlices(); ++i) {
            float distance = _vesselPath->getPosition(getParameterValueBySliceNumber(i)).SquaredEuclideanDistanceTo(p);
            if (distance < minDistance) {
                closestSliceId = i;
                minDistance = distance;
            }
        }

        return closestSliceId;
    }

    mitk::Point3D getSliceCenter(int s) const
    {
        if (_vesselPath->controlPointsCount() == 0) {
            return mitk::Point3D(0.0);
        }

        if (_vesselPath->controlPointsCount() == 1) {
            return _vesselPath->getControlPoint(0);
        }

        return _vesselPath->getPosition(getParameterValueBySliceNumber(s));
    }

    crimson::VesselPathAbstractData::ParameterType getParameterValueBySliceNumber(int slice) const
    {
        if (GetSlices() < 2) {
            return 0;
        }
        auto iter = _parameters.begin();
        std::advance(iter, std::min(slice, static_cast<int>(_parameters.size() - 1)));     
        return *iter;
    }

    int getSliceNumberByParameterValue(crimson::VesselPathAbstractData::ParameterType t) const
    {
        auto upper = _parameters.lower_bound(t);
        if (upper == _parameters.begin()) {
            return 0;
        }

        auto lower = upper;
        --lower;

        if (upper == _parameters.end() || (t - *lower) < (*upper - t)) {
            return std::distance(_parameters.begin(), lower);
        }
        return std::distance(_parameters.begin(), upper);
    }

private:
    VesselDrivenSlicedGeometry()
        : _vesselPath(nullptr)
        , _parametricDelta(1.0)
        , _referenceImageSpacing(1.0)
        , _resliceWindowSize(50.0)
        , _vesselObserverTag(0)
    {}

    VesselDrivenSlicedGeometry(const VesselDrivenSlicedGeometry& other)
        : mitk::SlicedGeometry3D()
        , _vesselPath(nullptr)
        , _parametricDelta(1.0)
        , _referenceImageSpacing(1.0)
        , _resliceWindowSize(50.0)
        , _vesselObserverTag(0)
    {
        // This constructor is a hugely bad hack to bypass SliceGeometry3D checks for all Geometry2Ds being initialized if
        m_SliceNavigationController = other.m_SliceNavigationController;
        InitializedVesselDrivenSlicedGeometry(other._vesselPath, other._parametricDelta, other._referenceImageSpacing, other._resliceWindowSize);
        // TODO: check
        // m_TimeBounds = other.m_TimeBounds;
    }

    virtual ~VesselDrivenSlicedGeometry()
    {
    }

    void _reinitializeGeometry()
    {
        _parameters.clear();
        _computeSliceToParameterMap();

        m_Slices = _parameters.size();
        mitk::PlaneGeometry::Pointer gnull = nullptr;
        m_PlaneGeometries.assign(m_Slices, gnull);

        mitk::SliceNavigationController* snc = GetSliceNavigationController();
        if (snc) {
            snc->GetSlice()->SetSteps(m_Slices);
            snc->GetSlice()->SetPos(snc->GetSlice()->GetPos()); // Clamp the position in case the number of steps decreased
            snc->Update();
        }

        this->Modified();
    }

    void _computeSliceToParameterMap()
    {
        if (_vesselPath->controlPointsCount() < 2) {
            _parameters.insert(0);
            return;
        }

        for (size_t i = 0; i < _vesselPath->controlPointsCount(); ++i) {
            _parameters.insert(_vesselPath->getControlPointParameterValue(i));
        }

        // Limit the angular change between consecutive slices. This allows to position the reslice window more precisely at sharp bends.
        double maxAngularDifference = vnl_math::pi / 90; // 2 degrees

        int nNormalSlices = ceil(_vesselPath->getParametricLength() / _parametricDelta);
        nNormalSlices = std::max(2, nNormalSlices);

        for (int i = 1; i < nNormalSlices - 1; ++i) {
            _parameters.insert(i * _vesselPath->getParametricLength() / (nNormalSlices - 1));
        }

        crimson::VesselPathAbstractData::VectorType curTangent = _vesselPath->getTangentVector(0);
        for (auto iter = _parameters.begin(); iter != --_parameters.end();) {
            mitk::ScalarType nextT = *std::next(iter);
            
            crimson::VesselPathAbstractData::VectorType nextTangent = _vesselPath->getTangentVector(nextT);

            if (curTangent * nextTangent < cos(maxAngularDifference)) {
                iter = --_parameters.insert((*iter + nextT) / 2).first; // Insert the new value and repeat for the current value
            }
            else {
                curTangent = nextTangent;
                ++iter;
            }
        }
    }

    crimson::VesselPathAbstractData::Pointer _vesselPath;
    crimson::VesselPathAbstractData::ParameterType _parametricDelta;
    mitk::Vector3D _referenceImageSpacing;
    mitk::ScalarType _resliceWindowSize;
    unsigned long _vesselObserverTag;
    std::set<mitk::ScalarType> _parameters;

private:
    virtual void InitializeSlicedGeometry(unsigned int ) override { assert(false); }
    virtual void InitializeEvenlySpaced(mitk::PlaneGeometry *, unsigned int ) override { assert(false); }
    virtual void InitializeEvenlySpaced(mitk::PlaneGeometry *, mitk::ScalarType , unsigned int ) override { assert(false); }
    virtual void InitializePlanes(const mitk::Geometry3D *, mitk::PlaneGeometry::PlaneOrientation , bool  = true, bool  = true, bool  = false)  { assert(false); }
};


}
