/***************************************************************************

  vidhrdw/odyssey2.c

***************************************************************************/

#include <assert.h>
#include "driver.h"
#include "cpu/i8039/i8039.h"
#include "video/generic.h"
#include "includes/odyssey2.h"


/* character sprite colors
   dark grey, red, green, orange, blue, violet, light grey, white
   dark back / grid colors
   black, dark blue, dark green, light green, red, violet, orange, light grey
   light back / grid colors
   black, blue, green, light green, red, violet, orange, light grey */

UINT8 odyssey2_colors[] =
{
	/* Background,Grid Dim */
	0x00,0x00,0x00,
	0x00,0x00,0xFF,   /* Blue */
	0x00,0x80,0x00,   /* DK Green */
	0xff,0x9b,0x60,
	0xCC,0x00,0x00,   /* Red */
	0xa9,0x80,0xff,
	0x82,0xfd,0xdb,
	0xFF,0xFF,0xFF,

	/* Background,Grid Bright */

	0x80,0x80,0x80,
	0x50,0xAE,0xFF,   /* Blue */
	0x00,0xFF,0x00,   /* Dk Green */
	0x82,0xfb,0xdb,   /* Lt Grey */
	0xEC,0x02,0x60,   /* Red */
	0xa9,0x80,0xff,   /* Violet */
	0xff,0x9b,0x60,   /* Orange */
	0xFF,0xFF,0xFF,

	/* Character,Sprite colors */
	0x71,0x71,0x71,   /* Dark Grey */
	0xFF,0x80,0x80,   /* Red */
	0x00,0xC0,0x00,   /* Green */
	0xff,0x9b,0x60,   /* Orange */
	0x50,0xAE,0xFF,   /* Blue */
	0xa9,0x80,0xff,   /* Violet */
	0x82,0xfb,0xdb,   /* Lt Grey */
	0xff,0xff,0xff    /* White */
};

UINT8 o2_shape[0x40][8]={
    { 0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00 }, // 0
    { 0x18,0x38,0x18,0x18,0x18,0x18,0x3C,0x00 },
    { 0x3C,0x66,0x0C,0x18,0x30,0x60,0x7E,0x00 },
    { 0x7C,0xC6,0x06,0x3C,0x06,0xC6,0x7C,0x00 },
    { 0xCC,0xCC,0xCC,0xFE,0x0C,0x0C,0x0C,0x00 },
    { 0xFE,0xC0,0xC0,0x7C,0x06,0xC6,0x7C,0x00 },
    { 0x7C,0xC6,0xC0,0xFC,0xC6,0xC6,0x7C,0x00 },
    { 0xFE,0x06,0x0C,0x18,0x30,0x60,0xC0,0x00 },
    { 0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x00 },
    { 0x7C,0xC6,0xC6,0x7E,0x06,0xC6,0x7C,0x00 },
    { 0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00 },
    { 0x18,0x7E,0x58,0x7E,0x58,0x7E,0x18,0x00 },
    { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 },
    { 0x3C,0x66,0x0C,0x18,0x18,0x00,0x18,0x00 },
    { 0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xFE,0x00 },
    { 0xFC,0xC6,0xC6,0xFC,0xC0,0xC0,0xC0,0x00 },
    { 0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00 },
    { 0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00 },
    { 0xFE,0xC0,0xC0,0xF8,0xC0,0xC0,0xFE,0x00 },
    { 0xFC,0xC6,0xC6,0xFC,0xD8,0xCC,0xC6,0x00 },
    { 0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00 },
    { 0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00 },
    { 0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00 },
    { 0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00 },
    { 0x7C,0xC6,0xC6,0xC6,0xDE,0xCC,0x76,0x00 },
    { 0x7C,0xC6,0xC0,0x7C,0x06,0xC6,0x7C,0x00 },
    { 0xFC,0xC6,0xC6,0xC6,0xC6,0xC6,0xFC,0x00 },
    { 0xFE,0xC0,0xC0,0xF8,0xC0,0xC0,0xC0,0x00 },
    { 0x7C,0xC6,0xC0,0xC0,0xCE,0xC6,0x7E,0x00 },
    { 0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00 },
    { 0x06,0x06,0x06,0x06,0x06,0xC6,0x7C,0x00 },
    { 0xC6,0xCC,0xD8,0xF0,0xD8,0xCC,0xC6,0x00 },
    { 0x38,0x6C,0xC6,0xC6,0xFE,0xC6,0xC6,0x00 },
    { 0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00 },
    { 0xC6,0xC6,0x6C,0x38,0x6C,0xC6,0xC6,0x00 },
    { 0x7C,0xC6,0xC0,0xC0,0xC0,0xC6,0x7C,0x00 },
    { 0xC6,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x00 },
    { 0xFC,0xC6,0xC6,0xFC,0xC6,0xC6,0xFC,0x00 },
    { 0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00 },
    { 0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00 },
    { 0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00 },
    { 0x00,0x66,0x3C,0x18,0x3C,0x66,0x00,0x00 },
    { 0x00,0x18,0x00,0x7E,0x00,0x18,0x00,0x00 },
    { 0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00 },
    { 0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00 },
    { 0xC6,0xE6,0xF6,0xFE,0xDE,0xCE,0xC6,0x00 },
    { 0x03,0x06,0x0C,0x18,0x30,0x60,0xC0,0x00 },
    { 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00 },
    { 0xCE,0xDB,0xDB,0xDB,0xDB,0xDB,0xCE,0x00 },
    { 0x00,0x00,0x3C,0x7E,0x7E,0x7E,0x3C,0x00 },
    { 0x1C,0x1C,0x18,0x1E,0x18,0x18,0x1C,0x00 },
    { 0x1C,0x1C,0x18,0x1E,0x18,0x34,0x26,0x00 },
    { 0x38,0x38,0x18,0x78,0x18,0x2C,0x64,0x00 },
    { 0x38,0x38,0x18,0x78,0x18,0x18,0x38,0x00 },
    { 0x00,0x18,0x0C,0xFE,0x0C,0x18,0x00,0x00 },
    { 0x18,0x3C,0x7E,0xFF,0xFF,0x18,0x18,0x00 },
    { 0x03,0x07,0x0F,0x1F,0x3F,0x7F,0xFF,0x00 },
    { 0xC0,0xE0,0xF0,0xF8,0xFC,0xFE,0xFF,0x00 },
    { 0x38,0x38,0x12,0xFE,0xB8,0x28,0x6C,0x00 },
    { 0xC0,0x60,0x30,0x18,0x0C,0x06,0x03,0x00 },
    { 0x00,0x00,0x0C,0x08,0x08,0x7F,0x3E,0x00 },
    { 0x00,0x03,0x63,0xFF,0xFF,0x18,0x08,0x00 },
    { 0x00,0x00,0x00,0x10,0x38,0xFF,0x7E,0x00 }
};


