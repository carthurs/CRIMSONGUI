/*=========================================================================
 
 Program:   Visualization Toolkit
 Module:    $RCSfile: vtkFeatureEdges.cxx,v $
 
 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
 
 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.
 
 =========================================================================*/
#include "vtkGetBoundaryFaces.h"

#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkMergePoints.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTriangleStrip.h"
#include "vtkUnsignedCharArray.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkPointData.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkFeatureEdges.h"
#include "vtkCellLocator.h"

#include <iostream>
#include <vector>

vtkStandardNewMacro(vtkGetBoundaryFaces);

// Construct object with feature angle = 50.0;
vtkGetBoundaryFaces::vtkGetBoundaryFaces()
{
    this->boundaries = vtkFeatureEdges::New();
    this->FeatureAngle = 50.0;
    this->newScalars = vtkIntArray::New();
    this->mesh = vtkPolyData::New();
    this->boundaryMesh = vtkPolyData::New();
    this->allIds = vtkIdList::New();
}

vtkGetBoundaryFaces::~vtkGetBoundaryFaces()
{
    if (this->boundaries)
    {
        this->boundaries->Delete();
    }
    if (this->newScalars)
    {
	this->newScalars->Delete();
    }
    if (this->mesh)
    {
	this->mesh->Delete();
    }
    if (this->boundaryMesh)
    {
	this->boundaryMesh->Delete();
    }
    if (this->allIds)
    {
	this->allIds->Delete();
    }
}

void vtkGetBoundaryFaces::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os,indent);
    os << indent << "Feature Angle: " << this->FeatureAngle << "\n";
}

// Generate Separated Surfaces with Region ID Numbers
int vtkGetBoundaryFaces::RequestData(
                                 vtkInformation *vtkNotUsed(request),
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector)
{
    // get the input and output
    vtkPolyData *input = vtkPolyData::GetData(inputVector[0]);
    vtkPolyData *output = vtkPolyData::GetData(outputVector);
    
    // Define variables used by the algorithm
    int reg = 0;                                          
    vtkSmartPointer<vtkPoints> inpts = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> inPolys = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> inStrips = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> inLines = vtkSmartPointer<vtkCellArray>::New();
    vtkPoints *newPts;
    vtkIdType numPts, numPolys;
    vtkIdType newId, cellId;

    //Variables for output points from Feature Edges
    vtkSmartPointer<vtkPoints> boundaryPts = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> boundaryLines = vtkSmartPointer<vtkCellArray>::New();

    //Get input points, polys and set the up in the vtkPolyData mesh
    inpts = input->GetPoints();
    inPolys = input->GetPolys();
    this->mesh->SetPoints(inpts);
    this->mesh->SetPolys(inPolys);

    //Get the number of Polys for scalar  allocation
    numPolys = input->GetNumberOfPolys();
    this->allIds->Allocate(numPolys);

    //Check the input to make sure it is there
    if (numPolys < 1)               
    {
        vtkDebugMacro("No input!");
	return 1;
    }

    //Set up Region scalar for each surface
    this->newScalars->SetNumberOfTuples(numPolys);

    //Set up Feature Edges for Boundary Edge Detection
    vtkPolyData* inputCopy = input->NewInstance();
    inputCopy->ShallowCopy(input);
    this->boundaries->SetInputData(inputCopy);
    this->boundaries->BoundaryEdgesOff();
    this->boundaries->ManifoldEdgesOff();
    this->boundaries->NonManifoldEdgesOff();
    this->boundaries->FeatureEdgesOn();
    this->boundaries->SetFeatureAngle(this->FeatureAngle);
    inputCopy->Delete();
    this->boundaries->Update();

    boundaryPts = this->boundaries->GetOutput()->GetPoints();
    boundaryLines = this->boundaries->GetOutput()->GetLines();
 
    this->boundaryMesh->SetPoints(boundaryPts);
    this->boundaryMesh->SetLines(boundaryLines);

    vtkDebugMacro("Starting Boundary Face Separation");

    //Build Links in the mesh to be able to perform complex polydata processes;
    this->mesh->BuildLinks();

    //Set Region value of each cell to be zero initially
    for(cellId = 0; cellId < numPolys ; cellId ++)
    {
        this->newScalars->InsertValue(cellId, reg);
    }

    //Go through each cell and perfrom region identification proces
    for (cellId=0; cellId< numPolys; cellId++)
    {
       //Check to make sure the value of the region at this cellId hasn't been set
       if (this->newScalars->GetValue(cellId) == 0)
       {
	   reg++;
	   //Call function to find all cells within certain region
	   this->FindBoundaryRegion(cellId,reg);
       }
    }

    //Copy all the input geometry and data to the output
    output->CopyStructure(input);
    output->GetPointData()->PassData(input->GetPointData());
    output->GetCellData()->PassData(input->GetCellData());

    //Add the new scalars array to the output
    this->newScalars->SetName("Regions");
    output->GetCellData()->AddArray(this->newScalars);
    output->GetCellData()->SetActiveScalars("Regions");

    return 1;
}

