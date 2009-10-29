/*****************************************************************************\
 *                        ANALYSIS PERFORMANCE TOOLS                         *
 *                                 MPItrace                                  *
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

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_STDIO_H
# include <stdio.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifdef HAVE_TIME_H
# include <time.h>
#endif
#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif

#include "events.h"
#include "labels.h"
#include "mpi_prv_events.h"
#include "omp_prv_events.h"
#include "pthread_prv_events.h"
#include "misc_prv_events.h"
#include "trace_mode.h"
#include "addr2info.h" 
#include "mpi2out.h"

#include "HardwareCounters.h"
#include "queue.h"

#define MAX_SYMBOL_NAME 150

typedef struct callback_t
{
  unsigned long long call_id;
  char name[MAX_SYMBOL_NAME];
  char module[MAX_SYMBOL_NAME];
}
callback_t;

typedef struct symbol_t
{
  unsigned long long address;
  char name[MAX_SYMBOL_NAME];
  char module[MAX_SYMBOL_NAME];
}
symbol_t;

#define MAX_CALLBACKS  1000
struct callback_t call_table[MAX_CALLBACKS];
long call_num = 0;

#define MAX_SYMBOLS 50000
struct symbol_t sym_table[MAX_SYMBOLS];
long sym_num = 0;

struct evttype_t MISC_events[MISC] = {
  {APPL_EV, APPL_LBL},
  {FLUSH_EV, FLUSH_LBL},
  {READ_EV, READ_LBL},
  {WRITE_EV, WRITE_LBL}
};

struct value_t MISC_values[MISC_VALUES] = {
  {EVT_BEGIN, EVT_BEGIN_LBL},
  {EVT_END, EVT_END_LBL}
};

struct color_t states_inf[STATES_NUMBER] = {
  {STATE_0, STATE0_LBL, STATE0_COLOR},
  {STATE_1, STATE1_LBL, STATE1_COLOR},
  {STATE_2, STATE2_LBL, STATE2_COLOR},
  {STATE_3, STATE3_LBL, STATE3_COLOR},
  {STATE_4, STATE4_LBL, STATE4_COLOR},
  {STATE_5, STATE5_LBL, STATE5_COLOR},
  {STATE_6, STATE6_LBL, STATE6_COLOR},
  {STATE_7, STATE7_LBL, STATE7_COLOR},
  {STATE_8, STATE8_LBL, STATE8_COLOR},
  {STATE_9, STATE9_LBL, STATE9_COLOR},
  {STATE_10, STATE10_LBL, STATE10_COLOR},
  {STATE_11, STATE11_LBL, STATE11_COLOR},
  {STATE_12, STATE12_LBL, STATE12_COLOR},
  {STATE_13, STATE13_LBL, STATE13_COLOR},
  {STATE_14, STATE14_LBL, STATE14_COLOR},
  {STATE_15, STATE15_LBL, STATE15_COLOR},
  {STATE_16, STATE16_LBL, STATE16_COLOR}
};

struct color_t gradient_inf[GRADIENT_NUMBER] = {
  {GRADIENT_0, GRADIENT0_LBL, GRADIENT0_COLOR},
  {GRADIENT_1, GRADIENT1_LBL, GRADIENT1_COLOR},
  {GRADIENT_2, GRADIENT2_LBL, GRADIENT2_COLOR},
  {GRADIENT_3, GRADIENT3_LBL, GRADIENT3_COLOR},
  {GRADIENT_4, GRADIENT4_LBL, GRADIENT4_COLOR},
  {GRADIENT_5, GRADIENT5_LBL, GRADIENT5_COLOR},
  {GRADIENT_6, GRADIENT6_LBL, GRADIENT6_COLOR},
  {GRADIENT_7, GRADIENT7_LBL, GRADIENT7_COLOR},
  {GRADIENT_8, GRADIENT8_LBL, GRADIENT8_COLOR},
  {GRADIENT_9, GRADIENT9_LBL, GRADIENT9_COLOR},
  {GRADIENT_10, GRADIENT10_LBL, GRADIENT10_COLOR},
  {GRADIENT_11, GRADIENT11_LBL, GRADIENT11_COLOR},
  {GRADIENT_12, GRADIENT12_LBL, GRADIENT12_COLOR},
  {GRADIENT_13, GRADIENT13_LBL, GRADIENT13_COLOR},
  {GRADIENT_14, GRADIENT14_LBL, GRADIENT14_COLOR}
};

struct rusage_evt_t rusage_evt_labels[RUSAGE_EVENTS_COUNT] = {
   { RUSAGE_UTIME_EV, RUSAGE_UTIME_LBL },
   { RUSAGE_STIME_EV, RUSAGE_STIME_LBL },
   { RUSAGE_MAXRSS_EV,   RUSAGE_MAXRSS_LBL },
   { RUSAGE_IXRSS_EV,    RUSAGE_IXRSS_LBL },
   { RUSAGE_IDRSS_EV,    RUSAGE_IDRSS_LBL },
   { RUSAGE_ISRSS_EV,    RUSAGE_ISRSS_LBL },
   { RUSAGE_MINFLT_EV,   RUSAGE_MINFLT_LBL },
   { RUSAGE_MAJFLT_EV,   RUSAGE_MAJFLT_LBL },
   { RUSAGE_NSWAP_EV,    RUSAGE_NSWAP_LBL },
   { RUSAGE_INBLOCK_EV,  RUSAGE_INBLOCK_LBL },
   { RUSAGE_OUBLOCK_EV,  RUSAGE_OUBLOCK_LBL },
   { RUSAGE_MSGSND_EV,   RUSAGE_MSGSND_LBL },
   { RUSAGE_MSGRCV_EV,   RUSAGE_MSGRCV_LBL },
   { RUSAGE_NSIGNALS_EV, RUSAGE_NSIGNALS_LBL },
   { RUSAGE_NVCSW_EV,    RUSAGE_NVCSW_LBL },
   { RUSAGE_NIVCSW_EV,   RUSAGE_NIVCSW_LBL }
};

struct mpistats_evt_t mpistats_evt_labels[MPI_STATS_EVENTS_COUNT] = {
   { MPI_STATS_P2P_COMMS_EV, MPI_STATS_P2P_COMMS_LBL },
   { MPI_STATS_P2P_BYTES_SENT_EV, MPI_STATS_P2P_BYTES_SENT_LBL },
   { MPI_STATS_P2P_BYTES_RECV_EV, MPI_STATS_P2P_BYTES_RECV_LBL },
   { MPI_STATS_GLOBAL_COMMS_EV, MPI_STATS_GLOBAL_COMMS_LBL },
   { MPI_STATS_GLOBAL_BYTES_SENT_EV, MPI_STATS_GLOBAL_BYTES_SENT_LBL },
   { MPI_STATS_GLOBAL_BYTES_RECV_EV, MPI_STATS_GLOBAL_BYTES_RECV_LBL },
   { MPI_STATS_TIME_IN_MPI_EV, MPI_STATS_TIME_IN_MPI_LBL }
};

/******************************************************************************
 ***  state_labels
 ******************************************************************************/
