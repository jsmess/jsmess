/***************************************************************************

    restrack.h

    Core MAME resource tracking.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "restrack.h"



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _callback_item callback_item;
struct _callback_item
{
	callback_item *	next;
	void			(*free_resources)(void);
};

typedef struct _malloc_entry malloc_entry;
struct _malloc_entry
{
	void *memory;
	size_t size;
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

/* malloc tracking */
static malloc_entry *malloc_list = NULL;
static int malloc_list_index = 0;
static int malloc_list_size = 0;

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
		int i;
		for (i = 0; i < size; i++)
			((UINT8 *)result)[i] = rand();
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
    auto_malloc_add - add pointer to malloc list
-------------------------------------------------*/

INLINE void auto_malloc_add(void *result, size_t size)
{
	/* make sure we have tracking space */
	if (malloc_list_index == malloc_list_size)
	{
		malloc_entry *list;

		/* if this is the first time, allocate 256 entries, otherwise double the slots */
		if (malloc_list_size == 0)
			malloc_list_size = 256;
		else
			malloc_list_size *= 2;

		/* realloc the list */
		list = realloc(malloc_list, malloc_list_size * sizeof(list[0]));
		if (list == NULL)
			fatalerror("Unable to extend malloc tracking array to %d slots", malloc_list_size);
		malloc_list = list;
	}
	malloc_list[malloc_list_index].memory = result;
	malloc_list[malloc_list_index].size = size;
	malloc_list_index++;
}


/*-------------------------------------------------
    auto_malloc_free - release auto_malloc'd memory
-------------------------------------------------*/

static void auto_malloc_free(void)
{
	/* start at the end and free everything till you reach the sentinel */
	while (malloc_list_index > 0 && malloc_list[--malloc_list_index].memory != NULL)
		free(malloc_list[malloc_list_index].memory);

	/* if we free everything, free the list */
	if (malloc_list_index == 0)
	{
		free(malloc_list);
		malloc_list = NULL;
		malloc_list_size = 0;
	}
}


/*-------------------------------------------------
    init_resource_tracking - initialize the
    resource tracking system
-------------------------------------------------*/

void init_resource_tracking(void)
{
	resource_tracking_tag = 0;
	add_free_resources_callback(auto_malloc_free);
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
    begin_resource_tracking - start tracking
    resources
-------------------------------------------------*/

void begin_resource_tracking(void)
{
	/* add a NULL as a sentinel */
	auto_malloc_add(NULL, 0);

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
}


/*-------------------------------------------------
    auto_malloc - allocate auto-freeing memory
-------------------------------------------------*/

void *_auto_malloc(size_t size, const char *file, int line)
{
	void *result;

	/* fail horribly if it doesn't work */
	result = _malloc_or_die(size, file, line);

	/* track this item in our list */
	auto_malloc_add(result, size);
	return result;
}


/*-------------------------------------------------
    auto_strdup - allocate auto-freeing string
-------------------------------------------------*/

char *_auto_strdup(const char *str, const char *file, int line)
{
	assert_always(str != NULL, "auto_strdup() requires non-NULL str");
	return strcpy(_auto_malloc(strlen(str) + 1, file, line), str);
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
    pointer_in_block - returns whether a pointer
    is within a memory block
-------------------------------------------------*/

static int pointer_in_block(const UINT8 *ptr, const UINT8 *block, size_t block_size)
{
	return (ptr >= block) && (ptr < (block + block_size));
}


/*-------------------------------------------------
    validate_auto_malloc_memory - validate that a
    block of memory has been allocated by auto_malloc()
-------------------------------------------------*/

void validate_auto_malloc_memory(void *memory, size_t memory_size)
{
	int i;
	int tag = 0;
	const UINT8 *this_memory = (const UINT8 *) memory;
	size_t this_memory_size = memory_size;

	assert(memory_size > 0);

	for (i = 0; i < malloc_list_size; i++)
	{
		if (malloc_list[i].memory != NULL)
		{
			const UINT8 *that_memory = (const UINT8 *) malloc_list[i].memory;
			size_t that_memory_size = malloc_list[i].size;

			if (pointer_in_block(this_memory, that_memory, that_memory_size))
			{
				if (!pointer_in_block(this_memory + this_memory_size - 1, that_memory, that_memory_size))
					fatalerror("Memory block [0x%p-0x%p] partially overlaps with allocated block [0x%p-0x%p]", this_memory, this_memory + this_memory_size - 1, that_memory, that_memory + that_memory_size - 1);
				return;
			}
			else if (pointer_in_block(that_memory, this_memory, this_memory_size))
			{
				if (!pointer_in_block(that_memory + that_memory_size - 1, this_memory, this_memory_size))
					fatalerror("Memory block [0x%p-0x%p] partially overlaps with allocated block [0x%p-0x%p]", this_memory, this_memory + this_memory_size - 1, that_memory, that_memory + that_memory_size - 1);
				return;
			}
		}
		else
		{
			tag++;
		}
	}
	fatalerror("Memory block [0x%p-0x%p] not found", this_memory, this_memory + this_memory_size - 1);
}

