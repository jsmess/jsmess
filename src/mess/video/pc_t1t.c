/***************************************************************************

	IBM PC junior
	Tandy 1000 Graphics Adapter (T1T) section

	Note that in the IBM PC Junior world, the term 'vga' is not the 'vga' that
	most people think of

***************************************************************************/

#include "driver.h"
#include "video/generic.h"

#include "includes/crtc6845.h"
#include "video/pc_cga.h" // cga monitor palette
#include "video/pc_aga.h" //europc charset
#include "video/pc_t1t.h"
#include "mscommon.h"

#define VERBOSE_T1T 1		/* T1T (Tandy 1000 Graphics Adapter) */

#if VERBOSE_T1T
#define T1T_LOG(N,M,A) \
	if(VERBOSE_T1T>=N){ if( M )logerror("%11.6f: %-24s",timer_get_time(),(char*)M ); logerror A; }
#else
#define T1T_LOG(n,m,a)
#endif

/***************************************************************************

	Static declarations

***************************************************************************/

static PALETTE_INIT( pcjr );
static VIDEO_START( pc_t1t );
static pc_video_update_proc pc_t1t_choosevideomode(int *width, int *height, struct crtc6845 *crtc);

/***************************************************************************

	MachineDriver stuff

**************************************************************************/

static unsigned short pcjr_colortable[] =
{
     0, 0, 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 7, 0, 8, 0, 9, 0,10, 0,11, 0,12, 0,13, 0,14, 0,15,
     1, 0, 1, 1, 1, 2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 1, 8, 1, 9, 1,10, 1,11, 1,12, 1,13, 1,14, 1,15,
     2, 0, 2, 1, 2, 2, 2, 3, 2, 4, 2, 5, 2, 6, 2, 7, 2, 8, 2, 9, 2,10, 2,11, 2,12, 2,13, 2,14, 2,15,
     3, 0, 3, 1, 3, 2, 3, 3, 3, 4, 3, 5, 3, 6, 3, 7, 3, 8, 3, 9, 3,10, 3,11, 3,12, 3,13, 3,14, 3,15,
     4, 0, 4, 1, 4, 2, 4, 3, 4, 4, 4, 5, 4, 6, 4, 7, 4, 8, 4, 9, 4,10, 4,11, 4,12, 4,13, 4,14, 4,15,
     5, 0, 5, 1, 5, 2, 5, 3, 5, 4, 5, 5, 5, 6, 5, 7, 5, 8, 5, 9, 5,10, 5,11, 5,12, 5,13, 5,14, 5,15,
     6, 0, 6, 1, 6, 2, 6, 3, 6, 4, 6, 5, 6, 6, 6, 7, 6, 8, 6, 9, 6,10, 6,11, 6,12, 6,13, 6,14, 6,15,
     7, 0, 7, 1, 7, 2, 7, 3, 7, 4, 7, 5, 7, 6, 7, 7, 7, 8, 7, 9, 7,10, 7,11, 7,12, 7,13, 7,14, 7,15,
/* flashing is done by dirtying the videoram buffer positions with attr bit #7 set */
     8, 0, 8, 1, 8, 2, 8, 3, 8, 4, 8, 5, 8, 6, 8, 7, 8, 8, 8, 9, 8,10, 8,11, 8,12, 8,13, 8,14, 8,15,
     9, 0, 9, 1, 9, 2, 9, 3, 9, 4, 9, 5, 9, 6, 9, 7, 9, 8, 9, 9, 9,10, 9,11, 9,12, 9,13, 9,14, 9,15,
    10, 0,10, 1,10, 2,10, 3,10, 4,10, 5,10, 6,10, 7,10, 8,10, 9,10,10,10,11,10,12,10,13,10,14,10,15,
    11, 0,11, 1,11, 2,11, 3,11, 4,11, 5,11, 6,11, 7,11, 8,11, 9,11,10,11,11,11,12,11,13,11,14,11,15,
    12, 0,12, 1,12, 2,12, 3,12, 4,12, 5,12, 6,12, 7,12, 8,12, 9,12,10,12,11,12,12,12,13,12,14,12,15,
    13, 0,13, 1,13, 2,13, 3,13, 4,13, 5,13, 6,13, 7,13, 8,13, 9,13,10,13,11,13,12,13,13,13,14,13,15,
    14, 0,14, 1,14, 2,14, 3,14, 4,14, 5,14, 6,14, 7,14, 8,14, 9,14,10,14,11,14,12,14,13,14,14,14,15,
    15, 0,15, 1,15, 2,15, 3,15, 4,15, 5,15, 6,15, 7,15, 8,15, 9,15,10,15,11,15,12,15,13,15,14,15,15,
/* the color sets for 1bpp graphics mode */
	 0,0, 0,1, 0,2, 0,3, 0,4, 0,5, 0,6, 0,7,
	 0,8, 0,9, 0,10, 0,11, 0,12, 0,13, 0,14, 0,15,
/* the color sets for 2bpp graphics mode */
     /*0, 2, 4, 6,*/  0,10,12,14,
     /*0, 3, 5, 7,*/  0,11,13,15, // only 2 sets!?
/* color sets for 4bpp graphics mode */
	 0,1,2,3, 4,5,6,7, 8,9,0xa,0xb, 0xc,0xd,0xe,0xf
};