static void Paraver_state_labels (FILE * fd)
{
  int i;

  fprintf (fd, "%s\n", STATES_LBL);
  for (i = 0; i < STATES_NUMBER; i++)
  {
    fprintf (fd, "%d    %s\n", states_inf[i].value, states_inf[i].label);
  }

  LET_SPACES (fd);
}


/******************************************************************************
 ***  state_colors
 ******************************************************************************/
static void Paraver_state_colors (FILE * fd)
{
  int i;

  fprintf (fd, "%s\n", STATES_COLOR_LBL);
  for (i = 0; i < STATES_NUMBER; i++)
  {
    fprintf (fd, "%d    {%d,%d,%d}\n", states_inf[i].value,
             states_inf[i].rgb[0], states_inf[i].rgb[1],
             states_inf[i].rgb[2]);
  }

  LET_SPACES (fd);
}

/******************************************************************************
 ***  gradient_colors
 ******************************************************************************/
static void Paraver_gradient_colors (FILE * fd)
{
  int i;

  fprintf (fd, "%s\n", GRADIENT_COLOR_LBL);
  for (i = 0; i < GRADIENT_NUMBER; i++)
  {
    fprintf (fd, "%d    {%d,%d,%d}\n", gradient_inf[i].value,
             gradient_inf[i].rgb[0],
             gradient_inf[i].rgb[1], gradient_inf[i].rgb[2]);
  }

  LET_SPACES (fd);
}

