/***************************************************************************

    pool.h

    Memory pool code

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#ifndef __POOL_H__

#include "osdcore.h"


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _memory_pool memory_pool;

typedef enum _memory_block_overlap memory_block_overlap;
enum _memory_block_overlap
{
	OVERLAP_NONE,
	OVERLAP_PARTIAL,
	OVERLAP_FULL
};



/***************************************************************************
    PROTOTYPES
***************************************************************************/

memory_pool *pool_create(void (*fail)(const char *message));
void pool_free(memory_pool *pool);

void *pool_malloc_file_line(memory_pool *pool, size_t size, const char *file, int line) ATTR_MALLOC;
void *pool_realloc_file_line(memory_pool *pool, void *ptr, size_t size, const char *file, int line);
char *pool_strdup_file_line(memory_pool *pool, const char *str, const char *file, int line);

#define pool_malloc(pool, size)				pool_malloc_file_line((pool), (size), __FILE__, __LINE__)
#define pool_realloc(pool, ptr, size)		pool_realloc_file_line((pool), (ptr), (size), __FILE__, __LINE__)
#define pool_strdup(pool, size)				pool_strdup_file_line((pool), (size), __FILE__, __LINE__)

memory_block_overlap pool_contains_block(memory_pool *pool, void *ptr, size_t size,
	void **found_block, size_t *found_block_size);

#endif /* __POOL_H__ */
