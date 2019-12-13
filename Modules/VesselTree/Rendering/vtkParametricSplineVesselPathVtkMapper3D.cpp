#include <mitkSlicedGeometry3D.h>

#include "vtkParametricSplineVesselPathData.h"
#include "vtkParametricSplineVesselPathVtkMapper3D.h"

#include <Utilities/vesselTreeUtils.h>

#include <vtkGlyphSource2D.h>
#include <vtkPolyDataMapper.h>
#include <vtkParametricFunctionSource.h>
#include <vtkPropAssembly.h>
#include <vtkFollower.h>
#include <vtkAssemblyPath.h>
#include <vtkAssemblyPaths.h>
#include <vtkTubeFilter.h>
#include <vtkClipPolyData.h>
#include <vtkPlane.h>

namespace crimson {

vtkParametricSplineVesselPathVtkMapper3D::LocalStorage::LocalStorage()
    : tubeFilter(vtkSmartPointer<vtkTubeFilter>::New())
    , cutPlane(vtkSmartPointer<vtkPlane>::New())
    , clipper(vtkSmartPointer<vtkClipPolyData>::New())
    , cutPlane2(vtkSmartPointer<vtkPlane>::New())
    , clipper2(vtkSmartPointer<vtkClipPolyData>::New())
    , clippedPolyDataMapper(vtkSmartPointer<vtkPolyDataMapper>::New())
    , clippedPolyDataActor(vtkSmartPointer<vtkActor>::New())
    , allProps(vtkSmartPointer<SimplePropAssembly>::New())
    , glyphScreenSize(6)
    , tubeWidthInPixels(2)
{
    // Setup spline mapper chain

    clipper->SetClipFunction(cutPlane);
    clipper->InsideOutOn();

    clipper2->SetInputConnection(clipper->GetOutputPort());
    clipper2->SetClipFunction(cutPlane2);
    clipper2->InsideOutOn();

    tubeFilter->CappingOn();
    tubeFilter->SetInputConnection(clipper2->GetOutputPort());

    clippedPolyDataMapper->SetInputConnection(tubeFilter->GetOutputPort());
    clippedPolyDataMapper->ScalarVisibilityOff();

    clippedPolyDataActor->SetMapper(clippedPolyDataMapper);
    clippedPolyDataActor->GetProperty()->LightingOff();

    // Setup glyph mapper chain

    // Populate props assembly
    allProps->AddPart(clippedPolyDataActor);
}

vtkParametricSplineVesselPathVtkMapper3D::LocalStorage::~LocalStorage()
{
}

vtkParametricSplineVesselPathVtkMapper3D::vtkParametricSplineVesselPathVtkMapper3D()
    : _glyphSource(vtkSmartPointer<vtkGlyphSource2D>::New())
    , _minSpacing(1)
{
}

void vtkParametricSplineVesselPathVtkMapper3D::SetDefaultProperties(mitk::DataNode* node, mitk::BaseRenderer* renderer, bool overwrite)
{
    node->AddProperty("color.selected", mitk::ColorProperty::New(1, 0, 0), renderer, overwrite);
    node->AddProperty("vesselpath.line_width", mitk::IntProperty::New(2), renderer, overwrite);
    node->AddProperty("vesselpath.selected_line_width", mitk::IntProperty::New(2), renderer, overwrite);
    node->AddProperty("vesselpath.editing_line_width", mitk::IntProperty::New(3), renderer, overwrite);
    node->AddProperty("vesselpath.glyph.color", mitk::ColorProperty::New(0.1, 0.2, 0.8), renderer, overwrite);
    node->AddProperty("vesselpath.glyph.color.selected", mitk::ColorProperty::New(0.15, 0.90, 0.1), renderer, overwrite);
    node->AddProperty("vesselpath.glyph.line_width", mitk::IntProperty::New(2), renderer, overwrite);
    node->AddProperty("vesselpath.glyph.filled", mitk::BoolProperty::New(false), renderer, overwrite);
    node->AddProperty("vesselpath.glyph.screen_size", mitk::IntProperty::New(5), renderer, overwrite);
    node->AddProperty("vesselpath.glyph.editing_screen_size", mitk::IntProperty::New(7), renderer, overwrite);

    Superclass::SetDefaultProperties(node, renderer, overwrite);
}

void vtkParametricSplineVesselPathVtkMapper3D::GenerateDataForRenderer(mitk::BaseRenderer* renderer)
{
    mitk::DataNode* node = this->GetDataNode();

    if (node == nullptr) {
        return;
    }

    vtkParametricSplineVesselPathData* vesselPath = dynamic_cast<vtkParametricSplineVesselPathData*>(node->GetData());

    if (vesselPath == nullptr) {
        return;
    }

    float spacing[3];
    renderer->GetCurrentWorldGeometry()->GetSpacing().ToArray(spacing);
    _minSpacing = *std::min_element(spacing, spacing + 3);

    // Check if the spline itself has changed and generate shared primitives if necessary
    if (_sharedPrimitivesGenerateTime.GetMTime() < vesselPath->GetMTime()) {
        _sharedPrimitivesGenerateTime.Modified();
        _generateSharedPrimitives(vesselPath, renderer);
    }

    LocalStorage* localStorage = _localStorageHandler.GetLocalStorage(renderer);

    // Update renderer-specific primitives 
    if (localStorage->localPrimitivesGenerateTime.GetMTime() < _sharedPrimitivesGenerateTime.GetMTime()) {
        localStorage->localPrimitivesGenerateTime.Modified();
        _generateLocalPrimitives(vesselPath, renderer);
    }

    bool visible = true;
    GetDataNode()->GetVisibility(visible, renderer, "visible");

    if (!visible) {
        localStorage->allProps->VisibilityOff();
        return;
    }
    localStorage->allProps->VisibilityOn();


    this->applyDefaultColorAndOpacityProperties(renderer);

    // Recalculate opacities for 2D renderer if the primitives or the view geometry has changed (slice rotation or slice number change)
    if (renderer->GetMapperID() == mitk::BaseRenderer::Standard2D &&
        (localStorage->opacityUpdateTime.GetMTime() < localStorage->localPrimitivesGenerateTime.GetMTime() ||
        localStorage->opacityUpdateTime.GetMTime() < renderer->GetCurrentWorldPlaneGeometryUpdateTime())) {

        localStorage->opacityUpdateTime.Modified();
        _update2DOpacities(renderer);
    }

    int glyphScreenSize = 5;
    bool isEditing = false;
    if (GetDataNode()->GetBoolProperty("vesselpath.editing", isEditing) && isEditing) {
        GetDataNode()->GetIntProperty("vesselpath.glyph.editing_screen_size", glyphScreenSize, renderer);
    }
    else {
        GetDataNode()->GetIntProperty("vesselpath.glyph.screen_size", glyphScreenSize, renderer);
    }

    // Update glyph scaling if the primitives or the view geometry has changed (slice rotation, zooming or slice number change)
    if (localStorage->scalingUpdateTime.GetMTime() < localStorage->localPrimitivesGenerateTime.GetMTime() ||
        localStorage->scalingUpdateTime.GetMTime() < renderer->GetMTime() ||
        //localStorage->scalingUpdateTime.GetMTime() < renderer->GetCameraController()->GetMTime() ||
        localStorage->scalingUpdateTime.GetMTime() < renderer->GetCameraRotationController()->GetMTime() ||
        localStorage->scalingUpdateTime.GetMTime() < renderer->GetSliceNavigationController()->GetSlice()->GetMTime() ||
        localStorage->glyphScreenSize != glyphScreenSize ||
        localStorage->tubeWidthInPixels != _getSplineLineWidth(renderer) ||
        renderer->GetMapperID() == mitk::BaseRenderer::Standard3D) {

        localStorage->scalingUpdateTime.Modified();
        localStorage->glyphScreenSize = glyphScreenSize;
        localStorage->tubeWidthInPixels = _getSplineLineWidth(renderer);
        _updateGlyphScale(vesselPath, renderer);
    }
}

void vtkParametricSplineVesselPathVtkMapper3D::_generateSharedPrimitives(vtkParametricSplineVesselPathData* /*vesselPath*/, mitk::BaseRenderer* /*renderer*/)
{
}

void vtkParametricSplineVesselPathVtkMapper3D::_generateLocalPrimitives(vtkParametricSplineVesselPathData* vesselPath, mitk::BaseRenderer* renderer)
{
    LocalStorage* localStorage = _localStorageHandler.GetLocalStorage(renderer);

    //////////////////////////////////////////////////////////////////////////
    // Spline
    //////////////////////////////////////////////////////////////////////////

    vtkSmartPointer<vtkPolyData> polyDataRepresentation = vesselPath->getPolyDataRepresentation();
    bool polyDataValid = polyDataRepresentation != nullptr;
    localStorage->clippedPolyDataActor->SetVisibility(polyDataValid);
    if (polyDataValid) {
        if (renderer->GetMapperID() == mitk::BaseRenderer::Standard2D) {
            // Create the tube
            localStorage->clipper->SetInputData(polyDataRepresentation);
        }
        else {
            localStorage->clippedPolyDataMapper->SetInputDataObject(polyDataRepresentation);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Glyphs
    //////////////////////////////////////////////////////////////////////////

    // Synchronize number of control point glyphs
    if (localStorage->glyphProps.size() < vesselPath->controlPointsCount()) {
        for (size_t  i = localStorage->glyphProps.size(); i < vesselPath->controlPointsCount(); ++i) {
            auto newGlyphMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            newGlyphMapper->SetInputConnection(_glyphSource->GetOutputPort());

            auto newGlyphActor = vtkSmartPointer<vtkFollower>::New();
            newGlyphActor->SetMapper(newGlyphMapper);
            newGlyphActor->SetCamera(renderer->GetVtkRenderer()->GetActiveCamera());

            localStorage->glyphProps.push_back(newGlyphActor);
            localStorage->allProps->AddPart(newGlyphActor);
        }
    }
    else {
        for (int i = localStorage->glyphProps.size() - vesselPath->controlPointsCount(); i > 0; --i) {
            localStorage->allProps->RemovePart(localStorage->glyphProps[localStorage->glyphProps.size() - i]);
        }
        localStorage->glyphProps.resize(vesselPath->controlPointsCount());
    }

    // Synchronize positions of glyphs
    for (size_t i = 0; i < vesselPath->controlPointsCount(); ++i) {
        vtkFollower* glyphActor = localStorage->glyphProps[i];

        double vtkControlPoint[3];
        mitk::itk2vtk(vesselPath->getPosition(vesselPath->getControlPointParameterValue(i)), vtkControlPoint);

        glyphActor->SetPosition(vtkControlPoint);
    }
}

void vtkParametricSplineVesselPathVtkMapper3D::_update2DOpacities(mitk::BaseRenderer* renderer)
{
    LocalStorage* localStorage = _localStorageHandler.GetLocalStorage(renderer);

    double distance = _getInterSliceDistance(renderer);
    if (distance < 0) {
        return;
    }

    vtkParametricSplineVesselPathData* vesselPath = static_cast<vtkParametricSplineVesselPathData*>(GetDataNode()->GetData());

    //////////////////////////////////////////////////////////////////////////
    // Update spline opacity
    //////////////////////////////////////////////////////////////////////////
    if (vesselPath->getPolyDataRepresentation() != nullptr) {
        auto slicedWorldGeometry = dynamic_cast<const mitk::SlicedGeometry3D*>(renderer->GetSliceNavigationController()->GetCurrentGeometry3D());
        if (slicedWorldGeometry) {
            mitk::Stepper* slice = renderer->GetSliceNavigationController()->GetSlice();
            int nextSliceId = std::min(slice->GetPos() + 1, slice->GetSteps() - 1);
            mitk::PlaneGeometry* nextPlaneGeometry = slicedWorldGeometry->GetPlaneGeometry(nextSliceId);

            int prevSliceId = std::max((int)slice->GetPos() - 1, 0);
            mitk::PlaneGeometry* prevPlaneGeometry = slicedWorldGeometry->GetPlaneGeometry(prevSliceId);

            mitk::PlaneGeometry* currentPlaneGeometry = slicedWorldGeometry->GetPlaneGeometry(slice->GetPos());

            if (nextPlaneGeometry && prevPlaneGeometry && currentPlaneGeometry) {
                mitk::Vector3D normalizedNormal = currentPlaneGeometry->GetNormal();
                normalizedNormal.Normalize();
                float deltaPrev = fabs((prevPlaneGeometry->GetCenter() - currentPlaneGeometry->GetOrigin()) * normalizedNormal) * 0.5;
                float deltaNext = fabs((nextPlaneGeometry->GetCenter() - currentPlaneGeometry->GetOrigin()) * normalizedNormal) * 0.5;

                deltaPrev = std::max(deltaPrev, std::max(deltaNext, deltaPrev) * 0.001f);
                deltaNext = std::max(deltaNext, std::max(deltaNext, deltaPrev) * 0.001f);

                mitk::Point3D prevOrigin = currentPlaneGeometry->GetOrigin() - normalizedNormal * deltaPrev;
                mitk::Point3D nextOrigin = currentPlaneGeometry->GetOrigin() + normalizedNormal * deltaNext;

                float sign = currentPlaneGeometry->SignedDistance(prevOrigin) < 0 ? -1.f : 1.f;

                localStorage->cutPlane->SetOrigin(prevOrigin[0], prevOrigin[1], prevOrigin[2]);
                localStorage->cutPlane2->SetOrigin(nextOrigin[0], nextOrigin[1], nextOrigin[2]);

                localStorage->cutPlane->SetNormal(sign * normalizedNormal[0], sign * normalizedNormal[1], sign * normalizedNormal[2]);
                localStorage->cutPlane2->SetNormal(-sign * normalizedNormal[0], -sign * normalizedNormal[1], -sign * normalizedNormal[2]);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Update glyph opacities
    //////////////////////////////////////////////////////////////////////////
    for (size_t i = 0; i < localStorage->glyphProps.size(); ++i) {
        vtkFollower* glyphActor = localStorage->glyphProps[i];

        mitk::Point3D pos;
        mitk::vtk2itk(glyphActor->GetPosition(), pos);
        glyphActor->GetProperty()->SetOpacity(_getOpacityAtPoint(renderer, pos, distance));
        glyphActor->Modified();
    }

}

void vtkParametricSplineVesselPathVtkMapper3D::_updateGlyphScale(vtkParametricSplineVesselPathData* vesselPath, mitk::BaseRenderer* renderer)
{
    LocalStorage* localStorage = _localStorageHandler.GetLocalStorage(renderer);

    for (size_t i = 0; i < localStorage->glyphProps.size(); ++i) {
        vtkFollower* glyphActor = localStorage->glyphProps[i];
        float desiredSizeInMM = localStorage->glyphScreenSize * VesselTreeUtils::getPixelSizeInMM(renderer, vesselPath->getControlPoint(i));
        glyphActor->SetScale(desiredSizeInMM); // std::min((double)desiredSizeInMM, _minReferenceImageSpacing * 6.0));
    }

    if (renderer->GetMapperID() == mitk::BaseRenderer::Standard2D) {
        localStorage->tubeFilter->SetRadius(localStorage->tubeWidthInPixels * 0.5 * VesselTreeUtils::getPixelSizeInMM(renderer, renderer->GetCurrentWorldPlaneGeometry()->GetOrigin()));
    }
}

double vtkParametricSplineVesselPathVtkMapper3D::_getInterSliceDistance(mitk::BaseRenderer* renderer)
{
    // Compute distance between two slices within this renderer
    auto slicedWorldGeometry = dynamic_cast<const mitk::SlicedGeometry3D*>(renderer->GetSliceNavigationController()->GetCurrentGeometry3D());
    if (!slicedWorldGeometry) {
        return -1;
    }

    mitk::Stepper* slice = renderer->GetSliceNavigationController()->GetSlice();
    mitk::PlaneGeometry* currentPlaneGeometry = slicedWorldGeometry->GetPlaneGeometry(slice->GetPos());

    int nextSliceId = std::min(slice->GetPos() + 1, slice->GetSteps() - 1);
    mitk::PlaneGeometry* nextPlaneGeometry = slicedWorldGeometry->GetPlaneGeometry(nextSliceId);

    int prevSliceId = std::max((int)slice->GetPos() - 1, 0);
    mitk::PlaneGeometry* prevPlaneGeometry = slicedWorldGeometry->GetPlaneGeometry(prevSliceId);

    if (!currentPlaneGeometry || !nextPlaneGeometry || !prevPlaneGeometry) {
        return -1;
    }

    auto normalizedNormal = currentPlaneGeometry->GetNormal();
    normalizedNormal.Normalize();
    float prevDistance = fabs((prevPlaneGeometry->GetCenter() - currentPlaneGeometry->GetOrigin()) * normalizedNormal);
    float nextDistance = fabs((nextPlaneGeometry->GetCenter() - currentPlaneGeometry->GetOrigin()) * normalizedNormal);

    return std::max(nextDistance, prevDistance) / 2;
}

double vtkParametricSplineVesselPathVtkMapper3D::_getOpacityAtPoint(mitk::BaseRenderer* renderer, const mitk::Point3D& pos, double interSliceDistance)
{
    const mitk::PlaneGeometry* currentPlaneGeometry = renderer->GetSliceNavigationController()->GetCurrentPlaneGeometry();
    mitk::Vector3D normalizedNormal = currentPlaneGeometry->GetNormal();
    normalizedNormal.Normalize();

    return 1.0 - std::min(fabs((pos - currentPlaneGeometry->GetOrigin()) * normalizedNormal) / interSliceDistance, 1.0);
}

int vtkParametricSplineVesselPathVtkMapper3D::_getSplineLineWidth(mitk::BaseRenderer* renderer)
{
    int splineLineWidth = 2;
    bool isSelected = GetDataNode()->IsSelected(renderer);
    bool isEditing = false;

    GetDataNode()->GetBoolProperty("vesselpath.editing", isEditing, renderer);
    if (isEditing) {
        GetDataNode()->GetIntProperty("vesselpath.editing_line_width", splineLineWidth, renderer);
    }
    else if (isSelected) {
        GetDataNode()->GetIntProperty("vesselpath.selected_line_width", splineLineWidth, renderer);
    }
    else {
        GetDataNode()->GetIntProperty("vesselpath.line_width", splineLineWidth, renderer);
    }
    return splineLineWidth;
}

vtkProp* vtkParametricSplineVesselPathVtkMapper3D::GetVtkProp(mitk::BaseRenderer* renderer)
{
    return _localStorageHandler.GetLocalStorage(renderer)->allProps;
}

void vtkParametricSplineVesselPathVtkMapper3D::UpdateVtkTransform(mitk::BaseRenderer*)
{
}

void vtkParametricSplineVesselPathVtkMapper3D::applyDefaultColorAndOpacityProperties(mitk::BaseRenderer* renderer)
{
    LocalStorage* localStorage = _localStorageHandler.GetLocalStorage(renderer);

    vtkParametricSplineVesselPathData* vesselPath = dynamic_cast<vtkParametricSplineVesselPathData*>(GetDataNode()->GetData());

    //////////////////////////////////////////////////////////////////////////
    // Spline
    //////////////////////////////////////////////////////////////////////////
    if (vesselPath->getPolyDataRepresentation() != nullptr) {
        // TODO: use the opacity property as a global scaling factor

        float color[3];
        GetDataNode()->GetColor(color, renderer, GetDataNode()->IsSelected() ? "color.selected" : "color");

        localStorage->clippedPolyDataActor->GetProperty()->SetColor(color[0], color[1], color[2]);
        localStorage->tubeFilter->SetNumberOfSides(12);

        localStorage->clippedPolyDataActor->GetProperty()->SetLineWidth(_getSplineLineWidth(renderer));
    }

    //////////////////////////////////////////////////////////////////////////
    // Glyphs
    //////////////////////////////////////////////////////////////////////////
    // Define glyph type
    bool isEditing = false;
    if (GetDataNode()->GetBoolProperty("vesselpath.editing", isEditing, renderer) && isEditing) {
        _glyphSource->SetGlyphTypeToSquare();
    }
    else {
        _glyphSource->SetGlyphTypeToCircle();
    }

    bool glyphFilled = false;
    GetDataNode()->GetBoolProperty("vesselpath.glyph.filled", glyphFilled, renderer);
    _glyphSource->SetFilled(glyphFilled);

    int glyphLineWidth = 2;
    GetDataNode()->GetIntProperty("vesselpath.glyph.line_width", glyphLineWidth, renderer);

    float color[3];
    GetDataNode()->GetColor(color, renderer, GetDataNode()->IsSelected() ? "vesselpath.glyph.color.selected" : "vesselpath.glyph.color");
    _glyphSource->SetColor(color[0], color[1], color[2]);

    for (vtkFollower* glyphActor: localStorage->glyphProps) {
        glyphActor->GetProperty()->SetLineWidth(glyphLineWidth);
    }
}


//vtkStandardNewMacro(SimplePropAssembly);
SimplePropAssembly* SimplePropAssembly::New()
{
    return new SimplePropAssembly();
}

// Render this assembly and all of its Parts. The rendering process is recursive.
int SimplePropAssembly::RenderTranslucentPolygonalGeometry(vtkViewport *ren)
{
    vtkProp *prop;
    vtkAssemblyPath *path;
    double fraction;
    int renderedSomething = 0;

    // Make sure the paths are up-to-date
    this->UpdatePaths();

    double numberOfItems = static_cast<double>(this->Parts->GetNumberOfItems());
    fraction = numberOfItems >= 1.0 ?
        this->AllocatedRenderTime / numberOfItems : this->AllocatedRenderTime;


    // render the Paths
    vtkCollectionSimpleIterator sit;
    for (this->Paths->InitTraversal(sit); (path = this->Paths->GetNextPath(sit));)
    {
        prop = path->GetLastNode()->GetViewProp();
        if (prop->GetVisibility())
        {
            prop->SetAllocatedRenderTime(fraction, ren);
            renderedSomething += prop->RenderTranslucentPolygonalGeometry(ren);
        }
    }

    return renderedSomething;
}

// Render this assembly and all of its Parts. The rendering process is recursive.
int SimplePropAssembly::RenderVolumetricGeometry(vtkViewport *ren)
{
    vtkProp *prop;
    vtkAssemblyPath *path;
    double fraction;
    int renderedSomething = 0;

    // Make sure the paths are up-to-date
    this->UpdatePaths();

    double numberOfItems = static_cast<double>(this->Parts->GetNumberOfItems());
    fraction = numberOfItems >= 1.0 ?
        this->AllocatedRenderTime / numberOfItems : this->AllocatedRenderTime;

    // render the Paths
    vtkCollectionSimpleIterator sit;
    for (this->Paths->InitTraversal(sit); (path = this->Paths->GetNextPath(sit));)
    {
        prop = path->GetLastNode()->GetViewProp();
        if (prop->GetVisibility())
        {
            prop->SetAllocatedRenderTime(fraction, ren);
            renderedSomething += prop->RenderVolumetricGeometry(ren);
        }
    }

    return renderedSomething;
}

// Render this assembly and all its parts. The rendering process is recursive.
int SimplePropAssembly::RenderOpaqueGeometry(vtkViewport *ren)
{
    vtkProp *prop;
    vtkAssemblyPath *path;
    double fraction;
    int   renderedSomething = 0;
    double numberOfItems = 0.0;

    // Make sure the paths are up-to-date
    this->UpdatePaths();

    numberOfItems = static_cast<double>(this->Parts->GetNumberOfItems());
    fraction = numberOfItems >= 1.0 ?
        this->AllocatedRenderTime / numberOfItems : this->AllocatedRenderTime;

    // render the Paths
    vtkCollectionSimpleIterator sit;
    for (this->Paths->InitTraversal(sit); (path = this->Paths->GetNextPath(sit));)
    {
        prop = path->GetLastNode()->GetViewProp();
        if (prop->GetVisibility())
        {
            prop->SetAllocatedRenderTime(fraction, ren);
            renderedSomething += prop->RenderOpaqueGeometry(ren);
        }
    }

    return renderedSomething;
}

// Render this assembly and all its parts. The rendering process is recursive.
int SimplePropAssembly::RenderOverlay(vtkViewport *ren)
{
    vtkProp *prop;
    vtkAssemblyPath *path;
    double fraction;
    int   renderedSomething = 0;
    double numberOfItems = 0.0;

    // Make sure the paths are up-to-date
    this->UpdatePaths();

    numberOfItems = static_cast<double>(this->Parts->GetNumberOfItems());
    fraction = numberOfItems >= 1.0 ?
        this->AllocatedRenderTime / numberOfItems : this->AllocatedRenderTime;

    vtkCollectionSimpleIterator sit;
    for (this->Paths->InitTraversal(sit); (path = this->Paths->GetNextPath(sit));)
    {
        prop = path->GetLastNode()->GetViewProp();
        if (prop->GetVisibility())
        {
            prop->SetAllocatedRenderTime(fraction, ren);
            renderedSomething += prop->RenderOverlay(ren);
        }
    }

    return renderedSomething;
}

}