/******************************************************************************
 ***  gradient_names
 ******************************************************************************/
static void Paraver_gradient_names (FILE * fd)
{
  int i;

  fprintf (fd, "%s\n", GRADIENT_LBL);
  for (i = 0; i < GRADIENT_NUMBER; i++)
    fprintf (fd, "%d    %s\n", gradient_inf[i].value, gradient_inf[i].label);

  LET_SPACES (fd);
}

/******************************************************************************
 ***  MISC_events_group1
 ******************************************************************************/

void MISC_events_group1 (FILE * fd)
{
  unsigned int event, value;

  fprintf (fd, "%s\n", TYPE_LABEL);
  for (event = 0; event < MISC; event++)
  {
    fprintf (fd, "%d    %d    %s\n", MISC_GRADIENT, MISC_events[event].type,
             MISC_events[event].label);
  }

  fprintf (fd, "%s\n", VALUES_LABEL);
  for (value = 0; value < MISC_VALUES; value++)
  {
    fprintf (fd, "%d      %s\n", MISC_values[value].value,
             MISC_values[value].label);
  }
  LET_SPACES (fd);

  fprintf (fd, "%s\n", TYPE_LABEL);
  fprintf (fd, "%d    %d    %s\n", MISC_GRADIENT, TRACING_EV, TRACING_LBL);

  fprintf (fd, "%s\n", VALUES_LABEL);
  fprintf (fd, "%d      %s\n", EVT_END, TRAC_DISABLED_LBL);
  fprintf (fd, "%d      %s\n", EVT_BEGIN, TRAC_ENABLED_LBL);
  LET_SPACES (fd);

  fprintf (fd, "%s\n", TYPE_LABEL);
  fprintf (fd, "%d    %d    %s\n", MISC_GRADIENT, IOSIZE_EV, IOSIZE_LBL);
  LET_SPACES (fd);

#if defined(DEAD_CODE)
#if defined(IS_BG_MACHINE)
  fprintf (fd, "%s\n", TYPE_LABEL);
  fprintf (fd, "%d    %d    %s\n", MISC_GRADIENT, BGL_PERSONALITY_TORUS_X,
           BGL_TORUS_X);
  fprintf (fd, "%d    %d    %s\n", MISC_GRADIENT, BGL_PERSONALITY_TORUS_Y,
           BGL_TORUS_Y);
  fprintf (fd, "%d    %d    %s\n", MISC_GRADIENT, BGL_PERSONALITY_TORUS_Z,
           BGL_TORUS_Z);
  fprintf (fd, "%d    %d    %s\n", MISC_GRADIENT,
           BGL_PERSONALITY_PROCESSOR_ID, BGL_PROCESSOR_ID);
  LET_SPACES (fd);
#endif
#endif /* DEAD_CODE */

  fprintf (fd, "%s\n", TYPE_LABEL);
  fprintf (fd, "%d    %d    %s\n", MPI_GRADIENT, MPI_GLOBAL_OP_SENDSIZE,
           MPI_GLOBAL_OP_SENDSIZE_LBL);
  fprintf (fd, "%d    %d    %s\n", MPI_GRADIENT, MPI_GLOBAL_OP_RECVSIZE,
           MPI_GLOBAL_OP_RECVSIZE_LBL);
  fprintf (fd, "%d    %d    %s\n", MPI_GRADIENT, MPI_GLOBAL_OP_ROOT,
           MPI_GLOBAL_OP_ROOT_LBL);
  fprintf (fd, "%d    %d    %s\n", MPI_GRADIENT, MPI_GLOBAL_OP_COMM,
           MPI_GLOBAL_OP_COMM_LBL);
  LET_SPACES (fd);

}

/******************************************************************************
 *** concat_user_labels
 ******************************************************************************/