static gfx_layout t1t_gfxlayout_4bpp =
{
	2,1,					/* 8 x 32 graphics */
    256,                    /* 256 codes */
	4,						/* 4 bit per pixel */
	{ 0,1,2,3 },			/* adjacent bit planes */
    /* x offsets */
	{ 0,4 },
    /* y offsets (we only use one byte to build the block) */
    { 0 },
	1*8 					/* every code takes 1 byte */
};

static gfx_layout t1t_charlayout =
{
	8,8,					/* 8 x 8 characters */
    128,                    /* 128 characters */
    1,                      /* 1 bits per pixel */
    { 0 },                  /* no bitplanes; 1 bit per pixel */
    /* x offsets */
    { 0,1,2,3,4,5,6,7 },
    /* y offsets */
	{ 0*8,1*8,2*8,3*8,
	  4*8,5*8,6*8,7*8 },
    8*8                     /* every char takes 8 bytes */
};

static gfx_decode t1000hx_gfxdecodeinfo[] =
{
	{ REGION_CPU1, 0xffa6e, &t1t_charlayout,			0,				128 },	/* single width */
	{ REGION_CPU1, 0xfc0a8, &t1t_charlayout,			0,				128 },	/* single width */
	{ REGION_GFX1,  0x1000, &t1t_gfxlayout_4bpp,		256*2+16*2+2*4,	16 },	/* 160x200 4bpp gfx */
    { -1 } /* end of array */
};

static gfx_decode t1000sx_gfxdecodeinfo[] =
{
	{ REGION_CPU1, 0xffa6e, &t1t_charlayout,			0,				128 },	/* single width */
	{ REGION_CPU1, 0xf40a3, &t1t_charlayout,			0,				128 },	/* single width */
	{ REGION_GFX1,  0x1000, &t1t_gfxlayout_4bpp,		256*2+16*2+2*4,	16 },	/* 160x200 4bpp gfx */
    { -1 } /* end of array */
};

