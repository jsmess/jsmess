/***************************************************************************

    restrack.c

    Core MAME resource tracking.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "restrack.h"
#include "pool.h"



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _callback_item callback_item;
struct _callback_item
{
	callback_item *	next;
	void			(*free_resources)(void);
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

/* pool list */
static memory_pool *pools[64];

/* resource tracking */
int resource_tracking_tag = 0;

/* free resource callbacks */
static callback_item *free_resources_callback_list;



/***************************************************************************
    CORE IMPLEMENTATION
***************************************************************************/

/*-------------------------------------------------
    _malloc_or_die - allocate memory or die
    trying
-------------------------------------------------*/

void *_malloc_or_die(size_t size, const char *file, int line)
{
	void *result;

	/* fail on attempted allocations of 0 */
	if (size == 0)
		fatalerror("Attempted to malloc zero bytes (%s:%d)", file, line);

	/* allocate and return if we succeeded */
#ifdef MALLOC_DEBUG
	result = malloc_file_line(size, file, line);
#else
	result = malloc(size);
#endif
	if (result != NULL)
	{
#ifdef MAME_DEBUG
		rand_memory(result, size);
#endif
		return result;
	}

	/* otherwise, die horribly */
	fatalerror("Failed to allocate %d bytes (%s:%d)", (int)size, file, line);
}


/*-------------------------------------------------
    add_free_resources_callback - add a callback to
    be invoked on end_resource_tracking()
-------------------------------------------------*/

void add_free_resources_callback(void (*callback)(void))
{
	callback_item *cb, **cur;

	cb = malloc_or_die(sizeof(*cb));
	cb->free_resources = callback;
	cb->next = NULL;

	for (cur = &free_resources_callback_list; *cur != NULL; cur = &(*cur)->next) ;
	*cur = cb;
}


/*-------------------------------------------------
    free_callback_list - free a list of callbacks
-------------------------------------------------*/

static void free_callback_list(callback_item **cb)
{
	while (*cb != NULL)
	{
		callback_item *temp = *cb;
		*cb = (*cb)->next;
		free(temp);
	}
}


/*-------------------------------------------------
    init_resource_tracking - initialize the
    resource tracking system
-------------------------------------------------*/

void init_resource_tracking(void)
{
	resource_tracking_tag = 0;
}


/*-------------------------------------------------
    exit_resource_tracking - tear down the
    resource tracking system
-------------------------------------------------*/

void exit_resource_tracking(void)
{
	while (resource_tracking_tag != 0)
		end_resource_tracking();
	free_callback_list(&free_resources_callback_list);
}


/*-------------------------------------------------
    memory_error - report a memory error
-------------------------------------------------*/

static void memory_error(const char *message)
{
	fatalerror("%s", message);
}



/*-------------------------------------------------
    begin_resource_tracking - start tracking
    resources
-------------------------------------------------*/

void begin_resource_tracking(void)
{
	memory_pool *new_pool;

	/* sanity check */
	assert_always(resource_tracking_tag < ARRAY_LENGTH(pools), "Too many memory pools");

	/* create a new pool */
	new_pool = pool_create(memory_error);
	if (!new_pool)
		fatalerror("Failed to allocate new memory pool");
	pools[resource_tracking_tag] = new_pool;

	/* increment the tag counter */
	resource_tracking_tag++;
}


/*-------------------------------------------------
    end_resource_tracking - stop tracking
    resources
-------------------------------------------------*/

void end_resource_tracking(void)
{
	callback_item *cb;

	/* call everyone who tracks resources to let them know */
	for (cb = free_resources_callback_list; cb; cb = cb->next)
		(*cb->free_resources)();

	/* decrement the tag counter */
	resource_tracking_tag--;

	/* free the memory pool */
	pool_free(pools[resource_tracking_tag]);
	pools[resource_tracking_tag] = NULL;
}


/*-------------------------------------------------
    current_pool - identifies the current memory
    pool
-------------------------------------------------*/

static memory_pool *current_pool(void)
{
	return pools[resource_tracking_tag - 1];
}


/*-------------------------------------------------
    auto_malloc - allocate auto-freeing memory
-------------------------------------------------*/

void *_auto_malloc(size_t size, const char *file, int line)
{
	return pool_malloc_file_line(current_pool(), size, file, line);
}


/*-------------------------------------------------
    auto_realloc - reallocate auto-freeing memory
-------------------------------------------------*/

void *_auto_realloc(void *ptr, size_t size, const char *file, int line)
{
	return pool_realloc_file_line(current_pool(), ptr, size, file, line);
}


/*-------------------------------------------------
    auto_strdup - allocate auto-freeing string
-------------------------------------------------*/

char *_auto_strdup(const char *str, const char *file, int line)
{
	return pool_strdup_file_line(current_pool(), str, file, line);
}


/*-------------------------------------------------
    auto_strdup_allow_null - allocate auto-freeing
    string if str is null
-------------------------------------------------*/

char *_auto_strdup_allow_null(const char *str, const char *file, int line)
{
	return (str != NULL) ? _auto_strdup(str, file, line) : NULL;
}


/*-------------------------------------------------
    auto_bitmap_alloc - allocate auto-freeing
    bitmap
-------------------------------------------------*/

static void *auto_bitmap_allocator(size_t size)
{
	return auto_malloc(size);
}

bitmap_t *auto_bitmap_alloc(int width, int height, bitmap_format format)
{
	return bitmap_alloc_custom(width, height, format, auto_bitmap_allocator);
}


/*-------------------------------------------------
    validate_auto_malloc_memory - validate that a
    block of memory has been allocated by auto_malloc()
-------------------------------------------------*/

void validate_auto_malloc_memory(void *memory, size_t memory_size)
{
	int i;
	memory_block_overlap overlap = OVERLAP_NONE;
	void *block_base = NULL;
	size_t block_size = 0;

	for (i = 0; (overlap == OVERLAP_NONE) && (i < resource_tracking_tag); i++)
	{
		overlap = pool_contains_block(pools[i], memory, memory_size,
			&block_base, &block_size);
	}

	switch(overlap)
	{
		case OVERLAP_NONE:
			fatalerror("Memory block [0x%p-0x%p] not found",
				memory,
				((UINT8 *) memory) + memory_size - 1);
			break;

		case OVERLAP_PARTIAL:
			fatalerror("Memory block [0x%p-0x%p] partially overlaps with allocated block [0x%p-0x%p]",
				memory,
				((UINT8 *) memory) + memory_size - 1,
				block_base,
				((UINT8 *) block_base) + block_size - 1);
			break;

		case OVERLAP_FULL:
			/* expected outcome */
			break;
	}
}
