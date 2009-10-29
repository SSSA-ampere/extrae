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

#ifndef __BUFFER_H__
#define __BUFFER_H__

#define MASK_NOFLUSH   (1 << 0)
#define MASK_CLUSTERED (1 << 1)



#include <sys/uio.h>
#if defined(HAVE_MRNET)
#include <pthread.h>
#endif
#include "record.h"

#define LOCK_AT_INSERT 1
//#define LOCK_AT_FLUSH 1

typedef int Mask_t; 

typedef struct Buffer
{
	int MaxEvents;
    int FillCount;
    event_t *FirstEvt;
    event_t *LastEvt;
    event_t *HeadEvt;
    event_t *CurEvt;

    int fd;

#if defined(HAVE_MRNET) 
    pthread_mutex_t Lock;
#endif
#if defined(MASKS_DEAD_CODE)
    int *Mask;
#endif /* MASKS_DEAD_CODE */
    Mask_t *NewMasks;

	int (*FlushCallback)(struct Buffer *);

} Buffer_t;

typedef struct
{
   Buffer_t *Buffer;
   int OutOfBounds;

   event_t *CurrentElement;
   event_t *StartBound;
   event_t *EndBound;
} BufferIterator_t;

typedef struct
{
    event_t *FirstAddr;
    event_t *LastAddr;
    int MaxBlocks;
    int NumBlocks;
    struct iovec *BlocksList;
} DataBlocks_t;

#define BLOCKS_CHUNK 50
#ifndef IOV_MAX
#define IOV_MAX 512
#endif

#define CALLBACK_FLUSH Buffer_Flush
#define CALLBACK_OVERWRITE Buffer_DiscardOldest
#define CALLBACK_DISCARD Buffer_DiscardAll

/*
enum {
    OVERWRITE,
    FLUSH,
    DISCARD,
	SYNC_FLUSH
};
*/

#define MAX_MULTIPLE_EVENTS 50

