/******************************************************************************

    palette.c

    Palette handling functions.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

******************************************************************************/

#include "driver.h"
#include <math.h>

#define VERBOSE 0


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define PEN_BRIGHTNESS_BITS		8
#define MAX_PEN_BRIGHTNESS		(4 << PEN_BRIGHTNESS_BITS)
#define MAX_SHADOW_PRESETS 		4



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _callback_item callback_item;
struct _callback_item
{
	callback_item *	next;
	void *			param;
	void 			(*notify)(void *param, int index, rgb_t newval);
};


/* typedef struct _palette_private palette_private; */
struct _palette_private
{
	rgb_t *raw_color;
	rgb_t *adjusted_color;

	UINT16 *pen_brightness;

	UINT16 shadow_factor, highlight_factor;

	pen_t total_colors;
	pen_t total_colors_with_ui;

	pen_t black_pen, white_pen;

	callback_item *notify_callback_list;

	UINT32 *shadow_table_base[MAX_SHADOW_PRESETS];
	int shadow_dr[MAX_SHADOW_PRESETS];
	int shadow_dg[MAX_SHADOW_PRESETS];
	int shadow_db[MAX_SHADOW_PRESETS];
	int shadow_noclip[MAX_SHADOW_PRESETS];
};


/*

    Machine->drv->total_colors = reported colors

    palette->total_colors =
        Machine->drv->total_colors
        + Machine->drv->total_colors (if shadows)
        + Machine->drv->total_colors (if highlights)

    palette->total_colors_with_ui =
        palette->total_colors

    raw_color[palette->total_colors + 2]
    adjusted_color[palette->total_colors + 2]

    machine->pens[palette->total_colors]
    palette->pen_brightness[palette->total_colors]

    machine->game_colortable[Machine->drv->color_table_len]
    machine->remapped_colortable[Machine->drv->color_table_len] (or -> machine->pens)

    palette->shadow_table_base[MAX_SHADOW_PRESETS]
        - contains array mapping pens to shadow pens
        - by default, tables 0 and 2 point to shadows, tables 1 and 3 point to hilights
        - shadows go from Machine->drv->total_colors to 2*Machine->drv->total_colors
        - hilights go from 2*Machine->drv->total_colors to 3*Machine->drv->total_colors



    palette_alloc strategy:

        palette->total_colors = machine->drv->total_colors;
        if (shadows && !rgb_direct) palette->total_colors += machine->drv->total_colors;
        if (hilights && !rgb_direct) palette->total_colors += machine->drv->total_colors;
        palette->total_colors_with_ui = palette->total_colors;

        max_total_colors = palette->total_colors + 2;

        Allocate palette->raw_color[max_total_colors]
            - initialize to RGB defaults

        Allocate palette->adjusted_color[max_total_colors]
            - initialize equal to palette->raw_color

        if (!rgb_direct) Notify_pen_changed for max_total_colors

        if (palette->total_colors > 0)

            Allocate machine->pens[palette->total_colors]
                - initialize 1:1

            Allocate palette_pen_brightness[machine->drv->total_colors]
                - initialize to 1.0

        if (machine->drv->color_table_len > 0)

            Allocate machine->game_colortable[machine->drv->color_table_len]
                - initialize to i % palette->total_colors

            Allocate machine->remapped_colortable[machine->drv->color_table_len]
                - not initialized

        if (shadows)

            Allocate shadow_table[0]/[2]

            if (!rgb_direct)
                table[i] = (i < machine->drv->total_colors) ? (i + machine->drv->total_colors) : i

            else
                table[i] = shadow(RGB555(i))

        if (hilights)

            Allocate shadow_table[1]/[3]

            if (!rgb_direct)
                table[i] = (i < machine->drv->total_colors) ? (i + 2 * machine->drv->total_colors) : i

            else
                table[i] = shadow(RGB555(i))

        machine->shadow_table = shadow_table[0]



    palette_config strategy:

        recompute_adjusted_colors
            - for all pens, internal_modify_pen with current palette->raw_color and palette->pen_brightness
            - updates palette->raw_color and palette->adjusted_color
            - if (!rgb_direct) updates machine->pens

*/


