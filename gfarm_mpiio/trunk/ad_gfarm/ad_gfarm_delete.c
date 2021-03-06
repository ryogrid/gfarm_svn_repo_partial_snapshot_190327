/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *
 *   Copyright (C) 2001 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_gfarm.h"
#include "adioi.h"

void ADIOI_GFARM_Delete(char *filename, int *error_code)
{
    int myrank, nprocs;

    *error_code = MPI_SUCCESS;

    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
#ifdef GFARM_DEBUG
    FPRINTF(stdout, "[%d/%d] ADIOI_GFARM_Delete called on %s\n", 
	    myrank, nprocs, filename);
#endif
}
