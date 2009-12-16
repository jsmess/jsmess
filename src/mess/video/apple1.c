/***************************************************************************

  apple1.c

  Functions to emulate the video hardware of the Apple I.

  The Apple I video hardware was basically a dumb video terminal; in
  fact it was based on Steve Wozniak's own design for a simple video
  terminal.  It had 40 columns by 24 lines of uppercase-only text.
  Text could only be output at 60 characters per second, one character
  per video frame.  The cursor (a blinking @) could only be advanced
  using spaces or carriage returns.  Carriage returns were the only
  control characters recognized.  Previously written text could not be
  altered, only scrolled off the top of the screen.

  The video memory used seven 1k-bit dynamic shift registers.  Six of
  these held the 6-bit visible character codes, and one stored the
  cursor location as a simple bitmap--the bit for the cursor position
  was set to 0, and all the other bits were 1.

  These shift registers were continuously recirculated, completing one
  cycle per video frame.  As a new line of characters was about to be
  scanned by the video beam, that character line would be recirculated
  into the shift registers and would simultaneously be stored into a
  6x40-bit line buffer (also a shift register).  At this point, if the
  cursor location was in this line, a new character could be written
  into that location in the shift registers and the cursor could be
  advanced.  (Carriage returns were not written into the shift
  registers; they only advanced the cursor.)

  The characters in the line buffer were recirculated 7 times to
  display the 8 scan lines of the characters, before being replaced by
  a new line of characters from the main shift registers.

  Cursor blinking was performed by a Signetics 555 timer IC whose
  output was gated into the character code signals as they passed into
  the line buffer.

  Character images were provided by a Signetics 2513 character
  generator ROM, a chip also used in computer terminals such as the
  ADM-3A.  This ROM had 9 address lines and 5 data lines; it contained
  64 character images, each 5 pixels wide by 8 pixels high, with one
  line of pixels being blank for vertical separation.  The video
  circuitry added the 2 pixels of horizontal separation for each
  character.

  A special CLEAR SCREEN switch on the keyboard, directly connected to
  the video hardware, could be used to clear the video memory and
  return the cursor to the home position.  This was completely
  independant of the processor.

  A schematic of the Apple I video hardware can be found in the
  Apple-1 Operation Manual; look for the schematic titled "Terminal
  Section".  Most of the functionality modeled here was determined by
  reading this schematic.  Many of the chips used were standard 74xx
  TTL chips, but the shift registers used for the video memory and
  line buffer were Signetics 25xx PMOS ICs.  These were already
  becoming obsolete when the Apple I was built, and detailed
  information on them is very hard to find today.

***************************************************************************/

#include "driver.h"
#include "includes/apple1.h"


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

static TILE_GET_INFO(terminal_gettileinfo)
{
	int ch, gfxfont, code, color;

	ch = current_terminal->mem[tile_index];
	code = ch & ((1 << current_terminal->char_bits) - 1);
	color = ch >> current_terminal->char_bits;
	gfxfont = current_terminal->gfx;

	if ((tile_index == current_terminal->cur_offset) && !current_terminal->cur_hidden && current_terminal->getcursorcode)
		code = current_terminal->getcursorcode(code);

	SET_TILE_INFO(
		gfxfont,	/* gfx */
		code,		/* character */
		color,		/* color */
		0);			/* flags */
}

static void terminal_draw(bitmap_t *dest, const rectangle *cliprect, struct terminal *terminal)
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

static void terminal_putchar(struct terminal *terminal, int x, int y, int ch)
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

static int terminal_getchar(struct terminal *terminal, int x, int y)
{
	int offs;

	verify_coords(terminal, x, y);
	offs = y * terminal->num_cols + x;
	return terminal->mem[offs];
}

static void terminal_putblank(struct terminal *terminal, int x, int y)
{
	terminal_putchar(terminal, x, y, terminal->blank_char);
}

static void terminal_dirtycursor(struct terminal *terminal)
{
	if (terminal->cur_offset >= 0)
		tilemap_mark_tile_dirty(terminal->tm, terminal->cur_offset);
}

