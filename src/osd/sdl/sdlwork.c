//
//  sdlwork.c - SDL OSD core work item functions
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

// MinGW does not have pthreads, defer to Aaron's implementation on that platform
#if defined(SDLMAME_WIN32)
#include "../windows/winwork.c"
#elif defined(SDLMAME_OS2)	// use separate OS/2 implementation
#include "os2work.c"
#else

// standard headers
#include <time.h>
#if defined(SDLMAME_UNIX) && !defined(SDLMAME_DARWIN)
#include <sys/time.h>
#endif
#include <pthread.h>
#include <unistd.h>
#include "osdcore.h"

#ifdef SDLMAME_DARWIN
#include <mach/mach.h>
#include "osxutils.h"
#endif
#include "sdlsync.h"


//============================================================
//  DEBUGGING
//============================================================

#define KEEP_STATISTICS			(0)



//============================================================
//  MACROS
//============================================================

#if KEEP_STATISTICS
#define add_to_stat(v,x)		do { osd_sync_add((v), (x)); } while (0)
#else
#define add_to_stat(v,x)		do { } while (0)
#endif

#ifndef YieldProcessor

#if defined(__i386__) || defined(__x86_64__)

INLINE void YieldProcessor(void)
{
	__asm__ __volatile__ ( " rep ; nop ;" );
}

#elif defined(__ppc__) || defined (__PPC__) || defined(__ppc64__) || defined(__PPC64__)

INLINE void YieldProcessor(void)
{
	__asm__ __volatile__ ( " nop \n" );
}

#endif

#endif



//============================================================
//  TYPE DEFINITIONS
//============================================================

typedef struct _mame_thread_info mame_thread_info;
struct _mame_thread_info
{
	osd_work_queue *	queue;			// pointer back to the queue
	osd_thread *		handle;			// handle to the thread
	osd_event *			wakeevent;		// wake event for the thread
	UINT8				active;			// are we actively processing work?
};


struct _osd_work_queue
{
	osd_lock *			critsect;		// critical section protecting the queue
	osd_work_item * volatile list;		// list of items in the queue
	osd_work_item ** volatile tailptr;	// pointer to the tail pointer of work items in the queue
	osd_work_item * volatile free;		// free list of work items
	volatile INT32		items;			// items in the queue
	volatile INT32		livethreads;	// number of live threads
	UINT32				threads;		// number of threads in this queue
	UINT32				flags;			// creation flags
	mame_thread_info *	thread;			// array of thread information
	osd_event *			workevent;		// shared event signalled when work is available
	osd_event *			doneevent;		// event signalled when work is complete
	osd_event *			exitevent;		// event signalled when thread should exit

#if KEEP_STATISTICS
	volatile INT32		itemsqueued;	// total items queued
	volatile INT32		setevents;		// number of times we called SetEvent
	volatile INT32		extraitems;		// how many extra items we got after the first in the queue loop
	volatile INT32		spinloops;		// how many times spinning bought us more items
#endif
};


struct _osd_work_item
{
	osd_work_item *		next;			// pointer to next item
	osd_work_queue *	queue;			// pointer back to the owning queue
	osd_work_callback 	callback;		// callback function
	void *				param;			// callback parameter
	void *				result;			// callback result
	osd_event *			event;			// event signalled when complete
	UINT32				flags;			// creation flags
	volatile INT32		refcount;		// reference count of threads processing
};



//============================================================
//  FUNCTION PROTOTYPES
//============================================================

static int effective_num_processors(void);
static void * worker_thread_entry(void *param);
static void worker_thread_process(osd_work_queue *queue, mame_thread_info *thread);
static void execute_work_item(osd_work_item *item);

//============================================================
//  osd_work_queue_alloc
//============================================================

