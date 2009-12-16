/*********************************************************************

    tagpool.c

    Utility code for maintaining tagged memory pools

*********************************************************************/

#include <assert.h>
#include <string.h>

#include "tagpool.h"


/***************************************************************************

    Tagpool code

***************************************************************************/

struct tag_pool_header
{
	struct tag_pool_header *next;
	const char *tagname;
	void *tagdata;
};

void tagpool_init(tag_pool *tpool)
{
	tpool->mempool = pool_alloc(NULL);
	tpool->header = NULL;
}

void tagpool_exit(tag_pool *tpool)
{
	pool_free(tpool->mempool);
	tpool->header = NULL;
}

void *tagpool_alloc(tag_pool *tpool, const char *tag, size_t size)
{
	struct tag_pool_header **header;
	struct tag_pool_header *newheader;

	newheader = (struct tag_pool_header *) pool_malloc(tpool->mempool, sizeof(struct tag_pool_header));
	if (!newheader)
		return NULL;

	newheader->tagdata = pool_malloc(tpool->mempool, size);
	if (!newheader->tagdata)
		return NULL;

	newheader->next = NULL;
	newheader->tagname = tag;
	memset(newheader->tagdata, 0, size);

	header = &tpool->header;
	while (*header)
		header = &(*header)->next;
	*header = newheader;

	return newheader->tagdata;
}

void *tagpool_lookup(tag_pool *tpool, const char *tag)
{
	struct tag_pool_header *header;

	assert(tpool);

	header = tpool->header;
	while(header && strcmp(header->tagname, tag))
		header = header->next;

	/* a tagpool_lookup with an invalid tag is BAD */
	assert(header);

	return header->tagdata;
}
