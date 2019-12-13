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

#ifndef CMD_H
#define CMD_H

#define MAXSTRINGLENGTH 1024
#define MAXCMDLINELENGTH 1024
#define MAXPATHLEN 1024
#define DATA_MAX_SIZE 100
#define CV_OK 1
#define CV_ERROR 0

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef WIN32
#define CALLTYPE __cdecl
#else
#define CALLTYPE
#endif

typedef struct Cmd {
  const char *name;
  int (*pt2Function)(char*);
} Cmd;

extern int cmd_input; 
extern FILE *fp_input;
extern "C" FILE *stddbg;
extern int verbose_;
extern int phasta_node_order_;

// parse script file
int cmd_proc (char *cmd, int *ok);
int cmd_token_get (int *p_n, char *string, char *token, int *end);
int cmd_set_input (int p_cmd_input, FILE *p_fp);

// general utility
extern "C" {
    int debugprint(FILE* , const char* , ...);
}

// actual commands
int CALLTYPE cmd_nodes(char*);
int CALLTYPE cmd_elements(char*);
int CALLTYPE cmd_noslip(char*);
int CALLTYPE cmd_prescribed_velocities(char*);
int CALLTYPE cmd_zero_pressure(char*);
int CALLTYPE cmd_pressure(char*);
int CALLTYPE cmd_write_restartdat(char*);
int CALLTYPE cmd_write_geombcdat(char*);
int CALLTYPE cmd_number_of_nodes(char*);
int CALLTYPE cmd_number_of_elements(char*);
int CALLTYPE cmd_number_of_mesh_edges(char*);
int CALLTYPE cmd_number_of_mesh_faces(char*);
int CALLTYPE cmd_number_of_solnvars(char*);
int CALLTYPE cmd_boundary_faces(char*);
int CALLTYPE cmd_adjacency(char*);
int CALLTYPE cmd_set_surface_id(char*);
int CALLTYPE cmd_deformable_wall(char*);
int CALLTYPE cmd_ascii_format(char*);
int CALLTYPE cmd_verbose(char*);
int CALLTYPE cmd_phasta_node_order(char*);
int CALLTYPE cmd_initial_pressure(char*);
int CALLTYPE cmd_initial_velocity(char*);
int CALLTYPE cmd_fix_free_edge_nodes(char*);
int CALLTYPE cmd_create_mesh_deformable(char*);
int CALLTYPE cmd_deformable_direct_solve(char*);
int CALLTYPE cmd_deformable_iterative_solve(char*);
int CALLTYPE cmd_deformable_write_vtk_mesh(char*);
int CALLTYPE cmd_deformable_write_feap(char*);
int CALLTYPE cmd_deformable_Evw(char*);
int CALLTYPE cmd_deformable_nuvw(char*);
int CALLTYPE cmd_deformable_thickness(char*);
int CALLTYPE cmd_deformable_pressure(char*);
int CALLTYPE cmd_deformable_kcons(char*);
int CALLTYPE cmd_append_displacements(char*);
int CALLTYPE cmd_read_restart_solution(char*);
int CALLTYPE cmd_read_restart_displacements(char*);
int CALLTYPE cmd_read_restart_accelerations(char*);
int CALLTYPE cmd_read_displacements(char*);
int CALLTYPE cmd_read_SWB_ORTHO(char*);
int CALLTYPE cmd_read_SWB_ISO(char*);
int CALLTYPE cmd_number_of_wall_Props(char*);
//int CALLTYPE cmd_scale_coordinates(char*);
//int CALLTYPE cmd_zero_scalar_flux(char*);
//int CALLTYPE cmd_scalar_flux(char*);
// added by Nan 6/24/09
int CALLTYPE cmd_read_TWB(char*);
int CALLTYPE cmd_read_EWB(char*);
int CALLTYPE cmd_write_displacements(char*);
int CALLTYPE cmd_write_pressures(char*);
int CALLTYPE cmd_write_wall_shear_stress(char*);

#endif // CMD_H
