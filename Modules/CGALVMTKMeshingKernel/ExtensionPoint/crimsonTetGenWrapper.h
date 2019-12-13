/*=========================================================================

Program:   VMTK
Module:    $RCSfile: crimsonTetGenWrapper.h,v $
Language:  C++
Date:      $Date: 2006/04/06 16:47:48 $
Version:   $Revision: 1.7 $

  Copyright (c) Luca Antiga, David Steinman. All rights reserved.
  See LICENCE file for details.

  Portions of this code are covered under the VTK copyright.
  See VTKCopyright.txt or http://www.kitware.com/VTKCopyright.htm 
  for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
  // .NAME crimsonTetGenWrapper - Converts linear elements to quadratic.
  // .SECTION Description
  // ...

#ifndef __crimsonTetGenWrapper_h
#define __crimsonTetGenWrapper_h

#include <string>
#include "vtkUnstructuredGridAlgorithm.h"
//#include "vtkPointSet.h"

class crimsonTetGenWrapper : public vtkUnstructuredGridAlgorithm
{
  public: 
  static crimsonTetGenWrapper *New();
  vtkTypeMacro(crimsonTetGenWrapper,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent); 

  vtkSetMacro(PLC,int);
  vtkGetMacro(PLC,int);
  vtkBooleanMacro(PLC,int);
  
  vtkSetMacro(Refine,int);
  vtkGetMacro(Refine,int);
  vtkBooleanMacro(Refine,int);

  vtkSetMacro(Coarsen,int);
  vtkGetMacro(Coarsen,int);
  vtkBooleanMacro(Coarsen,int);

  vtkSetMacro(NoBoundarySplit,int);
  vtkGetMacro(NoBoundarySplit,int);
  vtkBooleanMacro(NoBoundarySplit,int);

  vtkSetMacro(Quality,int);
  vtkGetMacro(Quality,int);
  vtkBooleanMacro(Quality,int);
  
  vtkSetMacro(MaxRatio,double);
  vtkGetMacro(MaxRatio,double);
  
  vtkSetMacro(VarVolume,int);
  vtkGetMacro(VarVolume,int);
  vtkBooleanMacro(VarVolume,int);

  vtkSetMacro(FixedVolume,int);
  vtkGetMacro(FixedVolume,int);
  vtkBooleanMacro(FixedVolume,int);

  vtkSetMacro(MaxVolume,double);
  vtkGetMacro(MaxVolume,double);

  vtkSetMacro(RemoveSliver,int);
  vtkGetMacro(RemoveSliver,int);
  vtkBooleanMacro(RemoveSliver,int);

  vtkSetMacro(MaxDihedral,double);
  vtkGetMacro(MaxDihedral,double);

  vtkSetMacro(MinDihedral,double);
  vtkGetMacro(MinDihedral,double);

  vtkSetMacro(RegionAttrib,int);
  vtkGetMacro(RegionAttrib,int);
  vtkBooleanMacro(RegionAttrib,int);

  vtkSetMacro(Epsilon,double);
  vtkGetMacro(Epsilon,double);

  vtkSetMacro(NoMerge,int);
  vtkGetMacro(NoMerge,int);
  vtkBooleanMacro(NoMerge,int);
  
  vtkSetMacro(DetectInter,int);
  vtkGetMacro(DetectInter,int);
  vtkBooleanMacro(DetectInter,int);

  vtkSetMacro(CheckClosure,int);
  vtkGetMacro(CheckClosure,int);
  vtkBooleanMacro(CheckClosure,int);

  vtkSetMacro(Order,int);
  vtkGetMacro(Order,int);

  vtkSetMacro(DoCheck,int);
  vtkGetMacro(DoCheck,int);
  vtkBooleanMacro(DoCheck,int);

  vtkSetMacro(Verbose,int);
  vtkGetMacro(Verbose,int);
  vtkBooleanMacro(Verbose,int);

  vtkSetStringMacro(CellEntityIdsArrayName);
  vtkGetStringMacro(CellEntityIdsArrayName);

  vtkSetStringMacro(TetrahedronVolumeArrayName);
  vtkGetStringMacro(TetrahedronVolumeArrayName);

  vtkSetStringMacro(SizingFunctionArrayName);
  vtkGetStringMacro(SizingFunctionArrayName);

  vtkSetMacro(OutputSurfaceElements,int);
  vtkGetMacro(OutputSurfaceElements,int);
  vtkBooleanMacro(OutputSurfaceElements,int);

  vtkSetMacro(OutputVolumeElements,int);
  vtkGetMacro(OutputVolumeElements,int);
  vtkBooleanMacro(OutputVolumeElements,int);

  vtkSetMacro(UseSizingFunction,int);
  vtkGetMacro(UseSizingFunction,int);
  vtkBooleanMacro(UseSizingFunction,int);

  vtkSetMacro(LastRunExitStatus,int);
 
  vtkSetMacro(OptimizationLevel,int);
  vtkGetMacro(OptimizationLevel,int);

  vtkSetMacro(FullString,std::string);
  vtkGetMacro(FullString,std::string);

  protected:
  crimsonTetGenWrapper();
  ~crimsonTetGenWrapper();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  std::string FullString;
  int PLC;
  int Refine;
  int Coarsen;
  int NoBoundarySplit;
  int Quality;
  double MaxRatio;
  int VarVolume;
  int FixedVolume;
  double MaxVolume;
  int RemoveSliver;
  double MinDihedral;
  double MaxDihedral;
  int RegionAttrib;
  double Epsilon;
  int NoMerge;
  int DetectInter;
  int CheckClosure;
  int Order;
  int DoCheck;
  int Verbose;
  int UseSizingFunction;
  int OptimizationLevel;
  int LastRunExitStatus;

  char* CellEntityIdsArrayName;
  char* TetrahedronVolumeArrayName;
  char* SizingFunctionArrayName;

  int OutputSurfaceElements;
  int OutputVolumeElements;

  private:
  crimsonTetGenWrapper(const crimsonTetGenWrapper&);  // Not implemented.
  void operator=(const crimsonTetGenWrapper&);  // Not implemented.
};

#endif
