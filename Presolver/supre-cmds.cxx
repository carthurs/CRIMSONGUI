/*********************************************************************

Copyright (c) 2000-2007, Stanford University, 
    Rensselaer Polytechnic Institute, Kenneth E. Jansen, 
    Charles A. Taylor (see SimVascular Acknowledgements file 
    for additional contributors to the source code).

All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions 
are met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer. 
Redistributions in binary form must reproduce the above copyright 
notice, this list of conditions and the following disclaimer in the 
documentation and/or other materials provided with the distribution. 
Neither the name of the Stanford University or Rensselaer Polytechnic
Institute nor the names of its contributors may be used to endorse or
promote products derived from this software without specific prior 
written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.

**********************************************************************/

#include "cmd.h"
#include "cvSolverIO.h"

#ifdef USE_ZLIB
#include "zlib.h"
#else
#include <stdlib.h>
#define gzopen fopen
#define gzprintf fprintf
#define gzFile FILE*
#define gzclose fclose
#define gzgets fgets
#define gzeof feof
#endif

#include <time.h>
#include <unordered_map>
#include <vector>

// =========
//   Cross
// =========

inline
void Cross( double ax, double ay, double az, 
            double bx, double by, double bz, 
            double *prodx, double *prody, double *prodz )
{
  (*prodx) = ay * bz - az * by;
  (*prody) = -( ax * bz - az * bx );
  (*prodz) = ax * by - ay * bx;
  return;
}


// =======
//   Dot
// =======

inline
double Dot( double ax, double ay, double az, 
            double bx, double by, double bz )
{
  double product;

  product = ax * bx + ay * by + az * bz;
  return product;
}

//
// some helpful global variables
//
extern const char* oformat;

extern int numNodes_;
extern int numElements_;
extern int numMeshEdges_;
extern int numMeshFaces_;
extern int numWallProps_;
extern int numSolnVars_;
extern int numBoundaryFaces_;
extern int** boundaryElements_;
extern double* nodes_;
extern int* elements_;
extern int* boundaryElementsIds_;
extern std::unordered_map<int, std::vector<int>> boundaryElementIdToIndicesMap_;
extern int* xadj_;
extern int xadjSize_;
extern int* adjncy_;
extern int adjncySize_;
extern int* iBC_;
extern int* iBCB_;
extern double* BCB_;
extern double init_p_;
extern double init_v_[3];
extern double* soln_;
extern double* dispsoln_;
extern double* SWBtp_;
extern double* TWBtp_;
extern double* EWBtp_;
extern double* acc_;
// global ndsurf 
extern int* ndsurfg_;

extern int   DisplacementNumElements_;
extern int*  DisplacementConn_[3];
extern int   DisplacementNumNodes_;
extern int*  DisplacementNodeMap_;
extern double* DisplacementSolution_;
extern double  Displacement_Evw_;
extern double  Displacement_nuvw_;
extern double  Displacement_thickness_;
extern double  Displacement_kcons_;
extern double  Displacement_pressure_;

int writeGEOMBCDAT(char* filename);
int writeRESTARTDAT(char* filename);
int readRESTARTDAT(char* infile,int readSoln, int readDisp, int readAcc);

int parseNum(char *cmd, int *num);
int parseFile(char *cmd);
int NWgetNextNonBlankLine(int *eof);
int NWcloseFile();
int setNodesWithCode(char *cmd, int val);
int setBoundaryFacesWithCode(char *cmd,int setSurfID, int surfID, 
                             int setCode, int code, double value);
int parseDouble(char *cmd, double *num);
int parseDouble2(char *cmd, double *num);
int parseDouble3(char *cmd, double *v1, double *v2, double *v3); 
int parseCmdStr(char *cmd, char *mystr);
int parseNum2(char *cmd, int *num); 
int check_node_order(int n0, int n1, int n2, int n3, int elementId, 
                     int *k0,int *k1, int *k2, int *k3);
int fixFreeEdgeNodes(char *cmd);
int createMeshForDispCalc(char *cmd);

extern gzFile fp_;
extern char buffer_[MAXCMDLINELENGTH];


int cmd_ascii_format(char*) {

    // enter
    debugprint(stddbg,"Entering cmd_ascii_format.\n");

    static const char* newformat = "ascii";
    oformat = newformat;

    // cleanup
    debugprint(stddbg,"Exiting cmd_ascii_format.\n");
    return CV_OK;
}

int cmd_verbose(char*) {

    verbose_ = 1;

    // enter
    debugprint(stddbg,"Entering cmd_verbose_format.\n");
    // cleanup
    debugprint(stddbg,"Exiting cmd_verbose_format.\n");
    return CV_OK;
}

int cmd_phasta_node_order(char*) {
    // enter
    debugprint(stddbg,"Entering cmd_phasta_node_order.\n");
    phasta_node_order_ = 1;
    // cleanup
    debugprint(stddbg,"Exiting cmd_phasta_node_order.\n");
    return CV_OK;
}

int cmd_number_of_nodes(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_number_of_nodes.\n");

    // do work
    numNodes_ = 0;
    if (parseNum(cmd, &numNodes_) == CV_ERROR) {
        return CV_ERROR;
    }
    debugprint(stddbg,"  Number of Nodes = %i\n",numNodes_);

    // cleanup
    debugprint(stddbg,"Exiting cmd_number_of_nodes.\n");
    return CV_OK;

}

int cmd_number_of_elements(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_number_of_elements.\n");

    // do work
    numElements_ = 0;
    if (parseNum(cmd, &numElements_) == CV_ERROR) {
        return CV_ERROR;
    }
    debugprint(stddbg,"  Number of Elements = %i\n",numElements_);

    // cleanup
    debugprint(stddbg,"Exiting cmd_number_of_elements.\n");
    return CV_OK;
}

int cmd_number_of_mesh_faces(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_number_of_mesh_faces.\n");

    // do work
    numMeshFaces_ = 0;
    if (parseNum(cmd, &numMeshFaces_) == CV_ERROR) {
        return CV_ERROR;
    }
    debugprint(stddbg,"  Number of Mesh Faces = %i\n",numMeshFaces_);

    // cleanup
    debugprint(stddbg,"Exiting cmd_number_of_mesh_faces.\n");
    return CV_OK;
}

int cmd_number_of_mesh_edges(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_number_of_mesh_edges.\n");

    // do work
    numMeshEdges_ = 0;
    if (parseNum(cmd, &numMeshEdges_) == CV_ERROR) {
        return CV_ERROR;
    }
    debugprint(stddbg,"number of mesh edges = %i\n",numMeshEdges_);

    // cleanup
    debugprint(stddbg,"Exiting cmd_number_of_mesh_edges.\n");
    return CV_OK;
}

int cmd_number_of_solnvars(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_number_of_solnvars.\n");

    // do work
    numSolnVars_ = 0;
    if (parseNum(cmd, &numSolnVars_) == CV_ERROR) {
        return CV_ERROR;
    }
    debugprint(stddbg,"  Number of Soln Vars = %i\n",numSolnVars_);

    // cleanup
    debugprint(stddbg,"Exiting cmd_number_of_solnvars.\n");
    return CV_OK;
}


int cmd_initial_pressure(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_initial_pressure.\n");

    // do work
    double p = 0;
    init_p_ = 0.0;
    if (parseDouble(cmd, &p) == CV_ERROR) {
        return CV_ERROR;
    }
    init_p_ = p;
    debugprint(stddbg,"  Initial Pressure = %lf\n",init_p_);

    // cleanup
    debugprint(stddbg,"Exiting cmd_initial_pressure.\n");
    return CV_OK;

}


int cmd_initial_velocity(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_initial_velocity.\n");

    // do work
    init_v_[0] = 0.0;
    init_v_[1] = 0.0;
    init_v_[2] = 0.0;
    if (parseDouble3(cmd, &init_v_[0], &init_v_[1], &init_v_[2]) == CV_ERROR) {
        return CV_ERROR;
    }
    debugprint(stddbg,"  Initial Velocity = <%lf,%lf,%lf>\n",init_v_[0],init_v_[1],init_v_[2]);

    // cleanup
    debugprint(stddbg,"Exiting cmd_initial_velocity.\n");
    return CV_OK;

}


