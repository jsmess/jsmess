#include "driver.h"
#include "video/generic.h"

#include "includes/hp48.h"

static unsigned char hp48_palette[] =
{
	49,70,64,	/* background */
	40,35,55,	/* symbol color */
	49,72,73,	/* lcd light */
	37,42,64	/* lcd dark */
};

/* 32 contrast steps */
unsigned short hp48_colortable[0x20][2] = {
	{ 0, 2 },
	{ 0, 2 },
	{ 0, 2 },
	{ 0, 2 },
	{ 0, 2 },
	{ 0, 2 },
	{ 0, 2 },
	{ 0, 2 },

	{ 0, 2 },
	{ 0, 2 },
	{ 0, 2 },
	{ 0, 3 },
	{ 0, 3 },
	{ 0, 3 },
	{ 0, 3 },
	{ 0, 3 },

	{ 0, 3 },
	{ 0, 3 },
	{ 0, 3 },
	{ 0, 3 },
	{ 0, 3 },
	{ 0, 3 },
	{ 2, 3 },
	{ 2, 3 },

	{ 2, 3 },
	{ 2, 3 },
	{ 2, 3 },
	{ 2, 3 },
	{ 2, 3 },
	{ 2, 3 },
	{ 2, 3 },
	{ 2, 3 }
};

PALETTE_INIT( hp48 )
{
	palette_set_colors(machine, 0, hp48_palette, sizeof(hp48_palette) / 3);
	memcpy(colortable,hp48_colortable,sizeof(hp48_colortable));
}

VIDEO_START( hp48 )
{
    videoram_size = 6 * 2 + 24;
    videoram = (UINT8*) auto_malloc (videoram_size);

#if 0
	{
		char backdrop_name[200];
	    /* try to load a backdrop for the machine */
		sprintf (backdrop_name, "%s.png", Machine->gamedrv->name);
		backdrop_load(backdrop_name, 8);
	}
#endif

	return video_start_generic(machine);
}

static void hp48_draw_special(mame_bitmap *bitmap,int x, int y, const char *figure, int color)
{
	int j, xi=0;
	for (j=0; figure[j]; j++) {
		switch (figure[j]) {
		case '1': 
			plot_pixel(bitmap, x+xi, y, color);
			xi++;
			break;
		case ' ': 
			xi++;
			break;
		case '\r':
			xi=0;
			y++;
			break;				
		};
	}
}


#define LCD_LINES (hp48_hardware.data[0x28]|((hp48_hardware.data[0x29]&3)<<4)

static const char *hp48_orange={ 
	"11111111111\r"
	"111 1111111\r"
	"11  1111111\r"
	"1         1\r"
	"11  11111 1\r"
	"111 11111 1\r"
	"111111111 1"
}, *hp48_blue= {
	"11111111111\r"
	"1111111 111\r"
	"1111111  11\r"
	"1         1\r"
	"1 11111  11\r"
	"1 11111 111\r"
	"1 111111111"
}, *hp48_alpha= {
	"          1\r"
	"   11111 1\r"
	"  1     1\r"
	" 1      1\r"
	" 1      1\r"
	" 1     11\r"
	"  11111  1"
}, *hp48_alarm= {
	"  1       1\r"
	" 1  1   1  1\r"
	"1  1  1  1  1\r"
	"1  1 111 1  1\r"
	"1  1  1  1  1\r"
	" 1  1   1  1\r"
	"  1       1"
}, *hp48_busy= {
	"11111111\r"
	" 1    1\r"
	"  1  1\r"
	"   11\r"
	"  1  1\r"
	" 1    1\r"
	"11111111"
}, *hp48_transmit={
	" 11\r"
	"1  1   1\r"
	"    1   1\r"
	"1111111111\r"
	"    1   1\r"
	"1  1   1\r"
	" 11"
};

#define DOWN 98
#define RIGHT 40
#define LCD_ENABLE hp48_hardware.data[0]&8
#define LCD_VERTICAL_OFFSET hp48_hardware.data[0]&7
#define LCD_BASE_ADDRESS (hp48_hardware.data[0x20]|(hp48_hardware.data[0x21]<<4)\
		|(hp48_hardware.data[0x22]<<8)|(hp48_hardware.data[0x23]<<12)\
		|(hp48_hardware.data[0x24]<<16))

#define LCD_LINE_OFFSET (hp48_hardware.data[0x25]|(hp48_hardware.data[0x26]<<4)\
		|(hp48_hardware.data[0x27]<<8))

VIDEO_UPDATE( hp48 )
{
	int x, y, i;
	int color[2];
	int contrast=(hp48_hardware.data[1]|((hp48_hardware.data[2]&1)<<4));

	/* HJB: we cannot initialize array with values from other arrays, thus... */
    color[0] = Machine->pens[0];
//    color[0] = Machine->pens[1];
	color[1] = Machine->pens[1];


	for (y=0,i=LCD_BASE_ADDRESS; y<64; y+=8, i+=LCD_LINE_OFFSET) {
		for (x=0; x<131; x++) {
			drawgfx(bitmap, Machine->gfx[0], 
					program_read_byte(i+x),
					contrast,0,0,
					x*2+RIGHT,y*2+DOWN,
					0, TRANSPARENCY_NONE,0);
		}
	}

	hp48_draw_special(bitmap,RIGHT+12,DOWN-13,hp48_orange,
					  hp48_hardware.data[0xb]&1?color[1]:color[0]);
	hp48_draw_special(bitmap,RIGHT+57,DOWN-13,hp48_blue,
					  hp48_hardware.data[0xb]&2?color[1]:color[0]);
	hp48_draw_special(bitmap,RIGHT+102,DOWN-13,hp48_alpha,
					  hp48_hardware.data[0xb]&4?color[1]:color[0]);
	hp48_draw_special(bitmap,RIGHT+147,DOWN-13,hp48_alarm,
					  hp48_hardware.data[0xb]&8?color[1]:color[0]);
	hp48_draw_special(bitmap,RIGHT+192,DOWN-13,hp48_busy,
					  hp48_hardware.data[0xc]&1?color[1]:color[0]);
	hp48_draw_special(bitmap,RIGHT+237,DOWN-13,hp48_transmit,
					  hp48_hardware.data[0xc]&2?color[1]:color[0]);
	return 0;
}
