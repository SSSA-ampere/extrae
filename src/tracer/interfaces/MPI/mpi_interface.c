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

#ifdef HAVE_STDIO_H
# include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#include <mpi.h>
#include "wrapper.h"
#include "mpi_wrapper.h"
#include "trace_hwc.h"

#if defined(ENABLE_LOAD_BALANCING)
# include "LB_MPI/MPI_interface.h"
#endif

#if defined(C_SYMBOLS) && defined(FORTRAN_SYMBOLS)
# define COMBINED_SYMBOLS
#endif

#define ENTER	TRUE
#define LEAVE	FALSE

#if defined(DEBUG_MPITRACE)
#	define DEBUG_INTERFACE(enter) \
	{ fprintf (stderr, "Task %d %s %s\n", TASKID, (enter)?"enters":"leaves", __func__); }
#else
#	define DEBUG_INTERFACE(enter)
#endif

/*
	NAME_ROUTINE_C/F/C2F are macros to translate MPI interface names to 
	patches that will be hooked by the DynInst mutator.

	_C -> converts names for C MPI symbols
	_F -> converts names for Fortran MPI symbols (ignoring the number of underscores,
	      i.e does not honor _UNDERSCORES defines and CtoF77 macro)
	      This is convenient when using the attribute construction of the compiler to
	      provide all the names for the symbols.
	_C2F-> converts names for Fortran MPI symbols (honoring _UNDERSCORES and
	      CtoF77 macro)
*/

#if defined(DYNINST_MODULE)
# define NAME_ROUTINE_C(x) PATCH_P##x  /* MPI_Send is converted to PATCH_PMPI_Send */
# define NAME_ROUTINE_F(x) patch_p##x  /* mpi_send is converted to patch_pmpi_send */
# define NAME_ROUTINE_FU(x) patch_P##x  /* mpi_send is converted to patch_Pmpi_send */
# define NAME_ROUTINE_C2F(x) CtoF77(patch_p##x)  /* mpi_send may be converted to patch_pmpi_send_ */
#else
# define NAME_ROUTINE_C(x) x
# define NAME_ROUTINE_F(x) x
# define NAME_ROUTINE_C2F(x) CtoF77(x)
#endif

unsigned int NumOpsGlobals = 0;
unsigned int CurrentOpGlobal = 0;

#if defined(FORTRAN_SYMBOLS)
# include "mpif.h"
#endif

#if defined(HAVE_ALIAS_ATTRIBUTE) 

/* This macro defines r1, r2 and r3 to be aliases to "orig" routine.
   params are the same parameters received by "orig" */

# if defined(DYNINST_MODULE)

/* MPI_F_SYMS define different Fortran synonymous using the __attribute__ 
	 compiler constructor. Use r3 in the UPPERCASE VERSION of the MPI call. */