/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void configure_rgb_shadows(running_machine *machine, int mode, float factor);
static void palette_exit(running_machine *machine);
static void palette_alloc(running_machine *machine, palette_private *palette);
static void palette_reset(void);
static void recompute_adjusted_colors(running_machine *machine);
static void internal_modify_pen(running_machine *machine, palette_private *palette, pen_t pen, rgb_t color, int pen_bright);



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    rgb_to_direct15 - convert an RGB triplet to
    a 15-bit OSD-specified RGB value
-------------------------------------------------*/

INLINE UINT16 rgb_to_direct15(rgb_t rgb)
{
	return ((RGB_RED(rgb) >> 3) << 10) | ((RGB_GREEN(rgb) >> 3) << 5) | ((RGB_BLUE(rgb) >> 3) << 0);
}



/*-------------------------------------------------
    adjust_palette_entry - adjust a palette
    entry for brightness and gamma
-------------------------------------------------*/

INLINE rgb_t adjust_palette_entry(rgb_t entry, int pen_bright)
{
	int r = (RGB_RED(entry) * pen_bright) >> PEN_BRIGHTNESS_BITS;
	int g = (RGB_GREEN(entry) * pen_bright) >> PEN_BRIGHTNESS_BITS;
	int b = (RGB_BLUE(entry) * pen_bright) >> PEN_BRIGHTNESS_BITS;
	return MAKE_RGB(r,g,b);
}



/*-------------------------------------------------
    notify_pen_changed - call all registered
    notifiers that a pen has changed
-------------------------------------------------*/

INLINE void notify_pen_changed(palette_private *palette, int pen, rgb_t newval)
{
	callback_item *cb;
	for (cb = palette->notify_callback_list; cb; cb = cb->next)
		(*cb->notify)(cb->param, pen, newval);
}



/*-------------------------------------------------
    palette_init - palette initialization that
    takes place before the display is created
-------------------------------------------------*/

void palette_init(running_machine *machine)
{
	palette_private *palette = auto_malloc(sizeof(*palette));
	int format = machine->screen[0].format;

	/* request cleanup */
	machine->palette_data = palette;
	add_exit_callback(machine, palette_exit);

	/* reset all our data */
	memset(palette, 0, sizeof(*palette));
	palette->shadow_factor = (int)(PALETTE_DEFAULT_SHADOW_FACTOR * (double)(1 << PEN_BRIGHTNESS_BITS));
	palette->highlight_factor = (int)(PALETTE_DEFAULT_HIGHLIGHT_FACTOR * (double)(1 << PEN_BRIGHTNESS_BITS));

	/* determine the color mode */
	switch (format)
	{
		case BITMAP_FORMAT_INDEXED16:
			/* indexed modes are fine for everything */
			break;

		case BITMAP_FORMAT_RGB15:
		case BITMAP_FORMAT_RGB32:
			/* RGB modes can't use color tables */
			assert(machine->drv->color_table_len == 0);
			break;

		case BITMAP_FORMAT_INVALID:
			/* invalid format means no palette - or at least it should */
			assert(machine->drv->total_colors == 0);
			assert(machine->drv->color_table_len == 0);
			return;

		default:
			fatalerror("Unsupported screen bitmap format!");
			break;
	}

	/* compute the total colors, including shadows and highlights */
	palette->total_colors = machine->drv->total_colors;
	if ((machine->drv->video_attributes & VIDEO_HAS_SHADOWS) && format == BITMAP_FORMAT_INDEXED16)
		palette->total_colors += machine->drv->total_colors;
	if ((machine->drv->video_attributes & VIDEO_HAS_HIGHLIGHTS) && format == BITMAP_FORMAT_INDEXED16)
		palette->total_colors += machine->drv->total_colors;
	palette->total_colors_with_ui = palette->total_colors;

	/* make sure we still fit in 16 bits */
	assert_always(palette->total_colors <= 65536, "Error: palette has more than 65536 colors.");

	/* allocate all the data structures */
	palette_alloc(machine, palette);

	/* set up save/restore of the palette */
	state_save_register_global_pointer(palette->raw_color, palette->total_colors);
	state_save_register_global_pointer(palette->pen_brightness, machine->drv->total_colors);
	state_save_register_func_postload(palette_reset);
}


