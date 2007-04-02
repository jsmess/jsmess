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


typedef struct _pool_hash_entry pool_hash_entry;
struct _pool_hash_entry
{
	pool_entry *	first;
	pool_entry **	lastptr;
};


struct _memory_pool
{
	pool_hash_entry hashtable[3797];
	void (*fail)(const char *message);
};



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void report_failure(memory_pool *pool, const char *format, ...);
static int hash_value(memory_pool *pool, void *ptr);



/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    pool_create - creates a new memory pool
-------------------------------------------------*/

memory_pool *pool_create(void (*fail)(const char *message))
{
	memory_pool *pool;
	int i;

	/* allocate memory for the pool itself */
	pool = malloc(sizeof(*pool));
	if (pool == NULL)
		return NULL;

	/* initialize the data structures */
	pool->fail = fail;
	for (i = 0; i < ARRAY_LENGTH(pool->hashtable); i++)
	{
		pool->hashtable[i].first = NULL;
		pool->hashtable[i].lastptr = &pool->hashtable[i].first;
	}

	return pool;
}


/*-------------------------------------------------
    pool_clear - frees all contained memory blocks
    in a pool
-------------------------------------------------*/

void pool_clear(memory_pool *pool)
{
	pool_entry *next_entry;
	pool_entry *entry;
	int i;

	/* free all data in the pool */
	for (i = 0; i < ARRAY_LENGTH(pool->hashtable); i++)
	{
		for (entry = pool->hashtable[i].first; entry != NULL; entry = next_entry)
		{
			next_entry = entry->next;
			free(entry);
		}

		/* reinitialize the data structures */
		pool->hashtable[i].first = NULL;
		pool->hashtable[i].lastptr = &pool->hashtable[i].first;
	}
}


/*-------------------------------------------------
    pool_free - frees a memory pool and all
    contained memory blocks
-------------------------------------------------*/

