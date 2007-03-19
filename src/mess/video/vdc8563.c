/***************************************************************************

  CBM Video Device Chip 8563

  copyright 2000 peter.trauner@jk.uni-linz.ac.at

***************************************************************************/
/*
 several graphic problems
 some are in the rastering engine and should be solved during its evalution
 rare and short documentation,
 registers and some words of description in the c128 user guide */

#include <math.h>
#include <stdio.h>
#include <assert.h>
#include "driver.h"
#include "mscommon.h"

#include "includes/crtc6845.h" // include only several register defines
#include "includes/vdc8563.h"

#define VERBOSE 0
#define VERBOSE_DBG	0

#if VERBOSE
#define DBG_LOG(N,M,A)      \
    if(VERBOSE>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M );
 logerror A; }
#else
#define DBG_LOG(N,M,A)
#endif

/* seems to be a motorola m6845 variant */

/* permutation for c64/vic6567 conversion to vdc8563
 0 --> 0 black
 1 --> 0xf white
 2 --> 8 red
 3 --> 7 cyan
 4 --> 0xb violett
 5 --> 4 green
 6 --> 2 blue
 7 --> 0xd yellow
 8 --> 0xa orange
 9 --> 0xc brown
 0xa --> 9 light red
 0xb --> 6 dark gray
 0xc --> 1 gray
 0xd --> 5 light green
 0xe --> 3 light blue
 0xf --> 0xf light gray
 */

/* x128
 commodore assignment!?
 black gray orange yellow dardgrey vio red lgreen
 lred lgray brown blue white green cyan lblue
*/
unsigned char vdc8563_palette[] =
{
#if 0
	0x00, 0x00, 0x00, /* black */
	0x70, 0x74, 0x6f, /* gray */
	0x21, 0x1b, 0xae, /* blue */
	0x5f, 0x53, 0xfe, /* light blue */
	0x1f, 0xd2, 0x1e, /* green */
	0x59, 0xfe, 0x59, /* light green */
	0x42, 0x45, 0x40, /* dark gray */
	0x30, 0xe6, 0xc6, /* cyan */
	0xbe, 0x1a, 0x24, /* red */
	0xfe, 0x4a, 0x57, /* light red */
	0xb8, 0x41, 0x04, /* orange */
	0xb4, 0x1a, 0xe2, /* purple */
	0x6a, 0x33, 0x04, /* brown */
	0xdf, 0xf6, 0x0a, /* yellow */
	0xa4, 0xa7, 0xa2, /* light gray */
	0xfd, 0xfe, 0xfc /* white */
#else
	/* vice */
	0,0,0, /* black */
	0x20,0x20,0x20, /* gray */
	0,0,0x80, /* blue */
	0,0,0xff, /* light blue */
	0,0x80,0, /* green */
	0,0xff,0, /* light green */
	0,0x80,0x80, /* cyan */
	0,0xff,0xff, /* light cyan */
	0x80,0,0, /* red */
	0xff,0,0, /* light red */
	0x80,0,0x80, /* purble */
	0xff,0,0xff, /* light purble */
	0x80,0x80,0, /* brown */
	0xff, 0xff, 0, /* yellow */
	0xc0,0xc0,0xc0, /* light gray */
	0xff, 0xff,0xff /* white */
#endif
};


static struct {
	int state;
	UINT8 reg[37];
	UINT8 index;

	UINT16 addr, src;

	UINT16 videoram_start, colorram_start, fontram_start;
	UINT16 videoram_size;

	int rastering;

	UINT8 *ram;
	UINT8 *dirty;
	UINT8 fontdirty[0x200];
	UINT16 mask, fontmask;

	double cursor_time;
	int	cursor_on;

	int changed;
} vdc;

