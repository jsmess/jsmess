//============================================================
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
#ifdef SDLMAME_WIN32
#include "../windows/winwork.c"
#elif SDLMAME_OS2	// use separate OS/2 implementation
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
#endif


//============================================================
//  TYPE DEFINITIONS
//============================================================

#define QUEUE_STATE_DOWORK			0
#define QUEUE_STATE_WORK_COMPLETE		1
#define QUEUE_STATE_EXIT			2

struct _osd_work_queue
{
	pthread_mutex_t		critsect;		// mutex protecting the queue (everything except for the state)
	osd_work_item * volatile list;		// list of items in the queue
	osd_work_item ** volatile tailptr;	// pointer to the tail pointer of work items in the queue
	osd_work_item * volatile free;		// free list of work items
	volatile long		items;			// items in the queue
	volatile long		livethreads;	// number of live threads
	UINT32				threads;		// number of threads in this queue
	pthread_t *			thread;			// array of thread handles
	volatile int		state;			// QUEUE_STATE_* above
	pthread_mutex_t		statelock;		// lock for state data
	pthread_cond_t		statecond;		// condition variable to wait for state change
};


#define WORK_STATE_INCOMPLETE		0
#define WORK_STATE_COMPLETE			1

struct _osd_work_item
{
	osd_work_item *		next;			// pointer to next item
	osd_work_queue *	queue;			// pointer back to the owning queue
	osd_work_callback	callback;		// callback function
	void *	    		param;			// callback parameter
	void *	    		result;			// callback result
	volatile int		state;			// WORK_STATE_* above
	pthread_mutex_t		statelock;		// lock for state data
	pthread_cond_t		statecond;		// condition variable to wait for state change
	UINT32			flags;	 		// creation flags
	UINT32			complete;		// are we finished yet?
};


//============================================================
//  FUNCTION PROTOTYPES
//============================================================

static void * worker_thread_entry(void *param);
static int execute_work_item(osd_work_item *item);



//============================================================
//  INLINE FUNCTIONS
//============================================================

INLINE void * compare_exchange_pointer(osd_work_queue *queue, void * volatile *ptr, void *exchange, void *compare)
{
	void *ret = NULL;

	pthread_mutex_lock( &queue->critsect );

#ifdef PTR64
	ret = (void*)(*((UINT64*)ptr));

	if ( *((UINT64*)ptr) == (UINT64)compare )
	{
		*((UINT64*)ptr) = (UINT64)exchange;
	}
#else
	ret = (void*)(*((UINT32*)ptr));

	if ( *((UINT32*)ptr) == (UINT32)compare )
	{
		*((UINT32*)ptr) = (UINT32)exchange;
	}
#endif

	pthread_mutex_unlock( &queue->critsect );

	return ret;
}

//============================================================
//  osd_work_queue_alloc
//============================================================