int cmd_nodes(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_nodes.\n");

    // do work
    if (numNodes_ == 0) {
      fprintf(stderr,"ERROR:  Must specify number of nodes before you read them in!\n");
      return CV_ERROR;
    }

    if (parseFile(cmd) == CV_ERROR) {
        return CV_ERROR;
    }

    nodes_ = new double [3*numNodes_];
    double x,y,z;

    // NOTE: currently node id is ignored,
    // nodes must be in sequential order!
    int nodeId;

    int eof = 0;

    for (int i = 0; i < numNodes_ ; i++) {
        if (NWgetNextNonBlankLine(&eof) == CV_ERROR) {
            delete nodes_;
            nodes_ = NULL;
            return CV_ERROR;
        }
        if (sscanf(buffer_,"%i %lf %lf %lf",&nodeId,&x,&y,&z) != 4) {
            fprintf(stderr,"WARNING:  line not of correct format (%s)\n",buffer_);
            delete nodes_;
            nodes_ = NULL;
            return CV_ERROR;
        }
        nodes_[0*numNodes_+i] = x;
        nodes_[1*numNodes_+i] = y;
        nodes_[2*numNodes_+i] = z;
    }
    NWcloseFile();

    // cleanup
    debugprint(stddbg,"Exiting cmd_nodes.\n");
    return CV_OK;

}

int cmd_elements(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_elements.\n");

    // do work
    if (numElements_ == 0) {
      fprintf(stderr,"ERROR:  Must specify number of elements before you read them in!\n");
      return CV_ERROR;
    }

    if (parseFile(cmd) == CV_ERROR) {
        return CV_ERROR;
    }

    elements_ = new int [4*numElements_];

    int n0,n1,n2,n3;

    // NOTE: currently element id is ignored,
    // elements must be in sequential order!
    int elementId;

    int eof = 0;

    for (int i = 0; i < numElements_ ; i++) {
        NWgetNextNonBlankLine(&eof);
        if (sscanf(buffer_,"%i %i %i %i %i",&elementId,&n0,&n1,&n2,&n3) != 5) {
            fprintf(stderr,"WARNING:  line not of correct format (%s)\n",buffer_);
            NWcloseFile();
            delete elements_;
            elements_ = NULL;
            return CV_ERROR;
        }

        int j0 = 0;
        int j1 = 0;
        int j2 = 0;
        int j3 = 0;

        check_node_order(n0, n1, n2, n3, elementId, 
                         &j0,&j1,&j2,&j3);

        elements_[0*numElements_+i] = j0;
        elements_[1*numElements_+i] = j1;
        elements_[2*numElements_+i] = j2;
        elements_[3*numElements_+i] = j3;
    }
    NWcloseFile();

    // cleanup
    debugprint(stddbg,"Exiting cmd_elements.\n");
    return CV_OK;

}

int cmd_boundary_faces(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_boundary_faces.\n");

    // do work
    if (numElements_ == 0) {
      fprintf(stderr,"ERROR:  Must specify number of elements before boundary faces!\n");
      return CV_ERROR;
    }
    if (numMeshFaces_ == 0) {
      fprintf(stderr,"ERROR:  Must specify number of mesh faces before boundary faces!\n");
      return CV_ERROR;
    }

    if (parseFile(cmd) == CV_ERROR) {
        return CV_ERROR;
    }

    // init data fort time this function is called
    if (boundaryElements_ == NULL) {
        boundaryElements_ = new int* [4];
        boundaryElements_[0] = new int [numMeshFaces_];
        boundaryElements_[1] = new int [numMeshFaces_]; 
        boundaryElements_[2] = new int [numMeshFaces_]; 
        boundaryElements_[3] = new int [numMeshFaces_];
        boundaryElementsIds_ = new int [numMeshFaces_];
    }

    int n0,n1,n2,n3;

    // NOTE: currently element id is ignored,
    // elements must be in sequential order!
    int elementId,matId;

    int eof = 0;

    int i = 0;
    for (i = 0; i < numMeshFaces_ ; i++) {
        if (NWgetNextNonBlankLine(&eof) == CV_ERROR) {
            if (eof != 0) break;
            return CV_ERROR;
        }
        if (sscanf(buffer_,"%i %i %i %i %i",&elementId,&matId,&n0,&n1,&n2) != 5) {
            fprintf(stderr,"WARNING:  line not of correct format (%s)\n",buffer_);
            continue;
        }

        n3 = -1;
        int j0 = 0;
        int j1 = 0;
        int j2 = 0;
        int j3 = 0;

        check_node_order(n0, n1, n2, n3, elementId, 
                         &j0,&j1,&j2,&j3);

        boundaryElements_[0][numBoundaryFaces_] = j0;
        boundaryElements_[1][numBoundaryFaces_] = j1;
        boundaryElements_[2][numBoundaryFaces_] = j2;
        boundaryElements_[3][numBoundaryFaces_] = j3;

        // note that I assume element numbering starts at 1,
        // whereas phasta assumes it started at zero!!
        boundaryElementsIds_[numBoundaryFaces_] = elementId - 1;
        boundaryElementIdToIndicesMap_[elementId - 1].push_back(numBoundaryFaces_);

        numBoundaryFaces_++;

    }
    NWcloseFile();

    // cleanup
    debugprint(stddbg,"Exiting cmd_boundary_elements.\n");
    return CV_OK;

}

int cmd_noslip(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_noslip.\n");

    // do work

    // no-slip code is 56
    setNodesWithCode(cmd,56);

    // cleanup
    debugprint(stddbg,"Exiting cmd_noslip.\n");
    return CV_OK;
}

int cmd_prescribed_velocities(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_prescribed_velocities.\n");

    // do work

    // no-slip code is 56
    setNodesWithCode(cmd,56);

    // cleanup
    debugprint(stddbg,"Exiting cmd_prescribed_velocities.\n");
    return CV_OK;
}

int cmd_fix_free_edge_nodes(char *cmd) {

    // enter
    debugprint(stddbg,"Entering fix_free_edge_nodes.\n");

    // do work
    if (fixFreeEdgeNodes(cmd) == CV_ERROR) {
        return CV_ERROR;
    }

    // cleanup
    debugprint(stddbg,"Exiting cmd_fix_free_edge_nodes.\n");
    return CV_OK;
}

int cmd_create_mesh_deformable(char *cmd) {

    // enter
    debugprint(stddbg,"Entering create_mesh_deformable.\n");

    // do work
    if (createMeshForDispCalc(cmd) == CV_ERROR) {
        return CV_ERROR;
    }

    // cleanup
    debugprint(stddbg,"Exiting create_mesh_deformable.\n");
    return CV_OK;
}

int cmd_zero_pressure(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_zero_pressure.\n");

    // do work
    double pressure = 0.0;
    int setSurfID = 0;
    int surfID = 0;
    int setPressure = 1;

    // should use bits and not ints here!!
    int code = 2;
    
    if (setBoundaryFacesWithCode(cmd,setSurfID,surfID,
                                 setPressure,code,pressure) == CV_ERROR) {
        return CV_ERROR;
    }

    // cleanup
    debugprint(stddbg,"Exiting cmd_zero_pressure.\n");
    return CV_OK;
}


int cmd_pressure(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_pressure.\n");

    // do work
    double pressure = 0.0;

    if (parseDouble2(cmd,&pressure) == CV_ERROR) {
        return CV_ERROR;
    }

    debugprint(stddbg,"  Pressure = %lf\n",pressure);

    int setSurfID = 0;
    int surfID = 0;
    int setPressure = 1;
    // should use bits and not ints here!!
    int code = 2;
    
    if (setBoundaryFacesWithCode(cmd,setSurfID,surfID,
                                 setPressure,code,pressure) == CV_ERROR) {
        return CV_ERROR;
    }

    // cleanup
    debugprint(stddbg,"Exiting cmd_pressure.\n");
    return CV_OK;
}


int cmd_deformable_wall(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_deformable_wall.\n");

    // do work
    double value = 0.0;
    int setSurfID = 0;
    int surfID = 0;
    int setCode = 1;
    // should use bits and not ints here!!
    int code = 16;
    
    if (setBoundaryFacesWithCode(cmd,setSurfID,surfID,
                                 setCode,code,value) == CV_ERROR) {
        return CV_ERROR;
    }

    // cleanup
    debugprint(stddbg,"Exiting cmd_deformable_wall.\n");
    return CV_OK;
}