static const struct {
	int stored,
		read;
} reg_mask[]= {
	{ 0xff, 0 },
	{ 0xff, 0 },
	{ 0xff, 0 },
	{ 0xff, 0 },
	{ 0xff, 0 },
	{ 0x1f, 0 },
	{ 0xff, 0 },
	{ 0xff, 0 },
	{  0x3, 0 }, //8
	{  0x1f, 0 },
	{  0x7f, 0 },
	{  0x1f, 0 },
	{  0xff, 0xff },
	{  0xff, 0xff },
	{  0xff, 0xff },
	{  0xff, 0xff },
	{  -1, 0xff }, //0x10
	{  -1, 0xff },
	{  0xff, 0xff },
	{  0xff, 0xff },
	{  0xff, -1 },
	{  0x1f, -1 },
	{  0xff, -1 },
	{  0xff, -1 },
	{  0xff, -1 },//0x18
	{  0xff, -1 },
	{  0xff, -1 },
	{  0xff, -1 },
	{  0xf0, -1 },
	{  0x1f, -1 },
	{  0xff, -1 },
	{  0xff, -1 },
	{  0xff, -1 }, //0x20
	{  0xff, -1 },
	{  0xff, -1 },
	{  0xff, -1 },
	{  0x0f, -1 },
};
#define REG(x) (vdc.reg[x]&reg_mask[x].stored)

void vdc8563_init (int ram16konly)
{
	memset(&vdc, 0, sizeof(vdc));
	vdc.cursor_time=0.0;
	vdc.state=1;

	if (ram16konly) {
		vdc.mask=0x3fff;
		vdc.fontmask=0x2000;
	} else {
		vdc.mask=0xffff;
		vdc.fontmask=0xe000;
	}

	vdc.rastering=1;
}

static void vdc_videoram_w(int offset, int data)
{
	offset&=vdc.mask;
    if (vdc.ram[offset]!=data) {
		vdc.ram[offset] = data;
		vdc.dirty[offset] = 1;
		if ((vdc.fontram_start&vdc.fontmask)==(offset&vdc.fontmask))
			vdc.fontdirty[(offset&0x1ff0)>>4]=1;
	}
}

INLINE int vdc_videoram_r(int offset)
{
	return vdc.ram[offset&vdc.mask];
}

void vdc8563_set_rastering(int on)
{
	vdc.rastering=on;
	vdc.changed|=1;
}

VIDEO_START( vdc8563 )
{
	vdc.ram=(UINT8*) auto_malloc(0x20000);
	vdc.dirty=vdc.ram+0x10000;

	return (!vdc.ram);
}

#define CHAR_WIDTH (((vdc.reg[0x16]&0xf0)>>4)+1)
#define CHAR_WIDTH_VISIBLE ((vdc.reg[0x16]&0xf)+1)

#define BLOCK_COPY (vdc.reg[0x18]&0x80)

#define MONOTEXT ((vdc.reg[0x19]&0xc0)==0)
#define TEXT ((vdc.reg[0x19]&0xc0)==0x40)
#define GRAPHIC (vdc.reg[0x19]&0x80)

#define FRAMECOLOR (vdc.reg[0x1a]&0xf)
#define MONOCOLOR (vdc.reg[0x1a]>>4)

#define LINEDIFF (vdc.reg[0x1b])
#define FONT_START ((vdc.reg[0x1c]&0xe0)<<8)
/* 0x1c 0x10 dram 0:4416, 1: 4164 */

/* 0x1d 0x1f counter for underlining */

#define FILLBYTE vdc.reg[0x1f]

#define CLOCK_HALFING (vdc.reg[25]&0x10)

/* 0x22 number of chars from start of line to positiv edge of display enable */
/* 0x23 number of chars from start of line to negativ edge of display enable */
/* 0x24 0xf number of refresh cycles per line */