void Concat_User_Labels (FILE * fd)
{
  char *str;
  char line[1024];
  FILE *labels;

  if ((str = getenv ("MPTRACE_LABELS")) != NULL)
  {
    labels = fopen (str, "r");
    if (labels == NULL)
      return;

    fprintf (fd, "\n");
    while (fscanf (labels, "%[^\n]\n", line) != EOF)
    {
      if (strlen (line) == 0)
      {
        line[0] = fgetc (labels);
        fprintf (fd, "%s\n", line);
        continue;
      }
      fprintf (fd, "%s\n", line);
    }
    fclose (labels);
    fprintf (fd, "\n");

  }
}

/******************************************************************************
 *** PARAVER_default_options
 ******************************************************************************/
static void Paraver_default_options (FILE * fd)
{
	fprintf (fd, "DEFAULT_OPTIONS\n\n");
	fprintf (fd, "LEVEL               %s\n", DEFAULT_LEVEL);
	fprintf (fd, "UNITS               %s\n", DEFAULT_UNITS);
	fprintf (fd, "LOOK_BACK           %d\n", DEFAULT_LOOK_BACK);
	fprintf (fd, "SPEED               %d\n", DEFAULT_SPEED);
	fprintf (fd, "FLAG_ICONS          %s\n", DEFAULT_FLAG_ICONS);
	fprintf (fd, "NUM_OF_STATE_COLORS %d\n", DEFAULT_NUM_OF_STATE_COLORS);
	fprintf (fd, "YMAX_SCALE          %d\n", DEFAULT_YMAX_SCALE);

	LET_SPACES (fd);

	fprintf (fd, "DEFAULT_SEMANTIC\n\n");
	fprintf (fd, "THREAD_FUNC          %s\n", DEFAULT_THREAD_FUNC);

	LET_SPACES (fd);
}

#if USE_HARDWARE_COUNTERS
int Exist_contador (fcounter_t *fcounter, long long EvCnt) 
{
  struct fcounter_t *aux_fc = fcounter;

  while (aux_fc != NULL)
  {
    if (aux_fc->counter == EvCnt)
      return 1;
    else
      aux_fc = aux_fc->prev;
  }
  return 0;
}


/******************************************************************************
 *** HWC_PARAVER_Labels
******************************************************************************/

