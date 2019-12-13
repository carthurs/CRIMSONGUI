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
#include <fstream>
using namespace std;

// added std::vector
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <array>
#include <boost/config.hpp>



// =========
//   Cross
// =========
inline
void Cross(double ax, double ay, double az,
double bx, double by, double bz,
double *prodx, double *prody, double *prodz)
{
    (*prodx) = ay * bz - az * by;
    (*prody) = -(ax * bz - az * bx);
    (*prodz) = ax * by - ay * bx;
    return;
}


// =======
//   Dot
// =======

inline
double Dot(double ax, double ay, double az,
double bx, double by, double bz)
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
extern int   DisplacementNumElements_;
extern int*  DisplacementConn_[3];
extern int   DisplacementNumNodes_;
extern int*  DisplacementNodeMap_;
extern int* ndsurfg_;

int writeGEOMBCDAT(char* filename);

gzFile fp_ = NULL;
char buffer_[MAXCMDLINELENGTH];

int NWopenFile(char* filename) {

    if (fp_ != NULL) {
        fprintf(stderr, "ERROR: Apparently a file is already open! (opening %s)\n", filename);
        return CV_ERROR;
    }

    fp_ = NULL;
    fp_ = gzopen(filename, "rb");
    if (fp_ == NULL) {
        fprintf(stderr, "ERROR: could not open file (%s)\n", filename);
        return CV_ERROR;
    }

    return CV_OK;
}


int NWcloseFile() {
    if (fp_ == NULL) {
        fprintf(stderr, "Closing a file already closed!\n");
        return CV_OK;
    }
    gzclose(fp_);
    fp_ = NULL;
    return CV_OK;
}


int NWgetNextLine(int *eof) {
    for (int i = 0; i < MAXCMDLINELENGTH; i++) {
        buffer_[i] = '\0';
    }
#ifdef USE_ZLIB
    gzgets(fp_,buffer_,MAXCMDLINELENGTH);
#else
    fgets(buffer_, MAXCMDLINELENGTH, fp_);
#endif

    *eof = gzeof(fp_);

    return CV_OK;
}


int NWgetNextNonBlankLine(int *eof) {
    NWgetNextLine(eof);
    for (int i = 0; i < MAXCMDLINELENGTH; i++) {
        if (buffer_[i] == '\0') break;
        if (buffer_[i] == '\n') break;
        if (buffer_[i] != ' ') return CV_OK;
    }
    return CV_ERROR;
}


int parseCmdStr(char *cmd, char *mystr) {
    // parse command string
    int n = 0;
    int end = 0;
    char ignored[MAXSTRINGLENGTH];
    ignored[0] = '\0';
    cmd_token_get(&n, cmd, ignored, &end);
    mystr[0] = '\0';
    cmd_token_get(&n, cmd, mystr, &end);
    return CV_OK;
}


int parseNum(char *cmd, int *num) {

    // parse command string
    char mystr[MAXSTRINGLENGTH];
    parseCmdStr(cmd, mystr);

    // do work
    *num = 0;
    if (sscanf(mystr, "%i", num) != 1) {
        fprintf(stderr, "error parsing num!\n");
        return CV_ERROR;
    }

    return CV_OK;

}


int parseNum2(char *cmd, int *num) {

    // parse command string
    int n = 0;
    int end = 0;
    char ignored[MAXSTRINGLENGTH];
    ignored[0] = '\0';
    cmd_token_get(&n, cmd, ignored, &end);
    char infile[MAXPATHLEN];
    infile[0] = '\0';
    cmd_token_get(&n, cmd, infile, &end);
    char surfidstr[MAXSTRINGLENGTH];
    surfidstr[0] = '\0';
    cmd_token_get(&n, cmd, surfidstr, &end);

    // do work

    *num = 0;

    int surfID = 0;
    if (sscanf(surfidstr, "%i", &surfID) != 1) {
        fprintf(stderr, "error parsing num!\n");
        return CV_ERROR;
    }

    *num = surfID;

    return CV_OK;

}