osd_work_queue *osd_work_queue_alloc(int flags)
{
	osd_work_queue *queue;
	int threadnum;
	int processors;

	// allocate a new queue
	queue = malloc(sizeof(*queue));
	if (queue == NULL)
		goto error;
	memset(queue, 0, sizeof(*queue));

	// allocate mutexes and conditions for the queue
	if ( pthread_mutex_init(&queue->statelock, NULL) != 0 )
		goto error;
	if ( pthread_cond_init(&queue->statecond, NULL) != 0 )
		goto error;

	// initialize the critical section
	if ( pthread_mutex_init(&queue-> critsect, NULL) != 0 )
		goto error;

	queue->tailptr = (osd_work_item **)&queue->list;

	// determine how many threads to create
	processors = 1;

#ifdef SDLMAME_DARWIN
	{
		struct host_basic_info host_basic_info;
		unsigned int count;
		kern_return_t r;
		mach_port_t my_mach_host_self;
		
		count = HOST_BASIC_INFO_COUNT;
		my_mach_host_self = mach_host_self();
		if ( ( r = host_info(my_mach_host_self, HOST_BASIC_INFO, (host_info_t)(&host_basic_info), &count)) == KERN_SUCCESS )
		{
			processors = host_basic_info.avail_cpus;
		}
		mach_port_deallocate(mach_task_self(), my_mach_host_self);
	}
#elif defined(_SC_NPROCESSORS_ONLN)
	processors = sysconf(_SC_NPROCESSORS_ONLN);
#endif

	if (processors == 1)
		queue->threads = (flags & WORK_QUEUE_FLAG_IO) ? 1 : 0;
	else
		queue->threads = (flags & WORK_QUEUE_FLAG_MULTI) ? processors : 1;

	queue->state = QUEUE_STATE_WORK_COMPLETE;

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
			if ( pthread_create(&queue->thread[threadnum], NULL, worker_thread_entry, queue) != 0 )
				goto error;
			
			// set its priority
			if (flags & WORK_QUEUE_FLAG_IO)
			{
				struct sched_param	sched;
				int					policy;
			
				if ( pthread_getschedparam( queue->thread[threadnum], &policy, &sched ) == 0 )
				{
					sched.sched_priority++;
					pthread_setschedparam( queue->thread[threadnum], policy, &sched );
				}
			}
				
			
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
	if ( pthread_mutex_lock(&queue->statelock) != 0 )
		return FALSE;

	do
	{
		struct timespec   ts;
		struct timeval    tp;

//		usleep(1000);

		if ( queue->state == QUEUE_STATE_WORK_COMPLETE )
		{
			pthread_mutex_unlock(&queue->statelock);
			return TRUE;
		}

		gettimeofday(&tp, NULL);
		ts.tv_sec  = tp.tv_sec;
		ts.tv_nsec = tp.tv_usec * 1000;
		ts.tv_sec += timeout / osd_ticks_per_second();

		if ( pthread_cond_timedwait(&queue->statecond, &queue->statelock, &ts) != 0 )
			return FALSE;
	} while( 1 );

	return FALSE;
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

		pthread_mutex_lock(&queue->statelock);
		queue->state = QUEUE_STATE_EXIT;
		pthread_mutex_unlock(&queue->statelock);

		// signal all the threads to exit
		pthread_cond_broadcast(&queue->statecond);

		// count the number of valid threads (we could be partially constructed)
		
		for (threadnum = 0; threadnum < queue->threads; threadnum++)
		{
			if (queue->thread[threadnum] != (pthread_t)NULL)
			{
				// wait for all the threads to exit
				pthread_join(queue->thread[threadnum], NULL);
			}
		}

		// free the list
		free(queue->thread);
	}

	// free all the mutexes and conditions
	pthread_cond_destroy(&queue->statecond);
	pthread_mutex_destroy(&queue->statelock);

	// free the critical section
	pthread_mutex_destroy(&queue->critsect);

	// free all items in the free list
	while (queue->free != NULL)
	{
		osd_work_item *item = (osd_work_item *)queue->free;
		queue->free = item->next;
		if (!(item->flags & WORK_ITEM_FLAG_AUTO_RELEASE))
		{
			pthread_cond_destroy(&item->statecond);
			pthread_mutex_destroy(&item->statelock);
		}
		free(item);
	}

	// free all items in the active list
	while (queue->list != NULL)
	{
		osd_work_item *item = (osd_work_item *)queue->list;
		queue->list = item->next;
		pthread_cond_destroy(&item->statecond);
		pthread_mutex_destroy(&item->statelock);
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
	} while (item != NULL && compare_exchange_pointer(queue,(void * volatile *)&queue->free, item->next, item) != item);

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
	item->state = WORK_STATE_INCOMPLETE;
	// allocate mutexes and conditions
	if ( pthread_mutex_init(&item->statelock, NULL) != 0 || pthread_cond_init(&item->statecond, NULL) != 0 )
	{
		return NULL;
	}

	// if no threads, just run it now
	if (queue->threads == 0)
	{
		return execute_work_item(item) ? item : NULL;
	}

	// otherwise, enqueue it
	pthread_mutex_lock(&queue->critsect);
	*queue->tailptr = item;
	queue->tailptr = (osd_work_item **)&item->next;
	queue->items++;
	pthread_mutex_unlock(&queue->critsect);

	// if we're not full up, signal the event
	pthread_mutex_lock(&queue->statelock);
	if ( queue->items == 1 )
		queue->state = QUEUE_STATE_DOWORK;
	pthread_mutex_unlock(&queue->statelock);
	
	if (queue->livethreads < queue->threads)
		pthread_cond_broadcast(&queue->statecond);

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

	// wait for the collection to be signalled
	if ( pthread_mutex_lock(&item->statelock) != 0 )
		return FALSE;

	do
	{
		struct timespec   ts;
		struct timeval    tp;

		if ( item->complete )
		{
			pthread_mutex_unlock(&item->statelock);
			return TRUE;
		}

		gettimeofday(&tp, NULL);
		ts.tv_sec  = tp.tv_sec;
		ts.tv_nsec = tp.tv_usec * 1000;
		ts.tv_sec += timeout / osd_ticks_per_second();

		if ( pthread_cond_timedwait(&item->statecond, &item->statelock, &ts) != 0 )
			return FALSE;
	} while( 1 );

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

	// release the mutexes and conditions
	pthread_cond_destroy(&item->statecond);
	pthread_mutex_destroy(&item->statelock);

	// add us to the free list on our queue
	do
	{
		next = (osd_work_item *)item->queue->free;
		item->next = next;
	} while (compare_exchange_pointer(item->queue,(void * volatile *)&item->queue->free, item, next) != next);
}


