#include "emu.h"
#include "emuopts.h"
#include "pool.h"
#include <ctype.h>
#include <stdarg.h>

/* Variables to hold the status of various game options */
static FILE *errorlog;

const game_driver *const drivers[1] = { NULL };
int rompath_extra;
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



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

enum _memory_block_overlap
{
	OVERLAP_NONE,
	OVERLAP_PARTIAL,
	OVERLAP_FULL
};
typedef enum _memory_block_overlap memory_block_overlap;



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

/* pool list */
static object_pool *pools[64];

/* resource tracking */
int resource_tracking_tag = 0;



/***************************************************************************
    PROTOTYPES
***************************************************************************/

/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/
/*-------------------------------------------------
    current_pool - identifies the current memory
    pool
-------------------------------------------------*/

static object_pool *current_pool(void)
{
	object_pool *pool;
	assert_always((resource_tracking_tag > 0) && (resource_tracking_tag <= ARRAY_LENGTH(pools)), "Invalid resource_tracking_tag");
	pool = pools[resource_tracking_tag - 1];
	assert_always(pool != NULL, "current_pool() is NULL");
	return pool;
}


/*-------------------------------------------------
    restrack_register_object - registers an
    object with the current pool
-------------------------------------------------*/

void *restrack_register_object(object_type type, void *ptr, size_t size, const char *file, int line)
{
	return pool_object_add_file_line(current_pool(), type, ptr, size, file, line);
}


/*-------------------------------------------------
    auto_malloc_file_line - allocate auto-
    freeing memory
-------------------------------------------------*/

void *auto_malloc_file_line(running_machine &machine, size_t size, const char *file, int line)
{
	void *result = pool_malloc_file_line(current_pool(), size, file, line);
	return result;
}


/*-------------------------------------------------
    auto_realloc_file_line - reallocate auto-
    freeing memory
-------------------------------------------------*/

void *auto_realloc_file_line(running_machine &machine, void *ptr, size_t size, const char *file, int line)
{
	object_pool *pool = current_pool();
	if (ptr != NULL)
	{
		int tag = resource_tracking_tag;
		for (; tag > 0; tag--)
		{
			pool = pools[tag - 1];
			if (pool_object_exists(pool, OBJTYPE_MEMORY, ptr))
				break;
		}
		assert_always(tag > 0, "Failed to find alloc in pool");
	}

	return pool_realloc_file_line(pool, ptr, size, file, line);
}


/*-------------------------------------------------
    auto_strdup_file_line - allocate auto-freeing
    string
-------------------------------------------------*/

char *auto_strdup_file_line(running_machine &machine, const char *str, const char *file, int line)
{
	return pool_strdup_file_line(current_pool(), str, file, line);
}


/*-------------------------------------------------
    auto_strdup_allow_null_file_line - allocate
    auto-freeing string if str is null
-------------------------------------------------*/

char *auto_strdup_allow_null_file_line(running_machine &machine, const char *str, const char *file, int line)
{
	return (str != NULL) ? auto_strdup_file_line(machine, str, file, line) : NULL;
}



void *malloc_or_die_file_line(size_t size, const char *file, int line)
{
	return malloc(size);
}
