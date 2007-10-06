//============================================================
//
//  sdlsync.c - SDL core synchronization functions
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

#ifndef SDLMAME_WIN32
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 	// for PTHREAD_MUTEX_RECURSIVE; needs to be here before other glibc headers are included
#endif
#endif

#ifdef SDLMAME_MACOSX
#include "SDL/SDL.h"
#else
#include "SDL.h"
#endif

#ifdef SDLMAME_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef SDLMAME_OS2
#define INCL_DOS
#include <os2.h>
#endif

#ifdef SDLMAME_MACOSX
#include <mach/mach.h>
#endif

// standard C headers
#include <math.h>
#include <unistd.h>

// MAME headers
#include "osdcore.h"
#include "mame.h"
#include "sdlsync.h"

#ifndef SDLMAME_WIN32
#ifndef SDLMAME_OS2
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>

struct _osd_lock {
 	pthread_mutex_t id;
 };
 
struct _osd_event {
	pthread_mutex_t 	mutex;
	pthread_cond_t 		cond;
	int					autoreset;
	volatile int		signalled;
};

struct _osd_thread {
	pthread_t		thread;
};

static osd_lock		*atomic_lck = NULL;

//============================================================
//  osd_lock_alloc
//============================================================

osd_lock *osd_lock_alloc(void)
{
	osd_lock *mutex;
	pthread_mutexattr_t mtxattr;

	mutex = (osd_lock *)calloc(1, sizeof(osd_lock));

	pthread_mutexattr_init(&mtxattr);
	pthread_mutexattr_settype(&mtxattr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mutex->id, &mtxattr);

	return mutex;
}

//============================================================
//  osd_lock_acquire
//============================================================

void osd_lock_acquire(osd_lock *lock)
{
	int r;

	r =	pthread_mutex_lock(&lock->id);
	if (r==0)
		return;
	printf("Error on lock: %d: %s\n", r, strerror(r));
}

//============================================================
//  osd_lock_try
//============================================================

int osd_lock_try(osd_lock *lock)
{
	int r;
	
	r = pthread_mutex_trylock(&lock->id);
	if (r==0)
		return 1;
	if (r!=EBUSY)
		printf("Error on trylock: %d: %s\n", r, strerror(r));
	return 0;
}

//============================================================
//  osd_lock_release
//============================================================

void osd_lock_release(osd_lock *lock)
{
	pthread_mutex_unlock(&lock->id);
}

//============================================================
//  osd_lock_free
//============================================================

void osd_lock_free(osd_lock *lock)
{
	pthread_mutex_unlock(&lock->id);
	pthread_mutex_destroy(&lock->id);
	free(lock);
}


//============================================================
//  osd_num_processors
//============================================================

int osd_num_processors(void)
{
	int processors = 1;

#ifdef SDLMAME_MACOSX
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
	return processors;
}

//============================================================
//  osd_event_alloc
//============================================================

osd_event *osd_event_alloc(int manualreset, int initialstate)
{
	osd_event *ev;
	pthread_mutexattr_t mtxattr;

	ev = (osd_event *)calloc(1, sizeof(osd_event));

	pthread_mutexattr_init(&mtxattr);
	pthread_mutex_init(&ev->mutex, &mtxattr);
	pthread_cond_init(&ev->cond, NULL);
	ev->signalled = initialstate;
	ev->autoreset = !manualreset;
	
	return ev;
}

//============================================================
//  osd_event_free
//============================================================

void osd_event_free(osd_event *event)
{
	pthread_mutex_destroy(&event->mutex);
	pthread_cond_destroy(&event->cond);
	free(event);
}

//============================================================
//  osd_event_set
//============================================================

void osd_event_set(osd_event *event)
{
	pthread_mutex_lock(&event->mutex);
	event->signalled = TRUE;
	pthread_cond_broadcast(&event->cond);
	pthread_mutex_unlock(&event->mutex);
}

//============================================================
//  osd_event_reset
//============================================================

