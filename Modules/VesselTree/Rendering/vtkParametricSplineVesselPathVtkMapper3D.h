#pragma once

#include <mitkVtkMapper.h>
#include <vtkSmartPointer.h>
#include <vtkCollection.h>
#include <VesselTreeExports.h>

class vtkActor;
class vtkMapper;
class vtkFollower;
class vtkPolyData;
class vtkPropAssembly;
class vtkGlyphSource2D;
class vtkParametricFunctionSource;
class vtkTubeFilter;
class vtkPlane;
class vtkClipPolyData;

namespace crimson {

class SimplePropAssembly : public vtkPropAssembly {
public:
    vtkTypeMacro(SimplePropAssembly, vtkPropAssembly);

    static SimplePropAssembly *New();

    int RenderOpaqueGeometry(vtkViewport *ren) override;
    int RenderTranslucentPolygonalGeometry(vtkViewport *ren) override;
    int RenderVolumetricGeometry(vtkViewport *ren) override;
    int RenderOverlay(vtkViewport *ren) override;
};


/*! \brief   A class handling rendering of vtkParametricSplineVesselPathData (both 2D and 3D). */
class VesselTree_EXPORT vtkParametricSplineVesselPathVtkMapper3D : public mitk::VtkMapper {
public:
    static void SetDefaultProperties(mitk::DataNode*, mitk::BaseRenderer* = NULL, bool = false);

    mitkClassMacro(vtkParametricSplineVesselPathVtkMapper3D, mitk::VtkMapper);
    itkNewMacro(Self);

    void applyDefaultColorAndOpacityProperties(mitk::BaseRenderer* renderer);
    vtkProp* GetVtkProp(mitk::BaseRenderer* renderer) override;
    void UpdateVtkTransform(mitk::BaseRenderer*) override;

    void CalculateTimeStep(mitk::BaseRenderer*) override { }

private:
    vtkParametricSplineVesselPathVtkMapper3D();
    ~vtkParametricSplineVesselPathVtkMapper3D() {}

    vtkParametricSplineVesselPathVtkMapper3D(const Self&);
    Self& operator=(const Self&);

    virtual void GenerateDataForRenderer(mitk::BaseRenderer* renderer) override;

    void _generateSharedPrimitives(vtkParametricSplineVesselPathData* vesselPath, mitk::BaseRenderer* renderer);
    void _generateLocalPrimitives(vtkParametricSplineVesselPathData* vesselPath, mitk::BaseRenderer* renderer);

    void _update2DOpacities(mitk::BaseRenderer* renderer);
    void _updateGlyphScale(vtkParametricSplineVesselPathData* vesselPath, mitk::BaseRenderer* renderer);
    double _getInterSliceDistance(mitk::BaseRenderer* renderer);
    double _getOpacityAtPoint(mitk::BaseRenderer* renderer, const mitk::Point3D& pos, double interSliceDistance);
    int _getSplineLineWidth(mitk::BaseRenderer* renderer);

private:

    class LocalStorage {
    public:
        LocalStorage();
        ~LocalStorage();

        vtkSmartPointer<vtkTubeFilter> tubeFilter;
        vtkSmartPointer<vtkPlane> cutPlane;
        vtkSmartPointer<vtkClipPolyData> clipper;
        vtkSmartPointer<vtkPlane> cutPlane2;
        vtkSmartPointer<vtkClipPolyData> clipper2;

        vtkSmartPointer<vtkMapper> clippedPolyDataMapper;
        vtkSmartPointer<vtkActor> clippedPolyDataActor;

        std::vector<vtkSmartPointer<vtkFollower>> glyphProps;

        vtkSmartPointer<SimplePropAssembly> allProps;
        itk::TimeStamp localPrimitivesGenerateTime;
        itk::TimeStamp opacityUpdateTime;
        itk::TimeStamp scalingUpdateTime;

        int glyphScreenSize;
        int tubeWidthInPixels;

    private:
        LocalStorage(const LocalStorage&);
        LocalStorage& operator=(const LocalStorage&);
    };

    mitk::LocalStorageHandler<LocalStorage> _localStorageHandler;

    itk::TimeStamp _sharedPrimitivesGenerateTime;

    vtkSmartPointer<vtkGlyphSource2D> _glyphSource;

    mitk::ScalarType _minSpacing;
};

}