int cmd_set_surface_id(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_set_surface_id.\n");

    // do work
    int surfID = 0;
    if (parseNum2(cmd,&surfID) == CV_ERROR) {
        return CV_ERROR;
    }

    debugprint(stddbg,"  Setting surfID to [%i]\n",surfID);

    double value = 0.0;
    int setSurfID = 1;
    int setCode = 0;
    // should use bits and not ints here!!
    int code = 0;
    
    if (setBoundaryFacesWithCode(cmd,setSurfID,surfID,
                                 setCode,code,value) == CV_ERROR) {
        return CV_ERROR;
    }

    // cleanup
    debugprint(stddbg,"Exiting cmd_set_surface_id.\n");
    return CV_OK;
}

int cmd_adjacency(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_adjacency.\n");

    // do work
    if (parseFile(cmd) == CV_ERROR) {
        return CV_ERROR;
    }

    xadjSize_ = 0;

    int eof = 0;

    if (NWgetNextNonBlankLine(&eof) == CV_ERROR) {
        return CV_ERROR;
    }

    if (sscanf(buffer_,"xadj: %i\n",&xadjSize_) != 1) {
        fprintf(stderr,"ERROR parsing line (%s)\n",buffer_);
        return CV_ERROR;
    }

   adjncySize_ = 0;

    if (NWgetNextNonBlankLine(&eof) == CV_ERROR) {
        return CV_ERROR;
    }

   if (sscanf(buffer_,"adjncy: %i\n",&adjncySize_) != 1) {
        fprintf(stderr,"ERROR parsing line (%s)\n",buffer_);
        return CV_ERROR;
   }

   xadj_ = new int [xadjSize_];
   adjncy_ = new int [adjncySize_];

   for (int i = 0; i < xadjSize_ ; i++) {
      if (NWgetNextNonBlankLine(&eof) == CV_ERROR) {
        return CV_ERROR;
      }
      if (sscanf(buffer_,"%i",&xadj_[i]) != 1) {
            fprintf(stderr,"ERROR:  line not of correct format (%s)\n",buffer_);          
            delete xadj_;
            delete adjncy_;
            return CV_ERROR;
      }
   }

   for (int i = 0; i < adjncySize_ ; i++) {
      if (NWgetNextNonBlankLine(&eof) == CV_ERROR) {
        return CV_ERROR;
      }
      if (sscanf(buffer_,"%i",&adjncy_[i]) != 1) {
            fprintf(stderr,"ERROR:  line not of correct format (%s)\n",buffer_);
            delete xadj_;
            delete adjncy_;
            return CV_ERROR;
      }
   }

   NWcloseFile();
   return CV_OK;

}

int cmd_deformable_Evw(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_deformable_Evw.\n");

    // do work
    double value = 0;
    Displacement_Evw_ = 0.0;
    if (parseDouble(cmd, &value) == CV_ERROR) {
        return CV_ERROR;
    }
    Displacement_Evw_ = value;
    debugprint(stddbg,"  Evw = %lf\n",Displacement_Evw_);

    // cleanup
    debugprint(stddbg,"Exiting cmd_deformable_Evw.\n");
    return CV_OK;

}

int cmd_deformable_nuvw(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_deformable_nuvw.\n");

    // do work
    double value = 0;
    Displacement_nuvw_ = 0.0;
    if (parseDouble(cmd, &value) == CV_ERROR) {
        return CV_ERROR;
    }
    Displacement_nuvw_ = value;
    debugprint(stddbg,"  nuvw = %lf\n",Displacement_nuvw_);

    // cleanup
    debugprint(stddbg,"Exiting cmd_deformable_nuvw.\n");
    return CV_OK;

}

int cmd_deformable_thickness(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_deformable_thickness.\n");

    // do work
    double value = 0;
    Displacement_thickness_ = 0.0;
    if (parseDouble(cmd, &value) == CV_ERROR) {
        return CV_ERROR;
    }
    Displacement_thickness_ = value;
    debugprint(stddbg,"  thickness = %lf\n",Displacement_thickness_);

    // cleanup
    debugprint(stddbg,"Exiting cmd_deformable_thickness.\n");
    return CV_OK;

}

int cmd_deformable_kcons(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_deformable_kcons.\n");

    // do work
    double value = 0;
    Displacement_kcons_ = 0.0;
    if (parseDouble(cmd, &value) == CV_ERROR) {
        return CV_ERROR;
    }
    Displacement_kcons_ = value;
    debugprint(stddbg,"  kcons = %lf\n",Displacement_kcons_);

    // cleanup
    debugprint(stddbg,"Exiting cmd_deformable_kcons.\n");
    return CV_OK;

}

int cmd_number_of_wall_Props(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_number_of_wall_Props.\n");

    // do work
    numWallProps_ = 0;
    if (parseNum(cmd, &numWallProps_) == CV_ERROR) {
        return CV_ERROR;
    }
    debugprint(stddbg,"  Number of Wall Properties = %i\n",numWallProps_);

    // if numWallProps = 0, then generate SWB matrix without reading any external file, by assuming constant properties
    // these constant properties have been previously entered in the supre file: E, nu, thickness, kcons
    if (numWallProps_ == 0) {

      int nProps = 10;                  //isotropic material
      int neltp = numBoundaryFaces_;
      int size = nProps*neltp;
      int i;
      double K11, K21, K33, K44;

      K11 = Displacement_Evw_/(1.0 - Displacement_nuvw_*Displacement_nuvw_);
      K21 = K11 * Displacement_nuvw_;
      K33 = K11 * 0.5 * (1.0 - Displacement_nuvw_);
      K44 = K11 * 0.5 * (1.0 - Displacement_nuvw_) * Displacement_kcons_;

      SWBtp_ = new double[size]();

      // define the actual isotropic entries: this assumes zero inital stress: S11=S21=S22=S31=S32=zero
      for (i = 0; i < neltp; i++) {
          SWBtp_[numBoundaryFaces_*0+i] = Displacement_thickness_;
          SWBtp_[numBoundaryFaces_*1+i] = 0;   
          SWBtp_[numBoundaryFaces_*2+i] = 0;
          SWBtp_[numBoundaryFaces_*3+i] = 0;
          SWBtp_[numBoundaryFaces_*4+i] = 0;
          SWBtp_[numBoundaryFaces_*5+i] = 0;
          SWBtp_[numBoundaryFaces_*6+i] = K11;
          SWBtp_[numBoundaryFaces_*7+i] = K21;
          SWBtp_[numBoundaryFaces_*8+i] = K33;
          SWBtp_[numBoundaryFaces_*9+i] = K44;
      }

    }

    // cleanup
    debugprint(stddbg,"Exiting cmd_number_of_wall_Props.\n");
    return CV_OK;

}


int cmd_deformable_pressure(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_deformable_pressure.\n");

    // do work
    double value = 0;
    Displacement_pressure_ = 0.0;
    if (parseDouble(cmd, &value) == CV_ERROR) {
        return CV_ERROR;
    }
    Displacement_pressure_ = value;
    debugprint(stddbg,"  pressure = %lf\n",Displacement_pressure_);

    // cleanup
    debugprint(stddbg,"Exiting cmd_deformable_pressure.\n");
    return CV_OK;

}

#ifdef WITH_DEFORMABLE
int calcInitDisplacements(double Evw,double nuvw,
                          double thickness,double pressure,double kcons, 
                          int use_direct_solve);

int cmd_deformable_solve(char *cmd,int use_direct_solve) {

  // enter
  debugprint(stddbg,"Entering cmd_deformable_solve.\n");

  debugprint(stddbg,"  Solver Params:\n");
  debugprint(stddbg,"    Use Direct Solver = %i\n",use_direct_solve);
  debugprint(stddbg,"    Evw               = %lf\n",Displacement_Evw_);
  debugprint(stddbg,"    nuvw              = %lf\n",Displacement_nuvw_);
  debugprint(stddbg,"    thickness         = %lf\n",Displacement_thickness_);
  debugprint(stddbg,"    kcons             = %lf\n",Displacement_kcons_);
  debugprint(stddbg,"    pressure          = %lf\n",Displacement_pressure_);

  calcInitDisplacements(Displacement_Evw_,
                        Displacement_nuvw_,
                        Displacement_thickness_,
                        Displacement_pressure_,
                        Displacement_kcons_,
                        use_direct_solve);

  int i,size,nsd,nshg;

  if (dispsoln_ == NULL) {

    nsd = 3;
    nshg = numNodes_;
    size = nsd*nshg;

    dispsoln_ = new double[size]();
  }

  // stick in displacements
  for (i = 0; i < DisplacementNumNodes_; i++) {
    int nid = DisplacementNodeMap_[i];
    dispsoln_[numNodes_*0+nid-1] = DisplacementSolution_[3*i+0];
    dispsoln_[numNodes_*1+nid-1] = DisplacementSolution_[3*i+1];   
    dispsoln_[numNodes_*2+nid-1] = DisplacementSolution_[3*i+2];
  }

  // exit
  debugprint(stddbg,"Exiting cmd_deformable_solve.\n");
  return CV_OK;

}