int parseDouble(char *cmd, double *num) {

    // parse command string
    char mystr[MAXSTRINGLENGTH];
    parseCmdStr(cmd, mystr);

    // do work
    *num = 0.0;
    if (sscanf(mystr, "%lf", num) != 1) {
        fprintf(stderr, "error parsing double!\n");
        return CV_ERROR;
    }

    return CV_OK;

}


int parseDouble2(char *cmd, double *num) {

    // parse command string

    int n = 0;
    int end = 0;
    char ignored[MAXSTRINGLENGTH];
    ignored[0] = '\0';
    cmd_token_get(&n, cmd, ignored, &end);
    ignored[0] = '\0';
    cmd_token_get(&n, cmd, ignored, &end);

    char mystr[MAXSTRINGLENGTH];
    mystr[0] = '\0';
    cmd_token_get(&n, cmd, mystr, &end);


    // do work
    *num = 0.0;
    if (sscanf(mystr, "%lf", num) != 1) {
        fprintf(stderr, "error parsing double!\n");
        return CV_ERROR;
    }

    return CV_OK;

}


int parseDouble3(char *cmd, double *v1, double *v2, double *v3) {

    // parse command string
    int n = 0;
    int end = 0;
    char ignored[MAXSTRINGLENGTH];
    ignored[0] = '\0';
    cmd_token_get(&n, cmd, ignored, &end);
    char str1[MAXPATHLEN];
    str1[0] = '\0';
    cmd_token_get(&n, cmd, str1, &end);
    char str2[MAXSTRINGLENGTH];
    str2[0] = '\0';
    cmd_token_get(&n, cmd, str2, &end);
    char str3[MAXSTRINGLENGTH];
    str3[0] = '\0';
    cmd_token_get(&n, cmd, str3, &end);

    // do work
    *v1 = 0.0;
    if (sscanf(str1, "%lf", v1) != 1) {
        fprintf(stderr, "error parsing double!\n");
        return CV_ERROR;
    }
    *v2 = 0.0;
    if (sscanf(str2, "%lf", v2) != 1) {
        fprintf(stderr, "error parsing double!\n");
        return CV_ERROR;
    }
    *v3 = 0.0;
    if (sscanf(str3, "%lf", v3) != 1) {
        fprintf(stderr, "error parsing double!\n");
        return CV_ERROR;
    }

    return CV_OK;

}


int parseFile(char *cmd) {

    // parse command string
    char infile[MAXPATHLEN];
    parseCmdStr(cmd, infile);

    // do work
    return NWopenFile(infile);

}

int setNodesWithCode(char *cmd, int val) {

    // enter
    debugprint(stddbg, "Entering setNodesWithCode.\n");

    // do work
    if (numNodes_ == 0) {
        fprintf(stderr, "ERROR:  Must specify number of nodes before you read them in!\n");
        return CV_ERROR;
    }

    if (parseFile(cmd) == CV_ERROR) {
        return CV_ERROR;
    }

    if (iBC_ == NULL) {
        iBC_ = new int[numNodes_]();
    }

    int eof = 0;
    int nodeId = 0;

    while (NWgetNextNonBlankLine(&eof) == CV_OK) {
        if (sscanf(buffer_, "%i", &nodeId) != 1) {
            fprintf(stderr, "WARNING:  line not of correct format (%s)\n", buffer_);
        }
        else {
            // this should be a bit set instead of an int!!
            iBC_[nodeId - 1] = val;
        }
    }
    if (eof == 0) return CV_ERROR;
    NWcloseFile();

    // cleanup
    debugprint(stddbg, "Exiting setNodesWithCode.\n");
    return CV_OK;

}