MACHINE_DRIVER_START( pcvideo_t1000hx )
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(80*8, 25*9)
	MDRV_SCREEN_VISIBLE_AREA(0,80*8-1, 0,25*9-1)
	MDRV_GFXDECODE(t1000hx_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(sizeof(cga_palette) / sizeof(cga_palette[0]))
	MDRV_COLORTABLE_LENGTH(sizeof(pcjr_colortable) / sizeof(pcjr_colortable[0]))
	MDRV_PALETTE_INIT(pcjr)

	MDRV_VIDEO_START(pc_t1t)
	MDRV_VIDEO_UPDATE(pc_video)
MACHINE_DRIVER_END

MACHINE_DRIVER_START( pcvideo_t1000sx )
	MDRV_IMPORT_FROM( pcvideo_t1000hx )
	MDRV_GFXDECODE( t1000sx_gfxdecodeinfo )
MACHINE_DRIVER_END

/***************************************************************************

	Methods

***************************************************************************/

/* Initialise the cga palette */
static PALETTE_INIT( pcjr )
{
	int i;
	for(i = 0; i < (sizeof(cga_palette) / 3); i++)
		palette_set_color(machine, i, cga_palette[i][0], cga_palette[i][1], cga_palette[i][2]);
	memcpy(colortable, pcjr_colortable, sizeof(pcjr_colortable));
}

static struct { 
	UINT8 mode_control, color_select;
	UINT8 status;

	int full_refresh;
	
	// used in tandy1000hx; used in pcjr???
	struct {
		UINT8 index;
		UINT8 data[0x20];
		/* see vgadoc
		   0 mode control 1
		   1 palette mask
		   2 border color
		   3 mode control 2
		   4 reset
		   0x10-0x1f palette registers 
		*/
	} reg;

	UINT8 bank;

	int pc_blink;
	int pc_framecnt;

	UINT8 *displayram;
} pcjr = { 0 };

/* crtc address line allow only decoding of 8kbyte memory
   so are in graphics mode 2 or 4 of such banks distinquished
   by line */

void pc_t1t_reset(void)
{
	videoram=NULL;
	pcjr.bank=0;
}


/***************************************************************************
  Mark all text positions with attribute bit 7 set dirty
 ***************************************************************************/
static void pc_t1t_blink_textcolors(int on)
{
	int i, offs, size;

	if (!videoram) return;
	if (pcjr.pc_blink == on) return;

    pcjr.pc_blink = on;
	offs = (crtc6845_get_start(crtc6845)* 2) % videoram_size;
	size = crtc6845_get_char_columns(crtc6845) * crtc6845_get_char_lines(crtc6845);

	if (dirtybuffer)
	{
		for (i = 0; i < size; i++)
		{
			if (videoram[offs+1] & 0x80)
				dirtybuffer[offs+1] = 1;
			if( (offs += 2) == videoram_size )
				offs = 0;
		}
	}
}

void pc_t1t_timer(void)
{
	if( ((++pcjr.pc_framecnt & 63) == 63) )
	{
		pc_t1t_blink_textcolors(pcjr.pc_framecnt&64);
	}
}

static void pc_t1t_cursor(struct crtc6845_cursor *cursor)
{
	if (dirtybuffer)
		dirtybuffer[cursor->pos*2] = 1;
}

static struct crtc6845_config config= { 14318180 /*?*/, pc_t1t_cursor };


static VIDEO_START( pc_t1t )
{
	return pc_video_start(&config, pc_t1t_choosevideomode, 0x8000) ? INIT_PASS : INIT_FAIL;
}

 READ8_HANDLER ( pc_t1t_videoram_r )
{
	int data = 0xff;
	if( videoram )
		data = videoram[offset];
	return data;
}

/*
 * 3d8 rW	T1T mode control register (see #P138)
 */
static void pc_t1t_mode_control_w(int data)
{
	T1T_LOG(1,"T1T_mode_control_w",("$%02x: colums %d, gfx %d, hires %d, blink %d\n", \
		data, (data&1)?80:40, (data>>1)&1, (data>>4)&1, (data>>5)&1));
	if( (pcjr.mode_control ^ data) & 0x3b )   /* text/gfx/width change */
		pcjr.full_refresh=1;
	pcjr.mode_control = data;
}

static int pc_t1t_mode_control_r(void)
{
    int data = pcjr.mode_control;
    return data;
}

/*
 * 3d9 ?W	color select register on color adapter
 */
static void pc_t1t_color_select_w(int data)
{
	T1T_LOG(1,"T1T_color_select_w",("$%02x\n", data));
	if (pcjr.color_select == data)
		return;
	pcjr.color_select = data;
	pcjr.full_refresh=1;
}

static int pc_t1t_color_select_r(void)
{
	int data = pcjr.color_select;
	T1T_LOG(1,"T1T_color_select_r",("$%02x\n", data));
    return data;
}

/*  Bitfields for T1T status register:
 *  Bit(s)  Description (Table P179)
 *  7-6 not used
 *  5-4 color EGA, color ET4000: diagnose video display feedback, select
 *      from color plane enable
 *  3   in vertical retrace
 *  2   (CGA,color EGA) light pen switch is off
 *      (MCGA,color ET4000) reserved (0)
 *  1   (CGA,color EGA) positive edge from light pen has set trigger
 *      (MCGA,color ET4000) reserved (0)
 *  0   horizontal retrace in progress
 *      =0  do not use memory
 *      =1  memory access without interfering with display
 *      (Genoa SuperEGA) horizontal or vertical retrace
 */
static int pc_t1t_status_r(void)
{
    int data = (input_port_0_r(0) & 0x08) | pcjr.status;
    pcjr.status ^= 0x01;
    return data;
}

/*
 * 3db -W	light pen strobe reset (on any value)
 */
static void pc_t1t_lightpen_strobe_w(int data)
{
	T1T_LOG(1,"T1T_lightpen_strobe_w",("$%02x\n", data));
//	pc_port[0x3db] = data;
}


/*
 * 3da -W	(mono EGA/mono VGA) feature control register
 *			(see PORT 03DAh-W for details; VGA, see PORT 03CAh-R)
 */
static void pc_t1t_vga_index_w(int data)
{
	T1T_LOG(1,"T1T_vga_index_w",("$%02x\n", data));
	pcjr.reg.index = data;
}

static void pc_t1t_vga_data_w(int data)
{
    pcjr.reg.data[pcjr.reg.index] = data;

	switch (pcjr.reg.index)
	{
        case 0x00: /* mode control 1 */
            T1T_LOG(1,"T1T_vga_mode_ctrl_1_w",("$%02x\n", data));
            break;
        case 0x01: /* palette mask (bits 3-0) */
            T1T_LOG(1,"T1T_vga_palette_mask_w",("$%02x\n", data));
            break;
        case 0x02: /* border color (bits 3-0) */
            T1T_LOG(1,"T1T_vga_border_color_w",("$%02x\n", data));
            break;
        case 0x03: /* mode control 2 */
            T1T_LOG(1,"T1T_vga_mode_ctrl_2_w",("$%02x\n", data));
            break;
        case 0x04: /* reset register */
            T1T_LOG(1,"T1T_vga_reset_w",("$%02x\n", data));
            break;
        /* palette array */
        case 0x10: case 0x11: case 0x12: case 0x13:
        case 0x14: case 0x15: case 0x16: case 0x17:
        case 0x18: case 0x19: case 0x1a: case 0x1b:
        case 0x1c: case 0x1d: case 0x1e: case 0x1f:
			T1T_LOG(1,"T1T_vga_palette_w",("[$%02x] $%02x\n", pcjr.reg.index - 0x10, data));
			palette_set_color(Machine, pcjr.reg.index-0x10, 
								 cga_palette[data&0xf][0],
								 cga_palette[data&0xf][1],
								 cga_palette[data&0xf][2]);
            break;
    }
}

static int pc_t1t_vga_data_r(void)
{
	int data = pcjr.reg.data[pcjr.reg.index];

	switch (pcjr.reg.index)
	{
        case 0x00: /* mode control 1 */
			T1T_LOG(1,"T1T_vga_mode_ctrl_1_r",("$%02x\n", data));
            break;
        case 0x01: /* palette mask (bits 3-0) */
			T1T_LOG(1,"T1T_vga_palette_mask_r",("$%02x\n", data));
            break;
        case 0x02: /* border color (bits 3-0) */
			T1T_LOG(1,"T1T_vga_border_color_r",("$%02x\n", data));
            break;
        case 0x03: /* mode control 2 */
			T1T_LOG(1,"T1T_vga_mode_ctrl_2_r",("$%02x\n", data));
            break;
        case 0x04: /* reset register */
			T1T_LOG(1,"T1T_vga_reset_r",("$%02x\n", data));
            break;
        /* palette array */
        case 0x10: case 0x11: case 0x12: case 0x13:
        case 0x14: case 0x15: case 0x16: case 0x17:
        case 0x18: case 0x19: case 0x1a: case 0x1b:
        case 0x1c: case 0x1d: case 0x1e: case 0x1f:
			T1T_LOG(1,"T1T_vga_palette_r",("[$%02x] $%02x\n", pcjr.reg.index - 0x10, data));
            break;
    }
	return data;
}

/*
 * 3df RW	display bank, access bank, mode
 * bit 0-2	Identifies the page of main memory being displayed in units of 16K.
 *			0: 0K, 1: 16K...7: 112K. In 32K modes (bits 6-7 = 2) only 0,2,4 and
 *			6 are valid, as the next page will also be used.
 *	   3-5	Identifies the page of main memory that can be read/written at B8000h
 *			in units of 16K. 0: 0K, 1: 16K...7: 112K. In 32K modes (bits 6-7 = 2)
 *			only 0,2,4 and 6 are valid, as the next page will also be used.
 *	   6-7	Display mode. 0: Text, 1: 16K graphics mode (4,5,6,8)
 *			2: 32K graphics mode (9,Ah)
 */
static void pc_t1t_bank_w(int data)
{
	if (pcjr.bank != data)
	{
		int dram, vram;
		pcjr.bank = data;
	/* it seems the video ram is mapped to the last 128K of main memory */
#if 1
		if ((data&0xc0)==0xc0) { /* needed for lemmings */
			dram = 0x80000 + ((data & 0x06) << 14);
			vram = 0x80000 + ((data & 0x30) << (14-3));
		} else {
			dram = 0x80000 + ((data & 0x07) << 14);
			vram = 0x80000 + ((data & 0x38) << (14-3));
		}
#else
		dram = (data & 0x07) << 14;
		vram = (data & 0x38) << (14-3);
#endif
        videoram = &memory_region(REGION_CPU1)[vram];
		pcjr.displayram = &memory_region(REGION_CPU1)[dram];
        if (dirtybuffer)
			memset(dirtybuffer, 1, videoram_size);
		T1T_LOG(1,"t1t_bank_w",("$%02x: display ram $%05x, video ram $%05x\n", data, dram, vram));
	}
}

static int pc_t1t_bank_r(void)
{
	int data = pcjr.bank;
    T1T_LOG(1,"t1t_bank_r",("$%02x\n", data));
    return data;
}

/*************************************************************************
 *
 *		T1T
 *		Tandy 1000 / PCjr
 *
 *************************************************************************/

WRITE8_HANDLER ( pc_T1T_w )
{
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			crtc6845_port_w(crtc6845,0,data);
			break;
		case 1: case 3: case 5: case 7:
			crtc6845_port_w(crtc6845,1,data);
			break;
		case 8:
			pc_t1t_mode_control_w(data);
			break;
		case 9:
			pc_t1t_color_select_w(data);
			break;
		case 10:
			pc_t1t_vga_index_w(data);
            break;
        case 11:
			pc_t1t_lightpen_strobe_w(data);
			break;
		case 12:
            break;
		case 13:
            break;
        case 14:
			pc_t1t_vga_data_w(data);
            break;
        case 15:
			pc_t1t_bank_w(data);
			break;
    }
}

 READ8_HANDLER ( pc_T1T_r )
{
	int data = 0xff;
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			data = crtc6845_port_r(crtc6845,0);
			break;

		case 1: case 3: case 5: case 7:
			data = crtc6845_port_r(crtc6845,1);
			break;

		case 8:
			data = pc_t1t_mode_control_r();
			break;

		case 9:
			data = pc_t1t_color_select_r();
			break;

		case 10:
			data = pc_t1t_status_r();
			break;

		case 11:
			/* -W lightpen strobe reset */
			break;

		case 12:
		case 13:
            break;

		case 14:
			data = pc_t1t_vga_data_r();
            break;

        case 15:
			data = pc_t1t_bank_r();
			break;
    }
	return data;
}