void HWC_PARAVER_Labels (FILE * pcfFD)
{
#if defined(PAPI_COUNTERS)
  struct fcounter_t *fcounter=NULL;
# if defined (PAPIv3)
	PAPI_event_info_t Myinfo;
# elif defined (PAPIv2)
	PAPI_preset_info_t Myinfo;
# endif
#elif defined(PMAPI_COUNTERS)
	pm_info2_t ProcessorMetric_Info; /* On AIX pre 5.3 it was pm_info_t */
	pm_groups_info_t HWCGroup_Info;
	pm_events2_t *evp = NULL;
	int j;
#endif
	int EvCnt;
	int cnt = 0;
	int rc;
	int AddedCounters = 0;
	CntQueue *queue;
	CntQueue *ptmp;


#if defined(PAPI_COUNTERS)
	/* PAPI Initialization */
	rc = PAPI_library_init (PAPI_VER_CURRENT);
	if (rc != PAPI_VER_CURRENT)
	{
		if (rc > 0)
			fprintf (stderr, "mpi2prv: ERROR PAPI library version mismatch!\n");
		fprintf (stderr, "mpi2prv: Can't use hardware counters!\n");
		fprintf (stderr, "mpi2prv: Papi library error: %s\n", PAPI_strerror (rc));
		if (rc == PAPI_ESYS)
			perror ("mpi2prv: The error is:");
		return;
	}
#elif defined(PMAPI_COUNTERS)
	rc = pm_initialize (PM_VERIFIED|PM_UNVERIFIED|PM_CAVEAT|PM_GET_GROUPS, &ProcessorMetric_Info, &HWCGroup_Info, PM_CURRENT);
	if (rc != 0)
		pm_error ("pm_initialize", rc);
#endif

	queue = &CountersTraced;

	for (ptmp = (queue)->prev; ptmp != (queue); ptmp = ptmp->prev)
	{
		for (cnt = 0; cnt < MAX_HWC; cnt++)
		{
			if (ptmp->Traced[cnt])
			{
				EvCnt = ptmp->Events[cnt];

#if defined(PAPI_COUNTERS)
# if defined (PAPIv3)
				if (PAPI_get_event_info (EvCnt,&Myinfo) == PAPI_OK)
# elif defined (PAPIv2)
				if (PAPI_query_event_verbose (EvCnt, &Myinfo) == PAPI_OK)
# endif
				{
					if (!(Exist_contador(fcounter,EvCnt)))
					{
						if (AddedCounters == 0)
							fprintf (pcfFD, "%s\n", TYPE_LABEL);

						AddedCounters++;

						INSERTAR_CONTADOR (fcounter, EvCnt);

# if defined (PAPIv3)
						fprintf (pcfFD, "%d  %d %s (%s)\n", 7, HWC_COUNTER_TYPE(EvCnt),
							Myinfo.short_descr, Myinfo.symbol );
# elif defined (PAPIv2)
						fprintf (pcfFD, "%d  %d %s (%s)\n", 7, HWC_COUNTER_TYPE(EvCnt),
							Myinfo.event_descr, Myinfo.event_label);
# endif
					}
				}
#elif defined(PMAPI_COUNTERS)
				/* find pointer to the event */
				for (j = 0; j < ProcessorMetric_Info.maxevents[cnt]; j++)
				{ 
					evp = ProcessorMetric_Info.list_events[cnt]+j;  
					if (EvCnt == evp->event_id)
						break;    
				}
				if (evp != NULL)
				{
					if (AddedCounters == 0)
						fprintf (pcfFD, "%s\n", TYPE_LABEL);

					fprintf (pcfFD, "%d  %lld %s (%s)\n", 7, (long long)HWC_COUNTER_TYPE(cnt, EvCnt), evp->short_name, evp->long_name);
					AddedCounters++;
				}
#endif
			}
		}
	}

	if (AddedCounters > 0)
		fprintf (pcfFD, "%d  %d %s\n", 7, HWC_GROUP_ID, "Active hardware counter set");

	LET_SPACES (pcfFD);
}
#endif

char * Rusage_Event_Label (int rusage_evt) {
   int i;

   for (i=0; i<RUSAGE_EVENTS_COUNT; i++) {
      if (rusage_evt_labels[i].evt_type == rusage_evt) {
         return rusage_evt_labels[i].label;
      }
   }
   return "Unknown getrusage event";
}

void Write_rusage_Labels (FILE * pcf_fd)
{
   int i;

   if (Rusage_Events_Found) {
      fprintf (pcf_fd, "%s\n", TYPE_LABEL);

      for (i=0; i<RUSAGE_EVENTS_COUNT; i++) {
         if (GetRusage_Labels_Used[i]) {
            fprintf(pcf_fd, "0    %d    %s\n", RUSAGE_BASE+i, Rusage_Event_Label(i));
         }
      }
      LET_SPACES (pcf_fd);
   }
}

char * MPIStats_Event_Label (int mpistats_evt)
{
   int i;

   for (i=0; i<MPI_STATS_EVENTS_COUNT; i++)
   {
      if (mpistats_evt_labels[i].evt_type == mpistats_evt) {
         return mpistats_evt_labels[i].label;
      }
   }
   return "Unknown MPI stats event";
}

void Write_MPIStats_Labels (FILE * pcf_fd)
{
   int i;

   if (MPIStats_Events_Found)
   {
      fprintf (pcf_fd, "%s\n", TYPE_LABEL);

      for (i=0; i<MPI_STATS_EVENTS_COUNT; i++) {
         if (MPIStats_Labels_Used[i]) {
            fprintf(pcf_fd, "0    %d    %s\n", MPI_STATS_BASE+i, MPIStats_Event_Label(i));
         }
      }
      LET_SPACES (pcf_fd);
   }
}

void Write_Trace_Mode_Labels (FILE * pcf_fd)
{
	fprintf (pcf_fd, "%s\n", TYPE_LABEL);
	fprintf (pcf_fd, "9    %d    %s\n", TRACING_MODE_EV, "Tracing mode:");
	fprintf (pcf_fd, "%s\n", VALUES_LABEL);
	fprintf (pcf_fd, "%d      %s\n", TRACE_MODE_DETAIL, "Detailed");
	fprintf (pcf_fd, "%d      %s\n", TRACE_MODE_BURSTS, "CPU Bursts");
	LET_SPACES (pcf_fd);
}