osd_work_queue *osd_work_queue_alloc(int flags)
{
	int numprocs = effective_num_processors();
	osd_work_queue *queue;
	int threadnum;

	// allocate a new queue
	queue = malloc(sizeof(*queue));
	if (queue == NULL)
		goto error;
	memset(queue, 0, sizeof(*queue));

	// initialize basic queue members
	queue->tailptr = (osd_work_item **)&queue->list;
	queue->flags = flags;

	// allocate events for the queue
	queue->workevent = osd_event_alloc(FALSE, FALSE);	// auto-reset, not signalled
	queue->doneevent = osd_event_alloc(TRUE, TRUE);		// manual reset, signalled
	queue->exitevent = osd_event_alloc(TRUE, FALSE);	// manual reset, not signalled
	if (queue->workevent == NULL || queue->doneevent == NULL || queue->exitevent == NULL)
		goto error;

	// initialize the critical section
	if ( (queue->critsect = osd_lock_alloc()) == NULL )
		goto error;

	// determine how many threads to create...
	// on a single-CPU system, create 1 thread for I/O queues, and 0 threads for everything else
	if (numprocs == 1)
		queue->threads = (flags & WORK_QUEUE_FLAG_IO) ? 1 : 0;

	// on an n-CPU system, create (n-1) threads for multi queues, and 1 thread for everything else
	else
		queue->threads = (flags & WORK_QUEUE_FLAG_MULTI) ? (numprocs - 1) : 1;

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
			mame_thread_info *thread = &queue->thread[threadnum];

			// set a pointer back to the queue
			thread->queue = queue;

			// create the per-thread wake event
			thread->wakeevent = osd_event_alloc(FALSE, FALSE);	// auto-reset, not signalled
			if (thread->wakeevent == NULL)
				goto error;

			// create the thread
			thread->handle = osd_thread_create(worker_thread_entry, thread);
			if (thread->handle == NULL)
				goto error;

			// set its priority: I/O threads get high priority because they are assumed to be
			// blocked most of the time; other threads just match the creator's priority
			if (flags & WORK_QUEUE_FLAG_IO)
				osd_thread_adjust_priority(thread->handle, 0);	// TODO: specify appropriate priority
			else
				osd_thread_adjust_priority(thread->handle, 0);	// TODO: specify appropriate priority
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

	// if no items, we're done
	if (queue->items == 0)
		return TRUE;

	// if this is a multi queue, help out rather than doing nothing
	if (queue->flags & WORK_QUEUE_FLAG_MULTI)
	{
		osd_ticks_t stopspin = osd_ticks() + timeout;

		// process what we can as a worker thread
		worker_thread_process(queue, NULL);

		// spin until we hit 0 items and then return TRUE
		while (queue->items != 0 && osd_ticks() < stopspin)
			YieldProcessor();
		return TRUE;
	}

	// reset our done event and double-check the items before waiting
	osd_event_reset(queue->doneevent);
	if (queue->items == 0)
		return TRUE;

	// wait for the collection to be signalled
	return (osd_event_wait(queue->doneevent, timeout));
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
		osd_event_set(queue->exitevent);

		// wait for all the threads to go away
		for (threadnum = 0; threadnum < queue->threads; threadnum++)
		{
			mame_thread_info *thread = &queue->thread[threadnum];

			// block on the thread going away, then close the handle
			if (thread->handle != NULL)
			{
				osd_thread_wait_free(thread->handle);
			}

			// clean up the wake event
			if (thread->wakeevent != NULL)
				osd_event_free(thread->wakeevent);
		}

		// free the list
		free(queue->thread);
	}

	// free all the events
	if (queue->workevent != NULL)
		osd_event_free(queue->workevent);
	if (queue->doneevent != NULL)
		osd_event_free(queue->doneevent);
	if (queue->exitevent != NULL)
		osd_event_free(queue->exitevent);

	// free the critical section
	osd_lock_free(queue->critsect);

	// free all items in the free list
	while (queue->free != NULL)
	{
		osd_work_item *item = (osd_work_item *)queue->free;
		queue->free = item->next;
		if (item->event != NULL)
			osd_event_free(item->event);
		free(item);
	}

	// free all items in the active list
	while (queue->list != NULL)
	{
		osd_work_item *item = (osd_work_item *)queue->list;
		queue->list = item->next;
		if (item->event != NULL)
			osd_event_free(item->event);
		free(item);
	}