void osd_event_reset(osd_event *event)
{
	pthread_mutex_lock(&event->mutex);
	event->signalled = FALSE;
	pthread_mutex_unlock(&event->mutex);
}

//============================================================
//  osd_event_wait
//============================================================

int osd_event_wait(osd_event *event, osd_ticks_t timeout)
{
	pthread_mutex_lock(&event->mutex);
	if (!timeout)
	{
		if (!event->signalled)
		{
			pthread_cond_wait(&event->cond, &event->mutex);
		}
	} 
	else
	{
		struct timespec   ts;
		struct timeval    tp;
		osd_ticks_t t;
		osd_ticks_t msec = timeout * 1000 / osd_ticks_per_second();

		gettimeofday(&tp, NULL);

		ts.tv_sec  = tp.tv_sec;
		ts.tv_nsec = tp.tv_usec * 1000 + (msec * 1000000);
		t = ts.tv_nsec / 1000000000;
		ts.tv_nsec = t % 1000000000;
		ts.tv_sec += t;

		if (!event->signalled)
			if ( pthread_cond_timedwait(&event->cond, &event->mutex, &ts) != 0 )
			{
				pthread_mutex_unlock(&event->mutex);
				return FALSE;
			}
	}
	if (event->autoreset)
		event->signalled = 0;
	pthread_mutex_unlock(&event->mutex);
	return TRUE;
}

//============================================================
//  osd_event_wait_multiple
//============================================================

int osd_event_wait_multiple(int num, osd_event *events[], osd_ticks_t timeout)
{
	int i;
	int ret;
	
	//FIXME: ugly and inefficient and ignores timeout
	do
	{
		for (i=0;i<num;i++)
		{
			ret = osd_event_wait(events[i], 10);
			if (ret == TRUE)
				return (i);
		}
		osd_sleep(20);
	}
	while (TRUE);
	return -1;
}
 
//============================================================
//  osd_atomic_lock
//============================================================

void osd_atomic_lock(void)
{
	if (!atomic_lck)
		atomic_lck = osd_lock_alloc();
	osd_lock_acquire(atomic_lck);
}

//============================================================
//  osd_atomic_unlock
//============================================================

void osd_atomic_unlock(void)
{
	osd_lock_release(atomic_lck);
}

//============================================================
//  osd_thread_create
//============================================================

osd_thread *osd_thread_create(osd_thread_callback callback, void *cbparam)
{
	osd_thread *thread;

	thread = (osd_thread *)calloc(1, sizeof(osd_thread));
	if ( pthread_create(&thread->thread, NULL, callback, cbparam) != 0 )
	{
		free(thread);
		return NULL;
	}
	return thread;
}

//============================================================
//  osd_thread_adjust_priority
//============================================================

int osd_thread_adjust_priority(osd_thread *thread, int adjust)
{
	struct sched_param	sched;
	int					policy;
			
	if ( pthread_getschedparam( thread->thread, &policy, &sched ) == 0 )
	{
		sched.sched_priority += adjust;
		if ( pthread_setschedparam(thread->thread, policy, &sched ) == 0)
			return TRUE;
		else
			return FALSE;
	}
	else
		return FALSE;
}

//============================================================
//  osd_thread_wait_free
//============================================================

void osd_thread_wait_free(osd_thread *thread)
{
	pthread_join(thread->thread, NULL);
	free(thread);
}

//============================================================
//  osd_compare_exchange32
//============================================================

#if defined(__i386__) || defined(__x86_64__)

INT32 osd_compare_exchange32(INT32 volatile *ptr, INT32 compare, INT32 exchange)
{
	register INT32 ret;
	__asm__ __volatile__ (
		" lock ; cmpxchg %[exchange], %[ptr] ;"
		: [ptr] "+m" (*ptr)
		, [ret] "=a" (ret)
		: [compare] "1" (compare)
		, [exchange] "q" (exchange)
		: "%cc"
	);
	return ret;
}

