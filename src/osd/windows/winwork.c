//============================================================
//
//  winwork.c - Win32 OSD core work item functions
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// standard windows headers
#define _WIN32_WINNT 0x0400
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>

#ifdef __GNUC__
#include <stdint.h>
#endif

// MAME headers
#include "osdcore.h"


//============================================================
//  TYPE DEFINITIONS
//============================================================

struct _osd_work_queue
{
	CRITICAL_SECTION critsect;		// critical section protecting the queue
	osd_work_item * volatile list;	// list of items in the queue
	osd_work_item ** volatile tailptr;// pointer to the tail pointer of work items in the queue
	osd_work_item * volatile free;	// free list of work items
	volatile LONG	items;			// items in the queue
	volatile LONG	livethreads;	// number of live threads
	UINT32			threads;		// number of threads in this queue
	HANDLE *		thread;			// array of thread handles
	HANDLE			workevent;		// event signalled when work is available
	HANDLE			doneevent;		// event signalled when work is complete
	HANDLE			exitevent;		// event signalled when thread should exit
};


struct _osd_work_item
{
	osd_work_item *	next;			// pointer to next item
	osd_work_queue *queue;			// pointer back to the owning queue
	osd_work_callback callback;		// callback function
	void *			param;			// callback parameter
	void *			result;			// callback result
	HANDLE			event;			// event signalled when complete
	UINT32			flags;			// creation flags
	UINT32			complete;		// are we finished yet?
};



//============================================================
//  FUNCTION PROTOTYPES
//============================================================

static void worker_thread_entry(void *param);
static int execute_work_item(osd_work_item *item);



//============================================================
//  INLINE FUNCTIONS
//============================================================

INLINE PVOID compare_exchange_pointer(PVOID volatile *ptr, PVOID exchange, PVOID compare)
{
	// the mingw headers don't put the volatile keyword on the first parameter
	// gcc also can't handle casting the result of a function
#ifdef __GNUC__
#ifdef PTR64
	UINT64 result = InterlockedCompareExchange64((UINT64)ptr, (UINT64)exchange, (UINT64)compare);
	return (PVOID)result;
#else
	LONG result = InterlockedCompareExchange((LPLONG)ptr, (LONG)exchange, (LONG)compare);
	return (PVOID)result;
#endif
#else
	return InterlockedCompareExchangePointer(ptr, exchange, compare);
#endif
}


INLINE LONG interlocked_increment(LONG volatile *addend)
{
	// the mingw headers don't put the volatile keyword on the first parameter
#ifdef __GNUC__
	return InterlockedIncrement((LPLONG)addend);
#else
	return InterlockedIncrement(addend);
#endif
}


INLINE LONG interlocked_decrement(LONG volatile *addend)
{
	// the mingw headers don't put the volatile keyword on the first parameter
#ifdef __GNUC__
	return InterlockedDecrement((LPLONG)addend);
#else
	return InterlockedDecrement(addend);
#endif
}



//============================================================
//  osd_work_queue_alloc
//============================================================

osd_work_queue *osd_work_queue_alloc(int flags)
{
	osd_work_queue *queue;
	SYSTEM_INFO info;
	int threadnum;

	// allocate a new queue
	queue = malloc(sizeof(*queue));
	if (queue == NULL)
		goto error;
	memset(queue, 0, sizeof(*queue));

	// allocate events for the queue
	queue->workevent = CreateEvent(NULL, FALSE, FALSE, NULL);	// auto-reset, not signalled
	queue->doneevent = CreateEvent(NULL, TRUE, TRUE, NULL);		// manual reset, signalled
	queue->exitevent = CreateEvent(NULL, TRUE, FALSE, NULL);	// manual reset, not signalled
	if (queue->workevent == NULL || queue->doneevent == NULL || queue->exitevent == NULL)
		goto error;

	// initialize the critical section
	InitializeCriticalSection(&queue->critsect);
	queue->tailptr = (osd_work_item **)&queue->list;

	// determine how many threads to create
	GetSystemInfo(&info);
	if (info.dwNumberOfProcessors == 1)
		queue->threads = (flags & WORK_QUEUE_FLAG_IO) ? 1 : 0;
	else
		queue->threads = (flags & WORK_QUEUE_FLAG_MULTI) ? info.dwNumberOfProcessors : 1;

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
			uintptr_t handle = _beginthread(worker_thread_entry, 0, queue);
			queue->thread[threadnum] = (HANDLE)handle;
			if (queue->thread[threadnum] == NULL)
				goto error;

			// set its priority
			if (flags & WORK_QUEUE_FLAG_IO)
				SetThreadPriority(queue->thread[threadnum], THREAD_PRIORITY_ABOVE_NORMAL);
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
	return (WaitForSingleObject(queue->doneevent, timeout * 1000 / osd_ticks_per_second()) == WAIT_OBJECT_0);
}


