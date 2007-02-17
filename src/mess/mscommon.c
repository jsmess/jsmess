/*********************************************************************

	mscommon.c

	MESS specific generic functions

*********************************************************************/

#include <assert.h>
#include "mscommon.h"

/***************************************************************************

	Terminal code

***************************************************************************/

typedef short termchar_t;

struct terminal
{
	tilemap *tm;
	int gfx;
	int blank_char;
	int char_bits;
	int num_cols;
	int num_rows;
	int (*getcursorcode)(int original_code);
	int cur_offset;
	int cur_hidden;
	termchar_t mem[1];
};

static struct terminal *current_terminal;

static void terminal_gettileinfo(int memory_offset)
{
	int ch, gfxfont, code, color;
	
	ch = current_terminal->mem[memory_offset];
	code = ch & ((1 << current_terminal->char_bits) - 1);
	color = ch >> current_terminal->char_bits;
	gfxfont = current_terminal->gfx;

	if ((memory_offset == current_terminal->cur_offset) && !current_terminal->cur_hidden && current_terminal->getcursorcode)
		code = current_terminal->getcursorcode(code);

	SET_TILE_INFO(
		gfxfont,	/* gfx */
		code,		/* character */
		color,		/* color */
		0)			/* flags */
}

struct terminal *terminal_create(
	int gfx, int blank_char, int char_bits,
	int (*getcursorcode)(int original_code),
	int num_cols, int num_rows)
{
	struct terminal *term;
	int char_width, char_height;

	char_width = Machine->gfx[gfx]->width;
	char_height = Machine->gfx[gfx]->height;

	term = (struct terminal *) auto_malloc(sizeof(struct terminal) - sizeof(term->mem)
		+ (num_cols * num_rows * sizeof(termchar_t)));

	term->tm = tilemap_create(terminal_gettileinfo, tilemap_scan_rows,
		TILEMAP_OPAQUE, char_width, char_height, num_cols, num_rows);

	term->gfx = gfx;
	term->blank_char = blank_char;
	term->char_bits = char_bits;
	term->num_cols = num_cols;
	term->num_rows = num_rows;
	term->getcursorcode = getcursorcode;
	term->cur_offset = -1;
	term->cur_hidden = 0;
	terminal_clear(term);
	return term;
}

void terminal_draw(mame_bitmap *dest, const rectangle *cliprect, struct terminal *terminal)
{
	current_terminal = terminal;
	tilemap_draw(dest, cliprect, terminal->tm, 0, 0);
	current_terminal = NULL;
}

static void verify_coords(struct terminal *terminal, int x, int y)
{
	assert(x >= 0);
	assert(y >= 0);
	assert(x < terminal->num_cols);
	assert(y < terminal->num_rows);
}

void terminal_putchar(struct terminal *terminal, int x, int y, int ch)
{
	int offs;

	verify_coords(terminal, x, y);

	offs = y * terminal->num_cols + x;
	if (terminal->mem[offs] != ch)
	{
		terminal->mem[offs] = ch;
		tilemap_mark_tile_dirty(terminal->tm, offs);
	}
}

int terminal_getchar(struct terminal *terminal, int x, int y)
{
	int offs;

	verify_coords(terminal, x, y);
	offs = y * terminal->num_cols + x;
	return terminal->mem[offs];
}

void terminal_putblank(struct terminal *terminal, int x, int y)
{
	terminal_putchar(terminal, x, y, terminal->blank_char);
}

static void terminal_dirtycursor(struct terminal *terminal)
{
	if (terminal->cur_offset >= 0)
		tilemap_mark_tile_dirty(terminal->tm, terminal->cur_offset);
}

void terminal_setcursor(struct terminal *terminal, int x, int y)
{
	terminal_dirtycursor(terminal);
	terminal->cur_offset = y * terminal->num_cols + x;
	terminal_dirtycursor(terminal);
}

void terminal_hidecursor(struct terminal *terminal)
{
	terminal->cur_hidden = 1;
	terminal_dirtycursor(terminal);
}

void terminal_showcursor(struct terminal *terminal)
{
	terminal->cur_hidden = 0;
	terminal_dirtycursor(terminal);
}

void terminal_getcursor(struct terminal *terminal, int *x, int *y)
{
	*x = terminal->cur_offset % terminal->num_cols;
	*y = terminal->cur_offset / terminal->num_cols;
}

void terminal_fill(struct terminal *terminal, int val)
{
	int i;
	for (i = 0; i < terminal->num_cols * terminal->num_rows; i++)
		terminal->mem[i] = val;
	tilemap_mark_all_tiles_dirty(terminal->tm);
}

void terminal_clear(struct terminal *terminal)
{
	terminal_fill(terminal, terminal->blank_char);
}

/***************************************************************************

	LED code

***************************************************************************/

void draw_led(mame_bitmap *bitmap, const char *led, int valueorcolor, int x, int y)
{
	char c;
	int i, xi, yi, mask, color;

	for (i=0, xi=0, yi=0; led[i]; i++)
	{
		c = led[i];
		if (c == '1')
		{
			plot_pixel(bitmap, x+xi, y+yi, valueorcolor);
		}
		else if (c >= 'a')
		{
			mask = 1 << (c - 'a');
			color = Machine->pens[(valueorcolor & mask) ? 1 : 0];
			plot_pixel(bitmap, x+xi, y+yi, color);
		}
		if (c != '\r')
		{
			xi++;
		}
		else
		{
			yi++;
			xi=0;
		}
	}
}

const char *radius_2_led =
	" 111\r"
	"11111\r"
	"11111\r"
	"11111\r"
	" 111";
