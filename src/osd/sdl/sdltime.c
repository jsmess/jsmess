//============================================================
//
//  sdltime.c - POSIX, SDL, and Win32 timing code
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

// standard sdl header
#include <SDL/SDL.h>
#include <unistd.h>

#ifdef SDLMAME_UNIX
#ifndef SDLMAME_DARWIN
#include <time.h>
#include <sys/time.h>
#endif
#endif

#ifdef SDLMAME_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#endif

#ifdef SDLMAME_DARWIN
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

#ifdef SDLMAME_OS2
#define INCL_DOS
#include <os2.h>
#endif

// MAME headers
#include "osdepend.h"


//============================================================
//  PROTOTYPES
//============================================================

static osd_ticks_t init_cycle_counter(void);
#if defined(SDLMAME_UNIX) && !defined(SDLMAME_DARWIN)
static osd_ticks_t time_cycle_counter(void);
#endif
#if defined(SDLMAME_WIN32) || defined(SDLMAME_OS2)
static osd_ticks_t performance_cycle_counter(void);
#endif
#ifdef SDLMAME_DARWIN
static osd_ticks_t mach_cycle_counter(void);
#endif

//============================================================
//  GLOBAL VARIABLES
//============================================================



//============================================================
//  STATIC VARIABLES
//============================================================

static osd_ticks_t suspend_adjustment;
static osd_ticks_t suspend_time;

// global cycle_counter function and divider
static osd_ticks_t		(*cycle_counter)(void) = init_cycle_counter;
static osd_ticks_t		(*ticks_counter)(void) = init_cycle_counter;
static osd_ticks_t		ticks_per_second;


//============================================================
//  init_cycle_counter
//
//  to avoid total grossness, this function is split by subarch
//============================================================

#ifdef SDLMAME_WIN32
static osd_ticks_t init_cycle_counter(void)
{
	osd_ticks_t start, end;
	osd_ticks_t a, b;
	int priority = GetThreadPriority(GetCurrentThread());
	LARGE_INTEGER frequency;

	suspend_adjustment = 0;
	suspend_time = 0;

	if (QueryPerformanceFrequency( &frequency ))
	{
		// use performance counter if available as it is constant
		cycle_counter = performance_cycle_counter;
		ticks_counter = performance_cycle_counter;

		ticks_per_second = frequency.QuadPart;

		// return the current cycle count
		return (*cycle_counter)();
	}
	else
	{
		fprintf(stderr, "Error!  Unable to QueryPerformanceFrequency!\n");
		exit(-1);
	}

	// temporarily set our priority higher
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	// wait for an edge on the timeGetTime call
	a = SDL_GetTicks();
	do
	{
		b = SDL_GetTicks();
	} while (a == b);

	// get the starting cycle count
	start = (*cycle_counter)();

	// now wait for 1/4 second total
	do
	{
		a = SDL_GetTicks();
	} while (a - b < 250);

	// get the ending cycle count
	end = (*cycle_counter)();

	// compute ticks_per_sec
	ticks_per_second = (end - start) * 4;

	// restore our priority
	SetThreadPriority(GetCurrentThread(), priority);

	// return the current cycle count
	return (*cycle_counter)();
}
#endif

#ifdef SDLMAME_DARWIN
static osd_ticks_t init_cycle_counter(void)
{
	osd_ticks_t start, end;
	osd_ticks_t a, b;

	suspend_adjustment = 0;
	suspend_time = 0;

	cycle_counter = mach_cycle_counter;
	ticks_counter = mach_cycle_counter;

	// wait for an edge on the timeGetTime call
	a = SDL_GetTicks();
	do
	{
		b = SDL_GetTicks();
	} while (a == b);

	// get the starting cycle count
	start = (*cycle_counter)();

	// now wait for 1/4 second total
	do
	{
		a = SDL_GetTicks();
	} while (a - b < 250);

	// get the ending cycle count
	end = (*cycle_counter)();

	// compute ticks_per_sec
	ticks_per_second = (end - start) * 4;

	// return the current cycle count
	return (*cycle_counter)();
}
#endif

#if defined(SDLMAME_UNIX) && !defined(SDLMAME_DARWIN)
static osd_ticks_t init_cycle_counter(void)
{
	osd_ticks_t start, end;
	osd_ticks_t a, b;

	suspend_adjustment = 0;
	suspend_time = 0;

	cycle_counter = time_cycle_counter;
	ticks_counter = time_cycle_counter;

	// wait for an edge on the timeGetTime call
	a = SDL_GetTicks();
	do
	{
		b = SDL_GetTicks();
	} while (a == b);

	// get the starting cycle count
	start = (*cycle_counter)();

	// now wait for 1/4 second total
	do
	{
		a = SDL_GetTicks();
	} while (a - b < 250);

	// get the ending cycle count
	end = (*cycle_counter)();

	// compute ticks_per_sec
	ticks_per_second = (end - start) * 4;

	// return the current cycle count
	return (*cycle_counter)();
}
#endif

