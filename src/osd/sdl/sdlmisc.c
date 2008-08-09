//============================================================
//
//  sdlmisc.c - SDL utility functions
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//  Note: previous versions of this file were from XMAME and marked as GPL.
//        all of that code is now gone/rewritten so that attribution has been removed.
//
//============================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>

#include "osdcore.h"

#if defined(SDLMAME_UNIX) && !defined(SDLMAME_DARWIN)
#include <sys/mman.h>
#endif
#ifdef SDLMAME_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#if defined(SDLMAME_UNIX)
#include <signal.h>
#include <unistd.h>
#endif

#ifdef SDLMAME_OS2
#define INCL_DOS
#include <os2.h>
#endif

//============================================================
//  osd_alloc_executable
//
//  allocates "size" bytes of executable memory.  this must take
//  things like NX support into account.
//============================================================

void *osd_alloc_executable(size_t size)
{
#if defined(SDLMAME_DARWIN)
	return (void *)malloc(size);
#elif defined(SDLMAME_FREEBSD)
	return (void *)mmap(0, size, PROT_EXEC|PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
#elif defined(SDLMAME_UNIX)
	return (void *)mmap(0, size, PROT_EXEC|PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, 0, 0);
#elif defined(SDLMAME_WIN32)
	return VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#elif defined(SDLMAME_OS2)
	void *p;
	
	DosAllocMem( &p, size, fALLOC );
	return p;
#else
#error Undefined SDLMAME SUBARCH!
#endif
}

//============================================================
//  osd_free_executable
//
//  frees memory allocated with osd_alloc_executable
//============================================================

void osd_free_executable(void *ptr, size_t size)
{
#ifdef SDLMAME_DARWIN
	free(ptr);
#elif defined(SDLMAME_UNIX)
	munmap(ptr, size);
#elif defined(SDLMAME_WIN32)
	VirtualFree(ptr, 0, MEM_RELEASE);
#elif defined(SDLMAME_OS2)
	DosFreeMem( ptr );
#else
#error Undefined SDLMAME SUBARCH!
#endif
}

//============================================================
//  osd_break_into_debugger
//============================================================

void osd_break_into_debugger(const char *message)
{
#ifdef SDLMAME_WIN32
	if (IsDebuggerPresent())
	{
		OutputDebugString(message);
		DebugBreak();
	}
#elif defined(SDLMAME_UNIX)
	#ifdef MAME_DEBUG
	printf("MAME exception: %s\n", message);
	printf("Attempting to fall into debugger\n");
	kill(getpid(), SIGTRAP); 
	#else
	printf("Ignoring MAME exception: %s\n", message);
	#endif
#endif
}

