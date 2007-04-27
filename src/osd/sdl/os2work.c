//============================================================
//
//  os2work.c - OS/2 OSD core work item functions
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

#define INCL_DOS
#include <os2.h>

#include <stdlib.h>
#include <unistd.h>

#include "osdcore.h"


//============================================================
//  TYPE DEFINITIONS
//============================================================

struct _osd_work_queue
{
    HMTX            hmtx;           // critical section protecting the queue
    osd_work_item * volatile list;  // list of items in the queue
    osd_work_item ** volatile tailptr;// pointer to the tail pointer of work items in the queue
    osd_work_item * volatile free;  // free list of work items
    volatile LONG   items;          // items in the queue
    volatile LONG   livethreads;    // number of live threads
    UINT32          threads;        // number of threads in this queue
    TID    *        thread;         // array of thread handles
    HEV             workevent;      // event signalled when work is available
    HEV             doneevent;      // event signalled when work is complete
    HEV             exitevent;      // event signalled when thread should exit
    HMTX			autoreset;      // mutex for auto-reset of workevent
};


struct _osd_work_item
{
    osd_work_item * next;           // pointer to next item
    osd_work_queue *queue;          // pointer back to the owning queue
    osd_work_callback callback;     // callback function
    void *          param;          // callback parameter
    void *          result;         // callback result
    HEV             event;          // event signalled when complete
    UINT32          flags;          // creation flags
    UINT32          complete;       // are we finished yet?
};



//============================================================
//  FUNCTION PROTOTYPES
//============================================================

static void worker_thread_entry(void *param);
static int execute_work_item(osd_work_item *item);



//============================================================
//  INLINE FUNCTIONS
//============================================================

INLINE PVOID compare_exchange_pointer( PVOID volatile *ptr, PVOID exchange, PVOID compare)
{
    PVOID ret = NULL;

    DosEnterCritSec();

    ret = *ptr;

    if ( *ptr == compare )
    {
        *ptr = exchange;
    }

    DosExitCritSec();

    return ret;
}


INLINE LONG interlocked_increment(LONG volatile *addend)
{
    DosEnterCritSec();
    ( *addend )++;
    DosExitCritSec();

    return *addend;
}


INLINE LONG interlocked_decrement(LONG volatile *addend)
{
    DosEnterCritSec();
    ( *addend )--;
    DosExitCritSec();

    return *addend;
}


//============================================================
//  osd_work_queue_alloc
//============================================================

osd_work_queue *osd_work_queue_alloc(int flags)
{
    osd_work_queue *queue;
    int processors;
    int threadnum;

    // allocate a new queue
    queue = malloc(sizeof(*queue));
    if (queue == NULL)
        goto error;
    memset(queue, 0, sizeof(*queue));

    // allocate events for the queue
    DosCreateEventSem( NULL, &queue->workevent, 0, FALSE );     // auto-reset, not signalled
    DosCreateEventSem( NULL, &queue->doneevent, 0, TRUE );      // manual reset, signalled
    DosCreateEventSem( NULL, &queue->exitevent, 0, FALSE );     // manual reset, not signalled
    if (queue->workevent == 0 || queue->doneevent == 0 || queue->exitevent == 0)
        goto error;

	// initialize the mutex for auto-reset of workevent
	DosCreateMutexSem( NULL, &queue->autoreset, 0, FALSE );
	
    // initialize the critical section
    DosCreateMutexSem( NULL, &queue->hmtx, 0, FALSE );
    queue->tailptr = (osd_work_item **)&queue->list;

    // determine how many threads to create
    processors = 1;

#if defined(_SC_NPROCESSORS_ONLN)
    processors = sysconf(_SC_NPROCESSORS_ONLN);
#endif

    if (processors == 1)
        queue->threads = (flags & WORK_QUEUE_FLAG_IO) ? 1 : 0;
    else
        queue->threads = (flags & WORK_QUEUE_FLAG_MULTI) ? processors : 1;

    // if we have threads, create them
    if (queue->threads > 0)
    {
        // allocate memory for thread array
        queue->thread = malloc(queue->threads * sizeof(queue->thread[0]));
        if (queue->thread == NULL)
            goto error;
        memset(queue->thread, 0, queue->threads * sizeof(queue->thread[0]));

        // iterate over threads
        for (threadnum = 0; threadnum < queue->threads; threadnum++)
        {
            // create the thread
            int handle = _beginthread(worker_thread_entry, NULL, 256 * 1024, queue);
            queue->thread[threadnum] = ( TID )handle;
            if (queue->thread[threadnum] == ( TID )-1)
                goto error;

            // set its priority
            if (flags & WORK_QUEUE_FLAG_IO)
                DosSetPriority( PRTYS_THREAD, PRTYC_NOCHANGE, +1, queue->thread[threadnum]);
        }
    }
    return queue;

error:
    osd_work_queue_free(queue);
    return NULL;
}


