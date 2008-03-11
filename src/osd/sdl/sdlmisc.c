/* Miscelancelous utility functions

   Copyright 2000 Hans de Goede

   This file and the acompanying files in this directory are free software;
   you can redistribute them and/or modify them under the terms of the GNU
   Library General Public License as published by the Free Software Foundation;
   either version 2 of the License, or (at your option) any later version.

   These files are distributed in the hope that they will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with these files; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
/* Changelog
Version 0.1, February 2000
-initial release (Hans de Goede)
Version 0.2, May 2000
-moved time system headers from .h to .c to avoid header conflicts (Hans de Goede)
-made uclock a function on systems without gettimeofday too (Hans de Goede)
-UCLOCKS_PER_SECOND is now always 1000000, instead of making it depent on
 the system headers. (Hans de Goede)
*/
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

void *osd_alloc_executable(size_t size)
{
#if defined(SDLMAME_DARWIN)
	return (void *)malloc(size);
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
	#ifdef ENABLE_DEBUGGER
	printf("MAME exception: %s\n", message);
	printf("Attempting to fall into debugger\n");
	kill(getpid(), SIGTRAP); 
	#else
	printf("Ignoring MAME exception: %s\n", message);
	#endif
#endif
}

