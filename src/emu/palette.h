/******************************************************************************

    palette.c

    Palette handling functions.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

    There are several levels of abstraction in the way MAME handles the palette,
    and several display modes which can be used by the drivers.

    Palette
    -------
    Note: in the following text, "color" refers to a color in the emulated
    game's virtual palette. For example, a game might be able to display 1024
    colors at the same time. If the game uses RAM to change the available
    colors, the term "palette" refers to the colors available at any given time,
    not to the whole range of colors which can be produced by the hardware. The
    latter is referred to as "color space".
    The term "pen" refers to one of the maximum MAX_PENS colors that can be
    used to generate the display.

    So, to summarize, the three layers of palette abstraction are:

    P1) The game virtual palette (the "colors")
    P2) MAME's MAX_PENS colors palette (the "pens")
    P3) The OS specific hardware color registers (the "OS specific pens")

    The array Machine->pens[] is a lookup table which maps game colors to OS
    specific pens (P1 to P3). When you are working on bitmaps at the pixel level,
    *always* use Machine->pens to map the color numbers. *Never* use constants.
    For example if you want to make pixel (x,y) of color 3, do:
    *BITMAP_ADDR(bitmap, <type>, y, x) = Machine->pens[3];


    Lookup table
    ------------
    Palettes are only half of the story. To map the gfx data to colors, the
    graphics routines use a lookup table. For example if we have 4bpp tiles,
    which can have 256 different color codes, the lookup table for them will have
    256 * 2^4 = 4096 elements. For games using a palette RAM, the lookup table is
    usually a 1:1 map. For games using PROMs, the lookup table is often larger
    than the palette itself so for example the game can display 256 colors out
    of a palette of 16.

    The palette and the lookup table are initialized to default values by the
    main core, but can be initialized by the driver using the function
    MachineDriver->vh_init_palette(). For games using palette RAM, that
    function is usually not needed, and the lookup table can be set up by
    properly initializing the color_codes_start and total_color_codes fields in
    the GfxDecodeInfo array.
    When vh_init_palette() initializes the lookup table, it maps gfx codes
    to game colors (P1 above). The lookup table will be converted by the core to
    map to OS specific pens (P3 above), and stored in Machine->remapped_colortable.


    Display modes
    -------------
    The available display modes can be summarized in three categories:
    1) Static palette. Use this for games which use PROMs for color generation.
        The palette is initialized by palette_init(), and never changed
        again.
    2) Dynamic palette. Use this for games which use RAM for color generation.
        The palette can be dynamically modified by the driver using the function
        palette_set_color().
    3) Direct mapped 16-bit or 32-bit color. This should only be used in special
        cases, e.g. to support alpha blending.
        MachineDriver->video_attributes must contain VIDEO_RGB_DIRECT.


    Shadows(Highlights) Quick Reference
    -----------------------------------

    1) declare MDRV_VIDEO_ATTRIBUTES( ... VIDEO_HAS_SHADOWS | VIDEO_HAS_HIGHLIGHTS ... )

    2) set gfx_drawmode_table[0-n] to DRAWMODE_NONE, DRAWMODE_SOURCE or DRAWMODE_SHADOW

    3) (optional) set shadow darkness or highlight brightness by
        palette_set_shadow_factor(0.0-1.0) or
        palette_set_highlight_factor(1.0-n.n)

    4) before calling drawgfx use
        palette_set_shadow_mode(0) to arm shadows or
        palette_set_shadow_mode(1) to arm highlights

    5) call drawgfx with the TRANSPARENCY_PEN_TABLE flag
        drawgfx( ..., cliprect, TRANSPARENCY_PEN_TABLE, transparent_color )

******************************************************************************/

#pragma once

#ifndef __PALETTE_H__
#define __PALETTE_H__

#include "memory.h"


/***************************************************************************
    CONSTANTS
***************************************************************************/