static UINT8 *odyssey2_display;
int odyssey2_vh_hpos;

union {
    UINT8 reg[0x100];
    struct {
	struct {
	    UINT8 y,x,color,res;
	} sprites[4];
	struct {
	    UINT8 y,x,ptr,color;
	} foreground[12];
	struct {
	    struct {
		UINT8 y,x,ptr,color;
	    } single[4];
	} quad[4];
	UINT8 shape[4][8];
	UINT8 control;
	UINT8 status;
	UINT8 collision;
	UINT8 color;
	UINT8 y;
	UINT8 x;
	UINT8 res;
	UINT8 shift1,shift2,shift3;
	UINT8 sound;
	UINT8 res2[5+0x10];
	UINT8 hgrid[2][0x10];
	UINT8 vgrid[0x10];
    } s;
} o2_vdc= { { 0 } };

static UINT8 collision;
static int line;
static double line_time;
static UINT32 o2_snd_shift[2];

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
VIDEO_START( odyssey2 )
{
	o2_snd_shift[0] = Machine->sample_rate / 983;
	o2_snd_shift[1] = Machine->sample_rate / 3933;

	odyssey2_vh_hpos = 0;
	odyssey2_display = (UINT8 *) auto_malloc(8 * 8 * 256);
	memset(odyssey2_display, 0, 8 * 8 * 256);
	return 0;
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/

PALETTE_INIT( odyssey2 )
{
	palette_set_colors(machine, 0, odyssey2_colors, sizeof(odyssey2_colors) / 3);
	colortable[0] = 0;
	colortable[1] = 1;
}

extern  READ8_HANDLER ( odyssey2_video_r )
{
    UINT8 data=0;
    int h;
    switch (offset) {
    case 0xa1: 
	h=(int)((timer_get_time()-line_time)*Machine->drv->cpu[0].cpu_clock);
	if (h>15) data|=1;
	if ((line>240)&&(line<260)) data|=8;
	break;
    case 0xa2: data=collision;break;
    case 0xa4: data=line;break;
    case 0xa5:
	data=(int)((timer_get_time()-line_time)*Machine->drv->cpu[0].cpu_clock)*8;
	break;
    default:
	data=o2_vdc.reg[offset];
    }
    return data;
}

extern WRITE8_HANDLER ( odyssey2_video_w )
{
	/* Update the sound */
	if( offset >= 0xa7 && offset <= 0xaa )
		stream_update( odyssey2_sh_channel );

	o2_vdc.reg[offset]=data;
}

extern  READ8_HANDLER ( odyssey2_t1_r )
{
    static int t=FALSE;
    t=!t;
    return t;
}

INTERRUPT_GEN( odyssey2_line )
{
    line_time = timer_get_time();
    line = (line + 1) % 262;

    switch (line) {
    case 252:
		cpunum_set_input_line(0, 0, ASSERT_LINE); /* vsync?? */
		break;
    case 253:
		cpunum_set_input_line(0, 0, CLEAR_LINE); /* vsync?? */
		break;
    }
}

INLINE void odyssey2_draw_box(UINT8 bg[][320], int x, int y, int width, int height, UINT8 color)
{
    int x1,y1;
    for (y1 = 0; y1 < height; y1++)
	{
		for (x1 = 0; x1 < width; x1++)
			bg[y+y1][x+x1] |= color;
    }
}

INLINE void odyssey2_draw(UINT8 bg[][320], UINT8 code, int x, int y, int scale_x, int scale_y, UINT8 color)
{
    int m,x1,y1;
    for (m=0x80; m>0; m>>=1, x+=scale_x)
	{
		if (code & m)
		{
			for (y1=0; y1<scale_y; y1++)
			{
				for (x1 = 0; x1 < scale_x; x1++)
					bg[y+y1][x+x1] |= color;
			}
		}
    }
}

// different bit ordering, maybe I should change rom
INLINE void odyssey2_draw_sprite(UINT8 bg[][320], UINT8 code, int x, int y, int scale_x, int scale_y, UINT8 color)
{
    int m,x1,y1;
    for (m=1; m<=0x80; m<<=1, x+=scale_x)
	{
		if (code & m)
		{
			for (y1=0; y1<scale_y; y1++)
			{
				for (x1 = 0; x1 < scale_x; x1++)
					bg[y+y1][x+x1] |= color;
			}
		}
    }
}

INLINE void odyssey2_draw_char(mame_bitmap *bitmap, UINT8 bg[][320], int x, int y, int ptr, int color)
{
    int n, i;
    int offset=ptr|((color&1)<<8);
    offset=(offset+(y>>1))&0x1ff;
    Machine->gfx[0]->colortable[1]=Machine->pens[16+((color&0xe)>>1)];

    // don't ask me about the technical background, but also this height thingy is needed
    // invaders aliens (!) and shoot (-)
    n = 8-(ptr&7)-((y>>1)&7);
    if (n<3) n+=7;
    for (i=0; i<n; i++) {
	if (y+i*2>=bitmap->height) break;		
	odyssey2_draw(bg, ((char*)o2_shape)[offset], x, y+i*2, 1, 2, 0x80);
	drawgfxzoom(bitmap, Machine->gfx[0], ((char*)o2_shape)[offset],0,
		    0,0,x,y+i*2,
		    0, TRANSPARENCY_PEN,0, 0x10000, 0x20000);	
	offset=(offset+1)&0x1ff;
    }
}

/***************************************************************************

  Refresh the video screen

***************************************************************************/

VIDEO_UPDATE( odyssey2 )
{
	int i, j, x, y;
	int color;
	UINT8 bg[300][320]= { { 0 } };

	assert(bitmap->width<=ARRAY_LENGTH(bg[0])
		&& bitmap->height<=ARRAY_LENGTH(bg));

	plot_box(bitmap, 0, 0, bitmap->width, bitmap->height, Machine->pens[(o2_vdc.s.color>>3)&0x7]);
	if (o2_vdc.s.control & 8)
	{
		// grid 8 points right compared to characters, sprites
#define WIDTH 16
#define HEIGHT 24
		int w=2;
		color=o2_vdc.s.color&7;
		color|=(o2_vdc.s.color>>3)&8;
		for (i=0, x=0; x<9; x++, i++) {
			for (j=1, y=0; y<9; y++, j<<=1) {
				if ( ((j<=0x80)&&(o2_vdc.s.hgrid[0][i]&j))
						||((j>0x80)&&(o2_vdc.s.hgrid[1][i]&1)) )
				{
					odyssey2_draw_box(bg,8+x*WIDTH,24+y*HEIGHT, WIDTH,3, 0x20);
					plot_box(bitmap,8+x*WIDTH,24+y*HEIGHT,WIDTH,3,Machine->pens[color]);
				}
		    }
		}
		if (o2_vdc.s.control&0x80) w=WIDTH;
		for (i=0, x=0; x<10; x++, i++) {
			for (j=1, y=0; y<8; y++, j<<=1)
			{
				if (o2_vdc.s.vgrid[i]&j) {
				    odyssey2_draw_box(bg,8+x*WIDTH,24+y*HEIGHT,w,HEIGHT, 0x10);
				    plot_box(bitmap,8+x*WIDTH,24+y*HEIGHT,w,HEIGHT,Machine->pens[color]);
				}
			}
		}
    }
    if (o2_vdc.s.control&0x20)
	{
		for (i=0; i<ARRAY_LENGTH(o2_vdc.s.foreground); i++) {
			odyssey2_draw_char(bitmap,bg,
				o2_vdc.s.foreground[i].x, o2_vdc.s.foreground[i].y,
				o2_vdc.s.foreground[i].ptr, o2_vdc.s.foreground[i].color);
		}
		for (i=0; i<ARRAY_LENGTH(o2_vdc.s.quad); i++)
		{
			x=o2_vdc.s.quad[i].single[0].x;
			for (j=0; j<ARRAY_LENGTH(o2_vdc.s.quad[0].single); j++, x+=2*8)
			{
				odyssey2_draw_char(bitmap,bg,
					x, y=o2_vdc.s.quad[i].single[0].y,
					o2_vdc.s.quad[i].single[j].ptr, o2_vdc.s.quad[i].single[j].color);
			}
		}
		for (i=0; i<ARRAY_LENGTH(o2_vdc.s.sprites); i++)
		{
			Machine->gfx[0]->colortable[1]=Machine->pens[16+((o2_vdc.s.sprites[i].color>>3)&7)];
			y=o2_vdc.s.sprites[i].y;
			x=o2_vdc.s.sprites[i].x;
			for (j=0; j<8; j++)
			{
				if (o2_vdc.s.sprites[i].color&4)
				{
					if (y+4*j>=bitmap->height) break;
					odyssey2_draw_sprite(bg, o2_vdc.s.shape[i][j], x, y+j*4, 2, 4, 1<<i);
					drawgfxzoom(bitmap, Machine->gfx[1], o2_vdc.s.shape[i][j],0,
						0,0,x,y+j*4,
						0, TRANSPARENCY_PEN,0,0x20000, 0x40000);
				}
				else
				{
					if (y+j*2>=bitmap->height) break;
					odyssey2_draw_sprite(bg, o2_vdc.s.shape[i][j], x, y+j*2, 1, 2, 1<<i);
					drawgfxzoom(bitmap, Machine->gfx[1], o2_vdc.s.shape[i][j],0,
						0,0,x,y+j*2,
						0, TRANSPARENCY_PEN,0, 0x10000, 0x20000);
				}
			}
		}
	}
	collision=0;
//	for (y=0; y<bitmap->height; y++) {
//		for (x=0; x<bitmap->width; x++) {
	for (y=0; y<300; y++)
	{
		for (x=0; x<320; x++)
		{
			switch (bg[y][x]) {
			case 0: case 1: case 2: case 4: case 8:
			case 0x10: case 0x20: case 0x80:
				break;

			default:
				if (bg[y][x]&o2_vdc.s.collision)
				{
				    collision|=bg[y][x]&(~o2_vdc.s.collision);
				}
				break;
			}
		}
	}
	return 0;
}

void odyssey2_sh_update( void *param,stream_sample_t **inputs, stream_sample_t **_buffer,int length )
{
	static UINT32 signal;
	static UINT16 count = 0;
	int ii;
	int period;
	stream_sample_t *buffer = _buffer[0];

	/* Generate the signal */
	signal = o2_vdc.s.shift3 | (o2_vdc.s.shift2 << 8) | (o2_vdc.s.shift1 << 16);

	if( o2_vdc.s.sound & 0x80 )	/* Sound is enabled */
	{
		for( ii = 0; ii < length; ii++, buffer++ )
		{
			*buffer = 0;
			*buffer = signal & 0x1;
			period = (o2_vdc.s.sound & 0x20) ? 11 : 44;
			if( ++count >= period )
			{
				count = 0;
				signal >>= 1;
				/* Loop sound */
				if( o2_vdc.s.sound & 0x40 )
				{
					signal |= *buffer << 23;
				}
			}

			/* Throw an interrupt if enabled */
			if( o2_vdc.s.control & 0x4 )
			{
				cpunum_set_input_line(0, 1, HOLD_LINE); /* Is this right? */
			}

			/* Adjust volume */
			*buffer *= o2_vdc.s.sound & 0xf;
			/* Pump the volume up */
			*buffer <<= 10;
		}
	}
	else
	{
		/* Sound disabled, so clear the buffer */
		for( ii = 0; ii < length; ii++, buffer++ )
			*buffer = 0;
	}
}