void vtkGetBoundaryFaces::FindBoundaryRegion(const vtkIdType cellId, int
reg)
{
    //Variables used in function
    int i;
    vtkIdType j,k,l; 
    static vtkIdType Id1=0;
    static vtkIdType Id2=0;
    vtkIdType *pts = 0;
    vtkIdType npts = 0;
    vtkIdType numNei, nei, p1, p2, nIds, neis;

    //Id List to store neighbor cells, added to member data allIds and delted at end of function
    vtkIdList *neiIds;
    neiIds = vtkIdList::New();
    neiIds->Allocate(VTK_CELL_SIZE);
    vtkIdType numBoundPts,numBoundLines; 

    //Id List to store the one neighbor cell for each set of nodes and a cell, deleted at end of function
    vtkIdList *neighbors;
    neighbors = vtkIdList::New();
    neighbors->Allocate(VTK_CELL_SIZE);

    //Variable for accessing neiIds list
    vtkIdType sz = 0;
    
    //Variables for the boundary cells adjacent to the boundary point
    vtkSmartPointer<vtkIdList> bLinesOne = vtkSmartPointer<vtkIdList>::New();
    vtkSmartPointer<vtkIdList> bLinesTwo = vtkSmartPointer<vtkIdList>::New();

    //Get output set of points and lines from the Feature Edges
    numBoundPts = this->boundaryMesh->GetPoints()->GetNumberOfPoints();
    numBoundLines = this->boundaryMesh->GetLines()->GetNumberOfCells();

    //Mark point with region number and get all points in the cell
    this->newScalars->InsertValue(cellId,reg);
    this->mesh->GetCellPoints(cellId,npts,pts);
    
    //Set up point arrays for checking of boundary points
    std::vector<double> boundPtComps(npts);
    std::vector<double> meshPt1Comps(npts);
    std::vector<double> meshPt2Comps(npts);

    //Get neighboring cell for each pair of points in current cell
    for (i=0; i < npts; i++)
    {
	p1 = pts[i];
	p2 = pts[(i+1)%(npts)];
	this->mesh->GetPoint(p1,meshPt1Comps.data());
	this->mesh->GetPoint(p2,meshPt2Comps.data());


	//Initial check to make sure the cell is in fact a face cell
	numNei = neighbors->GetNumberOfIds();
	this->mesh->GetCellEdgeNeighbors(cellId,p1,p2, neighbors);
	numNei = neighbors->GetNumberOfIds();

	//Check to make sure it is an oustide surface cell, i.e. one neighbor
	if (numNei==1)
	{
	    int count = 0;
	    for (j=0; j<numBoundPts;j++)
	    {

		//Check to see if cell is on the boundary, if it is get adjacent lines
		this->boundaryMesh->GetPoint(j,boundPtComps.data());
		if (boundPtComps[0]==meshPt1Comps[0] && boundPtComps[1]==meshPt1Comps[1] && boundPtComps[2]==meshPt1Comps[2])
		{ 
		    this->boundaryMesh->GetPointCells(j,bLinesOne);
		    count++;
		}
		if (boundPtComps[0]==meshPt2Comps[0] && boundPtComps[1]==meshPt2Comps[1] && boundPtComps[2]==meshPt2Comps[2])
		{ 
		    this->boundaryMesh->GetPointCells(j,bLinesTwo);
		    count++;
		}
	    }

	    nei=neighbors->GetId(0);
	    //if cell is not on the boundary, add new cell to check list
	    if (count < 2)
	    {
		if (this->newScalars->GetValue(nei)==0)
		{
		    neiIds->InsertId(sz,nei);
		    sz++;
		}
	    }
	    //if cell is on boundary, check to make sure it isn't false positive; don't add to check list
	    else
	    {
		bLinesOne->IntersectWith(bLinesTwo);
		if (bLinesOne->GetNumberOfIds() == 0)
		{
		    neiIds->InsertId(sz,nei);
		    sz++;
		}
	    }
	}
    }
    
    nIds = neiIds->GetNumberOfIds();
    neighbors->Delete();
    //If there are neighboring cells in the check list, run function recursively
    if (nIds>0)
    {
	//Add all Ids in current list to global list of Ids
	for (k=0; k< nIds;k++)
	{
	    neis = neiIds->GetId(k);
	    this->allIds->InsertId(Id1,neis);
	    Id1++;
	}
	neiIds->Delete();
	//Run function recursively for each Id just placed in the global list
	for (l=0; l< nIds;l++)
	{
	    neis = this->allIds->GetId(Id2);
            Id2++;
	    if (this->newScalars->GetValue(neis)==0)
	    {
		this->FindBoundaryRegion(neis,reg);
	    }
	}
    }
    
    else
    {
    neiIds->Delete();
    }
}
