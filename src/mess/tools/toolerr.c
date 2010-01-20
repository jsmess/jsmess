#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
/*-------------------------------------------------
    fatalerror - print a message and escape back
    to the OSD layer
-------------------------------------------------*/

extern "C" void fatalerror(const char *text, ...)
{
	va_list va;
	va_start(va, text);
	vfprintf(stderr, text, va);
	va_end(va);
	exit(-1);
}
