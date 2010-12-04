/*****************************************************************************
 *
 *   pc1403.c
 *   portable sharp pc1403 video emulator interface
 *   (sharp pocket computers)
 *
 *   Copyright (c) 2001 Peter Trauner, all rights reserved.
 *
 *   - This source code is released as freeware for non-commercial purposes.
 *   - You are free to use and redistribute this code in modified or
 *     unmodified form, provided you list me in the credits.
 *   - If you modify this source code, you must add a notice to each modified
 *     source file that it has been changed.  If you're a nice person, you
 *     will clearly mark each change too.  :)
 *   - If you wish to use this for commercial purposes, please contact me at
 *     peter.trauner@jk.uni-linz.ac.at
 *   - The author of this copywritten work reserves the right to change the
 *     terms of its usage and license at any time, including retroactively
 *   - This entire notice must remain in the source code.
 *
 * History of changes:
 * 21.07.2001 Several changes listed below were made by Mario Konegger
 *            (konegger@itp.tu-graz.ac.at)
 *        Placed the grafical symbols onto the right place and added
 *        some symbols, so the display is correct rebuit.
 *        Added a strange behaviour of the display concerning the on/off
 *        state and the BUSY-symbol, which I found out with experiments
 *        with my own pc1403.
 *****************************************************************************/

#include "emu.h"

#include "includes/pocketc.h"
#include "includes/pc1403.h"


VIDEO_START( pc1403 )
{
	pc1403_state *state = machine->driver_data<pc1403_state>();
	if (strcmp(machine->gamedrv->name, "pc1403h") == 0)
	{
		state->DOWN = 69;
		state->RIGHT = 155;
	}
	else
	{
		state->DOWN = 67;
		state->RIGHT = 152;
	}
}


READ8_HANDLER(pc1403_lcd_read)
{
	pc1403_state *state = space->machine->driver_data<pc1403_state>();
	return state->reg[offset];
}

WRITE8_HANDLER(pc1403_lcd_write)
{
	pc1403_state *state = space->machine->driver_data<pc1403_state>();
	state->reg[offset]=data;
}

static const POCKETC_FIGURE	line={ /* simple line */
	"11111",
	"11111",
	"11111e"
};
static const POCKETC_FIGURE busy={
	"11  1 1  11 1 1",
	"1 1 1 1 1   1 1",
	"11  1 1  1  1 1",
	"1 1 1 1   1  1",
	"11   1  11   1e"
}, def={
	"11  111 111",
	"1 1 1   1",
	"1 1 111 11",
	"1 1 1   1",
	"11  111 1e"
}, shift={
	" 11 1 1 1 111 111",
	"1   1 1 1 1    1",
	" 1  111 1 11   1",
	"  1 1 1 1 1    1",
	"11  1 1 1 1    1e"
}, hyp={
	"1 1 1 1 11",
	"1 1 1 1 1 1",
	"111 1 1 11",
	"1 1  1  1",
	"1 1  1  1e"
}, de={
	"11  111",
	"1 1 1",
	"1 1 111",
	"1 1 1",
	"11  111e"
}, g={
	" 11",
	"1",
	"1 1",
	"1 1",
	" 11e"
}, rad={
	"11   1  11",
	"1 1 1 1 1 1",
	"11  111 1 1",
	"1 1 1 1 1 1",
	"1 1 1 1 11e"
}, braces={
	" 1 1",
	"1   1",
	"1   1",
	"1   1",
	" 1 1e"
}, m={
	"1   1",
	"11 11",
	"1 1 1",
	"1   1",
	"1   1e"
}, e={
	"111",
	"1",
	"111",
	"1",
	"111e"
}, kana={ // katakana charset
	"  1     1 ",
	" 11111 111",
	"  1  1  1 ",
	" 1   1  1 ",
	"1   1  1e"
}, shoo={ // minor
	"    1    ",
	" 1  1  1 ",
	"1   1   1",
	"    1    ",
	"   1e"
}, sml={
	" 11 1 1 1",
	"1   111 1",
	" 1  1 1 1",
	"  1 1 1 1",
	"11  1 1 111e"
};

