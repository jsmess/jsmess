/***************************************************************************

    pool.c

    Resource pool code

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include <stdarg.h>
#include <stddef.h>
#include "pool.h"
#include "coreutil.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _pool_entry pool_entry;
struct _pool_entry
{
	pool_entry *next;
	size_t size;
	char buffer[1];
};



struct _resource_pool
{
	pool_entry *first;
	pool_entry *last;
	void (*fail)(const char *message);
};



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

static void report_failure(memory_pool *pool, const char *format, ...)
{
	va_list argptr;
	char message[256];

	if (pool->fail)
	{
		va_start(argptr, format);
		vsprintf(message, format, argptr);
		va_end(argptr);

		pool->fail(message);
	}
}



#ifdef MAME_DEBUG
static void rand_memory(void *memory, size_t length)
{
	UINT8 *bytes = (UINT8 *) memory;
	size_t i;

	for (i = 0; i < length; i++)
	{
		bytes[i] = (UINT8) rand();
	}

}
#endif /* MAME_DEBUG */



static pool_entry *alloc_entry(memory_pool *pool, size_t buffer_size, const char *file, int line)
{
	pool_entry *entry;

	/* allocate the new entry */
	entry = malloc(sizeof(*entry) - sizeof(entry->buffer) + buffer_size);
	if (!entry)
	{
		/* failure */
		report_failure(pool, "Failed to allocate %u bytes (%s:%d)", buffer_size, file, line);
		return NULL;
	}

	/* set up the new entry */
	memset(entry, 0, sizeof(*entry) - sizeof(entry->buffer));
	entry->size = buffer_size;

	/* add this entry to the linked list */
	if (pool->last)
		pool->last->next = entry;
	else
		pool->first = entry;
	pool->last = entry;

#ifdef MAME_DEBUG
	rand_memory(entry->buffer, buffer_size);
#endif

	return entry;
}



/*-------------------------------------------------
    pool_create - creates a new resource pool
-------------------------------------------------*/

memory_pool *pool_create(void (*fail)(const char *message))
{
	memory_pool *pool;
	pool = malloc(sizeof(*pool));
	if (!pool)
		return NULL;

	memset(pool, 0, sizeof(*pool));
	pool->fail = fail;
	return pool;
}



/*-------------------------------------------------
    pool_free - frees a resource pool and all
	contained resources
-------------------------------------------------*/

void pool_free(memory_pool *pool)
{
	pool_entry *entry;
	pool_entry *next_entry;

	entry = pool->first;
	while(entry)
	{
		next_entry = entry->next;
		free(entry);
		entry = next_entry;
	}
	free(pool);
}



/*-------------------------------------------------
    pool_malloc_file_line - allocates memory in a
	resource pool
-------------------------------------------------*/

void *pool_malloc_file_line(memory_pool *pool, size_t size, const char *file, int line)
{
	return pool_realloc_file_line(pool, NULL, size, file, line);
}



/*-------------------------------------------------
    pool_realloc_file_line - reallocates memory in
	a resource pool
-------------------------------------------------*/

void *pool_realloc_file_line(memory_pool *pool, void *ptr, size_t size, const char *file, int line)
{
	pool_entry **entry;
	pool_entry *new_entry;

	if (ptr)
	{
		/* find the entry that this block is referring to */
		for (entry = &pool->first; *entry; entry = &(*entry)->next)
		{
			if ((*entry)->buffer == ptr)
				break;
		}

		/* not found? */
		if (!*entry)
		{
			report_failure(pool, "Unknown pointer 0x%p", ptr);
			return NULL;
		}

		/* reallocate the entry */
		new_entry = realloc(*entry, offsetof(pool_entry, buffer) + size);
		if (!new_entry)
		{
			report_failure(pool, "Failure to realloc %u bytes", size);
			return NULL;
		}

		*entry = new_entry;
	}
	else
	{
		new_entry = alloc_entry(pool, size, file, line);
		if (!new_entry)
			return NULL;
	}
	return new_entry->buffer;
}



/*-------------------------------------------------
    pool_strdup_file_line - copies a string into a
	pool
-------------------------------------------------*/

char *pool_strdup_file_line(memory_pool *pool, const char *str, const char *file, int line)
{
	char *new_str;
	new_str = (char *) pool_malloc_file_line(pool, strlen(str) + 1, file, line);
	if (new_str)
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
	for (entry = pool->first; entry; entry = entry->next)
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

	if (found_block)
		*found_block = (void *) that_memory;
	if (found_block_size)
		*found_block_size = that_memory_size;
	return overlap;
}
