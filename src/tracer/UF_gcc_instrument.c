/*****************************************************************************\
 *                        ANALYSIS PERFORMANCE TOOLS                         *
 *                                  MPItrace                                 *
 *              Instrumentation package for parallel applications            *
 *****************************************************************************
 *                                                             ___           *
 *   +---------+     http:// www.cepba.upc.edu/tools_i.htm    /  __          *
 *   |    o//o |     http:// www.bsc.es                      /  /  _____     *
 *   |   o//o  |                                            /  /  /     \    *
 *   |  o//o   |     E-mail: cepbatools@cepba.upc.edu      (  (  ( B S C )   *
 *   | o//o    |     Phone:          +34-93-401 71 78       \  \  \_____/    *
 *   +---------+     Fax:            +34-93-401 25 77        \  \__          *
 *    C E P B A                                               \___           *
 *                                                                           *
 * This software is subject to the terms of the CEPBA/BSC license agreement. *
 *      You must accept the terms of this license to use this software.      *
 *                                 ---------                                 *
 *                European Center for Parallelism of Barcelona               *
 *                      Barcelona Supercomputing Center                      *
\*****************************************************************************/

/* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- *\
 | @file: $HeadURL$
 | 
 | @last_commit: $Date$
 | @version:     $Revision$
 | 
 | History:
\* -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- */
#include "common.h"

static char UNUSED rcsid[] = "$Id$";

#include <config.h>

#ifdef HAVE_STDIO_H
# include <stdio.h>
#endif
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif

#include "wrapper.h"
#include "UF_gcc_instrument.h"

/* Configure the hash so it uses up to 1 Mbyte */
#if SIZEOF_VOIDP == 8
# define MAX_UFs_l2  (17)
#elif SIZEOF_VOIDP == 4
# define MAX_UFs_l2  (18)
#else
# error "Error! Unknown SIZEOF_VOIDP value!"
#endif

#define MAX_UFs      (1<<MAX_UFs_l2)
#define MAX_UFs_mask ((1<<MAX_UFs_l2)-1)
#define UF_lookahead (64)

static void *UF_addresses[MAX_UFs];
static unsigned int UF_collisions, UF_count, UF_distance;

#if SIZEOF_VOIDP == 8
# define HASH(address) ((address>>3) & MAX_UFs_mask)
#elif SIZEOF_VOIDP == 4
# define HASH(address) ((address>>2) & MAX_UFs_mask)
#endif

static unsigned int UFMaxDepth = (1 << 31);
static unsigned int UFDepth = 0;
static int UF_tracing_enabled = FALSE;
static int LookForUFaddress (void *address);

/***
  __cyg_profile_func_enter, __cyg_profile_func_exit
  these routines are callback functions to instrument routines which have
  been compiled with -finstrument-functions (GCC)
***/

void __cyg_profile_func_enter (void *this_fn, void *call_site)
{
	UNREFERENCED_PARAMETER (call_site);

	if (mpitrace_on && UF_tracing_enabled && UFDepth < UFMaxDepth)
	{
		if (LookForUFaddress (this_fn))
		{
			TRACE_EVENTANDCOUNTERS (TIME, USRFUNC_EV, (uintptr_t) this_fn, TRACING_HWC_UF);
		}
	}

	UFDepth++;
}

void __cyg_profile_func_exit (void *this_fn, void *call_site)
{
	UNREFERENCED_PARAMETER (call_site);

	UFDepth--;

	if (mpitrace_on && UF_tracing_enabled && UFDepth < UFMaxDepth)
	{
		if (LookForUFaddress (this_fn))
		{
			TRACE_EVENTANDCOUNTERS (TIME, USRFUNC_EV, EVT_END, TRACING_HWC_UF);
		}
	}
}

static void AddUFtoInstrument (void *address)
{
	int i = HASH((long)address);

	if (UF_addresses[i] == NULL)
	{
		UF_addresses[i] = address;
		UF_count++;
	}
	else
	{
		int count = 1;
		while (UF_addresses[(i+count)%MAX_UFs] != NULL && count < UF_lookahead)
			count++;

		if (UF_addresses[(i+count)%MAX_UFs] == NULL)
		{
			UF_addresses[(i+count)%MAX_UFs] = address;
			UF_collisions++;
			UF_count++;
			UF_distance += count;
		}
		else
			fprintf (stderr, "mpitrace: Cannot add UF %p\n", address);
	}

	UF_tracing_enabled = TRUE;
}

static int LookForUFaddress (void *address)
{
	int i = HASH((long)address);
	int count = 0;

	while (UF_addresses[(i+count)%MAX_UFs] != address && 
		UF_addresses[(i+count)%MAX_UFs] != NULL &&
		count < UF_lookahead)
	{
		count++;
	}

	return UF_addresses[(i+count)%MAX_UFs] == address;
}

static void ResetUFtoInstrument (void)
{
	int i;
	for (i = 0; i < MAX_UFs; i++)
		UF_addresses[i] = NULL;
	UF_distance = UF_count = UF_collisions = 0;
}

void InstrumentUFroutines (int rank, char *filename)
{
	FILE *f = fopen (filename, "r");
	if (f != NULL)
	{
		char buffer[1024], fname[1024];
		unsigned long address;

		ResetUFtoInstrument ();

		fgets (buffer, sizeof(buffer), f);
		while (!feof(f))
		{
			sscanf (buffer, "%lx # %s", &address, fname);
			AddUFtoInstrument ((void*) address);
			fgets (buffer, sizeof(buffer), f);
		}
		fclose (f);

		if (rank == 0)
			fprintf (stdout, "mpitrace: Number of user functions traced: %u (collisions: %u, avg distance = %u)\n",
      UF_count, UF_collisions, UF_distance/UF_collisions);
	}
	else
	{
		if (strlen(filename) > 0)
			fprintf (stderr, "mpitrace: Warning! Cannot open %s file\n", filename);
	}

	if (UF_count > 0)
		UF_tracing_enabled = TRUE;
}

void setUFMaxDepth (unsigned int depth)
{
	UFMaxDepth = depth;
}