/*-------------------------------------------------
    palette_exit - free any allocated memory
-------------------------------------------------*/

static void palette_exit(running_machine *machine)
{
	palette_private *palette = machine->palette_data;

	/* free the list of notifiers */
	while (palette->notify_callback_list != NULL)
	{
		callback_item *temp = palette->notify_callback_list;
		palette->notify_callback_list = palette->notify_callback_list->next;
		free(temp);
	}
}


/*-------------------------------------------------
    palette_alloc - allocate memory for palette
    structures
-------------------------------------------------*/

static void palette_alloc(running_machine *machine, palette_private *palette)
{
	int format = machine->screen[0].format;
	int max_total_colors = palette->total_colors + 2;
	int i;

	/* allocate memory for the raw game palette */
	palette->raw_color = auto_malloc(max_total_colors * sizeof(palette->raw_color[0]));
	for (i = 0; i < max_total_colors; i++)
		palette->raw_color[i] = MAKE_RGB((i & 1) * 0xff, ((i >> 1) & 1) * 0xff, ((i >> 2) & 1) * 0xff);

	/* allocate memory for the adjusted game palette */
	palette->adjusted_color = auto_malloc(max_total_colors * sizeof(palette->adjusted_color[0]));
	for (i = 0; i < max_total_colors; i++)
		palette->adjusted_color[i] = palette->raw_color[i];

	/* for 16bpp, notify that all pens have changed */
	if (format == BITMAP_FORMAT_INDEXED16)
		for (i = 0; i < max_total_colors; i++)
			notify_pen_changed(palette, i, palette->adjusted_color[i]);

	/* allocate memory for the pen table */
	if (palette->total_colors > 0)
	{
		machine->pens = auto_malloc(palette->total_colors * sizeof(machine->pens[0]));
		for (i = 0; i < palette->total_colors; i++)
			machine->pens[i] = i;

		/* allocate memory for the per-entry brightness table */
		palette->pen_brightness = auto_malloc(machine->drv->total_colors * sizeof(palette->pen_brightness[0]));
		for (i = 0; i < machine->drv->total_colors; i++)
			palette->pen_brightness[i] = 1 << PEN_BRIGHTNESS_BITS;
	}
	else
	{
		/* this driver does not use a palette */
		machine->pens = NULL;
		palette->pen_brightness = NULL;
	}

	/* allocate memory for the colortables, if needed */
	if (machine->drv->color_table_len)
	{
		/* first for the raw colortable */
		machine->game_colortable = auto_malloc(machine->drv->color_table_len * sizeof(machine->game_colortable[0]));
		for (i = 0; i < machine->drv->color_table_len; i++)
			machine->game_colortable[i] = i % palette->total_colors;

		/* then for the remapped colortable */
		machine->remapped_colortable = auto_malloc(machine->drv->color_table_len * sizeof(machine->remapped_colortable[0]));
	}

	/* otherwise, keep the game_colortable NULL and point the remapped_colortable to the pens */
	else
	{
		machine->game_colortable = NULL;
		machine->remapped_colortable = machine->pens;	/* straight 1:1 mapping from palette to colortable */
	}

	/* if we have shadows, allocate shadow tables */
	if (machine->drv->video_attributes & VIDEO_HAS_SHADOWS)
	{
		pen_t *table = auto_malloc(65536 * sizeof(*table));

		/* palettized mode gets a single 64k table in slots 0 and 2 */
		if (format == BITMAP_FORMAT_INDEXED16)
		{
			palette->shadow_table_base[0] = palette->shadow_table_base[2] = table;
			for (i = 0; i < 65536; i++)
				table[i] = (i < machine->drv->total_colors) ? (i + machine->drv->total_colors) : i;
			palette_set_shadow_factor(machine, PALETTE_DEFAULT_SHADOW_FACTOR);
		}

		/* RGB mode gets two 32k tables in slots 0 and 2 */
		else
		{
			palette->shadow_table_base[0] = table;
			palette->shadow_table_base[2] = table + 32768;
			configure_rgb_shadows(machine, 0, PALETTE_DEFAULT_SHADOW_FACTOR);
		}
	}

	/* if we have hilights, allocate shadow tables */
	if (machine->drv->video_attributes & VIDEO_HAS_HIGHLIGHTS)
	{
		pen_t *table = auto_malloc(65536 * sizeof(*table));

		/* palettized mode gets a single 64k table in slots 1 and 3 */
		if (format == BITMAP_FORMAT_INDEXED16)
		{
			palette->shadow_table_base[1] = palette->shadow_table_base[3] = table;
			for (i = 0; i < 65536; i++)
				table[i] = (i < machine->drv->total_colors) ? (i + 2 * machine->drv->total_colors) : i;
			palette_set_highlight_factor(machine, PALETTE_DEFAULT_HIGHLIGHT_FACTOR);
		}

		/* RGB mode gets two 32k tables in slots 1 and 3 */
		else
		{
			palette->shadow_table_base[1] = table;
			palette->shadow_table_base[3] = table + 32768;
			configure_rgb_shadows(machine, 1, PALETTE_DEFAULT_HIGHLIGHT_FACTOR);
		}
	}

	/* set the default table */
	machine->shadow_table = palette->shadow_table_base[0];
}


