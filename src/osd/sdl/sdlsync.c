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

// standard C headers
#include <math.h>
#include <unistd.h>

// MAME headers
#include "osdcore.h"
#include "mame.h"

#ifndef SDLMAME_WIN32
#ifndef SDLMAME_OS2
#include <pthread.h>
#include <errno.h>

typedef struct _hidden_mutex_t hidden_mutex_t;
struct _hidden_mutex_t {
	pthread_mutex_t id;
};

//============================================================
//  osd_lock_alloc
//============================================================

osd_lock *osd_lock_alloc(void)
{
	hidden_mutex_t *mutex;
	pthread_mutexattr_t mtxattr;

	mutex = (hidden_mutex_t *)calloc(1, sizeof(hidden_mutex_t));

	pthread_mutexattr_init(&mtxattr);
	pthread_mutexattr_settype(&mtxattr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mutex->id, &mtxattr);

	return (osd_lock *)mutex;
}

//============================================================
//  osd_lock_acquire
//============================================================

void osd_lock_acquire(osd_lock *lock)
{
	hidden_mutex_t *mutex = (hidden_mutex_t *) lock;
	int r;
	
	r =	pthread_mutex_lock(&mutex->id);
	if (r==0)
		return;
	mame_printf_error("Error on lock: %d: %s\n", r, strerror(r));
}

//============================================================
//  osd_lock_try
//============================================================

int osd_lock_try(osd_lock *lock)
{
	hidden_mutex_t *mutex = (hidden_mutex_t *) lock;
	int r;
	
	r = pthread_mutex_trylock(&mutex->id);
	if (r==0)
		return 1;
	if (r!=EBUSY)
    	mame_printf_error("Error on trylock: %d: %s\n", r, strerror(r));
	return 0;
}

//============================================================
//  osd_lock_release
//============================================================

void osd_lock_release(osd_lock *lock)
{
	hidden_mutex_t *mutex = (hidden_mutex_t *) lock;

	pthread_mutex_unlock(&mutex->id);
}

//============================================================
//  osd_lock_free
//============================================================

void osd_lock_free(osd_lock *lock)
{
	hidden_mutex_t *mutex = (hidden_mutex_t *) lock;

	pthread_mutex_unlock(&mutex->id);
	pthread_mutex_destroy(&mutex->id);
	free(mutex);
}
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

