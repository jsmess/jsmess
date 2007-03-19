/*****************************************************************************
 *
 *	 pc1403.c
 *	 portable sharp pc1403 video emulator interface
 *   (sharp pocket computers)
 *
 *	 Copyright (c) 2001 Peter Trauner, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   peter.trauner@jk.uni-linz.ac.at
 *	 - The author of this copywritten work reserves the right to change the
 *	   terms of its usage and license at any time, including retroactively
 *	 - This entire notice must remain in the source code.
 *
 * History of changes:
 * 21.07.2001 Several changes listed below were made by Mario Konegger
 *            (konegger@itp.tu-graz.ac.at)
 *	      Placed the grafical symbols onto the right place and added
 *	      some symbols, so the display is correct rebuit.
 *	      Added a strange behaviour of the display concerning the on/off
 *	      state and the BUSY-symbol, which I found out with experiments
 *	      with my own pc1403.
 *****************************************************************************/

#include "driver.h"
#include "video/generic.h"

#include "includes/pocketc.h"
#include "includes/pc1401.h"
#include "includes/pc1403.h"

static struct {
	UINT8 reg[0x100];
} pc1403_lcd;

const struct { int x, y; } pos[]={
    { 152,67 }, // pc1403
    { 155,69 } // pc1403h
};

static int DOWN=67, RIGHT=152;

VIDEO_START( pc1403 )
{
	if (strcmp(Machine->gamedrv->name,"pc1403h")==0) {
		DOWN=pos[1].y;
		RIGHT=pos[1].x;
	}
    return video_start_pocketc(machine);
}


 READ8_HANDLER(pc1403_lcd_read)
{
    UINT8 data=pc1403_lcd.reg[offset];
    return data;
}

WRITE8_HANDLER(pc1403_lcd_write)
{
    pc1403_lcd.reg[offset]=data;
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
	int x, y, i, j;
	int color[3];
	/* HJB: we cannot initialize array with values from other arrays, thus... */
	color[0] = Machine->pens[pocketc_colortable[CONTRAST][0]];
	color[2] = Machine->pens[pocketc_colortable[CONTRAST][1]];
	color[1] = (pc1403_portc&1) ? color[2] : color[0];

	if (pc1403_portc&1) {
		for (x=RIGHT,y=DOWN,i=0; i<6*5;x+=2) {
			for (j=0; j<5;j++,i++,x+=2)
			drawgfx(bitmap, Machine->gfx[0], pc1403_lcd.reg[i],CONTRAST,0,0,
				x,y,
				0, TRANSPARENCY_NONE,0);
		}
		for (i=9*5; i<12*5;x+=2) {
			for (j=0; j<5;j++,i++,x+=2)
			drawgfx(bitmap, Machine->gfx[0], pc1403_lcd.reg[i],CONTRAST,0,0,
				x,y,
				0, TRANSPARENCY_NONE,0);
		}
		for (i=6*5; i<9*5;x+=2) {
			for (j=0; j<5;j++,i++,x+=2)
			drawgfx(bitmap, Machine->gfx[0], pc1403_lcd.reg[i],CONTRAST,0,0,
				x,y,
				0, TRANSPARENCY_NONE,0);
		}
		for (i=0x7b-3*5; i>0x7b-6*5;x+=2) {
			for (j=0; j<5;j++,i--,x+=2)
				drawgfx(bitmap, Machine->gfx[0], pc1403_lcd.reg[i],CONTRAST,0,0,
				x,y,
				0, TRANSPARENCY_NONE,0);
		}
		for (i=0x7b; i>0x7b-3*5;x+=2) {
			for (j=0; j<5;j++,i--,x+=2)
			drawgfx(bitmap, Machine->gfx[0], pc1403_lcd.reg[i],CONTRAST,0,0,
				x,y,
				0, TRANSPARENCY_NONE,0);
		}
		for (i=0x7b-6*5; i>0x7b-12*5;x+=2) {
			for (j=0; j<5;j++,i--,x+=2)
			drawgfx(bitmap, Machine->gfx[0], pc1403_lcd.reg[i],CONTRAST,0,0,
				x,y,
				0, TRANSPARENCY_NONE,0);
		}
	}
    /* if display is off, busy is always visible? it seems to behave like 
that. */
    /* But if computer is off, busy is hidden. */
    if(!(pc1403_portc&8))
    {if (pc1403_portc&1) pocketc_draw_special(bitmap,RIGHT,DOWN-13,busy,
			 pc1403_lcd.reg[0x3d]&1?color[2]:color[0]);
     else pocketc_draw_special(bitmap,RIGHT,DOWN-13,busy,color[2]);
    }
    else pocketc_draw_special(bitmap,RIGHT,DOWN-13,busy,color[0]);
    
    pocketc_draw_special(bitmap,RIGHT+18,DOWN-13,def,
			 pc1403_lcd.reg[0x3d]&2?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+43,DOWN-13,shift,
			 pc1403_lcd.reg[0x3d]&4?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+63,DOWN-13,hyp,
			 pc1403_lcd.reg[0x3d]&8?color[1]:color[0]);

    pocketc_draw_special(bitmap,RIGHT+155,DOWN-13,kana,
			 pc1403_lcd.reg[0x3c]&1?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+167,DOWN-13,shoo,
			 pc1403_lcd.reg[0x3c]&2?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+178,DOWN-13,sml,
			 pc1403_lcd.reg[0x3c]&4?color[1]:color[0]);

    pocketc_draw_special(bitmap,RIGHT+191,DOWN-13,de,
			 pc1403_lcd.reg[0x7c]&0x20?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+199,DOWN-13,g,
			 pc1403_lcd.reg[0x7c]&0x10?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+203,DOWN-13,rad,
			 pc1403_lcd.reg[0x7c]&8?color[1]:color[0]);

    pocketc_draw_special(bitmap,RIGHT+266,DOWN-13,braces,
			 pc1403_lcd.reg[0x7c]&4?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+274,DOWN-13,m,
			 pc1403_lcd.reg[0x7c]&2?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+281,DOWN-13,e,
			 pc1403_lcd.reg[0x7c]&1?color[1]:color[0]);
    
    pocketc_draw_special(bitmap, RIGHT+10,DOWN+27,line /* empty */,
			 (pc1403_lcd.reg[0x3c]&0x40) ?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+31,DOWN+27,line /*calc*/,
			 (pc1403_lcd.reg[0x3d]&0x40) ?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+52,DOWN+27,line/*run*/,
			 (pc1403_lcd.reg[0x3d]&0x20) ?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+73,DOWN+27,line/*prog*/,
			 (pc1403_lcd.reg[0x3d]&0x10) ? color[1]:color[0]);
    pocketc_draw_special(bitmap, RIGHT+94,DOWN+27,line /* empty */,
			 (pc1403_lcd.reg[0x3c]&0x20) ? color[1]:color[0]);
			 
    pocketc_draw_special(bitmap,RIGHT+232,DOWN+27,line/*matrix*/, 
			 pc1403_lcd.reg[0x3c]&0x10?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+253,DOWN+27,line/*stat*/,
			 pc1403_lcd.reg[0x3c]&8?color[1]:color[0]);
    pocketc_draw_special(bitmap,RIGHT+274,DOWN+27,line/*print*/,
			 pc1403_lcd.reg[0x7c]&0x40?color[1]:color[0]);
    
	return 0;
}