/*-------------------------------------------------
    palette_config - palette initialization that
    takes place after the display is created
-------------------------------------------------*/

void palette_config(running_machine *machine)
{
	palette_private *palette = machine->palette_data;
	int i;

	/* recompute the default palette and initalize the color correction table */
	recompute_adjusted_colors(machine);

	/* now let the driver modify the initial palette and colortable */
	if (machine->drv->init_palette)
		(*machine->drv->init_palette)(machine, machine->game_colortable, memory_region(REGION_PROMS));

	/* switch off the color mode */
	switch (machine->screen[0].format)
	{
		/* 16-bit paletteized case */
		case BITMAP_FORMAT_INDEXED16:
		{
			/* refresh the palette to support shadows in static palette games */
			for (i = 0; i < machine->drv->total_colors; i++)
				palette_set_color(machine, i, RGB_RED(palette->raw_color[i]), RGB_GREEN(palette->raw_color[i]), RGB_BLUE(palette->raw_color[i]));

			/* map the UI pens */
			if (palette->total_colors_with_ui <= 65534)
			{
				palette->total_colors_with_ui += 2;
				palette->black_pen = palette->total_colors + 0;
				palette->white_pen = palette->total_colors + 1;
			}
			else
			{
				palette->black_pen = 0;
				palette->white_pen = 65535;
			}
			notify_pen_changed(palette, palette->black_pen, palette->raw_color[palette->black_pen] = palette->adjusted_color[palette->black_pen] = MAKE_RGB(0x00,0x00,0x00));
			notify_pen_changed(palette, palette->white_pen, palette->raw_color[palette->white_pen] = palette->adjusted_color[palette->white_pen] = MAKE_RGB(0xff,0xff,0xff));
			break;
		}

		/* 15-bit direct case */
		case BITMAP_FORMAT_RGB15:
		{
			/* remap the game palette into direct RGB pens */
			for (i = 0; i < palette->total_colors; i++)
				machine->pens[i] = rgb_to_direct15(palette->raw_color[i]);

			/* map the UI pens */
			palette->black_pen = rgb_to_direct15(MAKE_RGB(0x00,0x00,0x00));
			palette->white_pen = rgb_to_direct15(MAKE_RGB(0xff,0xff,0xff));
			break;
		}

		/* 32-bit direct case */
		case BITMAP_FORMAT_RGB32:
		{
			/* remap the game palette into direct RGB pens */
			for (i = 0; i < palette->total_colors; i++)
				machine->pens[i] = palette->raw_color[i];

			/* map the UI pens */
			palette->black_pen = MAKE_RGB(0x00,0x00,0x00);
			palette->white_pen = MAKE_RGB(0xff,0xff,0xff);
			break;
		}

		/* screenless case */
		case BITMAP_FORMAT_INVALID:
			break;

		default:
			fatalerror("Unhandled bitmap format!");
			break;
	}

	/* now compute the remapped_colortable */
	for (i = 0; i < machine->drv->color_table_len; i++)
	{
		pen_t color = machine->game_colortable[i];

		/* check for invalid colors set by machine->drv->init_palette */
		assert(color < palette->total_colors);
		machine->remapped_colortable[i] = machine->pens[color];
	}
}