int cmd_deformable_direct_solve(char *cmd) {
    return cmd_deformable_solve(cmd,1);
}
int cmd_deformable_iterative_solve(char *cmd) {
    return cmd_deformable_solve(cmd,0);
}

int cmd_deformable_write_vtk_mesh(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_deformable_write_vtk_mesh.\n");

    char outfile[MAXPATHLEN];
 
    // do work
    parseCmdStr(cmd,outfile);

    // some simple validity checks
    if (numNodes_ == 0 || numSolnVars_ == 0) {
        fprintf(stderr,"ERROR:  Not all required info set!\n");
        return CV_ERROR;
    }

    int i,j;

    FILE *fp = NULL;
    fp = fopen(outfile,"w");
    if (fp == NULL) {
        fprintf(stderr,"ERROR: could not open file (%s)\n",outfile);
        return CV_ERROR;
    }

    int numPts      = DisplacementNumNodes_;
    int numElem     = DisplacementNumElements_;
    int *map        = DisplacementNodeMap_;
    int *conn[3];
    conn[0] = DisplacementConn_[0];
    conn[1] = DisplacementConn_[1];
    conn[2] = DisplacementConn_[2];

    fprintf(fp,"# vtk DataFile Version 3.0\n");
    fprintf(fp,"vtk output\n");
    fprintf(fp,"ASCII\n");
    fprintf(fp,"DATASET POLYDATA\n");
    fprintf(fp,"POINTS %i double\n",numPts);
    for (i = 0; i < numPts; i++) {
        j = map[i];
        double x = nodes_[0*numNodes_+j-1];
        double y = nodes_[1*numNodes_+j-1];
        double z = nodes_[2*numNodes_+j-1];
        fprintf(fp,"%lf %lf %lf\n",x,y,z);
    }   
    fprintf(fp,"POLYGONS %i %i\n",numElem,4*numElem);
    for (i = 0; i < numElem; i++) {
        fprintf(fp,"3 %i %i %i\n",conn[0][i],conn[1][i],conn[2][i]);
    }
    fprintf(fp,"CELL_DATA %i\n",numElem);
    fprintf(fp,"POINT_DATA %i\n",numPts);
    fprintf(fp,"SCALARS scalars float\n");
    fprintf(fp,"LOOKUP_TABLE default\n"); 
    for (i = 0; i < numPts; i++) {
        j = map[i];
        fprintf(fp,"%i\n",j);
    }   
    fclose(fp);

    // cleanup
    debugprint(stddbg,"Exiting cmd_write_vtk_mesh.\n");

    return CV_OK;

}

int cmd_deformable_write_feap(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_deformable_write_feap.\n");

    char outfile[MAXPATHLEN];
 
    // do work
    parseCmdStr(cmd,outfile);

    // some simple validity checks
    if (numNodes_ == 0 || numSolnVars_ == 0) {
        fprintf(stderr,"ERROR:  Not all required info set!\n");
        return CV_ERROR;
    }

    int i,j;

    FILE *fp = NULL;
    fp = fopen(outfile,"w");
    if (fp == NULL) {
        fprintf(stderr,"ERROR: could not open file (%s)\n",outfile);
        return CV_ERROR;
    }

    int numPts      = DisplacementNumNodes_;
    int numElem     = DisplacementNumElements_;
    int *map        = DisplacementNodeMap_;
    int *conn[3];
    conn[0] = DisplacementConn_[0];
    conn[1] = DisplacementConn_[1];
    conn[2] = DisplacementConn_[2];

    fprintf(fp,"FEAP - output from supre\n");
    fprintf(fp,"%i %i %i %i %i\n",numPts,numElem,3,3,3);
    fprintf(fp," ! blank termination record\n");
    fprintf(fp,"COORdinates\n");
    for (i = 0; i < numPts; i++) {
        j = map[i];
        double x = nodes_[0*numNodes_+j-1];
        double y = nodes_[1*numNodes_+j-1];
        double z = nodes_[2*numNodes_+j-1];
        fprintf(fp,"%lf %lf %lf\n",x,y,z);
    }
    fprintf(fp," ! blank termination record\n");   
    fprintf(fp,"ELEMent\n");
    for (i = 0; i < numElem; i++) {
        // feap node numbering starts at 1!!
        fprintf(fp,"%i %i %i\n",conn[0][i]+1,conn[1][i]+1,conn[2][i]+1);
    }
    fprintf(fp," ! blank termination record\n");

    // this is the same loop we use when solving for the initial conditions
    int numbcs = 0;

    for (i = 0; i < numPts; i++) {
      // should use bit test instead of an int
      if (iBC_[map[i]-1] == 56) {
        for (int idegree=0; idegree < 3; idegree++) {
            numbcs++;
        }
      }
    }

    fprintf(fp,"TDOFprescribed\n");
    fprintf(fp,"%i\n",numbcs);    
    fprintf(fp,"DISPlacement\n");

    for (i = 0; i < numPts; i++) {
      // should use bit test instead of an int
      if (iBC_[map[i]-1] == 56) {
        for (int idegree=0; idegree < 3; idegree++) {
            fprintf(fp,"%i %i 0.0\n",i+1,idegree+1);
        }
      }
    }
    
    // final blank not needed I guess
    //fprintf(fp," ! blank termination record\n");

    fclose(fp);

    // cleanup
    debugprint(stddbg,"Exiting cmd_deformable_write_feap.\n");

    return CV_OK;

}

#endif // WITH_DEFORMABLE

int cmd_write_restartdat(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_write_restart.\n");

    char infile[MAXPATHLEN];
 
    // do work
    parseCmdStr(cmd,infile);

    writeRESTARTDAT(infile);
   
    // cleanup
    debugprint(stddbg,"Exiting cmd_write_restart.\n");
    return CV_OK;
}


int cmd_read_restart_solution(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_read_restart_solution.\n");

    char infile[MAXPATHLEN];
 
    // do work
    parseCmdStr(cmd,infile);

    int readSoln = 1;
    int readDisp = 0;
    int readAcc  = 0;
    readRESTARTDAT(infile,readSoln,readDisp,readAcc);
   
    // cleanup
    debugprint(stddbg,"Exiting cmd_read_restart_solution.\n");
    return CV_OK;
}


int cmd_read_restart_displacements(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_read_restart_displacements.\n");

    char infile[MAXPATHLEN];
 
    // do work
    parseCmdStr(cmd,infile);

    int readSoln = 0;
    int readDisp = 1;
    int readAcc  = 0;
    readRESTARTDAT(infile,readSoln,readDisp,readAcc);
   
    // cleanup
    debugprint(stddbg,"Exiting cmd_read_restart_displacements.\n");
    return CV_OK;
}


int cmd_read_restart_accelerations(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_read_accelerations.\n");

    char infile[MAXPATHLEN];
 
    // do work
    parseCmdStr(cmd,infile);

    int readSoln = 0;
    int readDisp = 0;
    int readAcc  = 1;
    readRESTARTDAT(infile,readSoln,readDisp,readAcc);
   
    // cleanup
    debugprint(stddbg,"Exiting cmd_read_restart_accelerations.\n");
    return CV_OK;
}


int cmd_write_geombcdat(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_write_geombcdat.\n");

    // parse command string
    int n = 0;
    int end = 0;
    char ignored[MAXSTRINGLENGTH];
    ignored[0]='\0';
    cmd_token_get (&n, cmd, ignored, &end);
    char infile[MAXPATHLEN];
    infile[0]='\0';
    cmd_token_get (&n, cmd, infile, &end);
 
    // do work
    writeGEOMBCDAT(infile);

    // cleanup
    debugprint(stddbg,"Exiting cmd_write_geombcdat.\n");
    return CV_OK;
}