int check_node_order(int n0, int n1, int n2, int n3, int elementId,
    int *k0, int *k1, int *k2, int *k3) {

    int i, j0, j1, j2, j3;

    if (n3 >= 0) {

        if (phasta_node_order_ == 0) {
            // Not necessary if using phasta-conn
            // flip the middle to vertexes because we used a value of 1
            // for node ordering in meshsim and scorec used the value of 0
            j0 = n0;
            j1 = n2;
            j2 = n1;
            j3 = n3;
        }
        else {
            j0 = n0;
            j1 = n1;
            j2 = n2;
            j3 = n3;
        }

    }
    else {

        j0 = n0;
        j1 = n1;
        j2 = n2;
        j3 = -1;

        for (i = 0; i < 4; i++) {
            if (elements_[i*numElements_ + (elementId - 1)] != j0 &&
                elements_[i*numElements_ + (elementId - 1)] != j1 &&
                elements_[i*numElements_ + (elementId - 1)] != j2) {
                j3 = elements_[i*numElements_ + (elementId - 1)];
                break;
            }
        }
        if (j3 < 0) {
            //gzclose(fp);
            fprintf(stderr, "ERROR:  could not find nodes in element (%i %i %i %i)\n",
                elementId, n0, n1, n2);
            return CV_ERROR;
        }

    }

    double a[3];
    double b[3];
    double c[3];
    double norm0, norm1, norm2;

    a[0] = nodes_[0 * numNodes_ + j1 - 1] - nodes_[0 * numNodes_ + j0 - 1];
    a[1] = nodes_[1 * numNodes_ + j1 - 1] - nodes_[1 * numNodes_ + j0 - 1];
    a[2] = nodes_[2 * numNodes_ + j1 - 1] - nodes_[2 * numNodes_ + j0 - 1];
    b[0] = nodes_[0 * numNodes_ + j2 - 1] - nodes_[0 * numNodes_ + j0 - 1];
    b[1] = nodes_[1 * numNodes_ + j2 - 1] - nodes_[1 * numNodes_ + j0 - 1];
    b[2] = nodes_[2 * numNodes_ + j2 - 1] - nodes_[2 * numNodes_ + j0 - 1];
    c[0] = nodes_[0 * numNodes_ + j3 - 1] - nodes_[0 * numNodes_ + j0 - 1];
    c[1] = nodes_[1 * numNodes_ + j3 - 1] - nodes_[1 * numNodes_ + j0 - 1];
    c[2] = nodes_[2 * numNodes_ + j3 - 1] - nodes_[2 * numNodes_ + j0 - 1];

    Cross(a[0], a[1], a[2], b[0], b[1], b[2], &norm0, &norm1, &norm2);
    double mydot = Dot(norm0, norm1, norm2, c[0], c[1], c[2]);

    if (mydot > 0) {
        int tmpj = j0;
        j0 = j2;
        j2 = j1;
        j1 = tmpj;
        debugprint(stdout, "elementId %i : %i %i %i %i   (flipped) %lf\n", elementId, j0, j1, j2, j3, mydot);
    }
    else {
        //debugprint(stdout,"elementId %i : %i %i %i %i  %lf\n",elementId,j0,j1,j2,j3,mydot);
    }

    *k0 = j0; *k1 = j1; *k2 = j2; *k3 = j3;

    return CV_OK;
}