/*-------------------------------------------------
    palette_add_notifier - request a callback on
    changing of a palette entry
-------------------------------------------------*/

void palette_add_notifier(running_machine *machine, void (*callback)(void *, int, rgb_t), void *param)
{
	palette_private *palette = machine->palette_data;
	callback_item *cb;

	assert_always(mame_get_phase(machine) == MAME_PHASE_INIT, "Can only call palette_add_notifier at init time!");

	/* allocate memory */
	cb = malloc_or_die(sizeof(*cb));

	/* add us to the head of the list */
	cb->notify = callback;
	cb->param = param;
	cb->next = palette->notify_callback_list;
	palette->notify_callback_list = cb;
}


/*-------------------------------------------------
    palette_add_notifier - request a callback on
    changing of a palette entry
-------------------------------------------------*/

rgb_t *palette_get_raw_colors(running_machine *machine)
{
	palette_private *palette = machine->palette_data;
	return palette->raw_color;
}


/*-------------------------------------------------
    palette_add_notifier - request a callback on
    changing of a palette entry
-------------------------------------------------*/

rgb_t *palette_get_adjusted_colors(running_machine *machine)
{
	palette_private *palette = machine->palette_data;
	return palette->adjusted_color;
}


/*-------------------------------------------------

    palette_set_shadow_mode(mode)

        mode: 0 = use preset 0 (default shadow)
              1 = use preset 1 (default highlight)
              2 = use preset 2 *
              3 = use preset 3 *

    * Preset 2 & 3 work independently under 32bpp,
      supporting up to four different types of
      shadows at one time. They mirror preset 1 & 2
      in lower depth settings to maintain
      compatibility.


    palette_set_shadow_dRGB32(mode, dr, dg, db, noclip)

        mode:    0 to   3 (which preset to configure)

          dr: -255 to 255 ( red displacement )
          dg: -255 to 255 ( green displacement )
          db: -255 to 255 ( blue displacement )

        noclip: 0 = resultant RGB clipped at 0x00/0xff
                1 = resultant RGB wraparound 0x00/0xff


    * Color shadows only work under 32bpp.
      This function has no effect in lower color
      depths where

        palette_set_shadow_factor() or
        palette_set_highlight_factor()

      should be used instead.

    * 32-bit shadows are lossy. Even with zero RGB
      displacements the affected area will still look
      slightly darkened.

      Drivers should ensure all shadow pens in
      gfx_drawmode_table[] are set to DRAWMODE_NONE
      when RGB displacements are zero to avoid the
      darkening effect.

-------------------------------------------------*/