#ifdef SDLMAME_OS2
static osd_ticks_t init_cycle_counter(void)
{
	osd_ticks_t start, end;
	osd_ticks_t a, b;

	ULONG  frequency;
	PTIB   ptib;
	ULONG  ulClass;
	ULONG  ulDelta;

	DosGetInfoBlocks( &ptib, NULL );
	ulClass = HIBYTE( ptib->tib_ptib2->tib2_ulpri );
	ulDelta = LOBYTE( ptib->tib_ptib2->tib2_ulpri );

	suspend_adjustment = 0;
	suspend_time = 0;

	if ( DosTmrQueryFreq( &frequency ) == 0 )
	{
		// use performance counter if available as it is constant
		cycle_counter = performance_cycle_counter;
		ticks_counter = performance_cycle_counter;

		ticks_per_second = frequency;

		// return the current cycle count
		return (*cycle_counter)();
	}
	else
	{
		fprintf(stderr, "No Timer available!\n");
		exit(-1);
	}

	// temporarily set our priority higher
	DosSetPriority( PRTYS_THREAD, PRTYC_TIMECRITICAL, PRTYD_MAXIMUM, 0 );

	// wait for an edge on the timeGetTime call
	a = SDL_GetTicks();
	do
	{
		b = SDL_GetTicks();
	} while (a == b);

	// get the starting cycle count
	start = (*cycle_counter)();

	// now wait for 1/4 second total
	do
	{
		a = SDL_GetTicks();
	} while (a - b < 250);

	// get the ending cycle count
	end = (*cycle_counter)();

	// compute ticks_per_sec
	ticks_per_second = (end - start) * 4;

	// restore our priority
	DosSetPriority( PRTYS_THREAD, ulClass, ulDelta, 0 );

	// return the current cycle count
	return (*cycle_counter)();
}
#endif

//============================================================
//  performance_cycle_counter
//============================================================
#ifdef SDLMAME_WIN32
static osd_ticks_t performance_cycle_counter(void)
{
	LARGE_INTEGER performance_count;
	QueryPerformanceCounter(&performance_count);
	return (osd_ticks_t)performance_count.QuadPart;
}
#endif

#ifdef SDLMAME_OS2
static osd_ticks_t performance_cycle_counter(void)
{
    QWORD qwTime;

    DosTmrQueryTime( &qwTime );
    return (osd_ticks_t)qwTime.ulLo;
}
#endif

//============================================================
//  mach_cycle_counter
//============================================================
#ifdef SDLMAME_DARWIN
static osd_ticks_t mach_cycle_counter(void)
{
	return mach_absolute_time();
}
#endif

//============================================================
//  time_cycle_counter
//============================================================
#if defined(SDLMAME_UNIX) && !defined(SDLMAME_DARWIN)

static osd_ticks_t unix_ticks(void)
{
		struct timeval    tp;
		static osd_ticks_t start_sec = 0;
		
		gettimeofday(&tp, NULL);
		if (start_sec==0)
			start_sec = tp.tv_sec;
		return (tp.tv_sec - start_sec) * (osd_ticks_t) 1000000 + tp.tv_usec;
}


static osd_ticks_t time_cycle_counter(void)
{
	return unix_ticks();
}
#endif

//============================================================
//  osd_cycles
//============================================================

osd_ticks_t osd_ticks(void)
{
	return suspend_time ? suspend_time : (*cycle_counter)() - suspend_adjustment;
}



//============================================================
//  osd_ticks_per_second
//============================================================

osd_ticks_t osd_ticks_per_second(void)
{
	if (ticks_per_second == 0)
	{
		return 1;	// this isn't correct, but it prevents the crash
	}
	return ticks_per_second;
}



//============================================================
//  osd_profiling_ticks
//============================================================

#if defined(__i386__) || defined(__x86_64__)
static UINT64 last_result = 0;
static UINT64 current = 0;

static UINT64 rdtsc(void)
{
    UINT64 result;

    // use RDTSC
    __asm__ __volatile__ (
        "rdtsc"
        : "=A" (result)
    );
    return result;
}
#endif

osd_ticks_t osd_profiling_ticks(void)
{
#if defined(__i386__) || defined(__x86_64__)
    UINT64 result;
    UINT64 diff;

    result = rdtsc();
    if (result > last_result)
    {
        diff = result - last_result;
        current += diff;
    } else
        printf("less @ %lld\n", current);
    last_result = result;

    /* osd_ticks_t is defined as INT64 */
    current &= U64(0x7FFFFFFFFFFFFFFF);

    return current;
#else
    return (*ticks_counter)();
#endif
}

//============================================================
//  osd_sleep
//============================================================

void osd_sleep(osd_ticks_t duration)
{
	UINT32 msec;

	// make sure we've computed ticks_per_second
	if (ticks_per_second == 0)
		(void)osd_ticks();

	// convert to milliseconds, rounding down
	msec = (UINT32)(duration * 1000 / ticks_per_second);

	// only sleep if at least 2 full milliseconds
	if (msec >= 2)
	{
		// take a couple of msecs off the top for good measure
		msec -= 2;
		#ifdef SDLMAME_WIN32
		Sleep(msec);
		#else
		usleep(msec*1000);
		#endif
	}
}

