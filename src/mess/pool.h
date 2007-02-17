/*********************************************************************

	pool.h

	Utility code for maintaining memory pools and tagged memory pools

*********************************************************************/

#ifndef POOL_H
#define POOL_H

#include <stdlib.h>

/***************************************************************************

	Pool code

***************************************************************************/

typedef struct _memory_pool *memory_pool;

void pool_init(memory_pool *pool);
void pool_exit(memory_pool *pool);
void *pool_malloc(memory_pool *pool, size_t size);
void *pool_realloc(memory_pool *pool, void *ptr, size_t size);
char *pool_strdup(memory_pool *pool, const char *src);
char *pool_strdup_len(memory_pool *pool, const char *src, size_t len);
void pool_freeptr(memory_pool *pool, void *ptr);

/***************************************************************************

	Tagpool code

***************************************************************************/

typedef struct
{
	memory_pool mempool;
	struct tag_pool_header *header;
} tag_pool;

void tagpool_init(tag_pool *tpool);
void tagpool_exit(tag_pool *tpool);
void *tagpool_alloc(tag_pool *tpool, const char *tag, size_t size);
void *tagpool_lookup(tag_pool *tpool, const char *tag);

#endif /* POOL_H */
