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
	fprintf(stderr, "Error on lock: %d\n", r);
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
		fprintf(stderr, "Error on trylock: %d\n", r);
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

	pthread_mutex_destroy(&mutex->id);
	free(mutex);
}
#else	// SDLMAME_WIN32
#include "../windows/winsync.c"
#endif

