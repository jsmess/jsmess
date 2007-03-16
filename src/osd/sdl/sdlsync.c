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
#define _GNU_SOURCE 	// for PTHREAD_MUTEX_RECURSIVE; needs to be here before other glibc headers are included
#endif

#include <SDL/SDL.h>

#ifdef SDLMAME_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// standard C headers
#include <math.h>
#include <unistd.h>

// MAME headers
#include "osdcore.h"

#ifndef SDLMAME_WIN32
#include <pthread.h>

//============================================================
//  osd_lock_alloc
//============================================================

osd_lock *osd_lock_alloc(void)
{
	pthread_mutex_t *mutex;
	pthread_mutexattr_t *mtxattr;

	mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	mtxattr = (pthread_mutexattr_t *)malloc(sizeof(pthread_mutexattr_t));

	pthread_mutexattr_init(mtxattr);
	pthread_mutexattr_settype(mtxattr, PTHREAD_MUTEX_RECURSIVE);
	
	pthread_mutex_init(mutex, mtxattr);

	return (osd_lock *)mutex;
}

//============================================================
//  osd_lock_acquire
//============================================================

void osd_lock_acquire(osd_lock *lock)
{
	pthread_mutex_lock((pthread_mutex_t *)lock);	
}

//============================================================
//  osd_lock_try
//============================================================

int osd_lock_try(osd_lock *lock)
{
	if (!pthread_mutex_lock((pthread_mutex_t *)lock))
	{
		return TRUE;
	}

	return FALSE;
}

//============================================================
//  osd_lock_release
//============================================================

void osd_lock_release(osd_lock *lock)
{
	pthread_mutex_unlock((pthread_mutex_t *)lock);	
}

//============================================================
//  osd_lock_free
//============================================================

void osd_lock_free(osd_lock *lock)
{
	pthread_mutex_destroy((pthread_mutex_t *)lock);
	free(lock);
}
#else	// SDLMAME_WIN32
#include "../windows/winsync.c"
#endif