VIDEO_UPDATE( pc1403 )
{
	pc1403_state *state = screen->machine->driver_data<pc1403_state>();
	running_machine *machine = screen->machine;
	int x, y, i, j;
	int color[3];

	bitmap_fill(bitmap, cliprect, 11);

	/* HJB: we cannot initialize array with values from other arrays, thus... */
	color[0] = 7; // pocketc_colortable[CONTRAST][0];
	color[2] = 8; // pocketc_colortable[CONTRAST][1];
	color[1] = (state->portc & 1) ? color[2] : color[0];

	if (state->portc & 1)
	{
		for (x=state->RIGHT, y=state->DOWN, i=0; i<6*5; x+=2) {
			for (j=0; j<5; j++, i++, x+=2)
			drawgfx_opaque(bitmap, 0, screen->machine->gfx[0], state->reg[i],CONTRAST,0,0,
				x,y);
		}
		for (i=9*5; i<12*5; x+=2)
		{
			for (j=0; j<5; j++, i++, x+=2)
			drawgfx_opaque(bitmap, 0, screen->machine->gfx[0], state->reg[i],CONTRAST,0,0,
				x,y);
		}
		for (i=6*5; i<9*5; x+=2)
		{
			for (j=0; j<5; j++, i++, x+=2)
			drawgfx_opaque(bitmap, 0, screen->machine->gfx[0], state->reg[i],CONTRAST,0,0,
				x,y);
		}
		for (i=0x7b-3*5; i>0x7b-6*5; x+=2)
		{
			for (j=0; j<5; j++, i--, x+=2)
				drawgfx_opaque(bitmap, 0, screen->machine->gfx[0], state->reg[i],CONTRAST,0,0,
				x,y);
		}
		for (i=0x7b; i>0x7b-3*5; x+=2)
		{
			for (j=0; j<5; j++, i--, x+=2)
			drawgfx_opaque(bitmap, 0, screen->machine->gfx[0], state->reg[i],CONTRAST,0,0,
				x,y);
		}
		for (i=0x7b-6*5; i>0x7b-12*5; x+=2)
		{
			for (j=0; j<5; j++, i--, x+=2)
			drawgfx_opaque(bitmap, 0, screen->machine->gfx[0], state->reg[i],CONTRAST,0,0,
				x,y);
		}
	}
	/* if display is off, busy is always visible? it seems to behave like that. */
	/* But if computer is off, busy is hidden. */
	if(!(state->portc&8))
	{
		if (state->portc&1)
			pocketc_draw_special(bitmap, state->RIGHT, state->DOWN-13, busy,
				state->reg[0x3d] & 1 ? color[2] : color[0]);

		else pocketc_draw_special(bitmap, state->RIGHT, state->DOWN-13, busy, color[2]);
	}
	else
		pocketc_draw_special(bitmap, state->RIGHT, state->DOWN-13, busy, color[0]);

	pocketc_draw_special(bitmap, state->RIGHT+18, state->DOWN-13, def,
			 state->reg[0x3d] & 0x02 ? color[1] : color[0]);
	pocketc_draw_special(bitmap, state->RIGHT+43, state->DOWN-13, shift,
			 state->reg[0x3d] & 0x04 ? color[1] : color[0]);
	pocketc_draw_special(bitmap, state->RIGHT+63, state->DOWN-13, hyp,
			 state->reg[0x3d] & 0x08 ? color[1] : color[0]);

	pocketc_draw_special(bitmap, state->RIGHT+155, state->DOWN-13, kana,
			 state->reg[0x3c] & 0x01 ? color[1] : color[0]);
	pocketc_draw_special(bitmap, state->RIGHT+167, state->DOWN-13, shoo,
			 state->reg[0x3c] & 0x02 ? color[1] : color[0]);
	pocketc_draw_special(bitmap, state->RIGHT+178, state->DOWN-13, sml,
			 state->reg[0x3c] & 0x04 ? color[1] : color[0]);

	pocketc_draw_special(bitmap, state->RIGHT+191, state->DOWN-13, de,
			 state->reg[0x7c] & 0x20 ? color[1] : color[0]);
	pocketc_draw_special(bitmap, state->RIGHT+199, state->DOWN-13, g,
			 state->reg[0x7c] & 0x10 ? color[1] : color[0]);
	pocketc_draw_special(bitmap, state->RIGHT+203, state->DOWN-13, rad,
			 state->reg[0x7c] & 0x08 ? color[1] : color[0]);

	pocketc_draw_special(bitmap, state->RIGHT+266, state->DOWN-13, braces,
			 state->reg[0x7c] & 0x04 ? color[1] : color[0]);
	pocketc_draw_special(bitmap, state->RIGHT+274, state->DOWN-13, m,
			 state->reg[0x7c] & 0x02 ? color[1] : color[0]);
	pocketc_draw_special(bitmap, state->RIGHT+281, state->DOWN-13, e,
			 state->reg[0x7c] & 0x01 ? color[1] : color[0]);

	pocketc_draw_special(bitmap, state->RIGHT+10, state->DOWN+27, line /* empty */,
			 state->reg[0x3c] & 0x40 ? color[1] : color[0]);
	pocketc_draw_special(bitmap, state->RIGHT+31, state->DOWN+27, line /*calc*/,
			 state->reg[0x3d] & 0x40 ? color[1] : color[0]);
	pocketc_draw_special(bitmap, state->RIGHT+52, state->DOWN+27, line/*run*/,
			 state->reg[0x3d] & 0x20 ? color[1] : color[0]);
	pocketc_draw_special(bitmap, state->RIGHT+73, state->DOWN+27, line/*prog*/,
			 state->reg[0x3d] & 0x10 ? color[1] : color[0]);
	pocketc_draw_special(bitmap, state->RIGHT+94, state->DOWN+27, line /* empty */,
			 state->reg[0x3c] & 0x20 ? color[1] : color[0]);

	pocketc_draw_special(bitmap, state->RIGHT+232, state->DOWN+27, line/*matrix*/,
			 state->reg[0x3c] & 0x10 ? color[1] : color[0]);
	pocketc_draw_special(bitmap, state->RIGHT+253, state->DOWN+27, line/*stat*/,
			 state->reg[0x3c] & 0x08 ? color[1] : color[0]);
	pocketc_draw_special(bitmap, state->RIGHT+274, state->DOWN+27, line/*print*/,
			 state->reg[0x7c] & 0x40 ? color[1] : color[0]);

	return 0;
}


