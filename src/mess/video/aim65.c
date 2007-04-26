/******************************************************************************

	PeT mess@utanet.at Nov 2000
	Updated by Dan Boris, 3/4/2007

******************************************************************************/

#include "driver.h"
#include "video/generic.h"

#include "includes/aim65.h"

static int printer_x;
static int printer_y;
static int printer_dir;
static int flag_a;
static int flag_b;

UINT16 *printerRAM;

typedef struct {
	int digit[4]; // 16 segment digit, decoded from 7 bit data!
	int cursor_on[4];
} DL1416A;

static DL1416A dl1416a[5]= { { {0} } };

void dl1416a_write(int chip, int digit, int value, int cursor)
{
	digit^=3;
	if (cursor) {
		dl1416a[chip].cursor_on[0]=value&1;
		dl1416a[chip].cursor_on[1]=value&2;
		dl1416a[chip].cursor_on[2]=value&4;
		dl1416a[chip].cursor_on[3]=value&8;
	} else {
		dl1416a[chip].digit[digit]=value;
	}
}

// aim 65 users guide table 7.6
static const int dl1416a_segments[0x80]={ // witch segments must be turned on for this value!?
	0x0000, 0x0000, 0x0000, 0x0000, // 0x00 // not known
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, // 0x10
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x1121, 0x0180, 0x0f3c, // 0x20   ! " #
	0x0fbb, 0xaf99, 0x5979, 0x2000, //      $ % & '
	0x6000, 0x9000, 0xff00, 0x0f00, //      ( ) * +
	0x8000, 0x0a00, 0x0020, 0xa000, //      , - . /
	0x05e1, 0x0500, 0x0961, 0x0d21, // 0x30 0 1 2 3
	0x0d80, 0x0ca1, 0x0ce1, 0x0501, //      4 5 6 7
	0x0de1, 0x0da1, 0x0021, 0x8001, //      8 9 : ;
	0xa030, 0x0a30, 0x5030, 0x0607, //      < = > ?
	0x0c7f, 0x0acf, 0x073f, 0x00f3, // 0x40 @ A B C
	0x053f, 0x08f3, 0x08c3, 0x02fb, //      D E F G
	0x0acc, 0x0533, 0x0563, 0x68c0, //      H I J K
	0x00f0, 0x30cc, 0x50cc, 0x00ff, //      L M N O
	0x0ac7, 0x40ff, 0x4ac7, 0x0abb, // 0x50 P Q R S
	0x0503, 0x00fc, 0xa0c0, 0xc0cc, //      T U V W
	0xf000, 0x3400, 0xa033, 0x00e1, //      X Y Z [
	0x5000, 0x001e, 0xc000, 0x0030, //      \ ] ^ _
	0x0000, 0x0000, 0x0000, 0x0000, // 0x60
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, // 0x70
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000
};

static unsigned char aim65_palette[] =
{
  	0x20,0x02,0x05,
	0xc0, 0, 0,
};

PALETTE_INIT( aim65 )
{
	palette_set_colors(machine, 0, aim65_palette, sizeof(aim65_palette) / 3);
}

void aim65_printer_inc(void)
{
	if (printer_dir)
	{
		if (printer_x > 0) {
			printer_x--;
		}
		else {
			printer_dir = 0;
			printer_x++;
			printer_y++;
		}
	}
	else
	{
		if (printer_x < 9) {
			printer_x++;
		}
		else {
			printer_dir = 1;
			printer_x--;
			printer_y++;
		}
	}

	if (printer_y > 500) printer_y = 0;

	flag_a=0;
	flag_b=0;
}

void aim65_printer_cr(void) {
	printer_x=0;
	printer_y++;
	if (printer_y > 500) printer_y = 0;
	flag_a=flag_b=0;
}

void aim65_printer_data_a(UINT8 data) {
/*	if (flag_a == 0) {
		printerRAM[(printer_y * 20) + printer_x] |= data;
		flag_a = 1;
	}*/
}

void aim65_printer_data_b(UINT8 data) {
/*	data &= 0x03;

	if (flag_b == 0) {
		printerRAM[(printer_y * 20) + printer_x ] |= (data << 8);
		flag_b = 1;
	}*/
}

