#include "driver.h"
#include "video/generic.h"

#include "includes/pocketc.h"
#include "includes/pc1350.h"

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
	UINT8 reg[0x1000];
} pc1350_lcd;

 READ8_HANDLER(pc1350_lcd_read)
{
	int data;
	data=pc1350_lcd.reg[offset&0xfff];
	logerror("pc1350 read %.3x %.2x\n",offset,data);
	return data;
}

WRITE8_HANDLER(pc1350_lcd_write)
{
	logerror("pc1350 write %.3x %.2x\n",offset,data);
	pc1350_lcd.reg[offset&0xfff]=data;
}

int pc1350_keyboard_line_r(void)
{
	return pc1350_lcd.reg[0xe00];
}

/* pc1350
   24x4 5x8 no space between chars
   7000 .. 701d, 7200..721d, 7400 ..741d, 7600 ..761d, 7800 .. 781d
   7040 .. 705d, 7240..725d, 7440 ..745d, 7640 ..765d, 7840 .. 785d
   701e .. 703b, 721e..723b, 741e ..743b, 761e ..763b, 781e .. 783b
   705e .. 707b, 725e..727b, 745e ..747b, 765e ..767b, 785e .. 787b
   783c: 0 SHIFT 1 DEF 4 RUN 5 PRO 6 JAPAN 7 SML */
static int pc1350_addr[4]={ 0, 0x40, 0x1e, 0x5e };

#define DOWN 45
#define RIGHT 76

VIDEO_UPDATE( pc1350 )
{
	int x, y, i, j, k, b;
	int color[2];
	/* HJB: we cannot initialize array with values from other arrays, thus... */
    color[0] = Machine->pens[pocketc_colortable[PC1350_CONTRAST][0]];
	color[1] = Machine->pens[pocketc_colortable[PC1350_CONTRAST][1]];

	for (k=0, y=DOWN; k<4; y+=16,k++)
	{
		for (x=RIGHT, i=pc1350_addr[k]; i<0xa00; i+=0x200)
		{
			for (j=0; j<=0x1d; j++, x+=2)
			{
				for (b = 0; b < 8; b++)
				{
					plot_box(bitmap, x, y + b * 2, 2, 2,
						color[(pc1350_lcd.reg[j+i] >> b) & 1]);
				}
			}
		}
	}
	/* 783c: 0 SHIFT 1 DEF 4 RUN 5 PRO 6 JAPAN 7 SML */
	/* I don't know how they really look like in the lcd */
	pocketc_draw_special(bitmap,RIGHT-30,DOWN+45,shift,
						pc1350_lcd.reg[0x83c]&1?color[1]:color[0]);
	pocketc_draw_special(bitmap,RIGHT-30,DOWN+55,def,
						pc1350_lcd.reg[0x83c]&2?color[1]:color[0]);
	pocketc_draw_special(bitmap,RIGHT-30,DOWN+5,run,
						pc1350_lcd.reg[0x83c]&0x10?color[1]:color[0]);
	pocketc_draw_special(bitmap,RIGHT-30,DOWN+15,pro,
						pc1350_lcd.reg[0x83c]&0x20?color[1]:color[0]);
	pocketc_draw_special(bitmap,RIGHT-30,DOWN+25,japan,
						pc1350_lcd.reg[0x83c]&0x40?color[1]:color[0]);
	pocketc_draw_special(bitmap,RIGHT-30,DOWN+35,sml,
						pc1350_lcd.reg[0x83c]&0x80?color[1]:color[0]);
	return 0;
}