//============================================================
//  osd_work_queue_free
//============================================================

void osd_work_queue_free(osd_work_queue *queue)
{
	// if we have threads, clean them up
	if (queue->threads > 0 && queue->thread != NULL)
	{
		int threadnum, hcount;

		// signal all the threads to exit
		SetEvent(queue->exitevent);

		// count the number of valid threads (we could be partially constructed)
		for (threadnum = hcount = 0; threadnum < queue->threads; threadnum++)
			if (queue->thread[threadnum] != NULL)
				hcount++;

		// wait for all the threads to exit
		WaitForMultipleObjects(hcount, queue->thread, TRUE, INFINITE);

		// free the list
		free(queue->thread);
	}

	// free all the events
	if (queue->workevent != NULL)
		CloseHandle(queue->workevent);
	if (queue->doneevent != NULL)
		CloseHandle(queue->doneevent);
	if (queue->exitevent != NULL)
		CloseHandle(queue->exitevent);

	// free the critical section
	DeleteCriticalSection(&queue->critsect);

	// free all items in the free list
	while (queue->free != NULL)
	{
		osd_work_item *item = (osd_work_item *)queue->free;
		queue->free = item->next;
		if (item->event != NULL)
			CloseHandle(item->event);
		free(item);
	}

	// free all items in the active list
	while (queue->list != NULL)
	{
		osd_work_item *item = (osd_work_item *)queue->list;
		queue->list = item->next;
		if (item->event != NULL)
			CloseHandle(item->event);
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
	}

	// fill in the basics
	item->next = NULL;
	item->callback = callback;
	item->param = param;
	item->result = NULL;
	item->flags = flags;
	item->queue = queue;
	item->complete = FALSE;
	if (item->event != NULL)
		ResetEvent(item->event);

	// if no threads, just run it now
	if (queue->threads == 0)
		return execute_work_item(item) ? item : NULL;

	// otherwise, enqueue it
	EnterCriticalSection(&queue->critsect);
	*queue->tailptr = item;
	queue->tailptr = (osd_work_item **)&item->next;
	LeaveCriticalSection(&queue->critsect);

	// if we're not full up, signal the event
	if (interlocked_increment(&queue->items) == 1)
		ResetEvent(queue->doneevent);
	if (queue->livethreads < queue->threads)
		SetEvent(queue->workevent);

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
	if (item->event == NULL)
		item->event = CreateEvent(NULL, TRUE, FALSE, NULL);		// manual reset, not signalled

	// if we don't have an event, we need to spin (shouldn't ever really happen)
	if (item->event != NULL)
	{
		osd_ticks_t endtime = osd_ticks() + timeout;
		while (!item->complete && endtime - osd_ticks() > 0) ;
	}

	// otherwise, block on the event until done
	else if (!item->complete)
		WaitForSingleObject(item->event, timeout * 1000 / osd_ticks_per_second());

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
	HANDLE hlist[2];

	// fill in our handle list
	hlist[0] = queue->workevent;
	hlist[1] = queue->exitevent;

	// loop until we exit
	for ( ;; )
	{
		// block waiting for work or exit
		DWORD result = WaitForMultipleObjects(ARRAY_LENGTH(hlist), hlist, FALSE, INFINITE);

		// bail on exit
		if (result == WAIT_OBJECT_0 + 1)
			break;

		// loop until everything is processed
//again:
		while (queue->items != 0)
		{
			osd_work_item *item;

			// indicate that we are live
			interlocked_increment(&queue->livethreads);

			// pull an item off the head
			EnterCriticalSection(&queue->critsect);
			item = (osd_work_item *)queue->list;
			if (item != NULL)
			{
				queue->list = item->next;
				if (item->next == NULL)
					queue->tailptr = (osd_work_item **)&queue->list;
			}
			LeaveCriticalSection(&queue->critsect);

			// call the callback and signal its event
			execute_work_item(item);

			// decrement the count
			if (interlocked_decrement(&queue->items) == 0)
				SetEvent(queue->doneevent);

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
	if (item->event != NULL)
		SetEvent(item->event);

	// if it's an auto-release item, release it
	if (item->flags & WORK_ITEM_FLAG_AUTO_RELEASE)
	{
		osd_work_item_release(item);
		return FALSE;
	}

	return TRUE;
}