static void configure_rgb_shadows(running_machine *machine, int mode, float factor)
{
	int format = machine->screen[0].format;
	palette_private *palette = machine->palette_data;
	pen_t *table = palette->shadow_table_base[mode];
	int ifactor = (int)(factor * 256.0f);
	int i;

	/* only applies to RGB direct modes */
	assert(format != BITMAP_FORMAT_INDEXED16);
	assert(table != NULL);

	#if VERBOSE
		popmessage("shadow %d recalc %d %d %d %02x", mode, dr, dg, db, noclip);
	#endif

	/* regenerate the table */
	for (i = 0; i < 32768; i++)
	{
		int r = (pal5bit(i >> 10) * ifactor) >> 8;
		int g = (pal5bit(i >> 5) * ifactor) >> 8;
		int b = (pal5bit(i >> 0) * ifactor) >> 8;
		pen_t final;

		/* apply clipping */
		if (r < 0) r = 0; else if (r > 255) r = 255;
		if (g < 0) g = 0; else if (g > 255) g = 255;
		if (b < 0) b = 0; else if (b > 255) b = 255;
		final = MAKE_RGB(r, g, b);

		/* store either 16 or 32 bit */
		if (format == BITMAP_FORMAT_RGB32)
			table[i] = final;
		else
			table[i] = rgb_to_direct15(final);
	}
}


void palette_set_shadow_mode(running_machine *machine, int mode)
{
	palette_private *palette = machine->palette_data;
	assert(mode >= 0 && mode < MAX_SHADOW_PRESETS);
	machine->shadow_table = palette->shadow_table_base[mode];
}


void palette_set_shadow_dRGB32(running_machine *machine, int mode, int dr, int dg, int db, int noclip)
{
	int format = machine->screen[0].format;
	palette_private *palette = machine->palette_data;
	pen_t *table = palette->shadow_table_base[mode];
	int i;

	/* only applies to RGB direct modes */
	assert(format != BITMAP_FORMAT_INDEXED16);
	assert(table != NULL);

	/* clamp the deltas (why?) */
	if (dr < -0xff) dr = -0xff; else if (dr > 0xff) dr = 0xff;
	if (dg < -0xff) dg = -0xff; else if (dg > 0xff) dg = 0xff;
	if (db < -0xff) db = -0xff; else if (db > 0xff) db = 0xff;

	/* early exit if nothing changed */
	if (dr == palette->shadow_dr[mode] && dg == palette->shadow_dg[mode] && db == palette->shadow_db[mode] && noclip == palette->shadow_noclip[mode])
		return;
	palette->shadow_dr[mode] = dr;
	palette->shadow_dg[mode] = dg;
	palette->shadow_db[mode] = db;
	palette->shadow_noclip[mode] = noclip;

	#if VERBOSE
		popmessage("shadow %d recalc %d %d %d %02x", mode, dr, dg, db, noclip);
	#endif

	/* regenerate the table */
	for (i = 0; i < 32768; i++)
	{
		int r = pal5bit(i >> 10) + dr;
		int g = pal5bit(i >> 5) + dg;
		int b = pal5bit(i >> 0) + db;
		pen_t final;

		/* apply clipping */
		if (!noclip)
		{
			if (r < 0) r = 0; else if (r > 255) r = 255;
			if (g < 0) g = 0; else if (g > 255) g = 255;
			if (b < 0) b = 0; else if (b > 255) b = 255;
		}
		final = MAKE_RGB(r, g, b);

		/* store either 16 or 32 bit */
		if (format == BITMAP_FORMAT_RGB32)
			table[i] = final;
		else
			table[i] = rgb_to_direct15(final);
	}
}



/*-------------------------------------------------
    palette_get_total_colors_with_ui - returns
    the total number of palette entries including
    UI
-------------------------------------------------*/