/***************************************************************************
  Plot single text character
***************************************************************************/

static void t1t_plot_char(mame_bitmap *bitmap, const rectangle *r, UINT8 ch, UINT8 attr)
{
	int width;
	int height;
	int bgcolor;
	gfx_element *gfx;

	gfx = Machine->gfx[ch & 0x80 ? 1 : 0];
	drawgfx(bitmap, gfx, ch & 0x7f, attr, 
			0, 0, r->min_x, r->min_y, r, TRANSPARENCY_NONE, 0);

	height = r->max_y - r->min_y + 1;
	width = r->max_x - r->min_x + 1;

	if (height > 8)
	{
		bgcolor = gfx->colortable[gfx->color_granularity * (attr % gfx->total_colors)];
		plot_box(bitmap, r->min_x, r->min_y + 8, width, height - 8, bgcolor);
	}
}



/***************************************************************************
  Draw text mode with 40x25 characters (default) with high intensity bg.
***************************************************************************/

static void t1t_text_inten(mame_bitmap *bitmap, struct crtc6845 *crtc)
{
	int sx, sy;
	int	offs = crtc6845_get_start(crtc)*2;
	int lines = crtc6845_get_char_lines(crtc);
	int height = crtc6845_get_char_height(crtc);
	int columns = crtc6845_get_char_columns(crtc);
	rectangle r;
	struct crtc6845_cursor cursor;

	crtc6845_time(crtc);
	crtc6845_get_cursor(crtc, &cursor);

	for (sy=0, r.min_y=0, r.max_y=height-1; sy<lines; sy++, r.min_y+=height,r.max_y+=height) {

		for (sx=0, r.min_x=0, r.max_x=7; sx<columns; 
			 sx++, offs=(offs+2)&0x3fff, r.min_x+=8, r.max_x+=8)
		{
			if (!dirtybuffer || dirtybuffer[offs] || dirtybuffer[offs+1])
			{
				UINT8 ch = pcjr.displayram[offs];
				UINT8 attr = pcjr.displayram[offs + 1];
				
				t1t_plot_char(bitmap, &r, ch, attr);

				if (cursor.on && (pcjr.pc_framecnt & 32) && (offs == cursor.pos * 2))
				{
					int k = height - cursor.top;
					rectangle rect2 = r;
					rect2.min_y += cursor.top; 
					if (cursor.bottom<height)
						k=cursor.bottom-cursor.top+1;

					if (k>0)
						plot_box(bitmap, r.min_x, 
									r.min_y+cursor.top, 
									8, k, Machine->pens[7]);
				}

				if (dirtybuffer)
					dirtybuffer[offs] = dirtybuffer[offs + 1] = 0;
			}
		}
	}
}