int setBoundaryFacesWithCode(char* cmd, int setSurfID, int surfID, int setCode, int code, double value)
{

    // enter
    debugprint(stddbg, "Entering setBoundaryFacesWithCode.\n");

    int i;

    // parse command string
    if (parseFile(cmd) == CV_ERROR) {
        return CV_ERROR;
    }

    if (iBCB_ == NULL) {
        iBCB_ = new int[2 * numBoundaryFaces_]();
        BCB_ = new double[numBoundaryFaces_ * 6]();
    }

    if (ndsurfg_ == NULL) {
        ndsurfg_ = new int[numNodes_]();
    }

    int elementId, matId;

    int eof = 0;

    vector<vector<int>> elem_edge; // vector of element edges - stored as node pairs [n1,n2] *** added by KDL 07/02/14
    vector<vector<int>> surf_data; // vector of surface elements -stored as  [ids,n1,n2,n3]  *** added by KDL 07/02/14

    while (NWgetNextNonBlankLine(&eof) == CV_OK) {

        int n0, n1, n2;
        if (sscanf(buffer_, "%i %i %i %i %i", &elementId, &matId, &n0, &n1, &n2) != 5) {
            fprintf(stderr, "WARNING:  line not of correct format (%s)\n", buffer_);
            NWcloseFile();
            return CV_ERROR;
        }

        /*
         * store node pairings for each element *** added by KDL 07/02/14
         */
        /*
         if (setSurfID) {

         // arrange each edge's node pair - lowest index first
         vector <int> node_pair;

         // check and add n0 & n1
         if (n0 < n1) {
         node_pair.push_back(int(n0));
         node_pair.push_back(int(n1));
         } else {
         node_pair.push_back(int(n1));
         node_pair.push_back(int(n0));
         }

         elem_edge.push_back(node_pair);
         node_pair.clear();

         // check and add n1 & n2
         if (n1 < n2) {
         node_pair.push_back(int(n1));
         node_pair.push_back(int(n2));
         } else {
         node_pair.push_back(int(n2));
         node_pair.push_back(int(n1));
         }

         elem_edge.push_back(node_pair);
         node_pair.clear();

         // check and add n2 & n0
         if (n0 < n2) {
         node_pair.push_back(int(n0));
         node_pair.push_back(int(n2));
         } else {
         node_pair.push_back(int(n2));
         node_pair.push_back(int(n0));
         }

         elem_edge.push_back(node_pair);
         node_pair.clear();
         }
         */

        int j0 = n0;
        int j1 = n1;
        int j2 = n2;
        int j3 = -1;

        for (i = 0; i < 4; i++) {
            if (elements_[i * numElements_ + (elementId - 1)] != j0 && elements_[i * numElements_ + (elementId - 1)] != j1 &&
                elements_[i * numElements_ + (elementId - 1)] != j2) {
                j3 = elements_[i * numElements_ + (elementId - 1)];
                break;
            }
        }
        if (j3 < 0) {
            NWcloseFile();
            fprintf(stderr, "ERROR:  could not find nodes in element (%i %i %i %i)\n", elementId, n0, n1, n2);
            return CV_ERROR;
        }

        double a[3];
        double b[3];
        double c[3];
        double norm0, norm1, norm2;

        a[0] = nodes_[0 * numNodes_ + j1 - 1] - nodes_[0 * numNodes_ + j0 - 1];
        a[1] = nodes_[1 * numNodes_ + j1 - 1] - nodes_[1 * numNodes_ + j0 - 1];
        a[2] = nodes_[2 * numNodes_ + j1 - 1] - nodes_[2 * numNodes_ + j0 - 1];
        b[0] = nodes_[0 * numNodes_ + j2 - 1] - nodes_[0 * numNodes_ + j0 - 1];
        b[1] = nodes_[1 * numNodes_ + j2 - 1] - nodes_[1 * numNodes_ + j0 - 1];
        b[2] = nodes_[2 * numNodes_ + j2 - 1] - nodes_[2 * numNodes_ + j0 - 1];
        c[0] = nodes_[0 * numNodes_ + j3 - 1] - nodes_[0 * numNodes_ + j0 - 1];
        c[1] = nodes_[1 * numNodes_ + j3 - 1] - nodes_[1 * numNodes_ + j0 - 1];
        c[2] = nodes_[2 * numNodes_ + j3 - 1] - nodes_[2 * numNodes_ + j0 - 1];

        Cross(a[0], a[1], a[2], b[0], b[1], b[2], &norm0, &norm1, &norm2);
        double mydot = Dot(norm0, norm1, norm2, c[0], c[1], c[2]);

        if (mydot > 0) {
            std::swap(j0, j1);
            /*
            int tmpj = j0;
            j0 = j2;
            j2 = j1;
            j1 = tmpj;
            */
            fprintf(stdout, "elementId %i : %i %i %i %i   (flipped0) %lf\n", elementId, j0, j1, j2, j3, mydot);
        }
        else {
            // fprintf(stdout,"elementId %i : %i %i %i %i  %lf\n",elementId,j0,j1,j2,j3,mydot);
        }

        // find matching element already read in
        auto iter = boundaryElementIdToIndicesMap_.find(elementId - 1);

        if (iter == boundaryElementIdToIndicesMap_.end()) {
            fprintf(stderr, "ERROR: could not find pressure face in boundary faces!\n");
            return CV_ERROR;
        }

        for (int i : iter->second) {
            std::array<int, 4> elementNodeIds{
                { boundaryElements_[0][i], boundaryElements_[1][i], boundaryElements_[2][i], boundaryElements_[3][i] } };

            if (std::find(elementNodeIds.begin(), elementNodeIds.end(), j0) != elementNodeIds.end() &&
                std::find(elementNodeIds.begin(), elementNodeIds.end(), j1) != elementNodeIds.end() &&
                std::find(elementNodeIds.begin(), elementNodeIds.end(), j2) != elementNodeIds.end() && elementNodeIds[3] == j3) {
                if (setCode) {
                    iBCB_[i] = code;
                    BCB_[1 * numBoundaryFaces_ + i] = value;
                }
                if (setSurfID) {
                    iBCB_[numBoundaryFaces_ + i] = surfID;
                    //                    // add surf ID to global ndsurfl
                    //                    ndsurfg_[j0-1] = surfID;
                    //                    ndsurfg_[j1-1] = surfID;
                    //                    ndsurfg_[j2-1] = surfID;

                    // first time we hit a node it is 0
                    // if it then set to 1 it is left alone

                    if (ndsurfg_[j0 - 1] != 1) {
                        ndsurfg_[j0 - 1] = surfID;
                        // fprintf(stdout,"surfID %i\n",surfID);
                    }

                    if (ndsurfg_[j1 - 1] != 1) {
                        ndsurfg_[j1 - 1] = surfID;
                    }

                    if (ndsurfg_[j2 - 1] != 1) {
                        ndsurfg_[j2 - 1] = surfID;
                    }

                    if (surfID != 1) {

                        if (ndsurfg_[j0 - 1] != 0) {
                            ndsurfg_[j0 - 1] = surfID;
                            // fprintf(stdout,"surfID %i\n",surfID);
                        }
                        if (ndsurfg_[j1 - 1] != 0) {
                            ndsurfg_[j1 - 1] = surfID;
                        }

                        if (ndsurfg_[j2 - 1] != 0) {
                            ndsurfg_[j2 - 1] = surfID;
                        }
                    }

                    vector<int> elem_data;
                    elem_data.push_back(elementId);
                    elem_data.push_back(boundaryElements_[0][i]);
                    elem_data.push_back(boundaryElements_[1][i]);
                    elem_data.push_back(boundaryElements_[2][i]);
                    surf_data.push_back(elem_data);
                }
                break;
            }
        }
    }

    NWcloseFile();

    vector<vector<double>> edge_coords; // vector of edges node coordinates
    vector<vector<double>> surf_coords; // vector of surface node coordinates

    /*
     * Calculate radius to each node
     * Added by KDL 07/02/14
     */
    /*
    if (setSurfID) {

    int count[elem_edge.size()];
    vector<double> xyz;
    double x,y,z;

    // count frequency
    for (int j=0; j<elem_edge.size(); j++) {
    count[j] = 0;
    for (int k=0; k<elem_edge.size(); k++) {
    if (elem_edge.at(j).at(0) == elem_edge.at(k).at(0) &&
    elem_edge.at(j).at(1) == elem_edge.at(k).at(1) ) {
    count[j] += 1;
    }
    }
    }

    // if count == 1 it is unique
    for (int j=0; j<elem_edge.size(); j++) {
    if (count[j] == 1) {

    x = nodes_[0*numNodes_ + elem_edge.at(j).at(0) - 1 ];
    y = nodes_[1*numNodes_ + elem_edge.at(j).at(0) - 1 ];
    z = nodes_[2*numNodes_ + elem_edge.at(j).at(0) - 1 ];
    xyz.push_back(x);
    xyz.push_back(y);
    xyz.push_back(z);
    edge_coords.push_back(xyz);
    xyz.clear();

    x = nodes_[0*numNodes_ + elem_edge.at(j).at(1) - 1 ];
    y = nodes_[1*numNodes_ + elem_edge.at(j).at(1) - 1 ];
    z = nodes_[2*numNodes_ + elem_edge.at(j).at(1) - 1 ];
    xyz.push_back(x);
    xyz.push_back(y);
    xyz.push_back(z);
    edge_coords.push_back(xyz);
    xyz.clear();

    }
    }

    for (int j=0; j<surf_data.size(); j++) {
    int id,n[3];

    id = surf_data[j].at(0);
    n[0] = surf_data[j].at(1);
    n[1] = surf_data[j].at(2);
    n[2] = surf_data[j].at(3);

    printf("%8i %8i %8i %8i\n",id, n[0], n[1], n[2]);

    for (int l=0; l<3; l++) {
    double x = nodes_[0*numNodes_ + n[l] - 1 ];
    double y = nodes_[1*numNodes_ + n[l] - 1 ];
    double z = nodes_[2*numNodes_ + n[l] - 1 ];
    printf("%12.3e %12.3e %12.3e\n",x,y,z);
    }

    }




    }
    */

    // cleanup
    debugprint(stddbg, "Exiting setBoundaryFacesWithCode.\n");
    return CV_OK;
}