int writeCommonHeader(int *filenum) {

    int magic_number = 362436;
    int* mptr = &magic_number;

    int iarray[10];
    int size, nitems;

    /* before anything we put in the standard headers */

    writestring_( filenum, "# PHASTA Input File Version 2.0\n");
    writestring_( filenum, "# Byte Order Magic Number : 362436 \n");
    writestring_( filenum, "# Created by supre version 0.1\n");
    writestring_( filenum,"# CmdLineArgs : \n");

    // write out time
//     time_t timenow = time ( &timenow);
//     sprintf(wrtstr,"# %s\n", ctime( &timenow ));
//     writestring_( filenum, wrtstr );
    writestring_(filenum,"\n");

    int one=1;

    size = 1;
    nitems = 1;
    iarray[ 0 ] = 1;
    writeheader_( filenum, "byteorder magic number ",
                  (void*)iarray, &nitems, &size, "integer", oformat );

    writedatablock_( filenum, "byteorder magic number ",
                     (void*)mptr, &nitems, "integer", oformat );

    return CV_OK;

}


int cmd_append_displacements(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_append_displacements.\n");

    char filename[MAXPATHLEN];
 
    // do work
    parseCmdStr(cmd,filename);

    // some simple validity checks
    if (numNodes_ == 0 || numSolnVars_ == 0 || dispsoln_ == NULL) {
        fprintf(stderr,"ERROR:  Not all required info set!\n");
        return CV_ERROR;
    }

    int filenum = -1;
    openfile_ (filename, "append", &filenum);
    if (filenum < 0) {
        fprintf(stderr,"ERROR:  could not open file (%s)\n",filename);
        return CV_ERROR;
    }

    int nsd = 3;
    int lstep = 0;
    int nshg = numNodes_;
    int size = nsd*nshg;
      
    // append to file
    int nitems = 3; 

    int iarray[3];
    iarray[ 0 ] = nshg;
    iarray[ 1 ] = nsd;
    iarray[ 2 ] = lstep;
 
    writeheader_( &filenum, "displacement ",
                  ( void* )iarray, &nitems, &size,"double", oformat );

    nitems = size;
    writedatablock_( &filenum, "displacement ",
                     ( void* )(dispsoln_), &nitems, "double", oformat );

    closefile_( &filenum,"append");
 
    delete dispsoln_;

    // cleanup
    debugprint(stddbg,"Exiting cmd_append_displacements.\n");
    return CV_OK;
}


int cmd_read_displacements(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_read_displacements.\n");

    // do work
    if (numNodes_ == 0) {
      fprintf(stderr,"ERROR:  Must specify number of nodes before you read in displacements!\n");
      return CV_ERROR;
    }

    if (parseFile(cmd) == CV_ERROR) {
        return CV_ERROR;
    }

    if (dispsoln_ == NULL) {

      int nsd = 3;
      int nshg = numNodes_;
      int size = nsd*nshg;

      dispsoln_ = new double[size]();
    }

    double vx,vy,vz;
    int nodeId;
    int eof = 0;

    while (NWgetNextNonBlankLine(&eof) == CV_OK) {
      if (sscanf(buffer_,"%i %lf %lf %lf",&nodeId,&vx,&vy,&vz) != 4) {
        fprintf(stderr,"WARNING:  line not of correct format (%s)\n",buffer_);
        return CV_ERROR;
      }
      dispsoln_[numNodes_*0+nodeId-1] = vx;
      dispsoln_[numNodes_*1+nodeId-1] = vy;   
      dispsoln_[numNodes_*2+nodeId-1] = vz;
    }
    NWcloseFile();

    // cleanup
    debugprint(stddbg,"Exiting cmd_read_displacements.\n");
    return CV_OK;

}

// Added by Nan 7/30/09
int cmd_write_displacements(char* cmd) {

	char outfile[MAXPATHLEN];
	int numPts = DisplacementNumNodes_;
	int *map = DisplacementNodeMap_;
	int nodeId;
	FILE *fp = NULL;
    
	// enter
    debugprint(stddbg,"Entering cmd_write_displacements.\n");

	// do work 
	if (dispsoln_ == NULL) {
      fprintf(stderr,"ERROR:  Must read in displacements first!\n");
      return CV_ERROR;
	}

    // get the name of the output file
    parseCmdStr(cmd,outfile);

	// try to open the file for writing
	fp = fopen(outfile,"w");
    if (fp == NULL) {
      fprintf(stderr,"ERROR: could not open file (%s)\n",outfile);
      return CV_ERROR;
    }

    // write the displacements
    
	for (int i = 0; i < numPts; i++) {
	  nodeId = map[i];
	  fprintf(fp,"%i %lf %lf %lf \n",nodeId,dispsoln_[numNodes_*0+nodeId-1],
		                                    dispsoln_[numNodes_*1+nodeId-1],
										    dispsoln_[numNodes_*2+nodeId-1]); 
	}

    // cleanup
	fclose(fp);

    debugprint(stddbg,"Exiting cmd_deformable_write_displacements.\n");

    return CV_OK;
}

int cmd_write_pressures(char* cmd) {
	char outfile[MAXPATHLEN];
	int numPts = DisplacementNumNodes_;
	int *map = DisplacementNodeMap_;
	int nodeId;
	FILE *fp = NULL;
    
	// enter
    debugprint(stddbg,"Entering cmd_write_pressures.\n");

	// do work 
	if (soln_ == NULL) {
      fprintf(stderr,"ERROR:  Must read in solution first!\n");
      return CV_ERROR;
	}

    // get the name of the output file
    parseCmdStr(cmd,outfile);

	// try to open the file for writing
	fp = fopen(outfile,"w");
    if (fp == NULL) {
      fprintf(stderr,"ERROR: could not open file (%s)\n",outfile);
      return CV_ERROR;
    }

    // write the displacements
	for (int i = 0; i < numPts; i++) {
	  nodeId = map[i];
	  fprintf(fp,"%i %lf \n",nodeId,soln_[nodeId-1]); 
	}

    // cleanup
	fclose(fp);

    debugprint(stddbg,"Exiting cmd_deformable_write_pressures.\n");

    return CV_OK;
}

int cmd_read_SWB_ORTHO(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_read_SWB_ORTHO.\n");

    // do work
    if (numBoundaryFaces_ == 0) {
      fprintf(stderr,"ERROR:  Must specify number of boundary faces before you read in SWB_ORTHO!\n");
      return CV_ERROR;
    }

    if (parseFile(cmd) == CV_ERROR) {
      return CV_ERROR;
    }

    if (SWBtp_ == NULL) {

      int nProps = 21;                  //orthotropic material
      int neltp = numBoundaryFaces_;
      int size = nProps*neltp;

      SWBtp_ = new double[size]();
    }

    double thick, S11, S21, S22, S31, S32, K11, K21, K22, K31, K32, K33, K41, K42, K43, K44, K51, K52, K53, K54, K55;
    int faceId;
    int eof = 0;

    while (NWgetNextNonBlankLine(&eof) == CV_OK) {
      if (sscanf(buffer_,"%i %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",&faceId,&thick,&S11,&S21,&S22,&S31,&S32,&K11,&K21,&K22,&K31,&K32,&K33,&K41,&K42,&K43,&K44,&K51,&K52,&K53,&K54,&K55) != 22) {
        fprintf(stderr,"WARNING:  line not of correct format (%s)\n",buffer_);
        return CV_ERROR;
      }
      SWBtp_[numBoundaryFaces_*0+faceId-1] = thick;
      SWBtp_[numBoundaryFaces_*1+faceId-1] = S11;   
      SWBtp_[numBoundaryFaces_*2+faceId-1] = S21;
      SWBtp_[numBoundaryFaces_*3+faceId-1] = S22;
      SWBtp_[numBoundaryFaces_*4+faceId-1] = S31;
      SWBtp_[numBoundaryFaces_*5+faceId-1] = S32;
      SWBtp_[numBoundaryFaces_*6+faceId-1] = K11;
      SWBtp_[numBoundaryFaces_*7+faceId-1] = K21;
      SWBtp_[numBoundaryFaces_*8+faceId-1] = K22;
      SWBtp_[numBoundaryFaces_*9+faceId-1] = K31;
      SWBtp_[numBoundaryFaces_*10+faceId-1] = K32;
      SWBtp_[numBoundaryFaces_*11+faceId-1] = K33;
      SWBtp_[numBoundaryFaces_*12+faceId-1] = K41;
      SWBtp_[numBoundaryFaces_*13+faceId-1] = K42;
      SWBtp_[numBoundaryFaces_*14+faceId-1] = K43;
      SWBtp_[numBoundaryFaces_*15+faceId-1] = K44;
      SWBtp_[numBoundaryFaces_*16+faceId-1] = K51;
      SWBtp_[numBoundaryFaces_*17+faceId-1] = K52;
      SWBtp_[numBoundaryFaces_*18+faceId-1] = K53;
      SWBtp_[numBoundaryFaces_*19+faceId-1] = K54;
      SWBtp_[numBoundaryFaces_*20+faceId-1] = K55;
    }
    NWcloseFile();

    // cleanup
    debugprint(stddbg,"Exiting cmd_read_SWB_ORTHO.\n");
    return CV_OK;

}

