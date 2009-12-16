/*********************************************************************

    tagpool.h

    Utility code for maintaining tagged memory pools

*********************************************************************/

#ifndef TAGPOOL_H
#define TAGPOOL_H

#include <stdlib.h>
#include "pool.h"


/***************************************************************************

    Tagpool code

***************************************************************************/

typedef struct
{
	object_pool *mempool;
	struct tag_pool_header *header;
} tag_pool;

void tagpool_init(tag_pool *tpool);
void tagpool_exit(tag_pool *tpool);
void *tagpool_alloc(tag_pool *tpool, const char *tag, size_t size);
void *tagpool_lookup(tag_pool *tpool, const char *tag);

#endif /* TAGPOOL_H */