#elif defined(__ppc__) || defined(__PPC__) || defined(__ppc64__) || defined(__PPC64__)

INT32 osd_compare_exchange32(INT32 volatile *ptr, INT32 compare, INT32 exchange)
{
	register INT32 ret;
	__asm__ __volatile__ (
		"1: lwarx %[ret], 0, %[ptr] \n"
		" cmpw %[compare], %[ret] \n"
		" bne 2f \n"
		" sync \n"
		" stwcx. %[exchange], 0, %[ptr] \n"
		" bne- 1b \n"
		"2: "
		: [ret] "=&r" (ret)
		: [ptr] "r" (ptr)
		, [exchange] "r" (exchange)
		, [compare] "r" (compare)
		: "cr0"
	);
	return ret;
}
#else

INT32 osd_compare_exchange32(INT32 volatile *ptr, INT32 compare, INT32 exchange)
{
	INT32	ret;
	osd_atomic_lock();

	ret = *ptr;

	if ( *ptr == compare )
	{
		*ptr = exchange;
	}

	osd_atomic_unlock();	
	return ret;
}

#endif

//============================================================
//  osd_compare_exchange64
//============================================================

#if defined(__x86_64__)

INT64 osd_compare_exchange64(INT64 volatile *ptr, INT64 compare, INT64 exchange)
{
	register INT64 ret;
	__asm__ __volatile__ (
		" lock ; cmpxchg %[exchange], %[ptr] ;"
		: [ptr] "+m" (*ptr)
		, [ret] "=a" (ret)
		: [compare] "1" (compare)
		, [exchange] "q" (exchange)
		: "%cc"
	);
	return ret;
}

#elif defined(__ppc64__) || defined(__PPC64__)

INT64 osd_compare_exchange64(INT64 volatile *ptr, INT64 compare, INT64 exchange)
{
	register INT64 ret;
	__asm__ __volatile__ (
		"1: ldarx %[ret], 0, %[ptr] \n"
		" cmpd %[compare], %[ret] \n"
		" bne 2f \n"
		" stdcx. %[exchange], 0, %[ptr] \n"
		" bne-- 1b \n"
		"2: "
		: [ret] "=&r" (ret)
		: [ptr] "r" (ptr)
		, [exchange] "r" (exchange)
		, [compare] "r" (compare)
		: "cr0"
	);
	return ret;
}
#else

#ifdef PTR64
INT64 osd_compare_exchange64(INT64 volatile *ptr, INT64 compare, INT64 exchange)
{
	INT64	ret;
	osd_atomic_lock();

	ret = *ptr;

	if ( *ptr == compare )
	{
		*ptr = exchange;
	}

	osd_atomic_unlock();	
	return ret;
}
#endif

#endif

#else   // SDLMAME_OS2

struct _osd_lock
{
     HMTX   hmtx;
};

//============================================================
//  osd_lock_alloc
//============================================================

osd_lock *osd_lock_alloc(void)
{
     osd_lock *lock = malloc(sizeof(*lock));
     if (lock == NULL)
          return NULL;
     DosCreateMutexSem( NULL, &lock->hmtx, 0, FALSE );
     return lock;
}

//============================================================
//  osd_lock_acquire
//============================================================

void osd_lock_acquire(osd_lock *lock)
{
    DosRequestMutexSem( lock->hmtx, -1 );
}

//============================================================
//  osd_lock_try
//============================================================

int osd_lock_try(osd_lock *lock)
{
    return ( DosRequestMutexSem( lock->hmtx, 0 ) == 0 );
}

//============================================================
//  osd_lock_release
//============================================================

void osd_lock_release(osd_lock *lock)
{
    DosReleaseMutexSem( lock->hmtx );
}

//============================================================
//  osd_lock_free
//============================================================

void osd_lock_free(osd_lock *lock)
{
    DosCloseMutexSem( lock->hmtx );
    free( lock );
}
#endif
#else	// SDLMAME_WIN32
#include "../windows/winsync.c"
#endif

