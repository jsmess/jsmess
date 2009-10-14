#include <stdio.h>
#include <stdarg.h>

#include "mamecore.h"

/*-------------------------------------------------
    fatalerror - print a message and escape back
    to the OSD layer
-------------------------------------------------*/

void CLIB_DECL fatalerror(const char *text, ...)
{
	va_list va;
	va_start(va, text);
	vfprintf(stderr, text, va);
	va_end(va);
	exit(-1);
}
