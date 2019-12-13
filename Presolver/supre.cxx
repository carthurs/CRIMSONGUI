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

#include <stdlib.h>
#include <stdio.h>
#ifndef WIN32
#include "sys/param.h"
#else
#define MAXPATHLEN 1024
#endif
#include <string.h>
#include <unordered_map>
#include <vector>

#include "cmd.h"

// globals
const char* oformat = "binary";
// const char* oformat = "ascii";
int verbose_ = 0;
int phasta_node_order_ = 0;

int numNodes_ = 0;
int numElements_ = 0;
int numMeshEdges_ = 0;
int numMeshFaces_ = 0;
int numSolnVars_ = 0;
int numBoundaryFaces_ = 0;
double* nodes_ = NULL;
int* elements_ = NULL;
int** boundaryElements_ = NULL;
int* boundaryElementsIds_ = NULL;
std::unordered_map<int, std::vector<int>> boundaryElementIdToIndicesMap_;
int* xadj_ = NULL;
int xadjSize_ = 0;
int* adjncy_ = NULL;
int adjncySize_ = 0;
int* iBC_ = NULL;
int* iBCB_ = NULL;
int numWallProps_ = 0;
double* BCB_ = NULL;
double init_p_ = 0.0;
double init_v_[3];
double* soln_ = NULL;
double* dispsoln_ = NULL;
double* SWBtp_ = NULL;
double* TWBtp_ = NULL;
double* EWBtp_ = NULL;
double* acc_ = NULL;

int* ndsurfg_ = NULL;

int DisplacementNumElements_ = 0;
int* DisplacementConn_[3];
int DisplacementNumNodes_ = 0;
int* DisplacementNodeMap_ = NULL;
double* DisplacementSolution_ = NULL;
double Displacement_Evw_ = 0;
double Displacement_nuvw_ = 0;
double Displacement_thickness_ = 0;
double Displacement_kcons_ = 0;
double Displacement_pressure_ = 0;

int main(int argc, char* argv[])
{

    // default initial velocity
    init_v_[0] = 0.0001;
    init_v_[1] = 0.0001;
    init_v_[2] = 0.0001;
    // default initial pressure
    init_p_ = 0.0;

    char logname[MAXPATHLEN];
    char mname[MAXPATHLEN];
    char debug_file[MAXPATHLEN];
    char s[MAXCMDLINELENGTH];
    int stat = 0;

    stddbg = stdin;
    // stddbg = NULL;

    logname[0] = '\0';
    mname[0] = '\0';
    debug_file[0] = '\0';

    if (argc != 2) {
        fprintf(stdout, "usage: supre <params_file>\n");
        exit(-1);
    }

    // check to make sure file exists!
    debugprint(stddbg, "attempt to open [%s]\n", argv[1]);
    FILE* fp = fopen(argv[1], "r");
    if (fp == NULL) {
        fprintf(stderr, "ERROR opening file %s.\n", argv[1]);
        exit(-1);
    }

    // set the input to the cmd file
    cmd_set_input(0, fp);

    int cmd_number = 0;
    s[0] = '\0';

    while (fgets(s, MAXCMDLINELENGTH, fp) != (char*)0) {

        fprintf(stdout, "LINE %.4i: %.60s", cmd_number, s);
        if (cmd_proc(s, &stat) == CV_ERROR) {
            fprintf(stderr, "ERROR:  command could not be processed.\n");
            fclose(fp);
            exit(-1);
        }
        cmd_number++;

        s[0] = '\0';
    }

    fclose(fp);

    return 0;
}