#define PALETTE_DEFAULT_SHADOW_FACTOR (0.6)
#define PALETTE_DEFAULT_HIGHLIGHT_FACTOR (1/PALETTE_DEFAULT_SHADOW_FACTOR)



/***************************************************************************
    MACROS
***************************************************************************/

#define MAKE_RGB(r,g,b) 	((((r) & 0xff) << 16) | (((g) & 0xff) << 8) | ((b) & 0xff))
#define MAKE_ARGB(a,r,g,b)	(MAKE_RGB(r,g,b) | (((a) & 0xff) << 24))
#define RGB_ALPHA(rgb)		(((rgb) >> 24) & 0xff)
#define RGB_RED(rgb)		(((rgb) >> 16) & 0xff)
#define RGB_GREEN(rgb)		(((rgb) >> 8) & 0xff)
#define RGB_BLUE(rgb)		((rgb) & 0xff)

#define RGB_BLACK			(MAKE_RGB(0,0,0))
#define RGB_WHITE			(MAKE_RGB(255,255,255))



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

void palette_init(running_machine *machine);
void palette_config(running_machine *machine);
void palette_add_notifier(running_machine *machine, void (*callback)(void *, int, rgb_t), void *param);
int palette_get_total_colors_with_ui(running_machine *machine);

rgb_t *palette_get_raw_colors(running_machine *machine);
rgb_t *palette_get_adjusted_colors(running_machine *machine);

void palette_set_color(running_machine *machine, pen_t pen, UINT8 r, UINT8 g, UINT8 b);
rgb_t palette_get_color(running_machine *machine, pen_t pen);
void palette_set_colors(running_machine *machine, pen_t color_base, const UINT8 *colors, int color_count);

void palette_set_brightness(running_machine *machine, pen_t pen, double bright);
void palette_set_shadow_factor(running_machine *machine, double factor);
void palette_set_highlight_factor(running_machine *machine, double factor);

void palette_set_shadow_mode(running_machine *machine, int mode);
void palette_set_shadow_dRGB32(running_machine *machine, int mode, int dr, int dg, int db, int noclip);

pen_t get_black_pen(running_machine *machine);
pen_t get_white_pen(running_machine *machine);



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    pal1bit - convert a 1-bit value to 8 bits
-------------------------------------------------*/

INLINE UINT8 pal1bit(UINT8 bits)
{
	return (bits & 1) ? 0xff : 0x00;
}


/*-------------------------------------------------
    pal2bit - convert a 2-bit value to 8 bits
-------------------------------------------------*/

INLINE UINT8 pal2bit(UINT8 bits)
{
	bits &= 3;
	return (bits << 6) | (bits << 4) | (bits << 2) | bits;
}


/*-------------------------------------------------
    pal3bit - convert a 3-bit value to 8 bits
-------------------------------------------------*/

INLINE UINT8 pal3bit(UINT8 bits)
{
	bits &= 7;
	return (bits << 5) | (bits << 2) | (bits >> 1);
}


/*-------------------------------------------------
    pal4bit - convert a 4-bit value to 8 bits
-------------------------------------------------*/

INLINE UINT8 pal4bit(UINT8 bits)
{
	bits &= 0xf;
	return (bits << 4) | bits;
}


/*-------------------------------------------------
    pal5bit - convert a 5-bit value to 8 bits
-------------------------------------------------*/

INLINE UINT8 pal5bit(UINT8 bits)
{
	bits &= 0x1f;
	return (bits << 3) | (bits >> 2);
}


/*-------------------------------------------------
    pal6bit - convert a 6-bit value to 8 bits
-------------------------------------------------*/

INLINE UINT8 pal6bit(UINT8 bits)
{
	bits &= 0x3f;
	return (bits << 2) | (bits >> 4);
}


/*-------------------------------------------------
    pal7bit - convert a 7-bit value to 8 bits
-------------------------------------------------*/

INLINE UINT8 pal7bit(UINT8 bits)
{
	bits &= 0x7f;
	return (bits << 1) | (bits >> 6);
}


#endif	/* __PALETTE_H__ */