void Write_Clustering_Labels (FILE * pcf_fd)
{
	int i;

	if (MaxClusterId > 0)
	{
		fprintf (pcf_fd, "%s\n", TYPE_LABEL);
		fprintf (pcf_fd, "9    %d    %s\n", CLUSTER_ID_EV, CLUSTER_ID_LABEL);
		fprintf (pcf_fd, "%s\n", VALUES_LABEL);
		fprintf (pcf_fd, "0   End\n");
		fprintf (pcf_fd, "1   Missing Data\n");
		fprintf (pcf_fd, "2   Duration Filtered\n");
		fprintf (pcf_fd, "3   Range Filtered\n");
		fprintf (pcf_fd, "4   Threshold Filtered\n");
		fprintf (pcf_fd, "5   Noise\n");
		for (i=6; i<=MaxClusterId; i++)
		{
			fprintf (pcf_fd, "%d   Cluster %d\n", i, i-5);
		}
		LET_SPACES (pcf_fd);
	}
}

#ifdef HAVE_BFD
/******************************************************************************
 *** loadSYMfile
 ******************************************************************************/
void loadSYMfile (char *name)
{
	FILE *FD;
	char LINE[1024], fname[1024], modname[1024], Type;
	UINT64 address;
	int line, count = 0;

	if (!name)
		return;

	if (strlen(name) == 0)
		return;

	FD = (FILE *) fopen (name, "r");
	if (FD == NULL)
	{
		fprintf (stderr, "WARNING: Can\'t find file %s\n", name);
		return;
	}

	while (!feof (FD))
	{
		if (fgets (LINE, 1024, FD) == NULL)
			break;

		/* Example of line: U 0x100016d4 fA mpi_test.c 0 */

		sscanf (LINE, "%[^\n]\n", LINE);

		sscanf (LINE, "%c %llx %s %s %d", &Type, &address, fname, modname, &line);

    switch (Type)
    {
      case 'U':
      case 'P':
        Address2Info_AddSymbol (address, (Type=='U')?USER_FUNCTION_TYPE:OUTLINED_OPENMP_TYPE, fname, modname, line);
				count++;
        break;
			default:
				fprintf (stderr, "Error! Unexpected line in symbol file\n%s\n", LINE);
				break;
		}
	}

	fprintf (stdout, "mpi2prv: %d symbols successfully imported from %s file\n", count, name);

	fclose (FD);
}
#endif /* HAVE_BFD */

/******************************************************************************
 *** generatePCFfile
 ******************************************************************************/

int generatePCFfile (char *name)
{
	FILE *fd;

	fd = fopen (name, "w");
	if (fd == NULL)
		return -1;

	Paraver_default_options (fd);

	Paraver_state_labels (fd);
	Paraver_state_colors (fd);

	MISC_events_group1 (fd);

	MPITEvent_WriteEnabledOperations (fd);
	MPISoftCountersEvent_WriteEnabledOperations (fd);
	OMPEvent_WriteEnabledOperations (fd);
	pthreadEvent_WriteEnabledOperations (fd);
	MISCEvent_WriteEnabledOperations (fd);

	Concat_User_Labels (fd);

#if USE_HARDWARE_COUNTERS
	HWC_PARAVER_Labels (fd);
#endif

	Paraver_gradient_colors (fd);
	Paraver_gradient_names (fd);

#ifdef HAVE_BFD
	Address2Info_Write_MPI_Labels (fd, option_UniqueCallerID);
	Address2Info_Write_UF_Labels (fd, option_UniqueCallerID);
	Address2Info_Write_Sample_Labels (fd, option_UniqueCallerID);
#endif
	Write_rusage_Labels (fd);
	Write_MPIStats_Labels (fd);
	Write_Trace_Mode_Labels (fd);
	Write_Clustering_Labels (fd);

	fclose(fd);
    
	return 0;
}
