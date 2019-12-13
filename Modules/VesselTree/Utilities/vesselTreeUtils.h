#pragma once

#include <mitkBaseRenderer.h>
#include <mitkVector.h>

#include <vtkCamera.h>
#include <vtkTransform.h>

class VesselTreeUtils {
public:
    static float getPixelSizeInMM(mitk::BaseRenderer* renderer, const mitk::Point3D& pos)
    {
        renderer->GetVtkRenderer()->SetWorldPoint(pos[0], pos[1], pos[2], 1.0);
        renderer->GetVtkRenderer()->WorldToDisplay();
        double displayp[3];
        renderer->GetVtkRenderer()->GetDisplayPoint(displayp);
        displayp[0] += 1;
        renderer->GetVtkRenderer()->SetDisplayPoint(displayp);
        renderer->GetVtkRenderer()->DisplayToWorld();
        mitk::Point3D pos2;
        mitk::vtk2itk(renderer->GetVtkRenderer()->GetWorldPoint(), pos2);

        return pos.EuclideanDistanceTo(pos2);
    }
};