int fixFreeEdgeNodes(char *cmd) {

    // enter
    debugprint(stddbg, "Entering fixFreeEdgeNodes.\n");

    // parse command string
    if (parseFile(cmd) == CV_ERROR) {
        return CV_ERROR;
    }

    if (iBC_ == NULL) {
        iBC_ = new int[numNodes_]();
    }

    // count lines in file
    int eof = 0;
    int numLinesInFile = 0;
    while (NWgetNextNonBlankLine(&eof) == CV_OK) {
        numLinesInFile++;
    }
    NWcloseFile();

    debugprint(stddbg, "Number of lines in file: %i\n", numLinesInFile);

    if (numLinesInFile == 0) {
        return CV_ERROR;
    }

    struct EdgeHasher {
        std::size_t operator()(const std::array<int, 2>& e) const
        {
            return std::hash<int>{}(e[0]) * 31 + std::hash<int>{}(e[1]);
        }
    };

    std::unordered_set<std::array<int, 2>, EdgeHasher> edges;

    if (parseFile(cmd) == CV_ERROR) {
        return CV_ERROR;
    }


    eof = 0;

    while (NWgetNextNonBlankLine(&eof) == CV_OK) {

        int n0, n1, n2;
        int elementId, matId;
        if (sscanf(buffer_, "%i %i %i %i %i", &elementId, &matId, &n0, &n1, &n2) != 5) {
            fprintf(stderr, "WARNING:  line not of correct format (%s)\n", buffer_);
            NWcloseFile();
            return CV_ERROR;
        }

        auto add_or_remove_edge = [&edges](int n0, int n1)
        {
            #ifdef BOOST_NO_CXX11_UNIFIED_INITIALIZATION_SYNTAX
            std::array<int, 2> toInsert{{ std::min(n0, n1), std::max(n0, n1) }};
            auto iterBoolPair = edges.insert(toInsert);
            #else
            auto iterBoolPair = edges.insert({ std::min(n0, n1), std::max(n0, n1) });
            #endif
            if (!iterBoolPair.second) {
                // Remove duplicate if already present in set
                edges.erase(iterBoolPair.first);
            }
        };

        add_or_remove_edge(n0, n1);
        add_or_remove_edge(n1, n2);
        add_or_remove_edge(n0, n2);
    }

    NWcloseFile();

    for (const auto& edge : edges) {
        debugprint(stddbg, "  Fixing Node: %i\n", edge[0]);
        debugprint(stddbg, "  Fixing Node: %i\n", edge[1]);
        // no slip code
        // this should be a bit set instead of an int!!
        iBC_[edge[0] - 1] = 56;
        iBC_[edge[1] - 1] = 56;
    }

    // cleanup

    debugprint(stddbg, "Exiting fixFreeEdgeNodes.\n");
    return CV_OK;
}


