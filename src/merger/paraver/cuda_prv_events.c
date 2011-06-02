/*****************************************************************************\
 *                        ANALYSIS PERFORMANCE TOOLS                         *
 *                                   Extrae                                  *
 *              Instrumentation package for parallel applications            *
 *****************************************************************************
 *     ___     This library is free software; you can redistribute it and/or *
 *    /  __         modify it under the terms of the GNU LGPL as published   *
 *   /  /  _____    by the Free Software Foundation; either version 2.1      *
 *  /  /  /     \   of the License, or (at your option) any later version.   *
 * (  (  ( B S C )                                                           *
 *  \  \  \_____/   This library is distributed in hope that it will be      *
 *   \  \__         useful but WITHOUT ANY WARRANTY; without even the        *
 *    \___          implied warranty of MERCHANTABILITY or FITNESS FOR A     *
 *                  PARTICULAR PURPOSE. See the GNU LGPL for more details.   *
 *                                                                           *
 * You should have received a copy of the GNU Lesser General Public License  *
 * along with this library; if not, write to the Free Software Foundation,   *
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA          *
 * The GNU LEsser General Public License is contained in the file COPYING.   *
 *                                 ---------                                 *
 *   Barcelona Supercomputing Center - Centro Nacional de Supercomputacion   *
\*****************************************************************************/

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- *\
 | @file: $HeadURL: https://svn.bsc.es/repos/ptools/extrae/trunk/src/merger/paraver/trt_prv_events.c $
 | @last_commit: $Date: 2010-10-26 14:58:30 +0200 (dt, 26 oct 2010) $
 | @version:     $Revision: 476 $
\* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#include "common.h"

static char UNUSED rcsid[] = "$Id: trt_prv_events.c 476 2010-10-26 12:58:30Z harald $";

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_BFD
# include "addr2info.h"
#endif

#include "events.h"
#include "trt_prv_events.h"
#include "mpi2out.h"
#include "options.h"

#define CUDALAUNCH_INDEX      0
#define CUDABARRIER_INDEX     1
#define CUDAMEMCPY_INDEX      2

#define MAX_CUDA_INDEX         3

static int inuse[MAX_CUDA_INDEX] = { FALSE, FALSE, FALSE };

void Enable_CUDA_Operation (int tipus)
{
	if (tipus == CUDALAUNCH_EV)
		inuse[CUDALAUNCH_INDEX] = TRUE;
	else if (tipus == CUDABARRIER_EV)
		inuse[CUDABARRIER_INDEX] = TRUE;
	else if (tipus == CUDAMEMCPY_EV)
		inuse[CUDAMEMCPY_INDEX] = TRUE;
}

#if defined(PARALLEL_MERGE)

#include <mpi.h>
#include "mpi-aux.h"

void Share_CUDA_Operations (void)
{
	int res, i, tmp[MAX_CUDA_INDEX];

	res = MPI_Reduce (inuse, tmp, MAX_CUDA_INDEX, MPI_INT, MPI_BOR, 0,
		MPI_COMM_WORLD);
	MPI_CHECK(res, MPI_Reduce, "While sharing CUDA enabled operations");

	for (i = 0; i < MAX_CUDA_INDEX; i++)
		inuse[i] = tmp[i];
}

#endif

void CUDAEvent_WriteEnabledOperations (FILE * fd)
{
	if (inuse[CUDALAUNCH_INDEX])
	{
		fprintf (fd, "EVENT_TYPE\n"
		             "%d   %d    cudaLaunch\n", 0, CUDALAUNCH_EV);
		fprintf (fd, "VALUES\n0 End\n1 Begin\n\n");
	}
	if (inuse[CUDABARRIER_INDEX])
	{
		fprintf (fd, "EVENT_TYPE\n"
		             "%d   %d    cudaThreadSynchronize\n", 0, CUDABARRIER_EV);
		fprintf (fd, "VALUES\n0 End\n1 Begin\n\n");
	}
	if (inuse[CUDAMEMCPY_INDEX])
	{
		fprintf (fd, "EVENT_TYPE\n"
		             "%d   %d    cudaMemcpy\n\n", 0, CUDAMEMCPY_EV);
	}
}