WRITE8_HANDLER ( vdc8563_port_w )
{
	UINT8 i;

	if (offset & 1)
	{
		if ((vdc.index & 0x3f) < 37)
		{
			switch (vdc.index & 0x3f)
			{
			case 1: case 4: case 0x1b:
				vdc.reg[vdc.index]=data;
				vdc.videoram_size=CRTC6845_CHAR_LINES*(CRTC6845_CHAR_COLUMNS+LINEDIFF);
				vdc.changed=1;
				break;
			case 0xe: case 0xf: case 0xa: case 0xb:
				vdc.dirty[CRTC6845_CURSOR_POS&vdc.mask]=1;
				vdc.reg[vdc.index]=data;
				break;
			case 0xc: case 0xd:
				vdc.reg[vdc.index]=data;
				vdc.videoram_start=CRTC6845_VIDEO_START;
				vdc.changed=1;
				break;
			case 0x12:
				vdc.addr=(vdc.addr&0xff)|(data<<8);
				break;
			case 0x13:
				vdc.addr=(vdc.addr&0xff00)|data;
				break;
			case 0x20:
				vdc.src=(vdc.src&0xff)|(data<<8);
				break;
			case 0x21:
				vdc.src=(vdc.src&0xff00)|data;
				break;
			case 0x14: case 0x15:
				vdc.reg[vdc.index]=data;
				vdc.colorram_start=(vdc.reg[0x14]<<8)|vdc.reg[0x15];
				vdc.changed=1;
				break;
			case 0x1c:
				vdc.reg[vdc.index]=data;
				vdc.fontram_start=FONT_START;
				vdc.changed=1;
				break;
			case 0x16: case 0x19: case 0x1a:
				vdc.reg[vdc.index]=data;
				vdc.changed=1;
				break;
			case 0x1e:
				vdc.reg[vdc.index]=data;
				if (BLOCK_COPY) {
					DBG_LOG (2, "vdc block copy",
							 (errorlog, "src:%.4x dst:%.4x size:%.2x\n",
							  vdc.src, vdc.addr, data));
					i=data;do {
						vdc_videoram_w(vdc.addr++, vdc_videoram_r(vdc.src++));
					} while (--i!=0);
				} else {
					DBG_LOG (2, "vdc block set",
							 (errorlog, "dest:%.4x value:%.2x size:%.2x\n",
							  vdc.addr, FILLBYTE, data));
					i=data;do {
						vdc_videoram_w(vdc.addr++, FILLBYTE);
					} while (--i!=0);
				}
				break;
			case 0x1f:
				DBG_LOG (2, "vdc written",
						 (errorlog, "dest:%.4x size:%.2x\n",
						  vdc.addr, data));
				vdc.reg[vdc.index]=data;
				vdc_videoram_w(vdc.addr++, data);
				break;
			default:
				vdc.reg[vdc.index]=data;
				DBG_LOG (2, "vdc8563_port_w",
						 (errorlog, "%.2x:%.2x\n", vdc.index, data));
				break;
			}
		}
		DBG_LOG (3, "vdc8563_port_w",
				 (errorlog, "%.2x:%.2x\n", vdc.index, data));
	}
	else
	{
		vdc.index = data;
	}
}

 READ8_HANDLER ( vdc8563_port_r )
{
	int val;

	val = 0xff;
	if (offset & 1)
	{
		if ((vdc.index & 0x3f) < 37)
		{
			switch (vdc.index & 0x3f)
			{
			case 0x12:
				val=vdc.addr>>8;
				break;
			case 0x13:
				val=vdc.addr&0xff;
				break;
			case 0x1e:
				val=0;
				break;
			case 0x1f:
				val=vdc_videoram_r(vdc.addr);
				DBG_LOG (2, "vdc read", (errorlog, "%.4x %.2x\n", vdc.addr, val));
				break;
			case 0x20:
				val=vdc.src>>8;
				break;
			case 0x21:
				val=vdc.src&0xff;
				break;
			default:
				val=vdc.reg[vdc.index&0x3f]&reg_mask[vdc.index&0x3f].read;
			}
		}
		DBG_LOG (2, "vdc8563_port_r", (errorlog, "%.2x:%.2x\n", vdc.index, val));
	}
	else
	{
		val = vdc.index;
		if (vdc.state) val|=0x80;
	}
	return val;
}

static int vdc8563_clocks_in_frame(void)
{
	int clocks=CRTC6845_COLUMNS*CRTC6845_LINES;
	switch (CRTC6845_INTERLACE_MODE) {
	case CRTC6845_INTERLACE_SIGNAL: // interlace generation of video signals only
	case CRTC6845_INTERLACE: // interlace
		return clocks/2;
	default:
		return clocks;
	}
}