int createMeshForDispCalc(char *cmd) {

    // enter
    debugprint(stddbg, "Entering createMeshForDispCalc.\n");

    // parse command string
    if (parseFile(cmd) == CV_ERROR) {
        return CV_ERROR;
    }

    // count lines in file
    int eof = 0;
    int numLinesInFile = 0;
    while (NWgetNextNonBlankLine(&eof) == CV_OK) {
        numLinesInFile++;
    }
    NWcloseFile();

    debugprint(stddbg, "Number of lines in file: %i\n", numLinesInFile);

    if (numLinesInFile == 0) {
        return CV_ERROR;
    }

    std::vector<std::array<int, 3>> ids;
    ids.reserve(numLinesInFile);

    if (parseFile(cmd) == CV_ERROR) {
        return CV_ERROR;
    }


    eof = 0;
    int numElements = 0;
    while (NWgetNextNonBlankLine(&eof) == CV_OK) {
        int n0, n1, n2;
        int elementId, matId;

        if (sscanf(buffer_, "%i %i %i %i %i", &elementId, &matId, &n0, &n1, &n2) != 5) {
            fprintf(stderr, "WARNING:  line not of correct format (%s)\n", buffer_);
            NWcloseFile();
            return CV_ERROR;
        }

        #ifdef BOOST_NO_CXX11_UNIFIED_INITIALIZATION_SYNTAX
        {
            std::array<int, 3> toPushBack{ { n0, numElements, 0 } };
            ids.push_back(toPushBack);
        }
        {
            std::array<int, 3> toPushBack{ { n1, numElements, 1 } };
            ids.push_back(toPushBack);
        }
        {
            std::array<int, 3> toPushBack{ { n2, numElements, 2 } };
            ids.push_back(toPushBack);
        }
        #else
        ids.push_back({ { n0, numElements, 0 } });
        ids.push_back({ { n1, numElements, 1 } });
        ids.push_back({ { n2, numElements, 2 } });
        #endif

        numElements++;
    }

    NWcloseFile();

    // sort node ids so we can find duplicates
    std::sort(ids.begin(), ids.end());

    // count the number of unique nodes
    int numUniqueNodes = 1;
    int index0 = ids[0][0];
    for (int i = 0; i < ids.size(); i++) {
        if (ids[i][0] != index0) {
            numUniqueNodes++;
            index0 = ids[i][0];
        }
    }
    debugprint(stddbg, "  Number of Unique Nodes Found: %i\n", numUniqueNodes);

    // create renumbered connectivity for initial disp. calc.

    int* conn[3];
    conn[0] = new int[numElements];
    conn[1] = new int[numElements];
    conn[2] = new int[numElements];

    int* map = new int[numUniqueNodes];

    index0 = -1;
    int j = 0;

    ofstream mapping("mapping.dat");
    mapping << "i\t\tmap[i]" << endl;
    for (int i = 0; i < ids.size(); i++) {
        if (ids[i][0] != index0) {
            map[j] = ids[i][0];
            //debugprint(stddbg,"  map[%i] %i\n",j,map[j]);
            mapping << j + 1 << "\t\t" << map[j] << endl;
            index0 = ids[i][0];
            j++;
        }
        conn[ids[i][2]][ids[i][1]] = j - 1;
    }
    mapping.close();
    debugprint(stddbg, "  Number of Unique Nodes Found: %i\n", j);

    // set global variables
    DisplacementNumElements_ = numElements;
    DisplacementConn_[0] = conn[0];
    DisplacementConn_[1] = conn[1];
    DisplacementConn_[2] = conn[2];
    DisplacementNumNodes_ = numUniqueNodes;
    DisplacementNodeMap_ = map;

    debugprint(stddbg, "Exiting createMeshForDispCalc.\n");
    return CV_OK;
    return 0;
}

