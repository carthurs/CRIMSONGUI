/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGetBoundaryFaces.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGetBoundaryFaces - Get Boundary Faces from poldata and label them with integers
// .SECTION Description
// vtkGetBoundaryFaces is a filter to extract the boundary surfaces of a model, separate the surace into multiple regions and number each region. 

// .SECTION Caveats
// To see the coloring of the liens you may have to set the ScalarMode
// instance variable of the mapper to SetScalarModeToUseCellData(). (This
// is only a problem if there are point data scalars.)

// .SECTION See Also
// vtkExtractEdges

#ifndef __vtkGetBoundaryFaces_h
#define __vtkGetBoundaryFaces_h

#include <vtkPolyDataAlgorithm.h>

class vtkFeatureEdges;

class vtkGetBoundaryFaces : public
vtkPolyDataAlgorithm
{
public:
  static vtkGetBoundaryFaces* New();
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the feature angle for extracting feature edges.
  vtkSetClampMacro(FeatureAngle,double,0.0,180.0);
  vtkGetMacro(FeatureAngle,double);

protected:
  vtkGetBoundaryFaces();
  ~vtkGetBoundaryFaces();

  // Usual data generation method
  int RequestData(vtkInformation *vtkNotUsed(request), 
		  vtkInformationVector **inputVector, 
		  vtkInformationVector *outputVector);

  vtkFeatureEdges* boundaries;
  double FeatureAngle;
  vtkIntArray *newScalars;
  vtkPolyData *mesh;
  vtkPolyData *boundaryMesh;
  vtkIdList *allIds;

  void FindBoundaryRegion(const vtkIdType cellId, int reg);

private:
  vtkGetBoundaryFaces(const vtkGetBoundaryFaces&);  // Not implemented.
  void operator=(const vtkGetBoundaryFaces&);  // Not implemented.
};

#endif