//============================================================
//  osd_work_queue_items
//============================================================

int osd_work_queue_items(osd_work_queue *queue)
{
    // return the number of items currently in the queue
    return queue->items;
}


//============================================================
//  osd_work_queue_wait
//============================================================

int osd_work_queue_wait(osd_work_queue *queue, osd_ticks_t timeout)
{
    // if no threads, no waiting
    if (queue->threads == 0)
        return TRUE;

    // wait for the collection to be signalled
    return ( DosWaitEventSem( queue->doneevent, timeout * 1000 / osd_ticks_per_second()) == 0 );
}


//============================================================
//  osd_work_queue_free
//============================================================

void osd_work_queue_free(osd_work_queue *queue)
{
    // if we have threads, clean them up
    if (queue->threads > 0 && queue->thread != NULL)
    {
        int threadnum;

        // signal all the threads to exit
        DosPostEventSem( queue->exitevent );

        // count the number of valid threads (we could be partially constructed)
        for (threadnum = 0; threadnum < queue->threads; threadnum++)
            if (queue->thread[threadnum] != ( TID )-1)
                DosWaitThread( &queue->thread[threadnum], DCWW_WAIT );

        // free the list
        free(queue->thread);
    }

    // free all the events
    if (queue->workevent != 0)
        DosCloseEventSem(queue->workevent);
    if (queue->doneevent != 0)
        DosCloseEventSem(queue->doneevent);
    if (queue->exitevent != 0)
        DosCloseEventSem(queue->exitevent);

	// free the mutex
	DosCloseMutexSem( queue->autoreset );
	
    // free the critical section
    DosCloseMutexSem(queue->hmtx);

    // free all items in the free list
    while (queue->free != NULL)
    {
        osd_work_item *item = (osd_work_item *)queue->free;
        queue->free = item->next;
        if (item->event != 0)
            DosCloseEventSem(item->event);
        free(item);
    }

    // free all items in the active list
    while (queue->list != NULL)
    {
        osd_work_item *item = (osd_work_item *)queue->list;
        queue->list = item->next;
        if (item->event != 0)
            DosCloseEventSem(item->event);
        free(item);
    }

    // free the queue itself
    free(queue);
}


//============================================================
//  osd_work_item_queue
//============================================================

osd_work_item *osd_work_item_queue(osd_work_queue *queue, osd_work_callback callback, void *param, UINT32 flags)
{
    osd_work_item *item;

    // first allocate a new work item; try the free list first
    do
    {
        item = (osd_work_item *)queue->free;
    } while (item != NULL && compare_exchange_pointer((PVOID volatile *)&queue->free, item->next, item) != item);

    // if nothing, allocate something new
    if (item == NULL)
    {
        // allocate the item
        item = malloc(sizeof(*item));
        if (item == NULL)
            return NULL;

        memset( item, 0, sizeof( *item ));
    }

    // fill in the basics
    item->next = NULL;
    item->callback = callback;
    item->param = param;
    item->result = NULL;
    item->flags = flags;
    item->queue = queue;
    item->complete = FALSE;
    if (item->event != 0)
    {
        ULONG ulPost;
        DosResetEventSem(item->event, &ulPost);
    }

    // if no threads, just run it now
    if (queue->threads == 0)
        return execute_work_item(item) ? item : NULL;

    // otherwise, enqueue it
    DosRequestMutexSem( queue->hmtx, -1 );
    *queue->tailptr = item;
    queue->tailptr = (osd_work_item **)&item->next;
    DosReleaseMutexSem( queue->hmtx );

    // if we're not full up, signal the event
    if (interlocked_increment(&queue->items) == 1)
    {
        ULONG ulPost;
        DosResetEventSem(queue->doneevent, &ulPost);
    }
    if (queue->livethreads < queue->threads)
        DosPostEventSem(queue->workevent);

    return item;
}


