/***************************************************************************

    pool.c

    Memory pool code

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include <stdarg.h>
#include <stddef.h>
#include "pool.h"
#include "coreutil.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MAGIC_COOKIE	((size_t)0x12345678)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _pool_entry pool_entry;
struct _pool_entry
{
	memory_pool *	pool;
	pool_entry *	next;
	size_t 			size;
	size_t			cookie;
	char 			buffer[1];
};


struct _memory_pool
{
	pool_entry *	first;
	pool_entry **	lastptr;
	void (*fail)(const char *message);
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static pool_entry *alloc_entry(memory_pool *pool, size_t buffer_size, const char *file, int line);
static void report_failure(memory_pool *pool, const char *format, ...);



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    pool_create - creates a new memory pool
-------------------------------------------------*/

memory_pool *pool_create(void (*fail)(const char *message))
{
	memory_pool *pool;

	/* allocate memory for the pool itself */
	pool = malloc(sizeof(*pool));
	if (pool == NULL)
		return NULL;

	/* initialize the data structures */
	pool->first = NULL;
	pool->lastptr = &pool->first;
	pool->fail = fail;

	return pool;
}


/*-------------------------------------------------
    pool_free - frees a memory pool and all
    contained memory blocks
-------------------------------------------------*/

void pool_free(memory_pool *pool)
{
	pool_entry *next_entry;
	pool_entry *entry;

	/* free all data in the pool */
	for (entry = pool->first; entry != NULL; entry = next_entry)
	{
		next_entry = entry->next;
		free(entry);
	}
	free(pool);
}


/*-------------------------------------------------
    pool_malloc_file_line - allocates memory in a
    memory pool
-------------------------------------------------*/

void *pool_malloc_file_line(memory_pool *pool, size_t size, const char *file, int line)
{
	return pool_realloc_file_line(pool, NULL, size, file, line);
}


/*-------------------------------------------------
    pool_realloc_file_line - reallocates memory in
    a memory pool
-------------------------------------------------*/

void *pool_realloc_file_line(memory_pool *pool, void *ptr, size_t size, const char *file, int line)
{
	pool_entry *new_entry;

	/* if we're resizing or freeing a pointer, find the existing entry */
	if (ptr != NULL)
	{
		pool_entry *orig_entry;
		pool_entry **entry;

		/* recover the pool from the pointer (ignoring the specified pool) */
		orig_entry = (pool_entry *)((UINT8 *)ptr - offsetof(pool_entry, buffer));
		if (orig_entry->cookie != MAGIC_COOKIE)
		{
			report_failure(pool, "pool_realloc: Non-pool pointer 0x%p", ptr);
			return NULL;
		}
		pool = orig_entry->pool;

		/* note that we do not support freeing in this manner */
		if (size == 0)
		{
			report_failure(pool, "pool_realloc: Not allowed to realloc to size 0", ptr);
			return NULL;
		}

		/* find the entry that this block is referring to */
		for (entry = &pool->first; *entry != NULL; entry = &(*entry)->next)
			if ((*entry)->buffer == ptr)
				break;

		/* not found? */
		if (*entry == NULL)
		{
			report_failure(pool, "pool_realloc: Unknown pointer 0x%p", ptr);
			return NULL;
		}

		/* reallocate the entry */
		new_entry = realloc(*entry, offsetof(pool_entry, buffer) + size);
		if (new_entry == NULL)
		{
			report_failure(pool, "pool_realloc: Failure to realloc %u bytes", size);
			return NULL;
		}

		/* replace the entry with the new entry */
		*entry = new_entry;
	}

	/* if we don't have a pointer, just allocate a new entry */
	else
	{
		new_entry = alloc_entry(pool, size, file, line);
		if (new_entry == NULL)
			return NULL;
	}

	/* return a pointer to the new entry's buffer */
	return new_entry->buffer;
}


/*-------------------------------------------------
    pool_strdup_file_line - copies a string into a
    pool
-------------------------------------------------*/

