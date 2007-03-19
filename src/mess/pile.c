/****************************************************************************

	pile.h

	Utility code to implement "piles"; blocks of data that are built up
	and then detached

****************************************************************************/

#include <string.h>
#include <stdio.h>

#include "pile.h"

void pile_init(mess_pile *pile)
{
	pile->ptr = NULL;
	pile->physical_size = 0;
	pile->logical_size = 0;
}



void pile_delete(mess_pile *pile)
{
	if (pile->ptr)
	{
		free(pile->ptr);
		pile_init(pile);
	}
}



void pile_clear(mess_pile *pile)
{
	pile_delete(pile);
}



void *pile_detach(mess_pile *pile)
{
	void *ptr;

	ptr = realloc(pile->ptr, pile->logical_size);
	if (!ptr)
		ptr = pile->ptr;

	pile_init(pile);
	return ptr;
}



int pile_write(mess_pile *pile, const void *ptr, size_t size)
{
	size_t new_logical_size;
	size_t new_physical_size;
	size_t blocksize = 4096;
	char *new_ptr;

	new_logical_size = pile->logical_size + size;
	if (new_logical_size > pile->physical_size)
	{
		new_physical_size = new_logical_size + blocksize;
		new_physical_size -= new_physical_size % blocksize;

		new_ptr = (char *) realloc(pile->ptr, new_physical_size);
		if (!new_ptr)
			return -1;

		pile->ptr = new_ptr;
		pile->physical_size = new_physical_size;
	}
	memcpy(((char *) pile->ptr) + pile->logical_size, ptr, size);
	pile->logical_size = new_logical_size;
	return 0;
}



int pile_writebyte(mess_pile *pile, char byte, size_t size)
{
	int rc = 0;

	while(size--)
	{
		rc = pile_write(pile, &byte, 1);
		if (rc)
			return rc;
	}
	return rc;
}



int pile_vprintf(mess_pile *pile, const char *fmt, va_list args)
{
	int count;
	char buf[1024];
	
	count = vsnprintf(buf, sizeof(buf) / sizeof(buf[0]), fmt, args);
	if (pile_write(pile, buf, count * sizeof(*buf)))
		return 0;
	return count;
}



int pile_printf(mess_pile *pile, const char *fmt, ...)
{
	int count;
	va_list va;

	va_start(va, fmt);
	count = pile_vprintf(pile, fmt, va);
	va_end(va);
	return count;
}



int pile_putc(mess_pile *pile, char c)
{
	return pile_write(pile, &c, sizeof(c));
}



int pile_puts(mess_pile *pile, const char *s)
{
	size_t size;
	size = strlen(s);
	return pile_write(pile, s, size);
}