/***************************************************************************
  Draw text mode with 40x25 characters (default) and blinking colors.
***************************************************************************/

static void t1t_text_blink(mame_bitmap *bitmap, struct crtc6845 *crtc)
{
	int sx, sy;
	int	offs = crtc6845_get_start(crtc)*2;
	int lines = crtc6845_get_char_lines(crtc);
	int height = crtc6845_get_char_height(crtc);
	int columns = crtc6845_get_char_columns(crtc);
	rectangle r;
	struct crtc6845_cursor cursor;

	crtc6845_time(crtc);
	crtc6845_get_cursor(crtc, &cursor);

	for (sy=0, r.min_y=0, r.max_y=height-1; sy<lines; sy++, r.min_y+=height,r.max_y+=height)
	{
		for (sx=0, r.min_x=0, r.max_x=7; sx<columns; 
			sx++, offs=(offs+2)&0x3fff, r.min_x+=8, r.max_x+=8)
		{
			if (!dirtybuffer || dirtybuffer[offs] || dirtybuffer[offs+1])
			{
				UINT8 ch = pcjr.displayram[offs + 0];
				UINT8 attr = pcjr.displayram[offs + 1];
					
				if (attr & 0x80)	/* blinking ? */
				{
					if (pcjr.pc_blink)
						attr = (attr & 0x70) | ((attr & 0x70) >> 4);
					else
						attr = attr & 0x7f;
				}

				t1t_plot_char(bitmap, &r, ch, attr);

				if (cursor.on&& (pcjr.pc_framecnt & 32) && (offs == cursor.pos * 2))
				{
					int k = height-cursor.top;
					rectangle rect2 = r;
					rect2.min_y += cursor.top;

					if (cursor.bottom < height)
						k = cursor.bottom - cursor.top + 1;

					if (k>0)
						plot_box(bitmap, r.min_x, 
							r.min_y+cursor.top, 
							8, k, Machine->pens[7]);
				}

				if (dirtybuffer)
					dirtybuffer[offs] = dirtybuffer[offs + 1] = 0;
			}
		}
	}
}