static void vdc8563_time(void)
{
	double neu, ftime;
	neu=timer_get_time();

	if (vdc8563_clocks_in_frame()==0.0) return;
	ftime=16*vdc8563_clocks_in_frame()/2000000.0;
	if (CLOCK_HALFING) ftime*=2;
	switch (CRTC6845_CURSOR_MODE) {
	case CRTC6845_CURSOR_OFF: vdc.cursor_on=0;break;
	case CRTC6845_CURSOR_32FRAMES:
		ftime*=2;
	case CRTC6845_CURSOR_16FRAMES:
		if (neu-vdc.cursor_time>ftime) {
			vdc.cursor_time+=ftime;
			vdc.dirty[CRTC6845_CURSOR_POS&vdc.mask]=1;
			vdc.cursor_on^=1;
		}
		break;
	default:
		vdc.cursor_on=1;break;
	}
}

static void vdc8563_monotext_screenrefresh (mame_bitmap *bitmap, int full_refresh)
{
	int x, y, i;
	rectangle rect;
	int w=CRTC6845_CHAR_COLUMNS;
	int h=CRTC6845_CHAR_LINES;
	int height=CRTC6845_CHAR_HEIGHT;

	rect.min_x=Machine->screen[0].visarea.min_x;
	rect.max_x=Machine->screen[0].visarea.max_x;
	if (full_refresh) {
		memset(vdc.dirty+vdc.videoram_start, 1, vdc.videoram_size);
	}

	for (y=0, rect.min_y=height, rect.max_y=rect.min_y+height-1,
			 i=vdc.videoram_start&vdc.mask; y<h;
		 y++, rect.min_y+=height, rect.max_y+=height) {
		for (x=0; x<w; x++, i=(i+1)&vdc.mask) {
			if (vdc.dirty[i]) {
				drawgfx(bitmap,Machine->gfx[0],
						vdc.ram[i], FRAMECOLOR|(MONOCOLOR<<4), 0, 0,
						Machine->gfx[0]->width*x+8,height*y+height,
						&rect,TRANSPARENCY_NONE,0);
				if ((vdc.cursor_on)&&(i==(CRTC6845_CURSOR_POS&vdc.mask))) {
					int k=height-CRTC6845_CURSOR_TOP;
					if (CRTC6845_CURSOR_BOTTOM<height) k=CRTC6845_CURSOR_BOTTOM-CRTC6845_CURSOR_TOP+1;

					if (k>0)
						plot_box(bitmap, Machine->gfx[0]->width*x+8,
								 height*y+height+CRTC6845_CURSOR_TOP,
								 Machine->gfx[0]->width, k, Machine->pens[FRAMECOLOR]);
				}

				vdc.dirty[i]=0;
			}
		}
		i+=LINEDIFF;
	}
}

static void vdc8563_text_screenrefresh (mame_bitmap *bitmap, int full_refresh)
{
	int x, y, i, j;
	rectangle rect;
	int w=CRTC6845_CHAR_COLUMNS;
	int h=CRTC6845_CHAR_LINES;
	int height=CRTC6845_CHAR_HEIGHT;

	rect.min_x=Machine->screen[0].visarea.min_x;
	rect.max_x=Machine->screen[0].visarea.max_x;
	if (full_refresh) {
		memset(vdc.dirty+vdc.videoram_start, 1, vdc.videoram_size);
	}

	for (y=0, rect.min_y=height, rect.max_y=rect.min_y+height-1,
			 i=vdc.videoram_start&vdc.mask, j=vdc.colorram_start&vdc.mask; y<h;
		 y++, rect.min_y+=height, rect.max_y+=height) {
		for (x=0; x<w; x++, i=(i+1)&vdc.mask, j=(j+1)&vdc.mask) {
			if (vdc.dirty[i]||vdc.dirty[j])
			{
				{
					UINT16 ch, fg, bg;
					const UINT8 *charptr;
					int v, h;
					UINT16 *pixel;

					ch = vdc.ram[i] | ((vdc.ram[j] & 0x80) ? 0x100 : 0);
					charptr = &vdc.ram[(vdc.fontram_start + (ch * 16)) & vdc.mask];
					fg = ((vdc.ram[j] & 0x0F) >> 0) + 0x10;
					bg = ((vdc.ram[j] & 0x70) >> 4) + 0x10;

					for (v = 0; v < 16; v++)
					{
						for (h = 0; h < 8; h++)
						{
							pixel = BITMAP_ADDR16(bitmap, (y * height) + height + v, (x * 8) + 8 + h);
							*pixel = (charptr[v] & (0x80 >> h)) ? fg : bg;
						}
					}
				}

				if ((vdc.cursor_on)&&(i==(CRTC6845_CURSOR_POS&vdc.mask))) {
					int k=height-CRTC6845_CURSOR_TOP;
					if (CRTC6845_CURSOR_BOTTOM<height) k=CRTC6845_CURSOR_BOTTOM-CRTC6845_CURSOR_TOP+1;

					if (k>0)
						plot_box(bitmap, Machine->gfx[0]->width*x+8,
								 height*y+height+CRTC6845_CURSOR_TOP,
								 Machine->gfx[0]->width, k, Machine->pens[0x10|(vdc.ram[j]&0xf)]);
				}

				vdc.dirty[i]=0;
				vdc.dirty[j]=0;
			}
		}
		i+=LINEDIFF;
		j+=LINEDIFF;
	}
}