//============================================================
//  osd_work_item_wait
//============================================================

int osd_work_item_wait(osd_work_item *item, osd_ticks_t timeout)
{
    // if we're done already, just return
    if (item->complete)
        return TRUE;

    // if we don't have an event, create one
    if (item->event == 0)
        DosCreateEventSem( NULL, &item->event, 0, FALSE );  // manual reset, not signalled

    // if we don't have an event, we need to spin (shouldn't ever really happen)
    if (item->event == 0)
    {
        osd_ticks_t endtime = osd_ticks() + timeout;
        while (!item->complete && endtime - osd_ticks() > 0) ;
    }

    // otherwise, block on the event until done
    else if (!item->complete)
        DosWaitEventSem(item->event, timeout * 1000 / osd_ticks_per_second());

    return item->complete;
}


//============================================================
//  osd_work_item_result
//============================================================

void *osd_work_item_result(osd_work_item *item)
{
    return item->result;
}


//============================================================
//  osd_work_item_release
//============================================================

void osd_work_item_release(osd_work_item *item)
{
    osd_work_item *next;

    // make sure we're done first
    osd_work_item_wait(item, 100 * osd_ticks_per_second());

    // add us to the free list on our queue
    do
    {
        next = (osd_work_item *)item->queue->free;
        item->next = next;
    } while (compare_exchange_pointer((PVOID volatile *)&item->queue->free, item, next) != next);
}


//============================================================
//  worker_thread_entry
//============================================================

static void worker_thread_entry(void *param)
{
    osd_work_queue *queue = param;
    HMUX		   hmux;	
    SEMRECORD	   asr[ 2 ] = {{( HSEM )queue->workevent, 0 },
                               {( HSEM )queue->exitevent, 1 }};
    ULONG		   ulUser;
    ULONG		   ulPost;

	DosCreateMuxWaitSem( NULL, &hmux, 2, asr, DCMW_WAIT_ANY );
	
    // loop until we exit
    for ( ;; )
    {
        // block waiting for work or exit
        DosRequestMutexSem( queue->autoreset, -1 );

        DosWaitMuxWaitSem( hmux, -1, &ulUser );
        if( ulUser == 0 )
        	DosResetEventSem( queue->workevent, &ulPost );
		
		DosReleaseMutexSem( queue->autoreset );
		
        // bail on exit
		if( ulUser == 1 )
			break;
			
        // loop until everything is processed
//again:
        while (queue->items != 0)
        {
            osd_work_item *item;

            // indicate that we are live
            interlocked_increment(&queue->livethreads);

            // pull an item off the head
            DosRequestMutexSem( queue->hmtx, -1 );
            item = (osd_work_item *)queue->list;
            if (item != NULL)
            {
                queue->list = item->next;
                if (item->next == NULL)
                    queue->tailptr = (osd_work_item **)&queue->list;
            }
            DosReleaseMutexSem( queue->hmtx );

            // call the callback and signal its event
            execute_work_item(item);

            // decrement the count
            if (interlocked_decrement(&queue->items) == 0)
                DosPostEventSem(queue->doneevent);

            // decrement the live thread count
            interlocked_decrement(&queue->livethreads);
        }

        // hard loop for a while
/*{
    int count = 0;
        while (queue->items == 0 && count++ < 10000000) ;
    if (queue->items != 0) goto again;
}*/
    }

    DosCloseMuxWaitSem( hmux );
}


//============================================================
//  execute_work_item
//============================================================

static int execute_work_item(osd_work_item *item)
{
    // call the callback and stash the result
    item->result = (*item->callback)(item->param);

    // mark it complete and signal the event
    item->complete = TRUE;
    if (item->event != 0)
        DosPostEventSem(item->event);

    // if it's an auto-release item, release it
    if (item->flags & WORK_ITEM_FLAG_AUTO_RELEASE)
    {
        osd_work_item_release(item);
        return FALSE;
    }

    return TRUE;
}