static void terminal_setcursor(struct terminal *terminal, int x, int y)
{
	terminal_dirtycursor(terminal);
	terminal->cur_offset = y * terminal->num_cols + x;
	terminal_dirtycursor(terminal);
}

static void terminal_hidecursor(struct terminal *terminal)
{
	terminal->cur_hidden = 1;
	terminal_dirtycursor(terminal);
}

static void terminal_showcursor(struct terminal *terminal)
{
	terminal->cur_hidden = 0;
	terminal_dirtycursor(terminal);
}

static void terminal_getcursor(struct terminal *terminal, int *x, int *y)
{
	*x = terminal->cur_offset % terminal->num_cols;
	*y = terminal->cur_offset / terminal->num_cols;
}

static void terminal_fill(struct terminal *terminal, int val)
{
	int i;
	for (i = 0; i < terminal->num_cols * terminal->num_rows; i++)
		terminal->mem[i] = val;
	tilemap_mark_all_tiles_dirty(terminal->tm);
}

static void terminal_clear(struct terminal *terminal)
{
	terminal_fill(terminal, terminal->blank_char);
}

static struct terminal *terminal_create(
	running_machine *machine,
	int gfx, int blank_char, int char_bits,
	int (*getcursorcode)(int original_code),
	int num_cols, int num_rows)
{
	struct terminal *term;
	int char_width, char_height;

	char_width = machine->gfx[gfx]->width;
	char_height = machine->gfx[gfx]->height;

	term = (struct terminal *) auto_alloc_array(machine, char, sizeof(struct terminal) - sizeof(term->mem)
		+ (num_cols * num_rows * sizeof(termchar_t)));

	term->tm = tilemap_create(machine, terminal_gettileinfo, tilemap_scan_rows,
		char_width, char_height, num_cols, num_rows);

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


/**************************************************************************/


static struct terminal *apple1_terminal;

int apple1_vh_clrscrn_pressed = 0;		/* flag for CLEAR SCREEN switch */

/* The cursor blinking is generated by a free-running timer with a
   0.52-second period.  It is on for 2/3 of this period and off for
   1/3. */
#define CURSOR_OFF_LENGTH			(0.52/3)

/**************************************************************************/

static int apple1_getcursorcode(int original_code)
{
	/* Cursor uses symbol 0 (an @ sign) in the character generator ROM. */
	return 0;
}

/**************************************************************************/

VIDEO_START( apple1 )
{
	apple1_terminal = terminal_create(
		machine,
		0,			/* graphics font 0 (the only one we have) */
		32,			/* Blank character is symbol 32 in the ROM */
		8,			/* use 8 bits for the character code */
		apple1_getcursorcode,
		40, 24);	/* 40 columns, 24 rows */

	terminal_setcursor(apple1_terminal, 0, 0);
}

/* This function handles all writes to the video display. */
void apple1_vh_dsp_w (int data)
{
	int	x, y;
	int cursor_x, cursor_y;

	/* While CLEAR SCREEN is being held down, the hardware is forced
       to clear the video memory, so video writes have no effect. */
	if (apple1_vh_clrscrn_pressed)
		return;

	/* The video display port only accepts the 7 lowest bits of the char. */
	data &= 0x7f;

	terminal_getcursor(apple1_terminal, &cursor_x, &cursor_y);

	if (data == '\r') {
		/* Carriage-return moves the cursor to the start of the next
           line. */
		cursor_x = 0;
		cursor_y++;
	}
	else if (data < ' ') {
		/* Except for carriage-return, the video hardware completely
           ignores all control characters. */
		return;
	}
	else {
		/* For visible characters, only 6 bits of the ASCII code are
           used, because the 2513 character generator ROM only
           contains 64 symbols.  The low 5 bits of the ASCII code are
           used directly.  Bit 6 is ignored, since it is the same for
           all the available characters in the ROM.  Bit 7 is inverted
           before being used as the high bit of the 6-bit ROM symbol
           index, because the block of 32 ASCII symbols containing the
           uppercase letters comes first in the ROM. */

		int romindx = (data & 0x1f) | (((data ^ 0x40) & 0x40) >> 1);

		terminal_putchar(apple1_terminal, cursor_x, cursor_y, romindx);
		if (cursor_x < 39)
		{
			cursor_x++;
		}
		else
		{
			cursor_x = 0;
			cursor_y++;
		}
	}

	/* If the cursor went past the bottom line, scroll the text up one line. */
	if (cursor_y == 24)
	{
		for (y = 1; y < 24; y++)
			for (x = 0; x < 40; x++)
				terminal_putchar(apple1_terminal, x, y-1,
								 terminal_getchar(apple1_terminal, x, y));

		for (x = 0; x < 40; x++)
			terminal_putblank(apple1_terminal, x, 23);

		cursor_y--;
	}

	terminal_setcursor(apple1_terminal, cursor_x, cursor_y);
}

/* This function handles clearing the video display on cold-boot or in
   response to a press of the CLEAR SCREEN switch. */
void apple1_vh_dsp_clr (void)
{
	terminal_setcursor(apple1_terminal, 0, 0);
	terminal_clear(apple1_terminal);
}

/* Calculate how long it will take for the display to assert the RDA
   signal in response to a video display write.  This signal indicates
   the display has completed the write and is ready to accept another
   write. */
attotime apple1_vh_dsp_time_to_ready (running_machine *machine)
{
	int cursor_x, cursor_y;
	int cursor_scanline;
	double scanline_period = attotime_to_double(video_screen_get_scan_period(machine->primary_screen));
	double cursor_hfrac;

	/* The video hardware refreshes the screen by reading the
       character codes from its circulating shift-register memory.
       Because of the way this memory works, a new character can only
       be written into the cursor location at the moment this location
       is about to be read.  This happens during the first scanline of
       the cursor's character line, when the beam reaches the cursor's
       horizontal position. */

	terminal_getcursor(apple1_terminal, &cursor_x, &cursor_y);
	cursor_scanline = cursor_y * apple1_charlayout.height;

	/* Each scanline is composed of 455 pixel times.  The first 175 of
       these are the horizontal blanking period; the remaining 280 are
       for the visible part of the scanline. */
	cursor_hfrac = (175 + cursor_x * apple1_charlayout.width) / 455;

	if (video_screen_get_vpos(machine->primary_screen) == cursor_scanline) {
		/* video_screen_get_hpos() doesn't account for the horizontal
           blanking interval; it acts as if the scanline period is
           entirely composed of visible pixel times.  However, we can
           still use it to find what fraction of the current scanline
           period has elapsed. */
		double current_hfrac = video_screen_get_hpos(machine->primary_screen) /
							   video_screen_get_width(video_screen_first(machine->config));
		if (current_hfrac < cursor_hfrac)
			return double_to_attotime(scanline_period * (cursor_hfrac - current_hfrac));
	}

	return double_to_attotime(
		attotime_to_double(video_screen_get_time_until_pos(machine->primary_screen, cursor_scanline, 0)) +
		scanline_period * cursor_hfrac);
}

/* Blink the cursor on or off, as appropriate. */
static void apple1_vh_cursor_blink (running_machine *machine)
{
	static int blink_on = 1;		/* cursor is visible initially */
	int new_blink_on;

	/* The cursor is on for 2/3 of its blink period and off for 1/3.
       This is most easily handled by dividing the total elapsed time
       by the length of the off-portion of the cycle, giving us the
       number of one-third-cycles elapsed, then checking the result
       modulo 3. */

	if (((int) (attotime_to_double(timer_get_time(machine)) / CURSOR_OFF_LENGTH)) % 3 < 2)
		new_blink_on = 1;
	else
		new_blink_on = 0;

	if (new_blink_on != blink_on) {		/* have we changed state? */
		if (new_blink_on)
			terminal_showcursor(apple1_terminal);
		else
			terminal_hidecursor(apple1_terminal);
		blink_on = new_blink_on;
	}
}

VIDEO_UPDATE( apple1 )
{
	apple1_vh_cursor_blink(screen->machine);
	terminal_draw(bitmap, NULL, apple1_terminal);
	return 0;
}
