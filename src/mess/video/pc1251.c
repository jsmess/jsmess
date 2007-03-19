#include "driver.h"
#include "video/generic.h"

#include "includes/pocketc.h"
#include "includes/pc1251.h"

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
}, run={ 
	"11  1 1 1  1",
	"1 1 1 1 11 1",
	"11  1 1 1 11",
	"1 1 1 1 1  1",
	"1 1  1  1  1e" 
}, pro={ 
	"11  11   1  ",
	"1 1 1 1 1 1",
	"11  11  1 1",
	"1   1 1 1 1",
	"1   1 1  1e" 
}, japan={ 
	"  1  1  11   1  1  1",
	"  1 1 1 1 1 1 1 11 1",
	"  1 111 11  111 1 11",
	"1 1 1 1 1   1 1 1  1",
	" 1  1 1 1   1 1 1  1e" 
}, sml={ 
	" 11 1 1 1",
	"1   111 1",
	" 1  1 1 1",
	"  1 1 1 1",
	"11  1 1 111e" 
}, rsv={ 
	"11   11 1   1",
	"1 1 1   1   1",
	"11   1   1 1",
	"1 1   1  1 1",
	"1 1 11    1e" 
};

static struct {
	UINT8 reg[0x100];
} pc1251_lcd;

 READ8_HANDLER(pc1251_lcd_read)
{
	int data;
	data=pc1251_lcd.reg[offset&0xff];
	logerror("pc1251 read %.3x %.2x\n",offset,data);
	return data;
}

WRITE8_HANDLER(pc1251_lcd_write)
{
	logerror("pc1251 write %.3x %.2x\n",offset,data);
	pc1251_lcd.reg[offset&0xff]=data;
}

#define DOWN 62
#define RIGHT 68

VIDEO_UPDATE( pc1251 )
{
	int x, y, i, j;
	int color[2];
	/* HJB: we cannot initialize array with values from other arrays, thus... */
    color[0] = Machine->pens[pocketc_colortable[PC1251_CONTRAST][0]];
	color[1] = Machine->pens[pocketc_colortable[PC1251_CONTRAST][1]];

	for (x=RIGHT,y=DOWN,i=0; i<60;x+=3) {
		for (j=0; j<5;j++,i++,x+=3)
			drawgfx(bitmap, Machine->gfx[0], pc1251_lcd.reg[i],
					PC1251_CONTRAST,0,0,
					x,y,
					0, TRANSPARENCY_NONE,0);
	}
	for (i=0x7b; i>=0x40;x+=3) {
		for (j=0; j<5;j++,i--,x+=3)
			drawgfx(bitmap, Machine->gfx[0], pc1251_lcd.reg[i],
					PC1251_CONTRAST,0,0,
					x,y,
					0, TRANSPARENCY_NONE,0);
	}
	pocketc_draw_special(bitmap,RIGHT+134,DOWN-10,de,
						pc1251_lcd.reg[0x3c]&8?color[1]:color[0]);
	pocketc_draw_special(bitmap,RIGHT+142,DOWN-10,g,
						pc1251_lcd.reg[0x3c]&4?color[1]:color[0]);
	pocketc_draw_special(bitmap,RIGHT+146,DOWN-10,rad,
						pc1251_lcd.reg[0x3d]&4?color[1]:color[0]);
	pocketc_draw_special(bitmap,RIGHT+18,DOWN-10,def,
						pc1251_lcd.reg[0x3c]&1?color[1]:color[0]);
	pocketc_draw_special(bitmap,RIGHT,DOWN-10,shift,
						pc1251_lcd.reg[0x3d]&2?color[1]:color[0]);
	pocketc_draw_special(bitmap,RIGHT+38,DOWN-10,pro,
						pc1251_lcd.reg[0x3e]&1?color[1]:color[0]);
	pocketc_draw_special(bitmap,RIGHT+53,DOWN-10,run,
						pc1251_lcd.reg[0x3e]&2?color[1]:color[0]);
	pocketc_draw_special(bitmap,RIGHT+68,DOWN-10,rsv,
						pc1251_lcd.reg[0x3e]&4?color[1]:color[0]);

	/* 0x3c 1 def?, 4 g, 8 de
	   0x3d 2 shift, 4 rad, 8 error
	   0x3e 1 pro?, 2 run?, 4rsv?*/
	return 0;
}