//============================================================
//  worker_thread_entry
//============================================================

static void *worker_thread_entry(void *param)
{
	osd_work_queue *queue = (osd_work_queue *)param;

	// loop until we exit
	for ( ;; )
	{
		// block waiting for work or exit
		pthread_mutex_lock(&queue->statelock);
		pthread_cond_wait(&queue->statecond, &queue->statelock);
		pthread_mutex_unlock(&queue->statelock);

		// bail on exit
		if ( queue->state == QUEUE_STATE_EXIT )
			break;

		// loop until everything is processed
		while (queue->items != 0)
		{
			osd_work_item *item;

			// indicate that we are live
			pthread_mutex_lock(&queue->critsect);
			queue->livethreads++;

			// pull an item off the head
			item = (osd_work_item *)queue->list;
			if (item != NULL)
			{
				queue->list = item->next;
				if (item->next == NULL)
					queue->tailptr = (osd_work_item **)&queue->list;
			}
			pthread_mutex_unlock(&queue->critsect);

			// call the callback and signal its complete
			if (item)
			{
				// WARNING: if execute_work_item returns FALSE the item was
				// auto-destroyed and it's statecond is a bad thing to touch!
				if (execute_work_item(item))
				{
					pthread_cond_broadcast(&item->statecond);
				}
			}
			pthread_mutex_lock(&queue->critsect);
			// decrement the count
			queue->items--;

			// decrement the live thread count
			queue->livethreads--;
			pthread_mutex_unlock(&queue->critsect);
			
			if ( queue->items == 0 )
			{
				pthread_mutex_lock(&queue->statelock);
				queue->state = QUEUE_STATE_WORK_COMPLETE;
				pthread_mutex_unlock(&queue->statelock);
				pthread_cond_broadcast(&queue->statecond);
			}
		}
	}

	return 0;
}

//============================================================
//  execute_work_item
//============================================================

static int execute_work_item(osd_work_item *item)
{
	// call the callback and stash the result
	item->result = (*item->callback)(item->param);

	// mark it complete (and signal the event?)
	item->complete = TRUE;

	// if it's an auto-release item, release it
	if (item->flags & WORK_ITEM_FLAG_AUTO_RELEASE)
	{
		osd_work_item_release(item);
		return FALSE;
	}

	return TRUE;
}

#endif	// SDLMAME_WIN32