char *pool_strdup_file_line(memory_pool *pool, const char *str, const char *file, int line)
{
	char *new_str = (char *)pool_malloc_file_line(pool, strlen(str) + 1, file, line);
	if (new_str != NULL)
		strcpy(new_str, str);
	return new_str;
}


/*-------------------------------------------------
    pointer_in_block - returns whether a pointer
    is within a memory block
-------------------------------------------------*/

static int pointer_in_block(const UINT8 *ptr, const UINT8 *block, size_t block_size)
{
	return (ptr >= block) && (ptr < (block + block_size));
}


/*-------------------------------------------------
    pool_owns_pointer - is the given pointer
    owned by the pool?
-------------------------------------------------*/

int pool_owns_pointer(memory_pool *pool, void *ptr)
{
	pool_entry *entry;

	/* find the entry that this block is referring to */
	for (entry = pool->first; entry != NULL; entry = entry->next)
		if (entry->buffer == ptr)
			return TRUE;

	return FALSE;
}


/*-------------------------------------------------
    pool_contains_block - determines if a pool
    contains a memory block
-------------------------------------------------*/

memory_block_overlap pool_contains_block(memory_pool *pool, void *ptr, size_t size,
	void **found_block, size_t *found_block_size)
{
	memory_block_overlap overlap = OVERLAP_NONE;
	const UINT8 *this_memory = (const UINT8 *) ptr;
	size_t this_memory_size = size;
	pool_entry *entry;
	const UINT8 *that_memory = NULL;
	size_t that_memory_size = 0;

	/* loop through the memory blocks in this pool */
	for (entry = pool->first; entry != NULL; entry = entry->next)
	{
		that_memory = (const UINT8 *) entry->buffer;
		that_memory_size = entry->size;

		/* check for overlap */
		if (pointer_in_block(this_memory, that_memory, that_memory_size))
		{
			if (pointer_in_block(this_memory + this_memory_size - 1, that_memory, that_memory_size))
				overlap = OVERLAP_FULL;
			else
				overlap = OVERLAP_PARTIAL;
			break;
		}
		else if (pointer_in_block(that_memory, this_memory, this_memory_size))
		{
			if (pointer_in_block(that_memory + that_memory_size - 1, this_memory, this_memory_size))
				overlap = OVERLAP_FULL;
			else
				overlap = OVERLAP_PARTIAL;
			break;
		}

		that_memory = NULL;
		that_memory_size = 0;
	}

	if (found_block != NULL)
		*found_block = (void *) that_memory;
	if (found_block_size != NULL)
		*found_block_size = that_memory_size;
	return overlap;
}



/***************************************************************************
    INTERNAL FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    alloc_entry - allocate an entry for a pool
-------------------------------------------------*/

static pool_entry *alloc_entry(memory_pool *pool, size_t buffer_size, const char *file, int line)
{
	pool_entry *entry;

	/* allocate the new entry */
	entry = malloc(offsetof(pool_entry, buffer) + buffer_size);
	if (entry == NULL)
	{
		/* failure */
		report_failure(pool, "alloc_entry: Failed to allocate %u bytes (%s:%d)", buffer_size, file, line);
		return NULL;
	}

	/* set up the new entry */
	memset(entry, 0, offsetof(pool_entry, buffer));
	entry->pool = pool;
	entry->size = buffer_size;
	entry->cookie = MAGIC_COOKIE;

	/* add this entry to the linked list */
	*pool->lastptr = entry;
	pool->lastptr = &entry->next;

#ifdef MAME_DEBUG
	/* randomize memory for debugging */
	rand_memory(entry->buffer, buffer_size);
#endif

	return entry;
}


/*-------------------------------------------------
    report_failure - report a failure to the
    failure callback
-------------------------------------------------*/

static void report_failure(memory_pool *pool, const char *format, ...)
{
	/* only do the work if we have a callback */
	if (pool->fail != NULL)
	{
		char message[1024];
		va_list argptr;

		/* generate the final message text */
		va_start(argptr, format);
		vsprintf(message, format, argptr);
		va_end(argptr);

		/* call the callback with the message */
		(*pool->fail)(message);
	}
}
