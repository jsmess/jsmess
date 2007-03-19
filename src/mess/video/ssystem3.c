/******************************************************************************
 PeT mess@utanet.at
******************************************************************************/

#include "driver.h"
#include "video/generic.h"

#include "includes/ssystem3.h"

UINT8 ssystem3_led[5]= {0};

static unsigned char ssystem3_palette[] =
{
	0,12,12,
	80,82,75,
	0,12,12
};

static unsigned short ssystem3_colortable[1][2] = {
	{ 0, 1 },
};

PALETTE_INIT( ssystem3 )
{
	palette_set_colors(machine, 0, ssystem3_palette, sizeof(ssystem3_palette) / 3);
	memcpy(colortable,ssystem3_colortable,sizeof(ssystem3_colortable));
}

VIDEO_START( ssystem3 )
{
	// artwork seams to need this
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

	return video_start_generic(machine);
}

static const char led[]={
	"    aaaaaaaaaaaa\r"
	"  f  aaaaaaaaaa  b\r"
	"  ff  aaaaaaaa  bb\r"
	"  fff  aaaaaa  bbb\r"
	"  ffff        bbbb\r"
	"  ffff        bbbb\r"
	"  ffff        bbbb\r"
	"  ffff        bbbb\r"
	" ffff        bbbb\r"
	" ffff        bbbb\r"
	" fff          bbb\r"
    " f   gggggggg   b\r"
    "   gggggggggggg\r"
	"   gggggggggggg\r"
	" e   gggggggg   c\r"
	" eee          ccc\r"
	" eeee        cccc\r"
	" eeee        cccc\r"
	"eeee        cccc\r"
	"eeee        cccc\r"
	"eeee        cccc\r"
	"eeee        cccc\r"
	"eee  dddddd  ccc\r"
	"ee  dddddddd  cc\r"
    "e  dddddddddd  c\r"
    "  dddddddddddd"
};

static void ssystem3_draw_7segment(mame_bitmap *bitmap,int value, int x, int y)
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
		case 'h': 
			// this is more likely wired to the separate leds
			mask=0x80; 
			break;
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
} ssystem3_led_pos[5]={
	{150,123},
	{170,123},
	{200,123},
	{220,123},
	{125,123},
};

static const char* single_led=
"          c                                                                                                           1 1\r"
"   bb   ccccc   bb                                                                                                    1 1\r"
"   bb     c     bb                                                                                                    1 1\r"
" bb  bb       bb  bb                                                                                                  1 1\r"
" bb  bbbbbbbbbbb  bb                                                                                                111 111\r"
"                                                                                                                      1 1\r"
"  aa   9999999    dd                                                                                                  1 1\r"
"  aa              dd                                                                                                  1 1\r"
"     88888888888 d                                                                                                    1 1\r"
"    88         8 dddd\r"
"  8888 6666666 8 dddd\r"
"  8888 6     6\r"
"       6 777 6\r"
"       6 777 6\r"
"       6 777 6\r"
"      6  777  6\r"
"      6 77777 6\r"
"     6  77777  6                                                 2     3                                             4\r"
"     6 7777777 6                                                 2     3                                             4\r"
"    6  7777777  6                                                 2   3                                              4\r"
"    6 777777777 6                                                 2   3                                            44444\r"
"   6  777777777  6                                                 2 3                                               4\r"
"   6 77777777777 6                                                2   3                                            44444\r"
"  6       7       6                                               2   3                                              4\r"
"  6666666 7 6666666                                              2     3                                             4\r"
"          7                                                      2     3                                             4\r"
"  77777777777777777\r"
"\r"
"\r"
"\r"
" 5555555555555555555          000000   000000   00        00  0000000   00    00  00000000  00  00    00   0000000\r"
"5                   5        0000000  00000000  000      000  00000000  00    00  00000000  00  000   00  00000000\r"
"5                   5        00       00    00  0000    0000  00    00  00    00     00     00  0000  00  00\r"
"5                   5        00       00    00  00 00  00 00  00000000  00    00     00     00  00 00 00  00  0000\r"
"5                   5        00       00    00  00 00  00 00  0000000   00    00     00     00  00  0000  00    00\r"
"5                   5        0000000  00000000  00  0000  00  00        00000000     00     00  00   000  00000000\r"
" 55555555   55555555          000000   000000   00   00   00  00         000000      00     00  00    00   0000000"
;

static void ssystem3_draw_led(mame_bitmap *bitmap,INT16 color, int x, int y, int ch)
{
	int j, xi=0;
	for (j=0; single_led[j]; j++) {
		switch (single_led[j]) {
		default:
			if (ch==single_led[j]) {
				plot_pixel(bitmap, x+xi, y, color);
			}
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

VIDEO_UPDATE( ssystem3 )
{
	int i;

	for (i=0; i<4; i++) {
		ssystem3_draw_7segment(bitmap, ssystem3_led[i]&0x7f, ssystem3_led_pos[i].x, 
							   ssystem3_led_pos[i].y);
	}

	ssystem3_draw_led(bitmap, Machine->pens[ssystem3_led[4]&8?1:0], 
				 ssystem3_led_pos[4].x, ssystem3_led_pos[4].y, '0');
	ssystem3_draw_led(bitmap, Machine->pens[ssystem3_led[4]&8?1:0], 
				 ssystem3_led_pos[4].x, ssystem3_led_pos[4].y, '1');
	ssystem3_draw_led(bitmap, Machine->pens[ssystem3_led[4]&8?1:0], 
				 ssystem3_led_pos[4].x, ssystem3_led_pos[4].y, '2');
	ssystem3_draw_led(bitmap, Machine->pens[ssystem3_led[4]&8?1:0], 
				 ssystem3_led_pos[4].x, ssystem3_led_pos[4].y, '3');
	ssystem3_draw_led(bitmap, Machine->pens[ssystem3_led[4]&8?1:0], 
				 ssystem3_led_pos[4].x, ssystem3_led_pos[4].y, '4');
	ssystem3_draw_led(bitmap, Machine->pens[ssystem3_led[4]&8?1:0], 
				 ssystem3_led_pos[4].x, ssystem3_led_pos[4].y, '5');
	ssystem3_draw_led(bitmap, Machine->pens[ssystem3_led[4]&8?1:0], 
				 ssystem3_led_pos[4].x, ssystem3_led_pos[4].y, '6');
	return 0;
}