int cmd_read_SWB_ISO(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_read_SWB_ISO.\n");

    // do work
    if (numBoundaryFaces_ == 0) {
      fprintf(stderr,"ERROR:  Must specify number of boundary faces before you read in SWB_ISO!\n");
      return CV_ERROR;
    }

    if (parseFile(cmd) == CV_ERROR) {
      return CV_ERROR;
    }

    if (SWBtp_ == NULL) {

      int nProps = 10;                  //isotropic material
      int neltp = numBoundaryFaces_;
      int size = nProps*neltp;

      SWBtp_ = new double[size]();
    }

    double thick, S11, S21, S22, S31, S32, K11, K21, K33, K44;
    int faceId;
    int eof = 0;

    while (NWgetNextNonBlankLine(&eof) == CV_OK) {
      if (sscanf(buffer_,"%i %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",&faceId,&thick,&S11,&S21,&S22,&S31,&S32,&K11,&K21,&K33,&K44) != 11) {
        fprintf(stderr,"WARNING:  line not of correct format (%s)\n",buffer_);
        return CV_ERROR;
      }
      SWBtp_[numBoundaryFaces_*0+faceId-1] = thick;
      SWBtp_[numBoundaryFaces_*1+faceId-1] = S11;   
      SWBtp_[numBoundaryFaces_*2+faceId-1] = S21;
      SWBtp_[numBoundaryFaces_*3+faceId-1] = S22;
      SWBtp_[numBoundaryFaces_*4+faceId-1] = S31;
      SWBtp_[numBoundaryFaces_*5+faceId-1] = S32;
      SWBtp_[numBoundaryFaces_*6+faceId-1] = K11;
      SWBtp_[numBoundaryFaces_*7+faceId-1] = K21;
      SWBtp_[numBoundaryFaces_*8+faceId-1] = K33;
      SWBtp_[numBoundaryFaces_*9+faceId-1] = K44;
    }
    NWcloseFile();

    // cleanup
    debugprint(stddbg,"Exiting cmd_read_SWB_ISO.\n");
    return CV_OK;

}

int cmd_read_TWB(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_read_TWB.\n");

    // do work
    if (numBoundaryFaces_ == 0) {
      fprintf(stderr,"ERROR:  Must specify number of boundary faces before you read in TWB!\n");
      return CV_ERROR;
    }

    if (parseFile(cmd) == CV_ERROR) {
      return CV_ERROR;
    }

	if (TWBtp_ == NULL) {

      int nProps = 2;                // tissue stiffness and damping constants
      int neltp = numBoundaryFaces_;
      int size = nProps*neltp;

      TWBtp_ = new double[size]();
    }

	double suppstiff,suppvisc;
	int faceId;
	int eof = 0;

	while (NWgetNextNonBlankLine(&eof) == CV_OK) {
      if (sscanf(buffer_,"%i %lf %lf",&faceId,&suppstiff,&suppvisc) != 3) {
        fprintf(stderr,"WARNING:  line not of correct format (%s)\n",buffer_);
        return CV_ERROR;
      }
      TWBtp_[numBoundaryFaces_*0+faceId-1] = suppstiff;
      TWBtp_[numBoundaryFaces_*1+faceId-1] = suppvisc;   
    }
	NWcloseFile();

	// cleanup
    debugprint(stddbg,"Exiting cmd_read_TWB_ISO.\n");
    return CV_OK;
}

int cmd_read_EWB(char *cmd) {

    // enter
    debugprint(stddbg,"Entering cmd_read_EWB.\n");

    // do work
    if (numBoundaryFaces_ == 0) {
      fprintf(stderr,"ERROR:  Must specify number of boundary faces before you read in EWB!\n");
      return CV_ERROR;
    }

    if (parseFile(cmd) == CV_ERROR) {
      return CV_ERROR;
    }

	if (EWBtp_ == NULL) {

      int nProps = 1;                // State filter coefficient
      int neltp = numBoundaryFaces_;
      int size = nProps*neltp;

      EWBtp_ = new double[size]();
    }

	double statefcoeff;
	int faceId;
	int eof = 0;

	while (NWgetNextNonBlankLine(&eof) == CV_OK) {
      if (sscanf(buffer_,"%i %lf",&faceId,&statefcoeff) != 2) {
        fprintf(stderr,"WARNING:  line not of correct format (%s)\n",buffer_);
        return CV_ERROR;
      }
      EWBtp_[numBoundaryFaces_*0+faceId-1] = statefcoeff;
    }
	NWcloseFile();

	// cleanup
    debugprint(stddbg,"Exiting cmd_read_EWB.\n");
    return CV_OK;
}

int readRESTARTDAT(char* filename, int readSoln, int readDisp, int readAcc) {

  int intfromfile[50];
  int irstin;
  int ione=1, itwo=2, ithree=3,  iseven=7;
  int nshgl,numvar,lstep,iqsiz;

  openfile_(filename, "read", &irstin   );

  // read solution if desired
  if (readSoln) {
    readheader_(&irstin,"solution",(void*)intfromfile,&ithree,"double",oformat);
    nshgl=intfromfile[0];
    numvar=intfromfile[1];
    lstep=intfromfile[2];
    iqsiz=nshgl*numvar;
    soln_ = new double [iqsiz];
    readdatablock_(&irstin,"solution",(void*)soln_, &iqsiz, "double", oformat);
  }

  // read displacements if desired
  if (readDisp) {
    readheader_(&irstin,"displacement",(void*)intfromfile,&ithree,"double",oformat);
    nshgl=intfromfile[0];
    numvar=intfromfile[1];
    lstep=intfromfile[2];
    iqsiz=nshgl*numvar;
    dispsoln_ = new double [iqsiz];
    readdatablock_(&irstin,"displacement",(void*)dispsoln_, &iqsiz, "double", oformat);
  }

  // read accelerations if desired
  if (readAcc) {
    readheader_(&irstin,"time derivative of solution",(void*)intfromfile,&ithree,"double",oformat);
    nshgl=intfromfile[0];
    numvar=intfromfile[1];
    lstep=intfromfile[2];
    iqsiz=nshgl*numvar;
    acc_ = new double [iqsiz];
    readdatablock_(&irstin,"time derivative of solution",(void*)acc_, &iqsiz, "double", oformat);
  }

  closefile_( &irstin, "read" );

  return CV_OK;

}


int writeRESTARTDAT(char* filename) {

    // some simple validity checks
    if (numNodes_ == 0 || numSolnVars_ == 0) {
        fprintf(stderr,"ERROR:  Not all required info set!\n");
        return CV_ERROR;
    }

    int i;

    int filenum = -1;
    openfile_ (filename, "write", &filenum);
    if (filenum < 0) {
        fprintf(stderr,"ERROR:  could not open file (%s)\n",filename);
        return CV_ERROR;
    }

    // write out the commmon header
    writeCommonHeader(&filenum);

    char wrtstr[255];
    int iarray[10];
    int size, nitems;

    sprintf(wrtstr,"number of modes : < 0 > %d \n", numNodes_);
    writestring_( &filenum, wrtstr );

    sprintf(wrtstr,"number of variables : < 0 > %d \n", numSolnVars_);
    writestring_( &filenum, wrtstr );

    iarray[0] = numNodes_;
    iarray[1] = numSolnVars_;
    iarray[2] = 0;

    size = numNodes_*numSolnVars_;
    nitems = 3;
    writeheader_( &filenum, "solution",
                  (void*)iarray, &nitems, &size,"double", oformat );

    //
    //  zero all initial fields if solution has not 
    //  been read from file
    //

    if (soln_ == NULL) {

      soln_ = new double [size]();
      for (i = 0; i < numNodes_; i++) {
        soln_[i] = init_p_;
      }

      // use small non-zero initial velocity
      for (i = 0; i < numNodes_; i++) {
        soln_[1*numNodes_+i] = init_v_[0];
        soln_[2*numNodes_+i] = init_v_[1];
        soln_[3*numNodes_+i] = init_v_[2];
      }

    }

    writedatablock_( &filenum, "solution",
                       (void*)soln_, &size,
                         "double", oformat );


    int nsd, lstep, nshg;

    if (dispsoln_ != NULL) {
      nsd = 3;
      lstep = 0;
      nshg = numNodes_;
      size = nsd*nshg;
      nitems = 3;

      iarray[ 0 ] = nshg;
      iarray[ 1 ] = nsd;
      iarray[ 2 ] = lstep;
 
      writeheader_( &filenum, "displacement",
                  ( void* )iarray, &nitems, &size,"double", oformat );

      nitems = size;
      writedatablock_( &filenum, "displacement",
                     ( void* )(dispsoln_), &nitems, "double", oformat );
 
      delete dispsoln_;
    }

    delete[] soln_;


    closefile_ (&filenum,"write");

    return CV_OK;

}