VIDEO_START( aim65 )
{
	videoram_size = 600 * 10 * 2;
	printerRAM = (UINT16*)auto_malloc (videoram_size );
	memset(printerRAM, 0, videoram_size);
	printer_x = 0;
	printer_y = 0;
	printer_dir = 0;
	flag_a=0;
	flag_b=0;

#if 0
	{
		char backdrop_name[200];
	    /* try to load a backdrop for the machine */
		sprintf(backdrop_name, "%s.png", Machine->gamedrv->name);
		backdrop_load(backdrop_name, 2);
	}
#endif

	return video_start_generic(machine);
}

static const char led[] = {
	"   aaaaaaa    bbbbbb\r"
	"   aaaaaaa    bbbbbb\r"
	"hh         ii        cc\r"
	"hh mm      ii     nn cc\r"
	"hh  mm     ii    nn  cc\r"
	"hh   mm    ii   nn   cc\r"
	"hh    mm   ii   nn   cc\r"
	"hh     mm  ii  nn    cc\r"
	"hh      mm ii nn     cc\r"
	"hh         ii        cc\r"
	"  lllllll    jjjjjj\r"
	"  lllllll    jjjjjj\r"
	"gg         kk        dd\r"
	"gg      pp kk oo     dd\r"
	"gg     pp  kk  oo    dd\r"
	"gg    pp   kk  oo    dd\r"
	"gg   pp    kk    oo  dd\r"
	"gg  pp     kk    oo  dd\r"
	"gg pp      kk     oo dd\r"
	"gg         kk        dd\r"
	"   fffffff    eeeeee\r"
	"   fffffff    eeeeee"
};

static void aim65_draw_7segment(mame_bitmap *bitmap,int value, int x, int y)
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
		case 'i': mask=0x100; break;
		case 'j': mask=0x200; break;

		case 'k': mask=0x400; break;
		case 'l': mask=0x800; break;
		case 'm': mask=0x1000; break;
		case 'n': mask=0x2000; break;
		case 'o': mask=0x4000; break;
		case 'p': mask=0x8000; break;
		}

		if (mask!=0) {
			color=Machine->pens[(value&mask)?1:0];
			*BITMAP_ADDR16(bitmap, y+yi, x+xi) = color;
		}
		if (led[i]!='\r') xi++;
		else { yi++, xi=0; }
	}
}

static const struct {
	int x,y;
} aim65_led_pos[20]={
	{0,0},
	{30,0},
	{60,0},
	{90,0},
	{120,0},
	{150,0},
	{180,0},
	{210,0},
	{240,0},
	{270,0},

	{300,0},
	{330,0},
	{360,0},
	{390,0},
	{420,0},
	{450,0},
	{480,0},
	{510,0},
	{540,0},
	{570,0}
};


VIDEO_UPDATE( aim65 )
{
	int i, j;

/*	int x,y,b,color;
	int data;
	int dir;*/

	for (j=0; j<5; j++) {
		for (i=0; i<4; i++) {
			if (dl1416a[j].cursor_on[i]) {
				aim65_draw_7segment(bitmap, 0xffff,
									aim65_led_pos[j*4+i].x, aim65_led_pos[j*4+i].y);
			} else {
				aim65_draw_7segment(bitmap, dl1416a_segments[dl1416a[j].digit[i]],
									aim65_led_pos[j*4+i].x, aim65_led_pos[j*4+i].y);
			}
		}
	}

	/* Display printer output */
/*	dir = 1;
	for(y = 0; y<500;y++)
	{
		for(x = 0; x< 10; x++)
		{
			if (dir == 1) {
				data = printerRAM[y * 10  + x];
			}
			else {
				data = printerRAM[(y * 10) + (9 - x)];
			}


			for (b = 0; b<10; b++)
			{
				color=Machine->pens[(data & 0x1)?2:0];
				plot_pixel(bitmap,700 - ((b * 10) + x), y,color);
				data = data >> 1;
			}
		}

		if (dir == 0)
			dir = 1;
		else
			dir = 0;
	} */

	return 0;
}

