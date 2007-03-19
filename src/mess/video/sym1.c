/******************************************************************************
	SYM-1

	PeT mess@utanet.at May 2000

******************************************************************************/

#include "driver.h"
#include "video/generic.h"

#include "includes/sym1.h"

UINT8 sym1_led[6]= {0};

static unsigned char sym1_palette[] =
{
	0x20,0x02,0x05,
	0xc0, 0, 0
};

PALETTE_INIT( sym1 )
{
	palette_set_colors(machine, 0, sym1_palette, sizeof(sym1_palette) / 3);
}

VIDEO_START( sym1 )
{
    videoram_size = 6 * 2 + 24;
    videoram = (UINT8*) auto_malloc (videoram_size);

#if 0
	{
		char backdrop_name[200];
	    /* try to load a backdrop for the machine */
		sprintf (backdrop_name, "%s.png", Machine->gamedrv->name);
		backdrop_load(backdrop_name, 3);
	}
#endif

	if (video_start_generic(machine) != 0)
        return 1;

    return 0;
}

static const char led[] =
	"          aaaaaaaaa\r" 
	"       ff aaaaaaaaa bb\r"
	"       ff           bb\r"
	"       ff           bb\r"
	"       ff           bb\r" 
	"       ff           bb\r" 
	"      ff           bb\r" 
	"      ff           bb\r" 
	"      ff           bb\r" 
	"      ff           bb\r" 
	"         gggggggg\r" 
	"     ee  gggggggg cc\r" 
	"     ee           cc\r" 
	"     ee           cc\r" 
	"     ee           cc\r" 
	"     ee           cc\r" 
	"    ee           cc\r" 
	"    ee           cc\r" 
	"    ee           cc\r" 
	"    ee           cc\r" 
	"    ee ddddddddd cc\r" 
	"ii     ddddddddd      hh\r"
    "ii                    hh";

static void sym1_draw_7segment(mame_bitmap *bitmap,int value, int x, int y)
{
	int i, xi, yi, mask, color;

	for (i=0, xi=0, yi=0; led[i]; i++) {
		mask=0;
		switch (led[i]) {
		case 'a': mask=1; break;
		case 'b': mask=2; break;
		case 'c': mask=4; break;
		case 'd': mask=8; break;
		case 'e': mask=0x10; break;
		case 'f': mask=0x20; break;
		case 'g': mask=0x40; break;
		case 'h': mask=0x80; break;
		}
		
		if (mask!=0) {
			color=Machine->pens[(value&mask)?1:0];
			plot_pixel(bitmap, x+xi, y+yi, color);
		}
		if (led[i]!='\r') xi++;
		else { yi++, xi=0; }
	}
}

static const struct {
	int x,y;
} sym1_led_pos[8]={
	{594,262},
	{624,262},
	{653,262},
	{682,262},
	{711,262},
	{741,262},
	{80,228},
	{360,32}
};

static const char* single_led=
" 111\r"
"11111\r"
"11111\r"
"11111\r"
" 111"
;

static void sym1_draw_led(mame_bitmap *bitmap,INT16 color, int x, int y)
{
	int j, xi=0;
	for (j=0; single_led[j]; j++) {
		switch (single_led[j]) {
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

VIDEO_UPDATE( sym1 )
{
	int i;

	for (i=0; i<6; i++) {
		sym1_draw_7segment(bitmap, sym1_led[i], sym1_led_pos[i].x, sym1_led_pos[i].y);
//		sym1_draw_7segment(bitmap, sym1_led[i], sym1_led_pos[i].x-160, sym1_led_pos[i].y-120);
		sym1_led[i]=0;
	}

	sym1_draw_led(bitmap, Machine->pens[1], 
				 sym1_led_pos[6].x, sym1_led_pos[6].y);
	sym1_draw_led(bitmap, Machine->pens[1], 
				 sym1_led_pos[7].x, sym1_led_pos[7].y);
	return 0;
}