void pool_free(memory_pool *pool)
{
	pool_clear(pool);
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
	pool_entry *new_entry = NULL;
	pool_entry **entry = NULL;
	pool_hash_entry *hash_entry = NULL;
	pool_entry *old_entry;

	/* if we're resizing or freeing a pointer, find the existing entry */
	if (ptr != NULL)
	{
		pool_entry *orig_entry;

		/* recover the pool from the pointer (ignoring the specified pool) */
		orig_entry = (pool_entry *)((UINT8 *)ptr - offsetof(pool_entry, buffer));
		if (orig_entry->cookie != MAGIC_COOKIE)
		{
			report_failure(pool, "pool_realloc: Non-pool pointer 0x%p", ptr);
			return NULL;
		}
		pool = orig_entry->pool;

		/* find the entry that this block is referring to */
		hash_entry = &pool->hashtable[hash_value(pool, ptr)];
		for (entry = &hash_entry->first; *entry != NULL; entry = &(*entry)->next)
			if ((*entry)->buffer == ptr)
				break;

		/* not found? */
		if (*entry == NULL)
		{
			report_failure(pool, "pool_realloc: Unknown pointer 0x%p", ptr);
			return NULL;
		}

		if (size != 0)
		{
			/* reallocate the entry */
			new_entry = realloc(*entry, offsetof(pool_entry, buffer) + size);
			if (new_entry == NULL)
			{
				report_failure(pool, "pool_realloc: Failure to realloc %u bytes", size);
				return NULL;
			}

			/* keep the newly reallocated entry in the linked list */
			*entry = new_entry;
			if (new_entry->next == NULL)
				hash_entry->lastptr = &new_entry->next;
		}
	}
	else if (size != 0)
	{
		/* allocate new entry */
		new_entry = malloc(offsetof(pool_entry, buffer) + size);
		if (new_entry == NULL)
		{
			report_failure(pool, "pool_realloc: Failure to malloc %u bytes", size);
			return NULL;
		}

		/* a new entry, set up magic values */
		new_entry->pool = pool;
		new_entry->cookie = MAGIC_COOKIE;
	}

	/* track the old entry, if any, because we're about to mess with the chain */
	old_entry = (entry && *entry) ? *entry : NULL;

	/* if we have a new entry, perform activities done when allocating and reallocating */
	if (new_entry != NULL)
	{
		size_t old_size;

		/* specify the new entry's size */
		new_entry->size = size;

		/* did the memory block grow? */
		old_size = (old_entry != NULL) ? old_entry->size : 0;
		if (size > old_size)
		{
#ifdef MAME_DEBUG
			/* randomize memory for debugging */
			rand_memory(new_entry->buffer + old_size, size - old_size);
#endif
		}
	}

	/* the memory allocation (if any) has succeeded, now we need to relink
     * under the (presumably) new hash value */
	if (entry != NULL)
	{
		/* unlink this entry from the link list with the old hash value */
		*entry = (*entry)->next;
		if (*entry == NULL)
			hash_entry->lastptr = entry;
	}

	/* if we allocated memory, we now have to add this entry to the new chain */
	if (new_entry != NULL)
	{
		/* identify the new hash entry */
		hash_entry = &pool->hashtable[hash_value(pool, new_entry->buffer)];

		/* add this entry to the new hash entry's linked list */
		*hash_entry->lastptr = new_entry;
		hash_entry->lastptr = &new_entry->next;
		new_entry->next = NULL;
	}
	else if (old_entry != NULL)
	{
		/* looks like we're freeing the entry */
		free(old_entry);
	}

	return (new_entry != NULL) ? new_entry->buffer : NULL;
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
	pool_hash_entry *hash_entry;
	pool_entry *entry;

	/* identify the hash header */
	hash_entry = &pool->hashtable[hash_value(pool, ptr)];

	/* find the entry that this block is referring to */
	for (entry = hash_entry->first; entry != NULL; entry = entry->next)
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
	int i;
	memory_block_overlap overlap = OVERLAP_NONE;
	const UINT8 *this_memory = (const UINT8 *) ptr;
	size_t this_memory_size = size;
	pool_entry *entry;
	const UINT8 *that_memory = NULL;
	size_t that_memory_size = 0;

	/* loop through the memory blocks in this pool */
	for (i = 0; i < ARRAY_LENGTH(pool->hashtable); i++)
	{
		for (entry = pool->hashtable[i].first; entry != NULL; entry = entry->next)
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



/*-------------------------------------------------
    hash_value - returns the hash value for a
    pointer
-------------------------------------------------*/

static int hash_value(memory_pool *pool, void *ptr)
{
	UINT64 ptr_int = (UINT64) (size_t) ptr;
	return (int) ((ptr_int ^ (ptr_int >> 3)) % ARRAY_LENGTH(pool->hashtable));
}



/***************************************************************************
    TESTING FUNCTIONS
***************************************************************************/

static int has_memory_error;


/*-------------------------------------------------
    memory_error - report a memory error
-------------------------------------------------*/

static void memory_error(const char *message)
{
	printf("memory test failure: %s\n", message);
	has_memory_error = TRUE;
}


/*-------------------------------------------------
    test_memory_pools - unit tests for memory
    pool behavior
-------------------------------------------------*/

int test_memory_pools(void)
{
	memory_pool *pool;
	void *ptrs[16];
	int i;

	has_memory_error = FALSE;
	pool = pool_create(memory_error);
	memset(ptrs, 0, sizeof(ptrs));

	ptrs[0] = pool_malloc(pool, 50);
	ptrs[1] = pool_malloc(pool, 100);

	ptrs[0] = pool_realloc(pool, ptrs[0], 150);
	ptrs[1] = pool_realloc(pool, ptrs[1], 200);

	ptrs[2] = pool_malloc(pool, 250);
	ptrs[3] = pool_malloc(pool, 300);

	ptrs[0] = pool_realloc(pool, ptrs[0], 350);
	ptrs[1] = pool_realloc(pool, ptrs[1], 400);

	ptrs[2] = pool_realloc(pool, ptrs[2], 450);
	ptrs[3] = pool_realloc(pool, ptrs[3], 500);

	ptrs[0] = pool_realloc(pool, ptrs[0], 0);
	ptrs[1] = pool_realloc(pool, ptrs[1], 0);

	ptrs[2] = pool_realloc(pool, ptrs[2], 550);
	ptrs[3] = pool_realloc(pool, ptrs[3], 600);

	/* some heavier stress tests */
	for (i = 0; i < 512; i++)
	{
		ptrs[i % ARRAY_LENGTH(ptrs)] = pool_realloc(pool,
			ptrs[i % ARRAY_LENGTH(ptrs)], rand() % 1000);
	}

	pool_free(pool);
	return has_memory_error;
}