static void vdc8563_graphic_screenrefresh (mame_bitmap *bitmap, int full_refresh)
{
	int x, y, i, j, k;
	rectangle rect;
	int w=CRTC6845_CHAR_COLUMNS;
	int h=CRTC6845_CHAR_LINES;
	int height=CRTC6845_CHAR_HEIGHT;

	rect.min_x=Machine->screen[0].visarea.min_x;
	rect.max_x=Machine->screen[0].visarea.max_x;
	if (full_refresh) {
		memset(vdc.dirty, 1, vdc.mask+1);
	}

	for (y=0, rect.min_y=height, rect.max_y=rect.min_y+height-1,
			 i=vdc.videoram_start&vdc.mask; y<h;
		 y++, rect.min_y+=height, rect.max_y+=height) {
		for (x=0; x<w; x++, i=(i+1)&vdc.mask) {
			for (j=0; j<height; j++) {
				k=((i<<4)+j)&vdc.mask;
				if (vdc.dirty[k]) {
					drawgfx(bitmap,Machine->gfx[1],
							vdc.ram[k], FRAMECOLOR|(MONOCOLOR<<4), 0, 0,
							Machine->gfx[0]->width*x+8,height*y+height+j,
							&rect,TRANSPARENCY_NONE,0);
					vdc.dirty[k]=0;
				}
			}
		}
		i+=LINEDIFF;
	}
}

VIDEO_UPDATE( vdc8563 )
{
	int i;
	int full_refresh = 1;

	if (!vdc.rastering) return 0;
	vdc8563_time();

	full_refresh|=vdc.changed;
	if (GRAPHIC) { vdc8563_graphic_screenrefresh(bitmap, full_refresh);
	} else {
		for (i=0; i<512; i++) {
			if (full_refresh||vdc.fontdirty[i]) {
				decodechar(Machine->gfx[0],i,vdc.ram+(vdc.fontram_start&vdc.mask),
						   Machine->drv->gfxdecodeinfo[0].gfxlayout);
				vdc.fontdirty[i]=0;
			}
		}
		if (TEXT) vdc8563_text_screenrefresh(bitmap, full_refresh);
		else vdc8563_monotext_screenrefresh(bitmap, full_refresh);
	}

	if (full_refresh) {
		int w=CRTC6845_CHAR_COLUMNS;
		int h=CRTC6845_CHAR_LINES;
		int height=CRTC6845_CHAR_HEIGHT;

		plot_box(bitmap, 0, 0, Machine->gfx[0]->width*(w+2), height, Machine->pens[FRAMECOLOR]);

		plot_box(bitmap, 0, height, Machine->gfx[0]->width, height*h, Machine->pens[FRAMECOLOR]);

		plot_box(bitmap, Machine->gfx[0]->width*(w+1), height, Machine->gfx[0]->width, height*h,
				 Machine->pens[FRAMECOLOR]);

		plot_box(bitmap, 0, height*(h+1), Machine->gfx[0]->width*(w+2), height,
				 Machine->pens[FRAMECOLOR]);
	}

	vdc.changed=0;
	return 0;
}
