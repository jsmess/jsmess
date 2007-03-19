/*********************************************************************

	mscommon.h

	MESS specific generic functions

*********************************************************************/

#ifndef MSCOMMON_H
#define MSCOMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "driver.h"

/***************************************************************************

	Terminal code

***************************************************************************/

struct terminal;

struct terminal *terminal_create(
	int gfx, int blank_char, int char_bits,
	int (*getcursorcode)(int original_code),
	int num_cols, int num_rows);

void terminal_draw(mame_bitmap *dest, const rectangle *cliprect,
	struct terminal *terminal);
void terminal_putchar(struct terminal *terminal, int x, int y, int ch);
int terminal_getchar(struct terminal *terminal, int x, int y);
void terminal_putblank(struct terminal *terminal, int x, int y);
void terminal_setcursor(struct terminal *terminal, int x, int y);
void terminal_hidecursor(struct terminal *terminal);
void terminal_showcursor(struct terminal *terminal);
void terminal_getcursor(struct terminal *terminal, int *x, int *y);
void terminal_fill(struct terminal *terminal, int val);
void terminal_clear(struct terminal *terminal);

/***************************************************************************

	LED code

***************************************************************************/

/* draw_led() will both draw led (where the pixels are identified by '1' or
 * x-segment displays (where the pixels are masked with lowercase letters)
 *
 * the value of 'valueorcolor' is a mask when lowercase letters are in the led
 * string or is a color when '1' characters are in the led string
 */
void draw_led(mame_bitmap *bitmap, const char *led, int valueorcolor, int x, int y);

/* a radius two led:
 *
 *	" 111\r"
 *	"11111\r"
 *	"11111\r"
 *	"11111\r"
 *	" 111";
 */
extern const char *radius_2_led;

/**************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* MSCOMMON_H */