#if KEEP_STATISTICS
	printf("Items queued   = %9d\n", queue->itemsqueued);
	printf("SetEvent calls = %9d\n", queue->setevents);
	printf("Extra items    = %9d\n", queue->extraitems);
	printf("Spin loops     = %9d\n", queue->spinloops);
#endif

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
	} while (item != NULL && osd_compare_exchange_ptr((void * volatile *)&queue->free, item, item->next) != item);

	// if nothing, allocate something new
	if (item == NULL)
	{
		// allocate the item
		item = malloc(sizeof(*item));
		if (item == NULL)
			return NULL;
		item->event = NULL;
		item->queue = queue;
	}

	// fill in the basics
	item->next = NULL;
	item->callback = callback;
	item->param = param;
	item->result = NULL;
	item->flags = flags;
	item->refcount = 1;
	if (item->event != NULL)
		osd_event_reset(item->event);

	// if no threads, just run it now
	if (queue->threads == 0)
	{
		// execute the callback and clear the refcount back to 0
		item->result = (*item->callback)(item->param);
		item->refcount = 0;

		// handle auto-release; make sure we return NULL in that case
		if (flags & WORK_ITEM_FLAG_AUTO_RELEASE)
			osd_work_item_release(item);
	}

	// otherwise, enqueue the item and wake the threads
	else
	{
		// enqueue within the critical section
		osd_lock_acquire(queue->critsect);
		*queue->tailptr = item;
		queue->tailptr = (osd_work_item **)&item->next;
		osd_lock_release(queue->critsect);

		// increment the number of items in the queue
		osd_sync_add(&queue->items, 1);
		add_to_stat(&queue->itemsqueued, 1);

		// non-shared case
		if (!(flags & WORK_ITEM_FLAG_SHARED))
		{
			// if there are still some free threads, signal the shared work event
			if (queue->livethreads < queue->threads)
			{
				osd_event_set(queue->workevent);
				add_to_stat(&queue->setevents, 1);
			}
		}

		// shared case
		else
		{
			int threadnum;

			// signal the wake event for all non-active threads
			for (threadnum = 0; threadnum < queue->threads; threadnum++)
				if (!queue->thread[threadnum].active)
				{
					osd_event_set(queue->thread[threadnum].wakeevent);
					add_to_stat(&queue->setevents, 1);
				}
		}
	}

	// only return the item if it won't get released automatically
	return (flags & WORK_ITEM_FLAG_AUTO_RELEASE) ? NULL : item;
}


//============================================================
//  osd_work_item_wait
//============================================================

int osd_work_item_wait(osd_work_item *item, osd_ticks_t timeout)
{
	// if we're done already, just return
	if (item->refcount == 0)
		return TRUE;

	// if we don't have an event, create one
	if (item->event == NULL)
		item->event = osd_event_alloc(TRUE, FALSE);		// manual reset, not signalled

	// if we don't have an event, we need to spin (shouldn't ever really happen)
	if (item->event != NULL)
	{
		osd_ticks_t stopspin = osd_ticks() + timeout;
		while (item->refcount != 0 && osd_ticks() < stopspin)
			YieldProcessor();
	}

	// otherwise, block on the event until done
	else if (item->refcount != 0)
		osd_event_wait(item->event, timeout);

	// return TRUE if the refcount actually hit 0
	return (item->refcount == 0);
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
	} while (osd_compare_exchange_ptr((void * volatile *)&item->queue->free, next, item) != next);
}


//============================================================
//  effective_num_processors
//============================================================

static int effective_num_processors(void)
{
	char *procsoverride;
	int numprocs = 0;

	// if the OSDPROCESSORS environment variable is set, use that value if valid
	procsoverride = getenv("OSDPROCESSORS");
	if (procsoverride != NULL && sscanf(procsoverride, "%d", &numprocs) == 1 && numprocs > 0)
		return numprocs;

	// otherwise, fetch the info from the system
	return osd_num_processors();
}


