#include "driver.h"
#include "includes/hp48.h"

static const UINT8 hp48_palette[] =
{
	49,70,64,	/* background */
	40,35,55,	/* symbol color */
	49,72,73,	/* lcd light */
	37,42,64	/* lcd dark */
};


PALETTE_INIT( hp48 )
{
	int i;

	for ( i = 0; i < 4; i++ ) {
		palette_set_color_rgb( machine, i, hp48_palette[i*3], hp48_palette[i*3+1], hp48_palette[i*3+2] );
	}
}


VIDEO_START( hp48 )
{
    videoram_size = 6 * 2 + 24;
    videoram = (UINT8*) auto_malloc (videoram_size);
}


typedef const char *HP48_FIGURE;

static void hp48_draw_special(bitmap_t *bitmap,int x, int y, const HP48_FIGURE figure, int color)
{
	int j, xi=0;
	for (j=0; figure[j]; j++) {
		switch (figure[j]) {
		case '1':
			*BITMAP_ADDR16(bitmap, y, x+xi) = color;
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


static const HP48_FIGURE hp48_orange={
	"11111111111\r"
	"111 1111111\r"
	"11  1111111\r"
	"1         1\r"
	"11  11111 1\r"
	"111 11111 1\r"
	"111111111 1"
}, hp48_blue= {
	"11111111111\r"
	"1111111 111\r"
	"1111111  11\r"
	"1         1\r"
	"1 11111  11\r"
	"1 11111 111\r"
	"1 111111111"
}, hp48_alpha= {
	"          1\r"
	"   11111 1\r"
	"  1     1\r"
	" 1      1\r"
	" 1      1\r"
	" 1     11\r"
	"  11111  1"
}, hp48_alarm= {
	"  1       1\r"
	" 1  1   1  1\r"
	"1  1  1  1  1\r"
	"1  1 111 1  1\r"
	"1  1  1  1  1\r"
	" 1  1   1  1\r"
	"  1       1"
}, hp48_busy= {
	"11111111\r"
	" 1    1\r"
	"  1  1\r"
	"   11\r"
	"  1  1\r"
	" 1    1\r"
	"11111111"
}, hp48_transmit={
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
#define LCD_ENABLE (hp48_hardware.data[0]&8)
#define LCD_VERTICAL_OFFSET (hp48_hardware.data[0]&7)
#define LCD_MAIN_BASE_ADDRESS (hp48_hardware.data[0x20]|(hp48_hardware.data[0x21]<<4)\
		|(hp48_hardware.data[0x22]<<8)|(hp48_hardware.data[0x23]<<12)\
		|(hp48_hardware.data[0x24]<<16))
#define LCD_MENU_BASE_ADDRESS (hp48_hardware.data[0x30]|(hp48_hardware.data[0x31]<<4)\
		|(hp48_hardware.data[0x32]<<8)|(hp48_hardware.data[0x33]<<12)\
		|(hp48_hardware.data[0x34]<<16))
#define LCD_MAIN_SIZE (hp48_hardware.data[0x28]|(hp48_hardware.data[0x29]<<4))

#define LCD_LINE_OFFSET (hp48_hardware.data[0x25]|(hp48_hardware.data[0x26]<<4)\
		|(hp48_hardware.data[0x27]<<8))

VIDEO_UPDATE( hp48 )
{
	int x, y, i, p, data, max_y;
	int color[2];
//	int contrast=(hp48_hardware.data[1]|((hp48_hardware.data[2]&1)<<4));

	/* HJB: we cannot initialize array with values from other arrays, thus... */
    color[0] = machine->pens[0];
//    color[0] = machine->pens[1];
	color[1] = machine->pens[1];

	i = LCD_MAIN_BASE_ADDRESS;
	max_y = MIN(64, LCD_MAIN_SIZE + 1 );
	if ( max_y < 3 ) {
		max_y = 64;
	}

	i = LCD_MAIN_BASE_ADDRESS;
	for (y=0; y< max_y; y++) {
		int j = ( LCD_VERTICAL_OFFSET & 4 ? 1 : 0 );
		p = LCD_VERTICAL_OFFSET & 0x03;
		data = program_read_byte( i + j );
		for (x=0; x<131; x++) {
			*BITMAP_ADDR16(bitmap, DOWN + y, RIGHT + x) = ( ( data << p ) & 0x08 ) ? 3 : 2;
			p++;
			if ( p > 3 ) {
				j++;
				data = program_read_byte( i + j );
				p = 0;
			}
		}
		i += 34 + LCD_LINE_OFFSET;
	}

	i = LCD_MENU_BASE_ADDRESS;
	for ( ; y < 64; y++ ) {
		int j = 0;
		p = 0;
		data = program_read_byte( i + j );
		for ( x = 0; x < 131; x++ ) {
			*BITMAP_ADDR16(bitmap, DOWN + y, RIGHT + x) = ( ( data << p ) & 0x08 ) ? 3 : 2;
			p++;
			if ( p > 3 ) {
				j++;
				data = program_read_byte( i + j );
				p = 0;
			}
		}
		i += 34;
	}

	hp48_draw_special(bitmap,RIGHT+12,DOWN-13,hp48_orange,
					  hp48_hardware.data[0xb]&1?color[1]:color[1]);
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
