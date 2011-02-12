#include "emu.h"
#include <ctype.h>
#include <stdarg.h>

/* Variables to hold the status of various game options */
static FILE *errorlog;

const game_driver *const drivers[1] = { NULL };
int rompath_extra;
//int cheatfile;
//const char *db_filename;
int history_filename;
int mameinfo_filename;


void CLIB_DECL logerror(const char *text,...)
{
	va_list arg;
	va_start(arg,text);
	if (errorlog)
		vfprintf(errorlog,text,arg);
	va_end(arg);
}

/* ----------------------------------------------------------------------- */
/* total hack */

file_error mame_fopen(const char *searchpath, const char *filename, UINT32 openflags, emu_file **file)
{
	char buffer[2048];
	snprintf(buffer, sizeof(buffer), "crc/%s", filename);
	*file = (emu_file *) fopen(buffer, "r");
	return FILERR_NONE;
}

char *mame_fgets(char *s, int n, emu_file *file)
{
	return fgets(s, n, (FILE *) file);
}

UINT32 mame_fwrite(emu_file *file, const void *buffer, UINT32 length)
{
	return fwrite(buffer, 1, length, (FILE *) file);
}

void mame_fclose(emu_file *file)
{
	fclose((FILE *) file);
}

int CLIB_DECL mame_fprintf(emu_file *f, const char *fmt, ...)
{
	int rc;
	va_list va;
	va_start(va, fmt);
	rc = vfprintf(stderr, fmt, va);
	va_end(va);
	return rc;
}

void mame_printf_error(const char *format, ...)
{
}

void mame_printf_warning(const char *format, ...)
{
}

void mame_printf_info(const char *format, ...)
{
}

void mame_printf_debug(const char *format, ...)
{
}