int palette_get_total_colors_with_ui(running_machine *machine)
{
	int format = machine->screen[0].format;
	int result = machine->drv->total_colors;
	if (machine->drv->video_attributes & VIDEO_HAS_SHADOWS && format == BITMAP_FORMAT_INDEXED16)
		result += machine->drv->total_colors;
	if (machine->drv->video_attributes & VIDEO_HAS_HIGHLIGHTS && format == BITMAP_FORMAT_INDEXED16)
		result += machine->drv->total_colors;
	if (result <= 65534 && format != BITMAP_FORMAT_INVALID)
		result += 2;
	return result;
}



/*-------------------------------------------------
    internal_modify_single_pen - change a single
    pen and recompute its adjusted RGB value
-------------------------------------------------*/

static void internal_modify_single_pen(running_machine *machine, palette_private *palette, pen_t pen, rgb_t color, int pen_bright)
{
	rgb_t adjusted_color;

	/* skip if out of bounds or not ready */
	if (pen >= palette->total_colors)
		return;

	/* update the raw palette */
	palette->raw_color[pen] = color;

	/* now update the adjusted color if it's different */
	adjusted_color = adjust_palette_entry(color, pen_bright);
	if (adjusted_color != palette->adjusted_color[pen])
	{
		/* change the adjusted palette entry */
		palette->adjusted_color[pen] = adjusted_color;

		/* update the pen value */
		switch (machine->screen[0].format)
		{
			/* 16-bit palettized */
			case BITMAP_FORMAT_INDEXED16:
				notify_pen_changed(palette, pen, adjusted_color);
				break;

			/* 15/32-bit direct: update the machine->pens array */
			case BITMAP_FORMAT_RGB15:
				machine->pens[pen] = rgb_to_direct15(adjusted_color);
				break;

			case BITMAP_FORMAT_RGB32:
				machine->pens[pen] = adjusted_color;
				break;

			/* other formats unexpected */
			default:
				fatalerror("Unhandled bitmap format!");
				break;
		}
	}
}



/*-------------------------------------------------
    internal_modify_pen - change a pen along with
    its corresponding shadow/highlight
-------------------------------------------------*/

static void internal_modify_pen(running_machine *machine, palette_private *palette, pen_t pen, rgb_t color, int pen_bright) //* new highlight operation
{
	/* first modify the base pen */
	internal_modify_single_pen(machine, palette, pen, color, pen_bright);

	/* see if we need to handle shadow/highlight */
	if (pen < machine->drv->total_colors)
	{
		/* check for shadows */
		if (machine->drv->video_attributes & VIDEO_HAS_SHADOWS)
			internal_modify_single_pen(machine, palette, pen + machine->drv->total_colors, color, (pen_bright * palette->shadow_factor) >> PEN_BRIGHTNESS_BITS);

		/* check for highlights */
		if (machine->drv->video_attributes & VIDEO_HAS_HIGHLIGHTS)
			internal_modify_single_pen(machine, palette, pen + 2*machine->drv->total_colors, color, (pen_bright * palette->highlight_factor) >> PEN_BRIGHTNESS_BITS);
	}
}



/*-------------------------------------------------
    recompute_adjusted_colors - recompute the
    entire palette after some major event
-------------------------------------------------*/

static void recompute_adjusted_colors(running_machine *machine)
{
	palette_private *palette = machine->palette_data;
	int i;

	/* now update all the palette entries */
	for (i = 0; i < machine->drv->total_colors; i++)
		internal_modify_pen(machine, palette, i, palette->raw_color[i], palette->pen_brightness[i]);
}


/*-------------------------------------------------
    palette_reset - called after restore to
    actually update the palette
-------------------------------------------------*/

static void palette_reset(void)
{
	/* recompute everything */
	recompute_adjusted_colors(Machine);
}


/*-------------------------------------------------
    palette_set_color - set a single palette
    entry
-------------------------------------------------*/