/***************************************************************************
  Draw graphics mode with 320x200 pixels (default) with 2 bits/pixel.
  The cell size is 2x1 (double width pixels, 1 scanline is the real
  default but up to 32 are possible).
  Even scanlines are from T1T_base + 0x0000, odd from T1T_base + 0x2000
***************************************************************************/

static void t1t_gfx_2bpp(mame_bitmap *bitmap, struct crtc6845 *crtc)
{
	static const UINT16 palette[] =
	{
		0, 10, 12, 14,
		0, 11, 13, 15
	};

	pc_render_gfx_2bpp(bitmap, crtc, pcjr.displayram, &palette[pcjr.color_select ? 4 : 0], 2);
}



/***************************************************************************
  Draw graphics mode with 640x200 pixels (default).
  The cell size is 1x1 (1 scanline is the real default)
  Even scanlines are from T1T_base + 0x0000, odd from T1T_base + 0x2000
***************************************************************************/
static void t1t_gfx_1bpp(mame_bitmap *bitmap, struct crtc6845 *crtc)
{
	int i, sx, sy, sh;
	int	offs = crtc6845_get_start(crtc)*2;
	int lines = crtc6845_get_char_lines(crtc);
	int height = crtc6845_get_char_height(crtc);
	int columns = crtc6845_get_char_columns(crtc)*2;

	for (sy=0; sy<lines; sy++,offs=(offs+columns)&0x1fff) {

		for (sh=0; sh<height; sh++, offs|=0x2000) { // char line 0 used as a12 line in graphic mode
			if (!(sh&1)) { // char line 0 used as a12 line in graphic mode
				for (i=offs, sx=0; sx<columns; sx++, i=(i+1)&0x1fff) {
					if (dirtybuffer[i]) {
						drawgfx(bitmap, Machine->gfx[2], pcjr.displayram[i], pcjr.color_select&0xf, 0,0,sx*8,sy*height+sh,
								0,TRANSPARENCY_NONE,0);
						dirtybuffer[i]=0;
					}
				}
			} else {
				for (i=offs|0x2000, sx=0; sx<columns; sx++, i=((i+1)&0x1fff)|0x2000) {
					if (dirtybuffer[i]) {
						drawgfx(bitmap, Machine->gfx[2], pcjr.displayram[i], pcjr.color_select&0xf, 0,0,sx*8,sy*height+sh,
								0,TRANSPARENCY_NONE,0);
						dirtybuffer[i]=0;
					}
				}
			}
		}
	}
}



