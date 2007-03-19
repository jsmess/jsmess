/****************************************************************************

	pile.h

	Utility code to implement "piles"; blocks of data that are built up
	and then detached

****************************************************************************/

#ifndef PILE_H
#define PILE_H

#include <stdlib.h>
#include <stdarg.h>

typedef struct
{
	void *ptr;
	size_t physical_size;
	size_t logical_size;
} mess_pile;

void pile_init(mess_pile *pile);
void pile_delete(mess_pile *pile);
void pile_clear(mess_pile *pile);
void *pile_detach(mess_pile *pile);
int pile_write(mess_pile *pile, const void *ptr, size_t size);
int pile_writebyte(mess_pile *pile, char byte, size_t size);

INLINE void *pile_getptr(mess_pile *pile)
{
	return pile->ptr;
}

INLINE size_t pile_size(mess_pile *pile)
{
	return pile->logical_size;
}

int pile_vprintf(mess_pile *pile, const char *fmt, va_list args);
int pile_printf(mess_pile *pile, const char *fmt, ...);
int pile_putc(mess_pile *pile, char c);
int pile_puts(mess_pile *pile, const char *s);

#endif /* PILE_H */