int writeGEOMBCDAT(char* filename) {

    // some simple validity checks
    if (numNodes_ == 0 || numElements_ == 0 ||
        numSolnVars_ == 0 || numMeshEdges_ == 0 ||
        numMeshFaces_ == 0 || numBoundaryFaces_ == 0 ||
        nodes_ == NULL || elements_ == NULL ||
        xadjSize_ == 0 || adjncySize_ == 0 ||
        iBCB_ == NULL || BCB_ == NULL || iBC_ == NULL) {
        fprintf(stderr,"ERROR:  Not all required info set!\n");
        return CV_ERROR;
    }

    for (int i = numBoundaryFaces_; i < 2 * numBoundaryFaces_; ++i) {
        if (iBCB_[i] == 0) {
            fprintf(stderr, "ERROR:  iBCB fail\n", filename);
            return CV_ERROR;
        }
    }

    int i;

    int filenum = -1;
    openfile_ (filename, "write", &filenum);
    if (filenum < 0) {
        fprintf(stderr,"ERROR:  could not open file (%s)\n",filename);
        return CV_ERROR;
    }

    // write out the commmon header
    writeCommonHeader(&filenum);

    char wrtstr[255];
    int iarray[10];
    int size, nitems;

    sprintf(wrtstr,"number of processors : < 0 > 1 \n");
    writestring_( &filenum, wrtstr );
 
    sprintf(wrtstr,"number of variables : < 0 > %d \n", numSolnVars_);
    writestring_( &filenum, wrtstr );

    sprintf(wrtstr,"number of nodes : < 0 > %d \n", numNodes_);
    writestring_( &filenum, wrtstr );

    sprintf( wrtstr,"number of nodes in the mesh : < 0 > %d \n",numNodes_);
    writestring_( &filenum, wrtstr );

    sprintf( wrtstr,"number of edges in the mesh : < 0 > %d \n",numMeshEdges_);
    writestring_( &filenum, wrtstr );

    sprintf( wrtstr,"number of faces in the mesh : < 0 > %d \n",numMeshFaces_);
    writestring_( &filenum, wrtstr );

    sprintf(wrtstr,"number of modes : < 0 > %d \n", numNodes_);
    writestring_( &filenum, wrtstr );

    sprintf(wrtstr,"number of shapefunctions solved on processor : < 0 > %d \n", numNodes_);
    writestring_( &filenum, wrtstr );

    sprintf(wrtstr,"number of global modes : < 0 > %d \n", numNodes_);
    writestring_( &filenum, wrtstr );

    sprintf(wrtstr,"number of interior elements : < 0 > %d \n", numElements_);
    writestring_( &filenum, wrtstr );

    int numelb = numBoundaryFaces_;

    // count the number of non-zero entries for dirichlet 
    int numEBC = 0;
    for (i = 0; i < numNodes_; i++) {
        if (iBC_[i] != 0) numEBC++;
    }

    int numpbc = numEBC;

    int nen = 4;
    int tmpblk = 1;
    int tmpblkb = 1;
 
    sprintf(wrtstr,"number of boundary elements : < 0 > %d \n", numelb);
    writestring_( &filenum, wrtstr );

    sprintf(wrtstr,"maximum number of element nodes  : < 0 > %d \n", nen);
    writestring_( &filenum, wrtstr );

    sprintf(wrtstr,"number of interior tpblocks : < 0 > %d \n", tmpblk);
    writestring_( &filenum, wrtstr );

    sprintf(wrtstr,"number of boundary tpblocks : < 0 > %d \n", tmpblkb);
    writestring_( &filenum, wrtstr );

    sprintf(wrtstr,"number of nodes with Dirichlet BCs : < 0 > %d \n", numpbc);
    writestring_( &filenum, wrtstr );

    //
    //  write nodes
    //

    size = 3*numNodes_;
    nitems = 2 ;
    iarray[ 0 ] = numNodes_;
    iarray[ 1 ] = 3;

    writeheader_( &filenum, "co-ordinates ", (void*)iarray, &nitems, &size,
                  "double", oformat  );

    nitems = 3*numNodes_;
    writedatablock_( &filenum, "co-ordinates ", (void*)nodes_, &nitems,
                     "double", oformat );

    //
    // write global ndsurf
    //

    if (ndsurfg_ == NULL) {
        ndsurfg_ = new int[numNodes_]();
    }

    size = numNodes_;
    nitems = 2;
    iarray[0] = numNodes_;
    iarray[1] = 1;

    writeheader_( &filenum, "global node surface number ", (void*)iarray, &nitems, &size,
                  "integer", oformat  );

    nitems = numNodes_;
    writedatablock_( &filenum, "global node surface number ", (void*)ndsurfg_, &nitems,
                     "integer", oformat );

    // 
    // write elements
    //

    /* for interior each block */
    int bnen    = 4;                 /* nen of the block -- topology */
    int bpoly   = 1;                 /* polynomial order of the block */
    int nelblk  = numElements_;      /* numel of this block */
    int bnsh    = 4;                 /* nshape of this block */ 
    int bnshlb  = 3;
    int bnenbl  = 3;
    int blcsyst = 1;

    size = nelblk*bnsh;
    nitems = 7;
    iarray[ 0 ] = nelblk;
    iarray[ 1 ] = bnen;
    iarray[ 2 ] = bpoly;
    iarray[ 3 ] = bnsh;
    iarray[ 4 ] = bnshlb;
    iarray[ 5 ] = bnenbl;
    iarray[ 6 ] = blcsyst;
  
    writeheader_( &filenum, "connectivity interior linear tetrahedron ",
                  (void*)iarray, &nitems, &size,
                  "integer", oformat );

    nitems = nelblk*bnsh;
    writedatablock_( &filenum, "connectivity interior linear tetrahedron ", 
                   (void*)elements_, &nitems,
                   "integer", oformat );

    //  ien to sms
    //

    size = nelblk;
    nitems = 1;
    iarray[ 0 ] = nelblk;
    writeheader_( &filenum, "ien to sms linear tetrahedron ",
                  (void*)iarray, &nitems, &size,"integer", oformat );

    nitems = nelblk;
    int* ien_sms = new int [numElements_];
    for (i = 0; i < numElements_ ; i++) {
        ien_sms[i] = i;
    }
    writedatablock_( &filenum, "ien to sms linear tetrahedron ",
                     (void*)ien_sms, &nitems,
                         "integer", oformat );
    delete ien_sms;

    //  boundary elements
    //

    // ??
    int numNBC = 6;

    iarray[ 0 ] = numBoundaryFaces_;
    iarray[ 1 ] = bnen;
    iarray[ 2 ] = bpoly;
    iarray[ 3 ] = bnsh;
    iarray[ 4 ] = bnshlb;
    iarray[ 5 ] = bnenbl;
    iarray[ 6 ] = blcsyst;
    iarray[ 7 ] = numNBC;

    size = numBoundaryFaces_*bnsh;
    nitems = 8;
    writeheader_( &filenum, "connectivity boundary linear tetrahedron ",
                      (void*)iarray, &nitems, &size,
                      "integer", oformat );

    int* ienb = new int [4*numBoundaryFaces_];
    for (i = 0; i < numBoundaryFaces_; i++) {
        ienb[0*numBoundaryFaces_+i] = boundaryElements_[0][i];
        ienb[1*numBoundaryFaces_+i] = boundaryElements_[1][i];
        ienb[2*numBoundaryFaces_+i] = boundaryElements_[2][i];
        ienb[3*numBoundaryFaces_+i] = boundaryElements_[3][i];
    }

    nitems = numBoundaryFaces_*bnsh;
    writedatablock_( &filenum, "connectivity boundary linear tetrahedron ",
                      (void*)ienb, &nitems,"integer", oformat );

    delete ienb;

    //
    // ienb to sms linear tetrahedron
    //

    iarray [ 0 ] = numBoundaryFaces_;
    nitems = 1;
    size = numBoundaryFaces_ ;

    writeheader_( &filenum, "ienb to sms linear tetrahedron ",
                      (void*)iarray, &nitems, &size,
                      "integer", oformat );
    
    writedatablock_( &filenum, "ienb to sms linear tetrahedron ",
                      (void*)boundaryElementsIds_, &numBoundaryFaces_,"integer", oformat );
    

    // nbc codes linear tetrahedron  : < 5132 > 2566 4 1 4 3 3 1 6

    iarray[ 0 ] = numBoundaryFaces_;
    iarray[ 1 ] = bnen;
    iarray[ 2 ] = bpoly;
    iarray[ 3 ] = bnsh;
    iarray[ 4 ] = bnshlb;
    iarray[ 5 ] = bnenbl;
    iarray[ 6 ] = blcsyst;
    iarray[ 7 ] = numNBC;
    nitems  = 8;
    size = numBoundaryFaces_*2;

    writeheader_( &filenum, "nbc codes linear tetrahedron " , (void*)iarray, &nitems, &size,
                      "integer", oformat );

    // ???
    // ???
    // ???
    //nitems = numBoundaryFaces_*bnsh;  
    nitems  = numBoundaryFaces_*2;
 
    writedatablock_( &filenum, "nbc codes linear tetrahedron ", (void*)iBCB_, &nitems,
                         "integer", oformat );

    // nbc values linear tetrahedron  : < 15396 > 2566 4 1 4 3 3 1 6

    size = numBoundaryFaces_*6;
    nitems = 8;

    writeheader_( &filenum, "nbc values linear tetrahedron ", (void*)iarray, &nitems, &size,
                      "double", oformat );

    nitems = numBoundaryFaces_*6;
    writedatablock_( &filenum, "nbc values linear tetrahedron ", (void*)BCB_, &nitems,
                         "double", oformat );

    // bc mapping array  : < 3636 > 3636
    // bc codes array  : < 26 > 26

    int* iBC = new int [numEBC];
    int* iBCmap = new int[numNodes_];
    int count = 0;
    for (i = 0; i < numNodes_; i++) {
        iBCmap[i]=0;
        if (iBC_[i] != 0) {
            iBC[count] = iBC_[i];
            count++;
            iBCmap[i] = count; 
        }
    }

    iarray [ 0 ] = numNodes_;
    nitems = 1;
    size = numNodes_;

    writeheader_( &filenum, "bc mapping array ",
                      (void*)iarray, &nitems, &size,
                      "integer", oformat );
    
    writedatablock_( &filenum, "bc mapping array ",
                      (void*)iBCmap, &numNodes_,"integer", oformat );

    delete [] iBCmap;

    iarray [ 0 ] = numEBC;
    nitems = 1;
    size = numEBC ;

    writeheader_( &filenum, "bc codes array ",
                      (void*)iarray, &nitems, &size,
                      "integer", oformat );
    
    writedatablock_( &filenum, "bc codes array ",
                      (void*)iBC, &numEBC,"integer", oformat );

    delete iBC;

    // boundary condition array : < 312 > 312
    
    int numVars = 0;
    size = numEBC*(numVars+12);
    nitems = 1;
    iarray[ 0 ] = numEBC*(numVars+12);
    writeheader_( &filenum , "boundary condition array ", (void *)iarray, &nitems,
                  &size, "double", oformat );

    nitems = numEBC*(numVars+12);

    double *BCf = new double [nitems]();

    // this code creates only no-slip b.c.
    // for all nodes
    for (i = 0; i <numEBC; i++) {
        BCf[3*numEBC+i]=1.0;
    }

    writedatablock_( &filenum, "boundary condition array ", (void*)(BCf),
                     &nitems , "double", oformat );

    delete BCf;

    // SWBtp array

    int nProps, neltp;
    if (SWBtp_ != NULL) {
      if(numWallProps_ == 0) {        //a problem with constant material properties 
        nProps = 10;
	  }                              //is treated as an isotropic problem
      else if(numWallProps_ == 10) {
	    nProps = numWallProps_;
      }
      else{
	    nProps = numWallProps_;
      }
      neltp = numBoundaryFaces_;
      size = nProps*neltp;
      nitems = 2;

      iarray[ 0 ] = neltp;
      iarray[ 1 ] = nProps;
       
      writeheader_( &filenum, "SWB array",
                  ( void* )iarray, &nitems, &size,"double", oformat );

      nitems = size;
      writedatablock_( &filenum, "SWB array ",
                     ( void* )(SWBtp_), &nitems, "double", oformat );
 
      delete SWBtp_;
    }

	// TWB array
	if (TWBtp_ != NULL) {
	  nProps = 2;
      neltp = numBoundaryFaces_;
      size = nProps*neltp;

      iarray[ 0 ] = neltp;
      iarray[ 1 ] = nProps;
       
	  nitems = 2;
      writeheader_( &filenum, "TWB array",
                  ( void* )iarray, &nitems, &size,"double", oformat );

      nitems = size;
      writedatablock_( &filenum, "TWB array ",
                     ( void* )(TWBtp_), &nitems, "double", oformat );
 
      delete TWBtp_;
    }

	// EWB array
	if (EWBtp_ != NULL) {
	  nProps = 1;
      neltp = numBoundaryFaces_;
      size = nProps*neltp;

      iarray[ 0 ] = neltp;
      iarray[ 1 ] = nProps;
       
	  nitems = 2;
      writeheader_( &filenum, "EWB array",
                  ( void* )iarray, &nitems, &size, "double", oformat );

      nitems = size;
      writedatablock_( &filenum, "EWB array ",
                     ( void* )(EWBtp_), &nitems, "double", oformat );
 
      delete EWBtp_;
    }

    //
    // periodic masters array  : < 3636 > 3636 
    //

    iarray [ 0 ] = numNodes_;
    size = numNodes_;
    nitems = 1;
    writeheader_( &filenum, "periodic masters array ",
                      (void*)iarray, &nitems, &size,
                      "integer", oformat );
    int* periodic = new int [numNodes_]();

    nitems  = numNodes_;
    writedatablock_( &filenum, "periodic masters array ",
                      (void*)periodic, &nitems,"integer", oformat );

    delete periodic;

    //
    // keyword xadj  : < 17609 > 17608 0 
    // keyword adjncy  : < 67866 > 33933  
    // keyword vwgt  : < 17608 > 17608
    //

    nitems = 2;
    iarray[0] = numElements_;
    int sspebc = 0;
    iarray[1] = sspebc;
    writeheader_( &filenum, "keyword xadj ", (void*)iarray, &nitems,
                  &xadjSize_, "integer", oformat );

    writedatablock_( &filenum, "keyword xadj ", 
                     (void*)xadj_, &xadjSize_,"integer", oformat );

    nitems = 1;
    iarray[0] = numMeshFaces_ - numBoundaryFaces_;
    writeheader_( &filenum, "keyword adjncy ", (void*)iarray, &nitems,
                  &adjncySize_, "integer", oformat );

    writedatablock_( &filenum, "keyword adjncy ", (void*)adjncy_, &adjncySize_,
                     "integer", oformat );

    nitems = 1;
    iarray[0] = numElements_;
    writeheader_( &filenum, "keyword vwgt ", (void*)iarray, &nitems,
                  &numElements_, "integer", oformat );

    int* vwgt = new int [numElements_];
    for (i = 0; i < numElements_; i++) {
        vwgt[i] = 4;
    }

    writedatablock_( &filenum, "keyword vwgt ", (void*)vwgt, &numElements_,
                     "integer", oformat );

    delete vwgt;

    closefile_ (&filenum,"write");

    return CV_OK;

}