void palette_set_color(running_machine *machine, pen_t pen, UINT8 r, UINT8 g, UINT8 b)
{
	palette_private *palette = machine->palette_data;
	assert(pen < palette->total_colors);
	internal_modify_pen(machine, palette, pen, MAKE_RGB(r, g, b), palette->pen_brightness[pen]);
}


/*-------------------------------------------------
    palette_set_colors - set multiple palette
    entries from an array of RGB triples
-------------------------------------------------*/

void palette_set_colors(running_machine *machine, pen_t color_base, const UINT8 *colors, int color_count)
{
	while (color_count--)
	{
		palette_set_color(machine, color_base++, colors[0], colors[1], colors[2]);
		colors += 3;
	}
}


/*-------------------------------------------------
    palette_get_color - return a single palette
    entry
-------------------------------------------------*/

rgb_t palette_get_color(running_machine *machine, pen_t pen)
{
	palette_private *palette = machine->palette_data;

	/* record the result from the game palette */
	if (pen < palette->total_colors)
		return palette->raw_color[pen];

	/* special case the black pen */
	else if (pen == palette->black_pen)
		return MAKE_RGB(0,0,0);

	/* special case the white pen (crosshairs) */
	else if (pen == palette->white_pen)
		return MAKE_RGB(255,255,255);

	else
		popmessage("palette_get_color() out of range");
	return 0;
}


/*-------------------------------------------------
    palette_set_brightness - set the per-pen
    brightness factor
-------------------------------------------------*/

void palette_set_brightness(running_machine *machine, pen_t pen, double bright)
{
	palette_private *palette = machine->palette_data;

	/* compute the integral brightness value */
	int brightval = (int)(bright * (double)(1 << PEN_BRIGHTNESS_BITS));
	if (brightval > MAX_PEN_BRIGHTNESS)
		brightval = MAX_PEN_BRIGHTNESS;

	/* if it changed, update the array and the adjusted palette */
	if (palette->pen_brightness[pen] != brightval)
	{
		palette->pen_brightness[pen] = brightval;
		internal_modify_pen(machine, palette, pen, palette->raw_color[pen], brightval);
	}
}


/*-------------------------------------------------
    palette_set_shadow_factor - set the global
    shadow brightness factor
-------------------------------------------------*/

void palette_set_shadow_factor(running_machine *machine, double factor)
{
	palette_private *palette = machine->palette_data;

	/* compute the integral shadow factor value */
	int factorval = (int)(factor * (double)(1 << PEN_BRIGHTNESS_BITS));
	if (factorval > MAX_PEN_BRIGHTNESS)
		factorval = MAX_PEN_BRIGHTNESS;

	/* if it changed, update the entire palette */
	if (palette->shadow_factor != factorval)
	{
		palette->shadow_factor = factorval;
		recompute_adjusted_colors(machine);
	}
}


/*-------------------------------------------------
    palette_set_highlight_factor - set the global
    highlight brightness factor
-------------------------------------------------*/

void palette_set_highlight_factor(running_machine *machine, double factor)
{
	palette_private *palette = machine->palette_data;

	/* compute the integral highlight factor value */
	int factorval = (int)(factor * (double)(1 << PEN_BRIGHTNESS_BITS));
	if (factorval > MAX_PEN_BRIGHTNESS)
		factorval = MAX_PEN_BRIGHTNESS;

	/* if it changed, update the entire palette */
	if (palette->highlight_factor != factorval)
	{
		palette->highlight_factor = factorval;
		recompute_adjusted_colors(machine);
	}
}


/*-------------------------------------------------
    get_black_pen - use this if you need to
    fillbitmap() the background with black
-------------------------------------------------*/

pen_t get_black_pen(running_machine *machine)
{
	palette_private *palette = machine->palette_data;
	return palette->black_pen;
}

pen_t get_white_pen(running_machine *machine)
{
	palette_private *palette = machine->palette_data;
	return palette->white_pen;
}
