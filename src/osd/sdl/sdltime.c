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

#ifdef SDLMAME_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#endif

#ifdef SDLMAME_MACOSX
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

// MAME headers
#include "osdepend.h"


//============================================================
//  PROTOTYPES
//============================================================

static osd_ticks_t init_cycle_counter(void);
static osd_ticks_t rdtsc_cycle_counter(void);
#if defined(SDLMAME_MACOSX) || defined(SDLMAME_LINUX) || defined(SDLMAME_FREEBSD)
static osd_ticks_t time_cycle_counter(void);
#endif
#ifdef SDLMAME_WIN32
static osd_ticks_t performance_cycle_counter(void);
#endif
#ifdef SDLMAME_MACOSX
static osd_ticks_t mach_cycle_counter(void);
#endif

//============================================================
//  GLOBAL VARIABLES
//============================================================

// global cycle_counter function and divider
osd_ticks_t		(*cycle_counter)(void) = init_cycle_counter;
osd_ticks_t		(*ticks_counter)(void) = init_cycle_counter;
osd_ticks_t		ticks_per_second;
int			sdl_use_rdtsc = 0;


//============================================================
//  STATIC VARIABLES
//============================================================

static osd_ticks_t suspend_adjustment;
static osd_ticks_t suspend_time;


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

	// if the RDTSC instruction is available use it because
	// it is more precise and has less overhead than timeGetTime()
	if (!sdl_use_rdtsc && QueryPerformanceFrequency( &frequency ))
	{
		// use performance counter if available as it is constant
		cycle_counter = performance_cycle_counter;
		ticks_counter = rdtsc_cycle_counter;

		ticks_per_second = frequency.QuadPart;

		// return the current cycle count
		return (*cycle_counter)();
	}
	else
	{
		cycle_counter = rdtsc_cycle_counter;
		ticks_counter = rdtsc_cycle_counter;
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

#ifdef SDLMAME_MACOSX
static osd_ticks_t init_cycle_counter(void)
{
	osd_ticks_t start, end;
	osd_ticks_t a, b;

	suspend_adjustment = 0;
	suspend_time = 0;

	#ifdef X86_ASM	// Intel OSX only
	if (sdl_use_rdtsc)
	{
		cycle_counter = rdtsc_cycle_counter;
		ticks_counter = rdtsc_cycle_counter;
	}
	else
	{
		cycle_counter = mach_cycle_counter;
		ticks_counter = mach_cycle_counter;
	}
	#else	// PowerPC OSX can choose SDL or Mach (shouldn't make a difference)
	if (sdl_use_rdtsc)
	{
		cycle_counter = mach_cycle_counter;
		ticks_counter = mach_cycle_counter;
	}
	else
	{
		cycle_counter = time_cycle_counter;
		ticks_counter = time_cycle_counter;
	}
	#endif

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

#if defined(SDLMAME_LINUX) || defined(SDLMAME_FREEBSD)
static osd_ticks_t init_cycle_counter(void)
{
	osd_ticks_t start, end;
	osd_ticks_t a, b;

	suspend_adjustment = 0;
	suspend_time = 0;

	#ifdef X86_ASM
	if (sdl_use_rdtsc)
	{
		cycle_counter = rdtsc_cycle_counter;
		ticks_counter = rdtsc_cycle_counter;
	}
	else
	#endif
	{
		cycle_counter = time_cycle_counter;
		ticks_counter = time_cycle_counter;
	}

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

//============================================================
//  mach_cycle_counter
//============================================================
#ifdef SDLMAME_MACOSX
static osd_ticks_t mach_cycle_counter(void)
{
	return mach_absolute_time();
}
#endif

//============================================================
//  rdtsc_cycle_counter
//============================================================

#ifdef _MSC_VER

static osd_ticks_t rdtsc_cycle_counter(void)
{
	INT64 result;
	INT64 *presult = &result;

	__asm {
		__asm _emit 0Fh __asm _emit 031h	// rdtsc
		mov ebx, presult
		mov [ebx],eax
		mov [ebx+4],edx
	}

	return result;
}

#else

#ifdef X86_ASM
static osd_ticks_t rdtsc_cycle_counter(void)
{
	INT64 result;

	// use RDTSC
	__asm__ __volatile__ (
		"rdtsc"
		: "=A" (result)
	);

	return result;
}
#endif
#endif



//============================================================
//  time_cycle_counter
//============================================================
#if defined(SDLMAME_MACOSX) || defined(SDLMAME_LINUX) || defined(SDLMAME_FREEBSD)
static osd_ticks_t time_cycle_counter(void)
{
	return SDL_GetTicks();
}
#endif


//============================================================
//  nop_cycle_counter
//============================================================
#if 0
static osd_ticks_t nop_cycle_counter(void)
{
	return 0;
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
	return ticks_per_second;
}



//============================================================
//  osd_profiling_ticks
//============================================================

osd_ticks_t osd_profiling_ticks(void)
{
	return (*ticks_counter)();
}



//============================================================
//  sdl_timer_enable
//============================================================

void sdl_timer_enable(int enabled)
{
	osd_ticks_t actual_cycles;

	actual_cycles = (*cycle_counter)();
	if (!enabled)
	{
		suspend_time = actual_cycles;
	}
	else if (suspend_time > 0)
	{
		suspend_adjustment += actual_cycles - suspend_time;
		suspend_time = 0;
	}
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