#  define MPI_F_SYMS(r1,r2,r3,orig,params) \
    void NAME_ROUTINE_F(r1) params __attribute__ ((alias ("patch_p"#orig))); \
    void NAME_ROUTINE_F(r2) params __attribute__ ((alias ("patch_p"#orig))); \
    void NAME_ROUTINE_FU(r3) params __attribute__ ((alias ("patch_p"#orig)));
# else
#  define MPI_F_SYMS(r1,r2,r3,orig,params) \
    void r1 params __attribute__ ((alias (#orig))); \
    void r2 params __attribute__ ((alias (#orig))); \
    void r3 params __attribute__ ((alias (#orig)));

# endif
 
#endif

/* Some C libraries do not contain the mpi_init symbol (fortran)
	 When compiling the combined (C+Fortran) dyninst module, the resulting
	 module CANNOT be loaded if mpi_init is not found. The top #if def..
	 is a workaround for this situation
*/

#if defined(FORTRAN_SYMBOLS)

/*#if (defined(COMBINED_SYMBOLS) && defined(MPI_C_CONTAINS_FORTRAN_MPI_INIT) || \
     !defined(COMBINED_SYMBOLS) && defined(FORTRAN_SYMBOLS))
*/
/******************************************************************************
 ***  MPI_Init
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_init__,mpi_init_,MPI_INIT,mpi_init,(MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_init) (MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_init) (MPI_Fint *ierror)
#endif
{
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_init_enter (ierror);
#endif

	/* En qualsevol cas, cal cridar al Wrapper que inicialitzara tot el que cal */
	DEBUG_INTERFACE(ENTER)
	PMPI_Init_Wrapper (ierror);
	DEBUG_INTERFACE(LEAVE)

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_init_leave ();
#endif
}

/*
#endif  
*/
/*
#if (defined(COMBINED_SYMBOLS) && defined(MPI_C_CONTAINS_FORTRAN_MPI_INIT) || \
     !defined(COMBINED_SYMBOLS) && defined(FORTRAN_SYMBOLS))
*/

#if defined(MPI_HAS_INIT_THREAD)
/******************************************************************************
 ***  MPI_Init_thread
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_init_thread__,mpi_init_thread_,MPI_INIT_THREAD,mpi_init_thread,(MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_init_thread) (MPI_Fint *required, MPI_Fint *provided,
	MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_init_thread) (MPI_Fint *required, MPI_Fint *provided,
	MPI_Fint *ierror)
#endif
{
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_init_enter (ierror);
#endif

	/* En qualsevol cas, cal cridar al Wrapper que inicialitzara tot el que cal */
	DEBUG_INTERFACE(ENTER)
	PMPI_Init_thread_Wrapper (required, provided, ierror);
	DEBUG_INTERFACE(LEAVE)

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_init_leave ();
#endif
}
#endif /* MPI_HAS_INIT_THREAD */


/******************************************************************************
 ***  MPI_Finalize
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_finalize__,mpi_finalize_,MPI_FINALIZE,mpi_finalize, (MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_finalize) (MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_finalize) (MPI_Fint *ierror)
#endif
{
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_finalize_enter (ierror);
#endif

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_Finalize_Wrapper (ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else if (!mpitrace_on && CheckForControlFile)
	{
		/* This case happens when the tracing isn't activated due to the inexistance
			of the control file. Just remove the temporal files! */
		DEBUG_INTERFACE(ENTER)
		remove_temporal_files();
		remove_file_list();
		DEBUG_INTERFACE(LEAVE)
		CtoF77 (pmpi_finalize) (ierror);
	}
	else
		CtoF77 (pmpi_finalize) (ierror);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_finalize_leave ();
#endif
}


/******************************************************************************
 ***  MPI_BSend
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_bsend__,mpi_bsend_,MPI_BSEND,mpi_bsend,(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_bsend) (void *buf, MPI_Fint *count, MPI_Fint *datatype,
	MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror) 
#else
void NAME_ROUTINE_C2F(mpi_bsend) (void *buf, MPI_Fint *count,
	MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm,
	MPI_Fint *ierror)
#endif
{
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_bsend_enter (buf, count, datatype, dest, tag, comm, ierror);
#endif

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_BSend_Wrapper (buf, count, datatype, dest, tag, comm, ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_bsend) (buf, count, datatype, dest, tag, comm, ierror);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_bsend_leave ();
#endif
}

/******************************************************************************
 ***  MPI_SSend
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_ssend__,mpi_ssend_,MPI_SSEND,mpi_ssend,(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_ssend) (void *buf, MPI_Fint *count, MPI_Fint *datatype,
	MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_ssend) (void *buf, MPI_Fint *count, MPI_Fint *datatype,
	MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror)
#endif
{
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_ssend_enter (buf, count, datatype, dest, tag, comm, ierror);
#endif
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_SSend_Wrapper (buf, count, datatype, dest, tag, comm, ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_ssend) (buf, count, datatype, dest, tag, comm, ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_ssend_leave ();
#endif
}


/******************************************************************************
 ***  MPI_RSend
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_rsend__,mpi_rsend_,MPI_RSEND,mpi_rsend,(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_rsend) (void *buf, MPI_Fint *count, MPI_Fint *datatype,
	MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_rsend) (void *buf, MPI_Fint *count,
	MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm,
	MPI_Fint *ierror)
#endif
{
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_rsend_enter (buf, count, datatype, dest, tag, comm, ierror);
#endif
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_RSend_Wrapper (buf, count, datatype, dest, tag, comm, ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
 		CtoF77 (pmpi_rsend) (buf, count, datatype, dest, tag, comm, ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_rsend_leave ();
#endif
}

/******************************************************************************
 ***  MPI_Send
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_send__,mpi_send_,MPI_SEND,mpi_send,(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_send) (void *buf, MPI_Fint *count, MPI_Fint *datatype,
	MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_send) (void *buf, MPI_Fint *count,
	MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm,
	MPI_Fint *ierror)
#endif
{
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_send_enter (buf, count, datatype, dest, tag, comm, ierror);
#endif
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_Send_Wrapper (buf, count, datatype, dest, tag, comm, ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_send) (buf, count, datatype, dest, tag, comm, ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_send_leave ();
#endif
}


/******************************************************************************
 ***  MPI_IBSend
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_ibsend__,mpi_ibsend_,MPI_IBSEND,mpi_ibsend,(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_ibsend) (void *buf, MPI_Fint *count, MPI_Fint *datatype,
	MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request,
	MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_ibsend) (void *buf, MPI_Fint *count, MPI_Fint *datatype,
	MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request,
	MPI_Fint *ierror)
#endif
{
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_ibsend_enter (buf, count, datatype, dest, tag, comm, request, ierror);
#endif
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_IBSend_Wrapper (buf, count, datatype, dest, tag, comm, request,
                         ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_ibsend) (buf, count, datatype, dest, tag, comm, request,
                          ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_ibsend_leave ();
#endif
}

/******************************************************************************
 ***  MPI_ISend
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_isend__,mpi_isend_,MPI_ISEND,mpi_isend,(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_isend) (void *buf, MPI_Fint *count, MPI_Fint *datatype,
	MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request,
	MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_isend) (void *buf, MPI_Fint *count, MPI_Fint *datatype,
	MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request,
	MPI_Fint *ierror)
#endif
{
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_isend_enter (buf, count, datatype, dest, tag, comm, request, ierror);
#endif
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_ISend_Wrapper (buf, count, datatype, dest, tag, comm, request,
			ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_isend) (buf, count, datatype, dest, tag, comm, request,
			ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_isend_leave ();
#endif
}

/******************************************************************************
 ***  MPI_ISSend
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_issend__,mpi_issend_,MPI_ISSEND,mpi_issend,(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_issend) (void *buf, MPI_Fint *count, MPI_Fint *datatype,
	MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request,
	MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_issend) (void *buf, MPI_Fint *count,
	MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm,
	MPI_Fint *request, MPI_Fint *ierror)
#endif
{
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_issend_enter (buf, count, datatype, dest, tag, comm, request, ierror);
#endif
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_ISSend_Wrapper (buf, count, datatype, dest, tag, comm, request,
			ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_issend) (buf, count, datatype, dest, tag, comm, request,
			ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_issend_leave ();
#endif
}

/******************************************************************************
 ***  MPI_IRSend
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_irsend__,mpi_irsend_,MPI_IRSEND,mpi_irsend,(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_irsend) (void *buf, MPI_Fint *count, MPI_Fint *datatype,
	MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request,
	MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_irsend) (void *buf, MPI_Fint *count, MPI_Fint *datatype,
	MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request,
	MPI_Fint *ierror)
#endif
{
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_irsend_enter (buf, count, datatype, dest, tag, comm, request, ierror);
#endif
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_IRSend_Wrapper (buf, count, datatype, dest, tag, comm, request,
			ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_irsend) (buf, count, datatype, dest, tag, comm, request,
			ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_irsend_leave ();
#endif
}

/******************************************************************************
 ***  MPI_Recv
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_recv__,mpi_recv_,MPI_RECV,mpi_recv,(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_recv) (void *buf, MPI_Fint *count, MPI_Fint *datatype,
	MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status,
	MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_recv) (void *buf, MPI_Fint *count, MPI_Fint *datatype,
	MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, 
	MPI_Fint *ierror)
#endif
{
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_recv_enter (buf, count, datatype, source, tag, comm, status, ierror);
#endif
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_Recv_Wrapper (buf, count, datatype, source, tag, comm, status,
                       ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_recv) (buf, count, datatype, source, tag, comm, status,
                        ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_recv_leave ();
#endif
}

/******************************************************************************
 ***  MPI_IRecv
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_irecv__,mpi_irecv_,MPI_IRECV,mpi_irecv,(void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_irecv) (void *buf, MPI_Fint *count, MPI_Fint *datatype,
	MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request,
	MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_irecv) (void *buf, MPI_Fint *count, MPI_Fint *datatype,
	MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request,
	MPI_Fint *ierror)
#endif
{
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_irecv_enter (buf, count, datatype, source, tag, comm, request, ierror);
#endif
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_IRecv_Wrapper (buf, count, datatype, source, tag, comm, request,
                        ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_irecv) (buf, count, datatype, source, tag, comm, request,
                         ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_irecv_leave ();
#endif
}


/******************************************************************************
 ***  MPI_Reduce
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_reduce__,mpi_reduce_,MPI_REDUCE,mpi_reduce,(void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_reduce) (void *sendbuf, void *recvbuf, MPI_Fint *count,
	MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm,
	MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_reduce) (void *sendbuf, void *recvbuf, MPI_Fint *count,
	MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm,
	MPI_Fint *ierror)
#endif
{
	int sizeofcomm;
	MPI_Comm c = MPI_Comm_f2c(*comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_reduce_enter (sendbuf, recvbuf, count, datatype, op, root, comm, ierror);
#endif

	PMPI_Comm_size (c, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
  		CurrentOpGlobal = (++NumOpsGlobals);
#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_Reduce_Wrapper (sendbuf, recvbuf, count, datatype, op, root, comm,
                         ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_reduce) (sendbuf, recvbuf, count, datatype, op, root, comm,
                          ierror);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_reduce_leave ();
#endif
}

/******************************************************************************
 ***  MPI_Reduce_scatter
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_reduce_scatter__,mpi_reduce_scatter_,MPI_REDUCE_SCATTER,mpi_reduce_scatter,(void *sendbuf, void *recvbuf, MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_reduce_scatter) (void *sendbuf, void *recvbuf, MPI_Fint *recvcounts,
	MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_reduce_scatter) (void *sendbuf, void *recvbuf,
	MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm,
	MPI_Fint *ierror)
#endif
{
	int sizeofcomm;
	MPI_Comm c = MPI_Comm_f2c(*comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_reduce_scatter_enter (sendbuf, recvbuf, recvcounts, datatype, op, comm, ierror);
#endif

	PMPI_Comm_size (c, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_Reduce_Scatter_Wrapper (sendbuf, recvbuf, recvcounts, datatype, op,
			comm, ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_reduce_scatter) (sendbuf, recvbuf, recvcounts, datatype, op,
			comm, ierror);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_reduce_scatter_leave ();
#endif
}

/******************************************************************************
 ***  MPI_AllReduce
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_allreduce__,mpi_allreduce_,MPI_ALLREDUCE,mpi_allreduce,(void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_allreduce) (void *sendbuf, void *recvbuf,
	MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm,
	MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_allreduce) (void *sendbuf, void *recvbuf,
	MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm,
	MPI_Fint *ierror)
#endif
{
	int sizeofcomm;
	MPI_Comm c = MPI_Comm_f2c(*comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_allreduce_enter (sendbuf, recvbuf, count, datatype, op, comm, ierror);
#endif

	PMPI_Comm_size (c, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_AllReduce_Wrapper (sendbuf, recvbuf, count, datatype, op, comm,
                            ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_allreduce) (sendbuf, recvbuf, count, datatype, op, comm,
                             ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_allreduce_leave ();
#endif
}

/******************************************************************************
 ***  MPI_Probe
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_probe__,mpi_probe_,MPI_PROBE,mpi_probe,(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_probe) (MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm,
	MPI_Fint *status, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_probe) (MPI_Fint *source, MPI_Fint *tag,
	MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierror)
#endif
{
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Probe_Wrapper (source, tag, comm, status, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_probe) (source, tag, comm, status, ierror);
}


/******************************************************************************
 ***  MPI_IProbe
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_iprobe__,mpi_iprobe_,MPI_IPROBE,mpi_iprobe,(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_iprobe) (MPI_Fint *source, MPI_Fint *tag,
	MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_iprobe) (MPI_Fint *source, MPI_Fint *tag,
	MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror)
#endif
{
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_IProbe_Wrapper (source, tag, comm, flag, status, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_iprobe) (source, tag, comm, flag, status, ierror);
}


/******************************************************************************
 ***  MPI_Barrier
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_barrier__,mpi_barrier_,MPI_BARRIER,mpi_barrier,(MPI_Fint *comm, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_barrier) (MPI_Fint *comm, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_barrier) (MPI_Fint *comm, MPI_Fint *ierror)
#endif
{
	int sizeofcomm;
	MPI_Comm c = MPI_Comm_f2c(*comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_barrier_enter (comm, ierror);
#endif

	PMPI_Comm_size (c, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;
    
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Barrier_Wrapper (comm, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_barrier) (comm, ierror);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_barrier_leave ();
#endif
}


/******************************************************************************
 ***  MPI_Cancel
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_cancel__,mpi_cancel_,MPI_CANCEL,mpi_cancel,(MPI_Fint *request, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_cancel) (MPI_Fint *request, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_cancel) (MPI_Fint *request, MPI_Fint *ierror)
#endif
{
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Cancel_Wrapper (request, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_cancel) (request, ierror);
}

/******************************************************************************
 ***  MPI_Test
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_test__,mpi_test_,MPI_TEST,mpi_test,(MPI_Fint *request, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_test) (MPI_Fint *request, MPI_Fint *flag,
	MPI_Fint *status, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_test) (MPI_Fint *request, MPI_Fint *flag,
	MPI_Fint *status, MPI_Fint *ierror)
#endif
{
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Test_Wrapper (request, flag, status, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_test) (request, flag, status, ierror);
}

/******************************************************************************
 ***  MPI_Wait
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_wait__,mpi_wait_,MPI_WAIT,mpi_wait,(MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_wait) (MPI_Fint *request, MPI_Fint *status,
	MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_wait) (MPI_Fint *request, MPI_Fint *status,
	MPI_Fint *ierror)
#endif
{
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_wait_enter (request, status, ierror);
#endif
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Wait_Wrapper (request, status, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_wait) (request, status, ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_wait_leave ();
#endif
}

/******************************************************************************
 ***  MPI_WaitAll
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_waitall__,mpi_waitall_,MPI_WAITALL,mpi_waitall,(MPI_Fint * count, MPI_Fint array_of_requests[], MPI_Fint array_of_statuses[][SIZEOF_MPI_STATUS], MPI_Fint * ierror))

void NAME_ROUTINE_F(mpi_waitall) (MPI_Fint * count,
	MPI_Fint array_of_requests[], MPI_Fint array_of_statuses[][SIZEOF_MPI_STATUS],
	MPI_Fint * ierror)
#else
void NAME_ROUTINE_C2F(mpi_waitall) (MPI_Fint * count,
	MPI_Fint array_of_requests[], MPI_Fint array_of_statuses[][SIZEOF_MPI_STATUS],
	MPI_Fint * ierror)
#endif
{
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_waitall_enter (count, array_of_requests, array_of_statuses, ierror);
#endif
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_WaitAll_Wrapper (count, array_of_requests, array_of_statuses,
                          ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_waitall) (count, array_of_requests, array_of_statuses,
                           ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_waitall_leave ();
#endif
}


/******************************************************************************
 ***  MPI_WaitAny
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_waitany__,mpi_waitany_,MPI_WAITANY,mpi_waitany, (MPI_Fint *count, MPI_Fint array_of_requests[],MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_waitany) (MPI_Fint *count, MPI_Fint array_of_requests[],
	MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_waitany) (MPI_Fint *count, MPI_Fint array_of_requests[],
	MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierror)
#endif
{
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_waitany_enter (count, array_of_requests, index, status, ierror);
#endif
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_WaitAny_Wrapper (count, array_of_requests, index, status, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_waitany) (count, array_of_requests, index, status, ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_waitany_leave ();
#endif
}


/******************************************************************************
 ***  MPI_WaitSome
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_waitsome__,mpi_waitsome_,MPI_WAITSOME,mpi_waitsome, (MPI_Fint *incount, MPI_Fint array_of_requests[], MPI_Fint *outcount, MPI_Fint array_of_indices[], MPI_Fint array_of_statuses[][SIZEOF_MPI_STATUS], MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_waitsome) (MPI_Fint *incount,
	MPI_Fint array_of_requests[], MPI_Fint *outcount, MPI_Fint array_of_indices[],
	MPI_Fint array_of_statuses[][SIZEOF_MPI_STATUS], MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_waitsome) (MPI_Fint *incount, MPI_Fint array_of_requests[],
	MPI_Fint *outcount, MPI_Fint array_of_indices[],
	MPI_Fint array_of_statuses[][SIZEOF_MPI_STATUS], MPI_Fint *ierror)
#endif
{
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_waitsome_enter (incount, array_of_requests, outcount, array_of_indices, array_of_statuses, ierror);
#endif
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_WaitSome_Wrapper (incount, array_of_requests, outcount,
                           array_of_indices, array_of_statuses, ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_waitsome) (incount, array_of_requests, outcount,
                            array_of_indices, array_of_statuses, ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_waitsome_leave ();
#endif
}

/******************************************************************************
 ***  MPI_BCast
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_bcast__,mpi_bcast_,MPI_BCAST,mpi_bcast,(void *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_bcast) (void *buffer, MPI_Fint *count,
	MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_bcast) (void *buffer, MPI_Fint *count,
	MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
#endif
{
	int sizeofcomm;
	MPI_Comm c = MPI_Comm_f2c(*comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_bcast_enter (buffer, count, datatype, root, comm, ierror);
#endif

	PMPI_Comm_size (c, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_BCast_Wrapper (buffer, count, datatype, root, comm, ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_bcast) (buffer, count, datatype, root, comm, ierror);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_bcast_leave ();
#endif
}

/******************************************************************************
 ***  MPI_AllToAll
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_alltoall__,mpi_alltoall_,MPI_ALLTOALL,mpi_alltoall, (void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_alltoall) (void *sendbuf, MPI_Fint *sendcount,
	MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype,
	MPI_Fint *comm, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_alltoall) (void *sendbuf, MPI_Fint *sendcount,
	MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype,
	MPI_Fint *comm, MPI_Fint *ierror)
#endif
{
	int sizeofcomm;
	MPI_Comm c = MPI_Comm_f2c(*comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_alltoall_enter (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, ierror);
#endif

	PMPI_Comm_size (c, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif
	
		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_AllToAll_Wrapper (sendbuf, sendcount, sendtype, recvbuf, recvcount,
                           recvtype, comm, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_alltoall) (sendbuf, sendcount, sendtype, recvbuf, recvcount,
                            recvtype, comm, ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_lltoall_leave ();
#endif
}


/******************************************************************************
 ***  MPI_AllToAllV
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_alltoallv__,mpi_alltoallv_,MPI_ALLTOALLV,mpi_alltoallv, (void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sdispls, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_alltoallv) (void *sendbuf, MPI_Fint *sendcount,
	MPI_Fint *sdispls, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount,
	MPI_Fint *rdispls, MPI_Fint *recvtype,	MPI_Fint *comm, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_alltoallv) (void *sendbuf, MPI_Fint *sendcount,
	MPI_Fint *sdispls, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount,
	MPI_Fint *rdispls, MPI_Fint *recvtype,	MPI_Fint *comm, MPI_Fint *ierror)
#endif
{
	int sizeofcomm;
	MPI_Comm c = MPI_Comm_f2c(*comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_alltoallv_enter (sendbuf, sendcount, sdispls, sendtype, recvbuf, recvcount, rdispls, recvtype, comm, ierror);
#endif

	PMPI_Comm_size (c, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_AllToAllV_Wrapper (sendbuf, sendcount, sdispls, sendtype, recvbuf,
                            recvcount, rdispls, recvtype, comm, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_alltoallv) (sendbuf, sendcount, sdispls, sendtype, recvbuf,
                             recvcount, rdispls, recvtype, comm, ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_alltoallv_leave ();
#endif
}


/******************************************************************************
 ***  MPI_Allgather
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_allgather__,mpi_allgather_,MPI_ALLGATHER,mpi_allgather, (void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_allgather) (void *sendbuf, MPI_Fint *sendcount,
	MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype,
	MPI_Fint *comm, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_allgather) (void *sendbuf, MPI_Fint *sendcount,
	MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype,
	MPI_Fint *comm, MPI_Fint *ierror)
#endif
{
	int sizeofcomm;
	MPI_Comm c = MPI_Comm_f2c(*comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_allgather_enter (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, ierror);
#endif
	PMPI_Comm_size (c, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Allgather_Wrapper (sendbuf, sendcount, sendtype, recvbuf, recvcount,
                            recvtype, comm, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_allgather) (sendbuf, sendcount, sendtype, recvbuf, recvcount,
                             recvtype, comm, ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_allgather_leave ();
#endif
}


/******************************************************************************
 ***  MPI_Allgatherv
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_allgatherv__,mpi_allgatherv_,MPI_ALLGATHERV,mpi_allgatherv, (void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_allgatherv) (void *sendbuf, MPI_Fint *sendcount,
	MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *displs,
	MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_allgatherv) (void *sendbuf, MPI_Fint *sendcount,
	MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *displs,
	MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierror)
#endif
{
	int sizeofcomm;
	MPI_Comm c = MPI_Comm_f2c(*comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_allgatherv_enter (sendbuf, sendcount, sendtype, recvbuf, recvcount, displs, recvtype, comm, ierror);
#endif
	PMPI_Comm_size (c, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Allgatherv_Wrapper (sendbuf, sendcount, sendtype, recvbuf, recvcount,
                             displs, recvtype, comm, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_allgatherv) (sendbuf, sendcount, sendtype, recvbuf,
                              recvcount, displs, recvtype, comm, ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_allgatherv_leave ();
#endif
}


/******************************************************************************
 ***  MPI_Gather
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_gather__,mpi_gather_,MPI_GATHER,mpi_gather, (void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_gather) (void *sendbuf, MPI_Fint *sendcount,
	MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype,
	MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_gather) (void *sendbuf, MPI_Fint *sendcount,
	MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype,
	MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
#endif
{
	int sizeofcomm;
	MPI_Comm c = MPI_Comm_f2c(*comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_gather_enter (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, ierror);
#endif
	PMPI_Comm_size (c, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Gather_Wrapper (sendbuf, sendcount, sendtype, recvbuf, recvcount,
                         recvtype, root, comm, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_gather) (sendbuf, sendcount, sendtype, recvbuf, recvcount,
                          recvtype, root, comm, ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_gather_leave ();
#endif
}

/******************************************************************************
 ***  MPI_GatherV
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_gatherv__,mpi_gatherv_,MPI_GATHERV,mpi_gatherv, (void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_gatherv) (void *sendbuf, MPI_Fint *sendcount,
	MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *displs,
	MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_gatherv) (void *sendbuf, MPI_Fint *sendcount,
	MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *displs,
	MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
#endif
{
	int sizeofcomm;
	MPI_Comm c = MPI_Comm_f2c(*comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_gatherv_enter (sendbuf, sendcount, sendtype, recvbuf, recvcount, displs, recvtype, root, comm, ierror);
#endif
	PMPI_Comm_size (c, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_GatherV_Wrapper (sendbuf, sendcount, sendtype, recvbuf, recvcount,
                          displs, recvtype, root, comm, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_gatherv) (sendbuf, sendcount, sendtype, recvbuf, recvcount,
                           displs, recvtype, root, comm, ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_gatherv_leave ();
#endif
}

/******************************************************************************
 ***  MPI_Scatter
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_scatter__,mpi_scatter_,MPI_SCATTER,mpi_scatter,(void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_scatter) (void *sendbuf, MPI_Fint *sendcount,
	MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype,
	MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_scatter) (void *sendbuf, MPI_Fint *sendcount,
	MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype,
	MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
#endif
{
	int sizeofcomm;
	MPI_Comm c = MPI_Comm_f2c(*comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_scatter_enter (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, ierror);
#endif
	PMPI_Comm_size (c, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Scatter_Wrapper (sendbuf, sendcount, sendtype, recvbuf, recvcount,
                          recvtype, root, comm, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_scatter) (sendbuf, sendcount, sendtype, recvbuf, recvcount,
                           recvtype, root, comm, ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_scatter_leave ();
#endif
}

/******************************************************************************
 ***  MPI_ScatterV
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_scatterv__,mpi_scatterv_,MPI_SCATTERV,mpi_scatterv,(void *sendbuf, MPI_Fint *sendcount, MPI_Fint *displs, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_scatterv) (void *sendbuf, MPI_Fint *sendcount,
	MPI_Fint *displs, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount,
	MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_scatterv) (void *sendbuf, MPI_Fint *sendcount,
	MPI_Fint *displs, MPI_Fint *sendtype, void *recvbuf, MPI_Fint *recvcount,
	MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierror)
#endif
{
	int sizeofcomm;
	MPI_Comm c = MPI_Comm_f2c(*comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_scatterv_enter (sendbuf, sendcount, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, ierror);
#endif
	PMPI_Comm_size (c, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_ScatterV_Wrapper (sendbuf, sendcount, displs, sendtype, recvbuf,
                           recvcount, recvtype, root, comm, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_scatterv) (sendbuf, sendcount, displs, sendtype, recvbuf,
                            recvcount, recvtype, root, comm, ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_scatter_leave ();
#endif
}

/******************************************************************************
 ***  MPI_Comm_Rank
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_comm_rank__,mpi_comm_rank_,MPI_COMM_RANK,mpi_comm_rank,(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_comm_rank) (MPI_Fint *comm, MPI_Fint *rank,
	MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_comm_rank) (MPI_Fint *comm, MPI_Fint *rank,
	MPI_Fint *ierror)
#endif
{
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Comm_Rank_Wrapper (comm, rank, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_comm_rank) (comm, rank, ierror);
}

/******************************************************************************
 ***  MPI_Comm_Size
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_comm_size__,mpi_comm_size_,MPI_COMM_SIZE,mpi_comm_size,(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_comm_size) (MPI_Fint *comm, MPI_Fint *size,
	MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_comm_size) (MPI_Fint *comm, MPI_Fint *size,
	MPI_Fint *ierror)
#endif
{
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Comm_Size_Wrapper (comm, size, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_comm_size) (comm, size, ierror);
}

/******************************************************************************
 ***  MPI_Comm_Create
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_comm_create__,mpi_comm_create_,MPI_COMM_CREATE,mpi_comm_create,(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_comm_create) (MPI_Fint *comm, MPI_Fint *group,
	MPI_Fint *newcomm, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_comm_create) (MPI_Fint *comm, MPI_Fint *group,
	MPI_Fint *newcomm, MPI_Fint *ierror)
#endif
{
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Comm_Create_Wrapper (comm, group, newcomm, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_comm_create) (comm, group, newcomm, ierror);
}

/******************************************************************************
 ***  MPI_Comm_Dup
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_comm_dup__,mpi_comm_dup_,MPI_COMM_DUP,mpi_comm_dup,(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_comm_dup) (MPI_Fint *comm, MPI_Fint *newcomm,
	MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_comm_dup) (MPI_Fint *comm, MPI_Fint *newcomm,
	MPI_Fint *ierror)
#endif
{
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Comm_Dup_Wrapper (comm, newcomm, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_comm_dup) (comm, newcomm, ierror);
}


/******************************************************************************
 ***  MPI_Comm_Split
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_comm_split__,mpi_comm_split_,MPI_COMM_SPLIT,mpi_comm_split,(MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key, MPI_Fint *newcomm, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_comm_split) (MPI_Fint *comm, MPI_Fint *color,
	MPI_Fint *key, MPI_Fint *newcomm, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_comm_split) (MPI_Fint *comm, MPI_Fint *color,
	MPI_Fint *key, MPI_Fint *newcomm, MPI_Fint *ierror)
#endif
{
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Comm_Split_Wrapper (comm, color, key, newcomm, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_comm_split) (comm, color, key, newcomm, ierror);
}

/******************************************************************************
 *** MPI_Cart_create
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_cart_create__,mpi_cart_create_,MPI_CART_CREATE,mpi_cart_create, (MPI_Fint *comm_old, MPI_Fint *ndims, MPI_Fint *dims, MPI_Fint *periods, MPI_Fint *reorder, MPI_Fint *comm_cart, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_cart_create) (MPI_Fint *comm_old, MPI_Fint *ndims,
	MPI_Fint *dims, MPI_Fint *periods, MPI_Fint *reorder, MPI_Fint *comm_cart,
	MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_cart_create) (MPI_Fint *comm_old, MPI_Fint *ndims,
	MPI_Fint *dims, MPI_Fint *periods, MPI_Fint *reorder, MPI_Fint *comm_cart,
	MPI_Fint *ierror)
#endif
{
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Cart_create_Wrapper (comm_old, ndims, dims, periods, reorder,
                              comm_cart, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_cart_create) (comm_old, ndims, dims, periods, reorder,
                               comm_cart, ierror);
}

/******************************************************************************
 *** MPI_Cart_sub
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_cart_sub__,mpi_cart_sub_,MPI_CART_SUB,mpi_cart_sub, (MPI_Fint *comm, MPI_Fint *remain_dims, MPI_Fint *comm_new, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_cart_sub) (MPI_Fint *comm, MPI_Fint *remain_dims,
	MPI_Fint *comm_new, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_cart_sub) (MPI_Fint *comm, MPI_Fint *remain_dims,
	MPI_Fint *comm_new, MPI_Fint *ierror)
#endif
{
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Cart_sub_Wrapper (comm, remain_dims, comm_new, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_cart_sub) (comm, remain_dims, comm_new, ierror);
}

/******************************************************************************
 ***  MPI_Start
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_start__,mpi_start_,MPI_START,mpi_start, (MPI_Fint *request, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_start) (MPI_Fint *request, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_start) (MPI_Fint *request, MPI_Fint *ierror)
#endif
{
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Start_Wrapper (request, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_start) (request, ierror);
}

/******************************************************************************
 ***  MPI_Startall
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_startall__,mpi_startall_,MPI_STARTALL,mpi_startall, (MPI_Fint *count, MPI_Fint array_of_requests[], MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_startall) (MPI_Fint *count,
	MPI_Fint array_of_requests[], MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_startall) (MPI_Fint *count,
	MPI_Fint array_of_requests[], MPI_Fint *ierror)
#endif
{
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Startall_Wrapper (count, array_of_requests, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_startall) (count, array_of_requests, ierror);
}

/******************************************************************************
 ***  MPI_Request_free
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_request_free__,mpi_request_free_,MPI_REQUEST_FREE,mpi_request_free, (MPI_Fint *request, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_request_free) (MPI_Fint *request, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_request_free) (MPI_Fint *request, MPI_Fint *ierror)
#endif
{
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Request_free_Wrapper (request, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_request_free) (request, ierror);
}

/******************************************************************************
 ***  MPI_Recv_init
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_recv_init__,mpi_recv_init_,MPI_RECV_INIT,mpi_recv_init, (void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_recv_init) (void *buf, MPI_Fint *count,
	MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm,
	MPI_Fint *request, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_recv_init) (void *buf, MPI_Fint *count,
	MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm,
	MPI_Fint *request, MPI_Fint *ierror)
#endif
{
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Recv_init_Wrapper (buf, count, datatype, source, tag,
                            comm, request, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_recv_init) (buf, count, datatype, source, tag,
                             comm, request, ierror);
}

/******************************************************************************
 ***  MPI_Send_init
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_send_init__,mpi_send_init_,MPI_SEND_INIT,mpi_send_init, (void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_send_init) (void *buf, MPI_Fint *count,
	MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm,
	MPI_Fint *request, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_send_init) (void *buf, MPI_Fint *count,
	MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm,
	MPI_Fint *request, MPI_Fint *ierror)
#endif
{
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Send_init_Wrapper (buf, count, datatype, dest, tag,
                            comm, request, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_send_init) (buf, count, datatype, dest, tag,
                             comm, request, ierror);
}

/******************************************************************************
 ***  MPI_Bsend_init
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_bsend_init__,mpi_bsend_init_,MPI_BSEND_INIT,mpi_bsend_init, (void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_bsend_init) (void *buf, MPI_Fint *count,
	MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm,
	MPI_Fint *request, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_bsend_init) (void *buf, MPI_Fint *count,
	MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm,
	MPI_Fint *request, MPI_Fint *ierror)
#endif
{
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Bsend_init_Wrapper (buf, count, datatype, dest, tag,
                             comm, request, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_bsend_init) (buf, count, datatype, dest, tag,
                              comm, request, ierror);
}

/******************************************************************************
 ***  MPI_Rsend_init
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_rsend_init__,mpi_rsend_init_,MPI_RSEND_INIT,mpi_rsend_init, (void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_rsend_init) (void *buf, MPI_Fint *count,
	MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm,
	MPI_Fint *request, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_rsend_init) (void *buf, MPI_Fint *count,
	MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm,
	MPI_Fint *request, MPI_Fint *ierror)
#endif
{
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Rsend_init_Wrapper (buf, count, datatype, dest, tag,
                             comm, request, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_rsend_init) (buf, count, datatype, dest, tag,
                              comm, request, ierror);
}

/******************************************************************************
 ***  MPI_Ssend_init
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_ssend_init__,mpi_ssend_init_,MPI_SSEND_INIT,mpi_ssend_init, (void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_ssend_init) (void *buf, MPI_Fint *count, MPI_Fint *datatype,
	MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request,
	MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_ssend_init) (void *buf, MPI_Fint *count, MPI_Fint *datatype,
	MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request,
	MPI_Fint *ierror)
#endif
{
  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Ssend_init_Wrapper (buf, count, datatype, dest, tag,
                             comm, request, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_ssend_init) (buf, count, datatype, dest, tag,
                              comm, request, ierror);
}

/******************************************************************************
 ***  MPI_Scan
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_scan__,mpi_scan_,MPI_SCAN,mpi_scan, (void *sendbuf, void *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_scan) (void *sendbuf, void *recvbuf, MPI_Fint *count,
	MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_scan) (void *sendbuf, void *recvbuf, MPI_Fint *count,
	MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierror)
#endif
{
	int sizeofcomm;
	MPI_Comm c = MPI_Comm_f2c(*comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_scan_enter (sendbuf, recvbuf, count, datatype, op, comm, ierror);
#endif
	PMPI_Comm_size (c, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

  if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
    PMPI_Scan_Wrapper (sendbuf, recvbuf, count, datatype, op, comm, ierror);
		DEBUG_INTERFACE(LEAVE)
  }
  else
    CtoF77 (pmpi_scan) (sendbuf, recvbuf, count, datatype, op, comm, ierror);
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_scan_leave ();
#endif
}

/******************************************************************************
 ***  MPI_Sendrecv
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_sendrecv__,mpi_sendrecv_,MPI_SENDRECV,mpi_sendrecv, (void *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *dest, MPI_Fint *sendtag, void *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr))

void NAME_ROUTINE_F(mpi_sendrecv) (void *sendbuf, MPI_Fint *sendcount,
	MPI_Fint *sendtype, MPI_Fint *dest, MPI_Fint *sendtag, void *recvbuf,
	MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *source, MPI_Fint *recvtag,
	MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
#else
void NAME_ROUTINE_C2F(mpi_sendrecv) (void *sendbuf, MPI_Fint *sendcount,
	MPI_Fint *sendtype, MPI_Fint *dest, MPI_Fint *sendtag, void *recvbuf,
	MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *source, MPI_Fint *recvtag,
	MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
#endif
{
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_sendrecv_enter (sendbuf, sendcount, sendtype, dest, sendtag,
		recvbuf, recvcount, recvtype, source, recvtag, comm, status);
#endif

	if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
  	MPI_Sendrecv_Fortran_Wrapper (sendbuf, sendcount, sendtype, dest, sendtag,
    	recvbuf, recvcount, recvtype, source, recvtag, comm, status, ierr);
		DEBUG_INTERFACE(LEAVE)
  }
  else
  	CtoF77(pmpi_sendrecv) (sendbuf, sendcount, sendtype, dest, sendtag,
    	recvbuf, recvcount, recvtype, source, recvtag, comm, status, ierr);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_sendrecv_leave();
#endif
}

/******************************************************************************
 ***  MPI_Sendrecv_replace
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_sendrecv_replace__,mpi_sendrecv_replace_,MPI_SENDRECV_REPLACE,mpi_sendrecv_replace, (void *buf, MPI_Fint *count, MPI_Fint *type, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr))

void NAME_ROUTINE_F(mpi_sendrecv_replace) (void *buf, MPI_Fint *count,
	MPI_Fint *type, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *source,
	MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
#else
void NAME_ROUTINE_C2F(mpi_sendrecv_replace) (void *buf, MPI_Fint *count,
	MPI_Fint *type, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *source,
	MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
#endif
{
#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_sendrecv_replace_enter (buf, count, type, dest, sendtag, source,
		recvtag, comm, status);
#endif

	if (mpitrace_on)
  {
		DEBUG_INTERFACE(ENTER)
  	MPI_Sendrecv_replace_Fortran_Wrapper (buf, count, type, dest, sendtag, source, recvtag, comm, status, ierr);
		DEBUG_INTERFACE(LEAVE)
  }
  else
  	CtoF77(pmpi_sendrecv_replace) (buf, count, type, dest, sendtag, source, recvtag, comm, status, ierr);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_mpi_sendrecv_replace_leave ();
#endif
}

/*************************************************************
 **********************      MPIIO      **********************
 *************************************************************/

#if defined(MPI_SUPPORTS_MPI_IO)

/******************************************************************************
 ***  MPI_File_open
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_file_open__,mpi_file_open_,MPI_FILE_OPEN,mpi_file_open, (MPI_Fint *comm, char *filename, MPI_Fint *amode, MPI_Fint *info, MPI_File *fh, MPI_Fint *len))

void NAME_ROUTINE_F(mpi_file_open) (MPI_Fint *comm, char *filename,
	MPI_Fint *amode, MPI_Fint *info, MPI_File *fh, MPI_Fint *len)
#else
void NAME_ROUTINE_C2F(mpi_file_open) (MPI_Fint *comm, char *filename,
	MPI_Fint *amode, MPI_Fint *info, MPI_File *fh, MPI_Fint *len)
#endif
{
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_File_open_Fortran_Wrapper (comm, filename, amode, info, fh, len);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_file_open) (comm, filename, amode, info, fh, len);
}

/******************************************************************************
 ***  MPI_File_close
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_file_close__,mpi_file_close_,MPI_FILE_CLOSE,mpi_file_close, (MPI_File *fh, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_file_close) (MPI_File *fh, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_file_close) (MPI_File *fh, MPI_Fint *ierror)
#endif
{
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_File_close_Fortran_Wrapper (fh, ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_file_close) (fh, ierror);
}

/******************************************************************************
 ***  MPI_File_read
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_file_read__,mpi_file_read_,MPI_FILE_READ,mpi_file_read, (MPI_File *fh, void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_file_read) (MPI_File *fh, void *buf, MPI_Fint *count,
	MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_file_read) (MPI_File *fh, void *buf, MPI_Fint *count,
	MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
#endif
{ 
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_File_read_Fortran_Wrapper (fh, buf, count, datatype, status, ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_file_read) (fh, buf, count, datatype, status, ierror);
}

/******************************************************************************
 ***  MPI_File_read_all
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_file_read_all__,mpi_file_read_all_,MPI_FILE_READ_ALL,mpi_file_read_all, (MPI_File *fh, void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_file_read_all) (MPI_File *fh, void *buf,
	MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_file_read_all) (MPI_File *fh, void *buf,
	MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
#endif
{
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_File_read_all_Fortran_Wrapper (fh, buf, count, datatype, status, ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_file_read_all) (fh, buf, count, datatype, status, ierror);
}

/******************************************************************************
 ***  MPI_File_write
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_file_write__,mpi_file_write_,MPI_FILE_WRITE,mpi_file_write, (MPI_File *fh, void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_file_write) (MPI_File *fh, void *buf, MPI_Fint *count,
	MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_file_write) (MPI_File *fh, void *buf, MPI_Fint *count,
	MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
#endif
{
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_File_write_Fortran_Wrapper (fh, buf, count, datatype, status, ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_file_write) (fh, buf, count, datatype, status, ierror);
}

/******************************************************************************
 ***  MPI_File_write_all
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_file_write_all__,mpi_file_write_all_,MPI_FILE_WRITE_ALL,mpi_file_write_all, (MPI_File *fh, void *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_file_write_all) (MPI_File *fh, void *buf,
	MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_file_write_all) (MPI_File *fh, void *buf,
	MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror)
#endif
{
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_File_write_all_Fortran_Wrapper (fh, buf, count, datatype, status, ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_file_write_all) (fh, buf, count, datatype, status, ierror);
}

/******************************************************************************
 ***  MPI_File_read_at
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_file_read_at__,mpi_file_read_at_,MPI_FILE_READ_AT,mpi_file_read_at, (MPI_File *fh, MPI_Offset *offset, void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_file_read_at) (MPI_File *fh, MPI_Offset *offset,
	void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status,
	MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_file_read_at) (MPI_File *fh, MPI_Offset *offset,
	void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status,
	MPI_Fint *ierror)
#endif
{
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_File_read_at_Fortran_Wrapper (fh, offset, buf, count, datatype, status, ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_file_read_at) (fh, offset, buf, count, datatype, status, ierror);
}

/******************************************************************************
 ***  MPI_File_read_at_all
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_file_read_at_all__,mpi_file_read_at_all_,MPI_FILE_READ_AT_ALL,mpi_file_read_at_all, (MPI_File *fh, MPI_Offset *offset, void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_file_read_at_all) (MPI_File *fh, MPI_Offset *offset,
	void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status,
	MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_file_read_at_all) (MPI_File *fh, MPI_Offset *offset,
	void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status,
	MPI_Fint *ierror)
#endif
{
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_File_read_at_all_Fortran_Wrapper (fh, offset, buf, count, datatype, status, ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_file_read_at_all) (fh, offset, buf, count, datatype, status, ierror);
}

/******************************************************************************
 ***  MPI_file_write_at
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_file_write_at__,mpi_file_write_at_,MPI_FILE_WRITE_AT,mpi_file_write_at, (MPI_File *fh, MPI_Offset *offset, void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_file_write_at) (MPI_File *fh, MPI_Offset *offset,
	void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status,
	MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_file_write_at) (MPI_File *fh, MPI_Offset *offset,
	void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status,
	MPI_Fint *ierror)
#endif
{
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_File_write_at_Fortran_Wrapper (fh, offset, buf, count, datatype, status, ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_file_write_at) (fh, offset, buf, count, datatype, status, ierror);
}

/******************************************************************************
 ***  MPI_File_write_at_all
 ******************************************************************************/
#if defined(HAVE_ALIAS_ATTRIBUTE)
MPI_F_SYMS(mpi_file_write_at_all__,mpi_file_write_at_all_,MPI_FILE_WRITE_AT_ALL,mpi_file_write_at_all,(MPI_File *fh, MPI_Offset *offset, void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status, MPI_Fint *ierror))

void NAME_ROUTINE_F(mpi_file_write_at_all) (MPI_File *fh, MPI_Offset *offset,
	void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status,
	MPI_Fint *ierror)
#else
void NAME_ROUTINE_C2F(mpi_file_write_at_all) (MPI_File *fh, MPI_Offset *offset,
	void* buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Status *status,
	MPI_Fint *ierror)
#endif
{
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		PMPI_File_write_at_all_Fortran_Wrapper (fh, offset, buf, count, datatype, status, ierror);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		CtoF77 (pmpi_file_write_at_all) (fh, offset, buf, count, datatype, status, ierror);
}

#endif /* MPI_SUPPORTS_MPI_IO */

#endif /* defined(FORTRAN_SYMBOLS) */

#if defined(C_SYMBOLS)

/******************************************************************************
 ***  MPI_Init
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Init) (int *argc, char ***argv)
{
	int res;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Init_enter (argc, argv);
#endif

	/* This should be called always, whenever the tracing takes place or not */
	DEBUG_INTERFACE(ENTER)
	res = MPI_Init_C_Wrapper (argc, argv);
	DEBUG_INTERFACE(LEAVE)

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Init_leave ();
#endif

	return res;
}

#if defined(MPI_HAS_INIT_THREAD)
/******************************************************************************
 ***  MPI_Init_thread
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Init_thread) (int *argc, char ***argv, int required, int *provided)
{
	int res;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Init_enter (argc, argv);
#endif

	/* This should be called always, whenever the tracing takes place or not */
	DEBUG_INTERFACE(ENTER)
	res = MPI_Init_thread_C_Wrapper (argc, argv, required, provided);
	DEBUG_INTERFACE(LEAVE)

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Init_leave ();
#endif

	return res;
}
#endif /* MPI_HAS_INIT_THREAD */

/******************************************************************************
 ***  MPI_Finalize
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Finalize) (void)
{
	int res;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Finalize_enter ();
#endif

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Finalize_C_Wrapper ();
		DEBUG_INTERFACE(LEAVE)
	}
	else if (!mpitrace_on && CheckForControlFile)
	{
		/* This case happens when the tracing isn't activated due to the inexistance
			of the control file. Just remove the temporal files! */
		DEBUG_INTERFACE(ENTER)
		remove_temporal_files();
		remove_file_list();
		DEBUG_INTERFACE(LEAVE)
		res = PMPI_Finalize ();
	}
  else
		res = PMPI_Finalize ();

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Finalize_leave ();
#endif

	return res;
}


/******************************************************************************
 ***  MPI_Bsend
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Bsend) (void* buf, int count, MPI_Datatype datatype,
	int dest, int tag, MPI_Comm comm)
{
	int res;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Bsend_enter (buf, count, datatype, dest, tag, comm);
#endif

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Bsend_C_Wrapper (buf, count, datatype, dest, tag, comm);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Bsend (buf, count, datatype, dest, tag, comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Bsend_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Ssend
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Ssend) (void *buf, int count, MPI_Datatype datatype,
	int dest, int tag, MPI_Comm comm)
{
	int res;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Ssend_enter (buf, count, datatype, dest, tag, comm);
#endif

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Ssend_C_Wrapper (buf, count, datatype, dest, tag, comm);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Ssend (buf, count, datatype, dest, tag, comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Ssend_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Rsend
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Rsend) (void *buf, int count, MPI_Datatype datatype,
	int dest, int tag, MPI_Comm comm)
{
	int res;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Rsend_enter (buf, count, datatype, dest, tag, comm);
#endif

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Rsend_C_Wrapper (buf, count, datatype, dest, tag, comm);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Rsend (buf, count, datatype, dest, tag, comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Rsend_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Send
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Send) (void *buf, int count, MPI_Datatype datatype,
	int dest, int tag, MPI_Comm comm)
{
	int res;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Send_enter (buf, count, datatype, dest, tag, comm);
#endif

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Send_C_Wrapper (buf, count, datatype, dest, tag, comm);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Send (buf, count, datatype, dest, tag, comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Send_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Ibsend
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Ibsend) (void *buf, int count, MPI_Datatype datatype,
	int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
	int res;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Ibsend_enter (buf, count, datatype, dest, tag, comm, request);
#endif

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Ibsend_C_Wrapper (buf, count, datatype, dest, tag, comm, request);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Ibsend (buf, count, datatype, dest, tag, comm, request);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Ibsend_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Isend
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Isend) (void *buf, int count, MPI_Datatype datatype,
	int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
	int res;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Isend_enter (buf, count, datatype, dest, tag, comm, request);
#endif

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Isend_C_Wrapper (buf, count, datatype, dest, tag, comm, request);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Isend (buf, count, datatype, dest, tag, comm, request);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Isend_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Issend
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Issend) (void *buf, int count, MPI_Datatype datatype,
	int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
	int res;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Issend_enter (buf, count, datatype, dest, tag, comm, request);
#endif

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Issend_C_Wrapper (buf, count, datatype, dest, tag, comm, request);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Issend (buf, count, datatype, dest, tag, comm, request);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Issend_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Irsend
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Irsend) (void *buf, int count, MPI_Datatype datatype,
	int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
	int res;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Irsend_enter (buf, count, datatype, dest, tag, comm, request);
#endif

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Irsend_C_Wrapper (buf, count, datatype, dest, tag, comm, request);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Irsend (buf, count, datatype, dest, tag, comm, request);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Irsend_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Recv
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Recv) (void* buf, int count, MPI_Datatype datatype,
	int source, int tag, MPI_Comm comm, MPI_Status *status)
{
	int res;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Recv_enter (buf, count, datatype, source, tag, comm, status);
#endif

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Recv_C_Wrapper (buf, count, datatype, source, tag, comm, status);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Recv (buf, count, datatype, source, tag, comm, status);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Recv_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Irecv
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Irecv) (void* buf, int count, MPI_Datatype datatype,
	int source, int tag, MPI_Comm comm, MPI_Request *request)
{
	int res;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Irecv_enter (buf, count, datatype, source, tag, comm, request);
#endif

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Irecv_C_Wrapper (buf, count, datatype, source, tag, comm, request);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Irecv (buf, count, datatype, source, tag, comm, request);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Irecv_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Reduce
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Reduce) (void *sendbuf, void *recvbuf, int count,
	MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
	int res;
	int sizeofcomm;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Reduce_enter (sendbuf, recvbuf, count, datatype, op, root, comm);
#endif

	PMPI_Comm_size (comm, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Reduce_C_Wrapper 
			(sendbuf, recvbuf, count, datatype, op, root, comm);
		DEBUG_INTERFACE(LEAVE)
	}
  else
    res = PMPI_Reduce (sendbuf, recvbuf, count, datatype, op, root, comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Reduce_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Reduce_scatter
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Reduce_scatter) (void *sendbuf, void *recvbuf,
	int *recvcounts, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	int sizeofcomm;
	int res;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Reduce_scatter_enter (sendbuf, recvbuf, recvcounts, datatype, op, comm);
#endif

	PMPI_Comm_size (comm, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Reduce_Scatter_C_Wrapper (sendbuf, recvbuf, recvcounts, datatype,
			op, comm);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Reduce_scatter (sendbuf, recvbuf, recvcounts, datatype, op,
			comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Reduce_scatter_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Allreduce
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Allreduce) (void *sendbuf, void *recvbuf, int count,
	MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	int res;
	int sizeofcomm;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Allreduce_enter (sendbuf, recvbuf, count, datatype, op, comm);
#endif

	PMPI_Comm_size (comm, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Allreduce_C_Wrapper (sendbuf, recvbuf, count, datatype, op, comm);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Allreduce (sendbuf, recvbuf, count, datatype, op, comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Allreduce_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Probe
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Probe) (int source, int tag, MPI_Comm comm,
	MPI_Status *status)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Probe_C_Wrapper (source, tag, comm, status);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_Probe (source, tag, comm, status);
}

/******************************************************************************
 ***  MPI_Iprobe
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Iprobe) (int source, int tag, MPI_Comm comm, int *flag,
	MPI_Status *status)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Iprobe_C_Wrapper (source, tag, comm, flag, status);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_Iprobe (source, tag, comm, flag, status);
}

/******************************************************************************
 ***  MPI_Barrier
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Barrier) (MPI_Comm comm)
{
	int res;
	int sizeofcomm;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Barrier_enter (comm);
#endif

	PMPI_Comm_size (comm, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res =  MPI_Barrier_C_Wrapper (comm);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Barrier (comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Barrier_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Cancel
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Cancel) (MPI_Request *request)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Cancel_C_Wrapper (request);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_Cancel (request);
}

/******************************************************************************
 ***  MPI_Test
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Test) (MPI_Request *request, int *flag, MPI_Status *status)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Test_C_Wrapper (request, flag, status);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_Test (request, flag, status);
}

/******************************************************************************
 ***  MPI_Wait
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Wait) (MPI_Request *request, MPI_Status *status)
{
	int res;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Wait_enter (request, status);
#endif

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Wait_C_Wrapper (request, status);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Wait (request, status);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Wait_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Waitall
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Waitall) (int count, MPI_Request *requests,
	MPI_Status *statuses)
{
	int res;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Waitall_enter (count, requests, statuses);
#endif

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Waitall_C_Wrapper (count, requests, statuses);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Waitall (count, requests, statuses);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Waitall_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Waitany
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Waitany) (int count, MPI_Request *requests, int *index,
	MPI_Status *status)
{
	int res;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Waitany_enter (count, requests, index, status);
#endif

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Waitany_C_Wrapper (count, requests, index, status);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = (PMPI_Waitany (count, requests, index, status));

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Waitany_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Waitsome
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Waitsome) (int incount, MPI_Request * requests,
	int *outcount, int *indices, MPI_Status *statuses)
{
	int res;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Waitsome_enter (incount, requests, outcount, indices, statuses);
#endif

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Waitsome_C_Wrapper (incount,requests, outcount, indices,
			statuses);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Waitsome (incount, requests, outcount, indices, statuses);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Waitsome_leave ();
#endif
	return res;
}

/******************************************************************************
 ***  MPI_BCast
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Bcast) (void *buffer, int count, MPI_Datatype datatype,
	int root, MPI_Comm comm)
{
	int res;
	int sizeofcomm;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Bcast_enter (buffer, count, datatype, root, comm);
#endif

	PMPI_Comm_size (comm, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_BCast_C_Wrapper (buffer, count, datatype, root, comm);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Bcast (buffer, count, datatype, root, comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Bcast_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Alltoall
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Alltoall) (void *sendbuf, int sendcount,
	MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
	MPI_Comm comm)
{
	int res;
	int sizeofcomm;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Alltoall_enter (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
#endif

	PMPI_Comm_size (comm, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Alltoall_C_Wrapper
		  (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Alltoall
		  (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Alltoall_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Alltoallv
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Alltoallv) (void *sendbuf, int *sendcounts, int *sdispls,
	MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *rdispls,
	MPI_Datatype recvtype, MPI_Comm comm)
{
	int res;
	int sizeofcomm;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Alltoallv_enter (sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
#endif

	PMPI_Comm_size (comm, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Alltoallv_C_Wrapper
		  (sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts,
		  rdispls, recvtype, comm);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Alltoallv
		  (sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts,
		  rdispls, recvtype, comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Alltoallv_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Allgather
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Allgather) (void *sendbuf, int sendcount,
	MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
	MPI_Comm comm)
{
	int res;
	int sizeofcomm;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Allgather_enter (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
#endif

	PMPI_Comm_size (comm, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Allgather_C_Wrapper
		  (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
		  comm);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Allgather
		  (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
		  comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Allgather_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Allgatherv
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Allgatherv) (void *sendbuf, int sendcount,
	MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *displs,
	MPI_Datatype recvtype, MPI_Comm comm)
{
	int res;
	int sizeofcomm;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Allgatherv_enter (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
#endif

	PMPI_Comm_size (comm, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res =  MPI_Allgatherv_C_Wrapper
		  (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
		  recvtype, comm);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Allgatherv
		  (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
		  recvtype, comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Allgatherv_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Gather
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Gather) (void *sendbuf, int sendcount,
	MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
	int root, MPI_Comm comm)
{
	int res;
	int sizeofcomm;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Gather_enter (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
#endif

	PMPI_Comm_size (comm, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res =  MPI_Gather_C_Wrapper
		  (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root,
		  comm);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Gather
		  (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root,
		  comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Gather_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Gatherv
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Gatherv) (void *sendbuf, int sendcount,
	MPI_Datatype sendtype, void *recvbuf, int *recvcounts, int *displs,
	MPI_Datatype recvtype, int root, MPI_Comm comm)
{
	int res;
	int sizeofcomm;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Gatherv_enter (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm);
#endif

	PMPI_Comm_size (comm, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res =  MPI_Gatherv_C_Wrapper
            (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
             recvtype, root, comm);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Gatherv
		  (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs,
		  recvtype, root, comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Gatherv_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Scatter
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Scatter) (void *sendbuf, int sendcount,
	MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
	int root, MPI_Comm comm)
{
	int res;
	int sizeofcomm;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Scatter_enter (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
#endif

	PMPI_Comm_size (comm, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res =  MPI_Scatter_C_Wrapper
		  (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root,
		  comm);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Scatter
		  (sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root,
		  comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Scatter_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Scatterv
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Scatterv) (void *sendbuf, int *sendcounts, int *displs, 
	MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
	int root, MPI_Comm comm)
{
	int res;
	int sizeofcomm;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Scatterv_enter (sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);
#endif

	PMPI_Comm_size (comm, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Scatterv_C_Wrapper
            (sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount,
             recvtype, root, comm);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Scatterv
		  (sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount,
		  recvtype, root, comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Scatterv_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Comm_rank
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Comm_rank) (MPI_Comm comm, int *rank)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Comm_rank_C_Wrapper (comm, rank);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_Comm_rank (comm, rank);
}

/******************************************************************************
 ***  MPI_Comm_size
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Comm_size) (MPI_Comm comm, int *size)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Comm_size_C_Wrapper (comm, size);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_Comm_size (comm, size);
}

/******************************************************************************
 ***  MPI_Comm_create
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Comm_create) (MPI_Comm comm, MPI_Group group,
	MPI_Comm *newcomm)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Comm_create_C_Wrapper (comm, group, newcomm);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
    		return PMPI_Comm_create (comm, group, newcomm);
}

/******************************************************************************
 ***  MPI_Comm_dup
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Comm_dup) (MPI_Comm comm, MPI_Comm *newcomm)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
    res = MPI_Comm_dup_C_Wrapper (comm, newcomm);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
  else
    return PMPI_Comm_dup (comm, newcomm);
}

/******************************************************************************
 ***  MPI_Comm_split
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Comm_split) (MPI_Comm comm, int color, int key,
	MPI_Comm *newcomm)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Comm_split_C_Wrapper (comm, color, key, newcomm);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_Comm_split (comm, color, key, newcomm);
}


/******************************************************************************
 *** MPI_Cart_create
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Cart_create) (MPI_Comm comm_old, int ndims, int *dims,
	int *periods, int reorder, MPI_Comm *comm_cart)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Cart_create_C_Wrapper (comm_old, ndims, dims, periods, reorder,
                                      comm_cart);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_Cart_create (comm_old, ndims, dims, periods, reorder,
                             comm_cart);
}

/******************************************************************************
 *** MPI_Cart_sub
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Cart_sub) (MPI_Comm comm, int *remain_dims,
	MPI_Comm *comm_new)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res =  MPI_Cart_sub_C_Wrapper (comm, remain_dims, comm_new);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_Cart_sub (comm, remain_dims, comm_new);
}

/******************************************************************************
 ***  MPI_Start
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Start) (MPI_Request *request)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res =  MPI_Start_C_Wrapper (request);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_Start (request);
}

/******************************************************************************
 ***  MPI_Startall
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Startall) (int count, MPI_Request *requests)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
    res = MPI_Startall_C_Wrapper (count, requests);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
  else
    return PMPI_Startall (count, requests);
}

/******************************************************************************
 ***  MPI_Request_free
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Request_free) (MPI_Request *request)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res =  MPI_Request_free_C_Wrapper (request);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_Request_free (request);
}

/******************************************************************************
 ***  MPI_Recv_init
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Recv_init) (void *buf, int count, MPI_Datatype datatype,
	int source, int tag, MPI_Comm comm, MPI_Request *request)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Recv_init_C_Wrapper
		  (buf, count, datatype, source, tag, comm, request);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_Recv_init
		  (buf, count, datatype, source, tag, comm, request);
}

/******************************************************************************
 ***  MPI_Send_init
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Send_init) (void *buf, int count, MPI_Datatype datatype,
	int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Send_init_C_Wrapper
		  (buf, count, datatype, dest, tag, comm, request);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_Send_init (buf, count, datatype, dest, tag, comm, request);
}

/******************************************************************************
 ***  MPI_Bsend_init
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Bsend_init) (void *buf, int count, MPI_Datatype datatype,
	int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Bsend_init_C_Wrapper
		  (buf, count, datatype, dest, tag, comm, request);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_Bsend_init (buf, count, datatype, dest, tag, comm, request);
}


/******************************************************************************
 ***  MPI_Rsend_init
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Rsend_init) (void *buf, int count, MPI_Datatype datatype,
	int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Rsend_init_C_Wrapper
		  (buf, count, datatype, dest, tag, comm, request);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_Rsend_init (buf, count, datatype, dest, tag, comm, request);
}

/******************************************************************************
 ***  MPI_Ssend_init
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Ssend_init) (void *buf, int count, MPI_Datatype datatype,
	int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Ssend_init_C_Wrapper
		  (buf, count, datatype, dest, tag, comm, request);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_Ssend_init (buf, count, datatype, dest, tag, comm, request);
}

/******************************************************************************
 ***  MPI_Scan
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Scan) (void *sendbuf, void *recvbuf, int count,
	MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
	int res;
	int sizeofcomm;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Scan_enter (sendbuf, recvbuf, count, datatype, op, comm);
#endif

	PMPI_Comm_size (comm, &sizeofcomm);
	if (sizeofcomm == NumOfTasks)
	{
        CurrentOpGlobal = (++NumOpsGlobals);

#if USE_HARDWARE_COUNTERS
		if (mpitrace_on && tracejant)
			HARDWARE_COUNTERS_CHANGE (TIME,  CHANGE_GLOPS, THREADID );
#endif

		if (CheckForControlFile)
			CheckControlFile();
		if (CheckForGlobalOpsTracingIntervals)
			CheckGlobalOpsTracingIntervals();
	}
	else CurrentOpGlobal = 0;

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Scan_C_Wrapper (sendbuf, recvbuf, count, datatype, op, comm);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Scan (sendbuf, recvbuf, count, datatype, op, comm);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Scan_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Sendrecv
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Sendrecv) (void *sendbuf, int sendcount,
	MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount,
	MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm,
	MPI_Status * status) 
{
	int res;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Sendrecv_enter (sendbuf, sendcount, sendtype, dest, sendtag,
		recvbuf, recvcount, recvtype, source, recvtag, comm, status);
#endif

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Sendrecv_C_Wrapper (sendbuf, sendcount, sendtype, dest, sendtag,
		  recvbuf, recvcount, recvtype, source, recvtag, comm, status); 
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Sendrecv (sendbuf, sendcount, sendtype, dest, sendtag,
		  recvbuf, recvcount, recvtype, source, recvtag, comm, status); 

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Sendrecv_leave ();
#endif

	return res;
}

/******************************************************************************
 ***  MPI_Sendrecv_replace
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_Sendrecv_replace) (void *buf, int count, MPI_Datatype type,
	int dest, int sendtag, int source, int recvtag, MPI_Comm comm,
	MPI_Status* status)
{
	int res;

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Sendrecv_replace_enter (buf, count, type, dest, sendtag, source,
		recvtag, comm, status);
#endif

	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_Sendrecv_replace_C_Wrapper (buf, count, type, dest, sendtag,
		  source, recvtag, comm, status);
		DEBUG_INTERFACE(LEAVE)
	}
	else
		res = PMPI_Sendrecv_replace (buf, count, type, dest, sendtag, source,
		  recvtag, comm, status);

#if defined(ENABLE_LOAD_BALANCING)
	DLB_MPI_Sendrecv_replace_leave ();
#endif

	return res;
}

#if defined(MPI_SUPPORTS_MPI_IO)

/******************************************************************************
 ***  MPI_File_open
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_File_open) (MPI_Comm comm, char * filename, int amode,
	MPI_Info info, MPI_File *fh)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_File_open_C_Wrapper (comm, filename, amode, info, fh);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_File_open (comm, filename, amode, info, fh);
}

/******************************************************************************
 ***  MPI_File_read_all
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_File_close) (MPI_File* fh)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_File_close_C_Wrapper (fh);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_File_close (fh);
}

/******************************************************************************
 ***  MPI_File_read_all
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_File_read) (MPI_File fh, void* buf, int count,
	MPI_Datatype datatype, MPI_Status* status)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res =  MPI_File_read_C_Wrapper (fh, buf, count, datatype, status);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_File_read (fh, buf, count, datatype, status);
}

/******************************************************************************
 ***  MPI_File_read_all
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_File_read_all) (MPI_File fh, void* buf, int count,
	MPI_Datatype datatype, MPI_Status* status)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_File_read_all_C_Wrapper (fh, buf, count, datatype, status);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_File_read_all (fh, buf, count, datatype, status);
}

/******************************************************************************
 ***  MPI_File_write
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_File_write) (MPI_File fh, void * buf, int count,
	MPI_Datatype datatype, MPI_Status* status)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_File_write_C_Wrapper (fh, buf, count, datatype, status);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_File_write (fh, buf, count, datatype, status);
}

/******************************************************************************
 ***  MPI_File_write_all
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_File_write_all) (MPI_File fh, void* buf, int count, 
	MPI_Datatype datatype, MPI_Status* status)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_File_write_all_C_Wrapper (fh, buf, count, datatype, status);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_File_write_all (fh, buf, count, datatype, status);
}

/******************************************************************************
 ***  MPI_File_read_at
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_File_read_at) (MPI_File fh, MPI_Offset offset, void* buf,
	int count, MPI_Datatype datatype, MPI_Status* status)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_File_read_at_C_Wrapper (fh, offset, buf, count, datatype, status);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_File_read_at (fh, offset, buf, count, datatype, status);
}

/******************************************************************************
 ***  MPI_File_read_at_all
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_File_read_at_all) (MPI_File fh, MPI_Offset offset,
	void* buf, int count, MPI_Datatype datatype, MPI_Status* status)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_File_read_at_all_C_Wrapper (fh, offset, buf, count, datatype, status);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_File_read_at_all (fh, offset, buf, count, datatype, status);
}

/******************************************************************************
 ***  MPI_File_write_at
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_File_write_at) (MPI_File fh, MPI_Offset offset, void * buf,
	int count, MPI_Datatype datatype, MPI_Status* status)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_File_write_at_C_Wrapper (fh, offset, buf, count, datatype, status);
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_File_write_at (fh, offset, buf, count, datatype, status);
}


/******************************************************************************
 ***  MPI_File_write_at_all
 ******************************************************************************/
int NAME_ROUTINE_C(MPI_File_write_at_all) (MPI_File fh, MPI_Offset offset,
	void* buf, int count, MPI_Datatype datatype, MPI_Status* status)
{
	int res;
	if (mpitrace_on)
	{
		DEBUG_INTERFACE(ENTER)
		res = MPI_File_write_at_all_C_Wrapper (fh, offset, buf, count, datatype, status);	
		DEBUG_INTERFACE(LEAVE)
		return res;
	}
	else
		return PMPI_File_write_at_all (fh, offset, buf, count, datatype, status);
}

#endif /* MPI_SUPPORTS_MPI_IO */

#endif /* defined(C_SYMBOLS) */

/**************************************************************************
 **
 ** Interfaces to gather network routes and counters!
 **
 **************************************************************************/

#include "misc_interface.h"

#if defined(C_SYMBOLS)

# if defined(HAVE_ALIAS_ATTRIBUTE)

INTERFACE_ALIASES_C(trace_network_counters, MPItrace_network_counters, (void))
void MPItrace_network_counters (void)
{
	if (mpitrace_on)
		OMPItrace_network_counters_Wrapper ();
}

INTERFACE_ALIASES_C(trace_network_routes, MPItrace_network_routes, (int mpi_rank))
void MPItrace_network_routes (int mpi_rank)
{
	if (mpitrace_on)
		OMPItrace_network_routes_Wrapper (mpi_rank);
}

INTERFACE_ALIASES_C(trace_set_tracing_tasks, MPItrace_set_tracing_tasks, (int from, int to))
void MPItrace_set_tracing_tasks (int from, int to)
{
	if (mpitrace_on)
		OMPItrace_tracing_tasks_Wrapper (from, to);
}

# else /* HAVE_ALIAS_ATTRIBUTE */

/*** FORTRAN BINDINGS + non alias routine duplication ****/
 
# define apiTRACE_NETWORK_ROUTES(x) \
    void x##trace_network_routes (int mpi_rank) \
   { \
    if (mpitrace_on) \
        OMPItrace_network_routes_Wrapper (mpi_rank); \
   }
EXPAND_ROUTINE_WITH_PREFIXES(apiTRACE_NETWORK_ROUTES);

# define apiTRACE_USER_FUNCTION(x) \
	void x##trace_user_function (int enter) \
	{ \
		if (mpitrace_on) \
			OMPItrace_user_function_Wrapper (enter); \
	}
EXPAND_ROUTINE_WITH_PREFIXES(apiTRACE_USER_FUNCTION);

#define apiTRACE_SETTRACINGTASKS(x) \
	void x##trace_set_tracing_tasks (int from, int to) \
   { \
   	if (mpitrace_on) \
      	OMPItrace_tracing_tasks_Wrapper (from, to); \
   }
EXPAND_ROUTINE_WITH_PREFIXES(apiTRACE_SETTRACINGTASKS);

# endif /* HAVE_ALIAS_ATTRIBUTE */

#endif /* defined(C_SYMBOLS) */


#if defined(FORTRAN_SYMBOLS)

# if defined(HAVE_ALIAS_ATTRIBUTE)

INTERFACE_ALIASES_F(trace_network_counters,TRACE_NETWORK_COUNTERS,mpitrace_network_counters, (void))
void mpitrace_network_counters (void)
{
	if (mpitrace_on)
		OMPItrace_network_counters_Wrapper ();
}

INTERFACE_ALIASES_F(trace_network_routes,TRACE_NETWORK_ROUTES,mpitrace_network_routes, (int *mpi_rank))
void mpitrace_network_routes (int *mpi_rank)
{
	if (mpitrace_on)
		OMPItrace_network_routes_Wrapper (*mpi_rank);
}

INTERFACE_ALIASES_F(trace_set_tracing_tasks,TRACE_SET_TRACING_TASKS,mpitrace_set_tracing_tasks, (int *from, int *to))
void mpitrace_set_tracing_tasks (int *from, int *to)
{
	if (mpitrace_on)
		OMPItrace_tracing_tasks_Wrapper (*from, *to);
}

# else /* HAVE_ALIAS_ATTRIBUTE */

#  define apifTRACE_NETWORK_COUNTERS(x) \
	void CtoF77(x##trace_network_counters) () \
	{ \
		if (mpitrace_on) \
			OMPItrace_network_counters_Wrapper (); \
	}
EXPAND_F_ROUTINE_WITH_PREFIXES(apifTRACE_NETWORK_COUNTERS);

#  define apifTRACE_NETWORK_ROUTES(x) \
	void CtoF77(x##trace_network_routes) (int *mpi_rank) \
	{ \
		if (mpitrace_on) \
			OMPItrace_network_routes_Wrapper (*mpi_rank); \
	}
EXPAND_F_ROUTINE_WITH_PREFIXES(apifTRACE_NETWORK_ROUTES);

#define apifTRACE_SETTRACINGTASKS(x) \
	void CtoF77(x##trace_set_tracing_tasks) (int *from, int *to) \
	{ \
		if (mpitrace_on) \
			OMPItrace_tracing_tasks_Wrapper (*from, *to); \
	}
EXPAND_F_ROUTINE_WITH_PREFIXES(apifTRACE_SETTRACINGTASKS)

# endif /* HAVE_ALIAS_ATTRIBUTE */

#endif /* defined(FORTRAN_SYMBOLS) */