#if defined(__cplusplus)
extern "C" {
#endif

Buffer_t * new_Buffer (int n_events, char *file);
void Buffer_Free (Buffer_t *buffer);
unsigned long long Buffer_GetFileSize (Buffer_t *buffer);
void Buffer_SetFlushCallback (Buffer_t *buffer, int (*callback)(struct Buffer *));
int Buffer_ExecuteFlushCallback (Buffer_t *buffer);
void Buffer_Close (Buffer_t *buffer);
int Buffer_IsClosed (Buffer_t *buffer);
int Buffer_IsEmpty (Buffer_t *buffer);
int Buffer_IsFull (Buffer_t *buffer);
event_t * Buffer_GetHead (Buffer_t *buffer);
event_t * Buffer_GetTail (Buffer_t *buffer);
int Buffer_GetFillCount (Buffer_t *buffer);
int Buffer_RemainingEvents (Buffer_t *buffer);
int Buffer_EnoughSpace (Buffer_t *buffer, int num_events);
event_t * Buffer_GetNext (Buffer_t *buffer, event_t *current);
#if defined(MASKS_DEAD_CODE)
void Buffer_ClearMask (Buffer_t *buffer);
void Buffer_MaskIn (Buffer_t *buffer, event_t *evt);
void Buffer_MaskOut (Buffer_t *buffer, event_t *evt);
int Buffer_IsMaskedOut (Buffer_t *buffer, event_t *evt);
int Buffer_IsMaskedIn (Buffer_t *buffer, event_t *evt);
void Buffer_MaskRegionIn (Buffer_t *buffer, event_t *first_evt, event_t *last_evt);
void Buffer_MaskRegionOut (Buffer_t *buffer, event_t *first_evt, event_t *last_evt);
#endif /* MASKS_DEAD_CODE */
void Buffer_Lock (Buffer_t *buffer);
void Buffer_Unlock (Buffer_t *buffer);
void Buffer_InsertSingle(Buffer_t *buffer, event_t *new_event);
void Buffer_InsertMultiple(Buffer_t *buffer, event_t *events_list, int num_events);
int Buffer_Flush(Buffer_t *buffer);
void Filter_Buffer(Buffer_t *buffer, event_t *first_event, event_t *last_event, DataBlocks_t *io_db);
int Buffer_DiscardOldest (Buffer_t *buffer);
int Buffer_Discard10Pct (Buffer_t *buffer);
int Buffer_DiscardAll (Buffer_t *buffer);
void Action_When_Buffer_Is_Full (Buffer_t *buffer);
//void advance_current(Buffer_t *buffer);
//int linearize(Buffer_t *buffer, event_t **buffer);

BufferIterator_t * BufferIterator_NewForward (Buffer_t *buffer);
BufferIterator_t * BufferIterator_NewBackward (Buffer_t *buffer);
BufferIterator_t * BufferIterator_NewRange (Buffer_t *buffer, unsigned long long start_time, unsigned long long end_time);
void BufferIterator_Next (BufferIterator_t *it);
void BufferIterator_Previous (BufferIterator_t *it);
int BufferIterator_OutOfBounds (BufferIterator_t *it);
event_t * BufferIterator_GetEvent (BufferIterator_t *it);

#define BIT_NewForward(buffer)  BufferIterator_NewForward(buffer)
#define BIT_NewBackward(buffer) BufferIterator_NewBackward(buffer)
#define BIT_NewRange(buffer,start,end) BufferIterator_NewRange(buffer, start, end)
#define BIT_Next(it)            BufferIterator_Next(it)
#define BIT_Prev(it)            BufferIterator_Previous(it)
#define BIT_OutOfBounds(it)     BufferIterator_OutOfBounds(it)
#define BIT_GetEvent(it)        BufferIterator_GetEvent(it)
#define BIT_Free(it)            BufferIterator_Free(it)
#if defined(MASKS_DEAD_CODE)
void BufferIterator_MaskIn (BufferIterator_t *it);
void BufferIterator_MaskOut (BufferIterator_t *it);
int BufferIterator_IsMaskedIn (BufferIterator_t *it);
int BufferIterator_IsMaskedOut (BufferIterator_t *it);
#define BIT_MaskIn(it) BufferIterator_MaskIn(it)
#define BIT_MaskOut(it) BufferIterator_MaskOut(it)
#define BIT_IsMaskedIn(it) BufferIterator_IsMaskedIn(it)
#define BIT_IsMaskedOut(it) BufferIterator_IsMaskedOut(it)
#endif /* MASKS_DEAD_CODE */

void NewMask_Wipe (Buffer_t *buffer);
void NewMask_Set (Buffer_t *buffer, event_t *event, int mask_id);
void NewMask_SetAll (Buffer_t *buffer, event_t *event);
void NewMask_SetRegion (Buffer_t *buffer, event_t *start, event_t *end, int mask_id);
void NewMask_Unset (Buffer_t *buffer, event_t *event, int mask_id);
void NewMask_UnsetAll (Buffer_t *buffer, event_t *event);
void NewMask_UnsetRegion (Buffer_t *buffer, event_t *start, event_t *end, int mask_id);
void NewMask_Flip (Buffer_t *buffer, event_t *event, int mask_id);
int NewMask_IsSet (Buffer_t *buffer, event_t *event, int mask_id);
int NewMask_IsUnset (Buffer_t *buffer, event_t *event, int mask_id);

void BufferIterator_MaskSet (BufferIterator_t *it, int mask_id);
void BufferIterator_MaskSetAll (BufferIterator_t *it);
void BufferIterator_MaskUnset (BufferIterator_t *it, int mask_id);
void BufferIterator_MaskUnsetAll (BufferIterator_t *it);;
int BufferIterator_IsMaskSet (BufferIterator_t *it, int mask_id);
int BufferIterator_IsMaskUnset (BufferIterator_t *it, int mask_id);

#define BIT_MaskSet(it, mask_id)      BufferIterator_MaskSet(it, mask_id)
#define BIT_MaskSetAll(it)            BufferIterator_MaskSetAll(it)
#define BIT_MaskUnset(it, mask_id)    BufferIterator_MaskUnset(it, mask_id)
#define BIT_MaskUnsetAll(it)          BufferIterator_MaskUnsetAll(it)
#define BIT_IsMaskSet(it, mask_id)    BufferIterator_IsMaskSet(it, mask_id)
#define BIT_IsMaskUnset(it, mask_id)  BufferIterator_IsMaskUnset(it, mask_id)

#if defined(__cplusplus)
}
#endif


#if defined(NO_ASSERTS)

#define ASSERT_VALID_BUFFER(buf)
#define ASSERT_VALID_ITERATOR(it)
#define ASSERT_VALID_BOUNDS(it)

#else

#define ASSERT_VALID_BUFFER(buf) ASSERT(buf != NULL, "Invalid buffer (NullPtr)")
#define ASSERT_VALID_ITERATOR(it) ASSERT(it != NULL, "Invalid buffer iterator (NullPtr)")
#define ASSERT_VALID_BOUNDS(it)                                          \
{                                                                        \
   ASSERT_VALID_ITERATOR(it);                                            \
   ASSERT(!BIT_OutOfBounds(it), "Buffer iterator is out of bounds");     \
} 

#endif /* NO_ASSERTS */

#define CIRCULAR_STEP(current,step,first,last,overflow) \
{                                                       \
   current = current + step;                            \
   if (current >= last)                                 \
   {                                                    \
      *overflow = TRUE;                                 \
      current = first + (current - last);               \
   }                                                    \
   else if (current < first)                            \
   {                                                    \
      *overflow = TRUE;                                 \
      current = last - (first - current);               \
   }                                                    \
   else                                                 \
   {                                                    \
      *overflow = FALSE;                                \
   }                                                    \
}

#endif /* __BUFFER_H__ */