/***************************************************************************
  Draw graphics mode with 160x200 pixels (default) and 4 bits/pixel.
  The cell size is 4x1 (quadruple width pixels, 1 scanline is the real
  default but up to 32 are possible).
  Scanlines (scanline % 4) are from CGA_base + 0x0000,
  CGA_base + 0x2000
***************************************************************************/

static void t1t_gfx_4bpp(mame_bitmap *bitmap, struct crtc6845 *crtc)
{
	pc_render_gfx_4bpp(bitmap, crtc, pcjr.displayram, NULL, 4);
}



/***************************************************************************
  Choose the appropriate video mode
***************************************************************************/

static pc_video_update_proc pc_t1t_choosevideomode(int *width, int *height, struct crtc6845 *crtc)
{
	int xfactor = 8;
	int yfactor = 1;
	pc_video_update_proc proc = NULL;

	switch( pcjr.mode_control & 0x3b ) {
	case 0x08:
	case 0x09:
		proc = t1t_text_inten;
		break;

	case 0x28:
	case 0x29:
		proc = t1t_text_blink;
		break;

    case 0x0a: case 0x0b: case 0x2a: case 0x2b:
		switch (pcjr.bank & 0xc0) {
		case 0x00:	/* hmm.. text in graphics? */
		case 0x40:
			proc = t1t_gfx_2bpp;
			break;

		case 0x80:
			proc = t1t_gfx_4bpp;
			xfactor = 4;
			break;

		case 0xc0:
			proc = t1t_gfx_4bpp;
			xfactor = 4;
			break;
		}
		break;

	case 0x18: case 0x19: case 0x1a: case 0x1b:
	case 0x38: case 0x39: case 0x3a: case 0x3b:
		switch (pcjr.bank & 0xc0) {
		case 0x00:	/* hmm.. text in graphics? */
		case 0x40:
			proc = t1t_gfx_1bpp;
			xfactor = 16;
			break;

		case 0x80:
		case 0xc0:
			proc = t1t_gfx_2bpp;
			break;
        }
		break;
    }

	*width *= xfactor;
	*height *= yfactor;
	return proc;
}
