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

/*------------------------------------------------------------*
 *                                                            *
 *                ****  command processor  ****               *
 *                                                            *
 *------------------------------------------------------------*/

#include "cmd.h"
 
FILE* stddbg;
int cmd_input = 1;
FILE *fp_input = stdin;

static Cmd cmd_table[] = {
  {"ascii_format", cmd_ascii_format},
  {"number_of_nodes", cmd_number_of_nodes},
  {"number_of_elements", cmd_number_of_elements},
  {"number_of_mesh_edges", cmd_number_of_mesh_edges},
  {"number_of_mesh_faces", cmd_number_of_mesh_faces},
  {"number_of_variables", cmd_number_of_solnvars},
  {"nodes",      cmd_nodes},
  {"elements",      cmd_elements},
  {"noslip", cmd_noslip},
  {"prescribed_velocities", cmd_prescribed_velocities},
  {"zero_pressure", cmd_zero_pressure},
  {"pressure", cmd_pressure},
  {"read_restart_solution", cmd_read_restart_solution},
  {"read_restart_displacements", cmd_read_restart_displacements},
  {"read_restart_accelerations", cmd_read_restart_accelerations},
  {"read_displacements",cmd_read_displacements},
  {"number_of_wall_Props", cmd_number_of_wall_Props},
  {"read_SWB_ORTHO",cmd_read_SWB_ORTHO},
  {"read_SWB_ISO",cmd_read_SWB_ISO},
  {"read_TWB",cmd_read_TWB},  // added by Nan 6/24/09
  {"read_EWB",cmd_read_EWB}, 
  {"write_displacements", cmd_write_displacements },
  {"write_pressures",cmd_write_pressures},
  {"write_restart", cmd_write_restartdat},
  {"write_geombc", cmd_write_geombcdat},
  {"boundary_faces", cmd_boundary_faces},
  {"adjacency", cmd_adjacency},
  {"set_surface_id", cmd_set_surface_id},
  {"deformable_wall", cmd_deformable_wall},
  {"verbose", cmd_verbose},
  {"phasta_node_order",cmd_phasta_node_order},
  {"initial_pressure",cmd_initial_pressure},
  {"initial_velocity",cmd_initial_velocity},
  {"fix_free_edge_nodes",cmd_fix_free_edge_nodes},
  {"deformable_create_mesh", cmd_create_mesh_deformable},
#ifdef WITH_DEFORMABLE
  {"deformable_write_vtk_mesh", cmd_deformable_write_vtk_mesh},
  {"deformable_write_feap", cmd_deformable_write_feap},
  {"deformable_direct_solve", cmd_deformable_direct_solve},
  {"deformable_solve", cmd_deformable_iterative_solve},
  {"deformable_Evw",cmd_deformable_Evw},
  {"deformable_nuvw",cmd_deformable_nuvw},
  {"deformable_thickness",cmd_deformable_thickness},
  {"deformable_pressure",cmd_deformable_pressure},
  {"deformable_kcons",cmd_deformable_kcons},
  {"append_displacements",cmd_append_displacements},
#endif // WITH_DEFORMABLE
  {NULL,       NULL}};

int debugprint(FILE* fp, const char *fmt, ...)
{
   if (fp == NULL) return CV_OK;
   if (!verbose_) return CV_OK;

   va_list argp;
   va_start(argp, fmt);
   vfprintf(stdout, fmt, argp);
   va_end(argp);
   fflush(stdout);
   return CV_OK;
}


/*------------------------------------------------------------*
 *                                                            *
 *             ****  cmd_proc  ****                           *
 *                                                            *
 * process a command.                                         *
 *------------------------------------------------------------*/

int cmd_proc (char *cmd, int *ok) {

  int i, j;

  int (*pt2Function)(char*);

  char *s;

  char name[MAXSTRINGLENGTH];

 /**************
  ***  body  ***
  **************/

  if (cmd[0] == '#' || cmd[0] == '\n') {
    return CV_OK;
    }

  sscanf(cmd, "%s", name);
 
  debugprint(stddbg,"command being processed is %s. \n",name);
  
  for (i = 0; cmd_table[i].name != NULL; i++) {
    if (!strcmp(name, cmd_table[i].name)) {
      pt2Function = cmd_table[i].pt2Function;
      return (*pt2Function)(cmd);
      //return CV_OK;
      }
    }
  
  for (i = 0; i < strlen(cmd); i++) {
    if ((cmd[i] != ' ') && (cmd[i] != '\n')) {
      fprintf(stderr, "\n  **** error: unknown command: %s", cmd);
      return CV_ERROR;
      }
    }

  return CV_OK;

}


/*------------------------------------------------------------* 
 *                                                            *  
 *              ****  cmd_token_get  ****                     *  
 *                                                            *  
 * get the next blank separated string.                       *  
 *------------------------------------------------------------*/

int cmd_token_get (int *p_n, char *string, char *token, int *end) {

  int i, j;

  int quote;

  int n;

  int len;

  static int c, state = 0;

 /**************
  ***  body  ***
  **************/

  len = strlen(string);
  n = *p_n;

  i = 0;
  quote = 0;
  token[0] = '\0';
  *end = 0;

  if (n >= len)
    return CV_ERROR;

  while ((c = string[n++]) != '\0') { 
    if ((c == ' ') || (c == '\t')) {
      if (i != 0) {
        token[i] = '\0';
	*p_n = n;
	return CV_OK;
	}
      }
    else if (c == '"') {
      while ((c = string[n++]) != '\0') {
        if (c == '"') {
          token[i] = '\0';
	  *p_n = n;
	  return CV_OK;
	  }
        else if (c == '\n') { 
          token[i] = '\0';
 	  *p_n = n;
          *end = 1;
	  return CV_OK;
	  }

        token[i++] = c;
        token[i] = '\0';
	}
      }
    else if (c == '\n') {
      *end = 1;

      if (i != 0) {
        token[i] = '\0';
	*p_n = n;
        return CV_OK;
	}
      else {
        token[i] = '\0';
	*p_n = n;
        return CV_OK;
	}
      }
    else {
      token[i++] = c;
      token[i] = '\0';
      }
    }

  *p_n = n;
  *end = 1;

  if (i == 0) 
    return CV_ERROR;

  return CV_OK;
}


/*------------------------------------------------------------*
 *                                                            *
 *                  ****  cmd_set_input  ****                 *
 *                                                            *
 *------------------------------------------------------------*/

int cmd_set_input (int p_cmd_input, FILE *p_fp) {

 /**************
  ***  body  ***
  **************/

  cmd_input = p_cmd_input;

  if (!p_cmd_input) {
    fp_input = p_fp;
    }
  else {
    fp_input = stdin;
    }

  return CV_OK;
  
}