//============================================================
//  worker_thread_entry
//============================================================

static void *worker_thread_entry(void *param)
{
	mame_thread_info *thread = param;
	osd_work_queue *queue = thread->queue;
	osd_event *hlist[3];

	// fill in our handle list
	hlist[0] = queue->workevent;
	hlist[1] = thread->wakeevent;
	hlist[2] = queue->exitevent;

	// loop until we exit
	for ( ;; )
	{
		// block waiting for work or exit
		int result = 0;

		// bail on exit, and only wait if there are no pending items in queue
		if (queue->items == 0)
			result = osd_event_wait_multiple(ARRAY_LENGTH(hlist), hlist, 10000000);
		if (result == 2)
			break;

		// indicate that we are live
		thread->active = TRUE;
		osd_sync_add(&queue->livethreads, 1);

		// process work items
		for ( ;; )
		{
			osd_ticks_t stopspin;

			// process as much as we can
			worker_thread_process(queue, thread);

			// spin for a while looking for more work
			stopspin = osd_ticks() + osd_ticks_per_second() / 1000;
			while (queue->items == 0 && osd_ticks() < stopspin)
				YieldProcessor();

			// if nothing more, release the processor
			if (queue->items == 0)
				break;
			add_to_stat(&queue->spinloops, 1);
		}

		// decrement the live thread count
		thread->active = FALSE;
		osd_sync_add(&queue->livethreads, -1);
	}
	//return 0;
	return NULL;
}


//============================================================
//  worker_thread_process
//============================================================

static void worker_thread_process(osd_work_queue *queue, mame_thread_info *thread)
{
	// loop until everything is processed
	while (queue->items != 0)
	{
		int removed_item = FALSE;
		osd_work_item *item;

		// use a critical section to synchronize the removal of items
		osd_lock_acquire(queue->critsect);
		{
			item = (osd_work_item *)queue->list;

			// if it is a regular item, or if it is a shared item that is complete, remove it
			if (item != NULL && (!(item->flags & WORK_ITEM_FLAG_SHARED) || item->refcount == 0))
			{
				removed_item = TRUE;
				queue->list = item->next;
				if (item->next == NULL)
					queue->tailptr = (osd_work_item **)&queue->list;
			}
		}
		osd_lock_release(queue->critsect);

		// call the callback and signal its event
		if (item != NULL && item->refcount != 0)
			execute_work_item(item);

		// if we removed an item, decrement the item count and signal done if appropriate
		if (removed_item && osd_sync_add(&queue->items, -1) == 0)
		{
			// we don't need to set the doneevent for multi queues because they spin
			if (!(item->flags & WORK_QUEUE_FLAG_MULTI))
				osd_event_set(queue->doneevent);

			// if it's an auto-release item, release it
			if (item->flags & WORK_ITEM_FLAG_AUTO_RELEASE)
				osd_work_item_release(item);
		}

		// if we removed an item and there's still work to do, bump the stats
		if (removed_item && queue->items != 0)
			add_to_stat(&queue->extraitems, 1);
	}
}


//============================================================
//  execute_work_item
//============================================================

static void execute_work_item(osd_work_item *item)
{
	void *result;

	// add a claim to this work item; if it had already been completed, back off and exit
	if (osd_sync_add(&item->refcount, 1) == 1)
	{
		item->refcount = 0;
		return;
	}

	// call the callback and stash the result
	result = (*item->callback)(item->param);

	// mark it complete and signal the event
	if (osd_sync_add(&item->refcount, -1) == 1 && osd_compare_exchange32(&item->refcount, 1, 0) == 1)
	{
		// set the result and signal the event
		item->result = result;
		if (item->event != NULL)
			osd_event_set(item->event);
	}
}

#endif	// SDLMAME_WIN32
