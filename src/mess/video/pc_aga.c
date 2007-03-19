#include "video/pc_aga.h"
#include "video/pc_cga.h"
#include "video/pc_mda.h"
#include "includes/crtc6845.h"
#include "includes/amstr_pc.h"
#include "video/pc_video.h"
#include "video/generic.h"

static pc_video_update_proc pc_aga_choosevideomode(int *width, int *height, struct crtc6845 *crtc);


gfx_layout europc_cga_charlayout =
{
	8,16,					/* 8 x 32 characters */
	256,                    /* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7 },
	/* y offsets */
	{ 0*8,1*8,2*8,3*8,
		4*8,5*8,6*8,7*8,
		8*8,9*8,10*8,11*8,
		12*8,13*8,14*8,15*8 },
	8*16                     /* every char takes 8 bytes */
};

gfx_layout europc_mda_charlayout =
{
	9,32,					/* 9 x 32 characters (9 x 15 is the default, but..) */
	256,					/* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7,7 },	/* pixel 7 repeated only for char code 176 to 223 */
	/* y offsets */
	{
		0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8,
		0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
		8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8
	},
	8*16
};

gfx_layout pc200_mda_charlayout =
{
	9,32,					/* 9 x 32 characters (9 x 15 is the default, but..) */
	256,					/* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7,7 },	/* pixel 7 repeated only for char code 176 to 223 */
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8,
	  0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	8*16 					/* every char takes 8 bytes (upper half) */
};

gfx_layout pc200_cga_charlayout =
{
	8,16,               /* 8 x 16 characters */
	256,                    /* 256 characters */
	1,                      /* 1 bits per pixel */
	{ 0 },                  /* no bitplanes; 1 bit per pixel */
	/* x offsets */
	{ 0,1,2,3,4,5,6,7 },
	/* y offsets */
		{ 0*8,1*8,2*8,3*8,
			4*8,5*8,6*8,7*8,
			0*8,1*8,2*8,3*8,
			4*8,5*8,6*8,7*8 },
	8*16                     /* every char takes 16 bytes */
};


gfx_decode europc_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &europc_cga_charlayout,	   0, 256 },   /* single width */
	{ REGION_GFX1, 0x0000, &europc_cga_charlayout,	                0, 256 },   /* single width */
	{ REGION_GFX1, 0x0000, &europc_cga_charlayout,	                0, 256 },   /* single width */
	{ REGION_GFX1, 0x0000, &europc_cga_charlayout,	                0, 256 },   /* single width */
	{ REGION_GFX1, 0x1000, &europc_mda_charlayout,      256*2+16*2+96*4, 256 },   /* single width */
	{ REGION_GFX1, 0x1000, &europc_mda_charlayout,      256*2+16*2+96*4, 256 },   /* single width */
	{ REGION_GFX1, 0x1000, &europc_mda_charlayout,      256*2+16*2+96*4, 256 },   /* single width */
	{ REGION_GFX1, 0x1000, &europc_mda_charlayout,      256*2+16*2+96*4, 256 },   /* single width */
    { -1 } /* end of array */
};

gfx_decode aga_gfxdecodeinfo[] =
{
/* The four CGA fonts */
	{ REGION_GFX1, 0x1000, &pc200_cga_charlayout,		   0, 256 },   /* single width */
	{ REGION_GFX1, 0x3000, &pc200_cga_charlayout,                     0, 256 },   /* single width */
	{ REGION_GFX1, 0x5000, &pc200_cga_charlayout,                     0, 256 },   /* single width */
	{ REGION_GFX1, 0x7000, &pc200_cga_charlayout,                     0, 256 },   /* single width */
/* The four MDA fonts */
	{ REGION_GFX1, 0x0000, &pc200_mda_charlayout,       256*2+16*2+96*4, 256 },   /* single width */
	{ REGION_GFX1, 0x2000, &pc200_mda_charlayout,       256*2+16*2+96*4, 256 },   /* single width */
	{ REGION_GFX1, 0x4000, &pc200_mda_charlayout,       256*2+16*2+96*4, 256 },   /* single width */
	{ REGION_GFX1, 0x6000, &pc200_mda_charlayout,       256*2+16*2+96*4, 256 },   /* single width */
    { -1 } /* end of array */
};

/* Initialise the cga palette */
PALETTE_INIT( pc_aga )
{
	int i;
	for(i = 0; i < (sizeof(cga_palette) / 3); i++)
		palette_set_color(machine, i, cga_palette[i][0], cga_palette[i][1], cga_palette[i][2]);
	memcpy(colortable,cga_colortable,sizeof(cga_colortable));
	memcpy((char*)colortable+sizeof(cga_colortable), mda_colortable, sizeof(mda_colortable));
}

static struct {
	AGA_MODE mode;
} aga;



/*************************************
 *
 *	AGA MDA/CGA read/write handlers
 *
 *************************************/

static READ8_HANDLER ( pc_aga_mda_r )
{
	if (aga.mode==AGA_MONO)
		return pc_MDA_r(offset);
	return 0xff;
}

static WRITE8_HANDLER ( pc_aga_mda_w )
{
	if (aga.mode==AGA_MONO)
		pc_MDA_w(offset, data);
}

static READ8_HANDLER ( pc_aga_cga_r )
{
	if (aga.mode==AGA_COLOR)
		return pc_cga8_r(offset);
	return 0xff;
}

static WRITE8_HANDLER ( pc_aga_cga_w )
{
	if (aga.mode==AGA_COLOR)
		pc_cga8_w(offset, data);
}



/*************************************/

void pc_aga_set_mode(AGA_MODE mode)
{
	aga.mode = mode;

	switch (aga.mode) {
	case AGA_COLOR:
		crtc6845_set_clock(crtc6845, 10000000/*?*/);
		break;
	case AGA_MONO:
		crtc6845_set_clock(crtc6845, 10000000/*?*/);
		break;
	case AGA_OFF:
		break;
	}
}

extern void pc_aga_timer(void)
{
	switch (aga.mode) {
	case AGA_COLOR: ;break;
	case AGA_MONO: pc_mda_timer();break;
	case AGA_OFF: break;
	}
}

static void pc_aga_cursor(struct crtc6845_cursor *cursor)
{
	switch (aga.mode) {
	case AGA_COLOR:
		pc_cga_cursor(cursor);
		break;

	case AGA_MONO:
		pc_mda_cursor(cursor);
		break;

	case AGA_OFF:
		break;
	}
}


static struct crtc6845_config config= { 14318180 /*?*/, pc_aga_cursor };

VIDEO_START( pc_aga )
{
	int buswidth;

	pc_mda_europc_init();

	buswidth = cputype_databus_width(Machine->drv->cpu[0].cpu_type, ADDRESS_SPACE_PROGRAM);
	switch(buswidth) {
	case 8:
		memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x3b0, 0x3bf, 0, 0, pc_aga_mda_r );
		memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3b0, 0x3bf, 0, 0, pc_aga_mda_w );
		memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc_aga_cga_r );
		memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc_aga_cga_w );
		break;

	default:
		fatalerror("CGA:  Bus width %d not supported\n", buswidth);
		break;
	}

	if (!pc_video_start(&config, pc_aga_choosevideomode, videoram_size))
		return 1;
	pc_aga_set_mode(AGA_COLOR);
	return 0;
}


VIDEO_START( pc200 )
{
	int buswidth;

	if (video_start_pc_aga(machine))
		return 1;

	buswidth = cputype_databus_width(Machine->drv->cpu[0].cpu_type, ADDRESS_SPACE_PROGRAM);
	switch(buswidth) {
	case 8:
		memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc200_cga_r );
		memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc200_cga_w );
		break;

	default:
		fatalerror("CGA:  Bus width %d not supported\n", buswidth);
		break;
	}
	return 0;
}

/***************************************************************************
  Choose the appropriate video mode
***************************************************************************/
static pc_video_update_proc pc_aga_choosevideomode(int *width, int *height, struct crtc6845 *crtc)
{
	pc_video_update_proc proc = NULL;

	switch (aga.mode) {
	case AGA_COLOR:
		proc =  pc_cga_choosevideomode(width, height, crtc);
		break;
	case AGA_MONO:
		proc =  pc_mda_choosevideomode(width, height, crtc);
		break;
	case AGA_OFF:
		break;
	}
	return proc;
}

WRITE8_HANDLER ( pc_aga_videoram_w )
{
	switch (aga.mode) {
	case AGA_COLOR:
		if (offset>=0x8000)
			pc_video_videoram_w(offset-0x8000, data);
		break;
	case AGA_MONO:
		pc_video_videoram_w(offset,data);
		break;
	case AGA_OFF: break;
	}
}

 READ8_HANDLER( pc_aga_videoram_r )
{
	switch (aga.mode) {
	case AGA_COLOR: 
		if (offset>=0x8000) return videoram[offset-0x8000];
		return 0;
	case AGA_MONO:
		return videoram[offset];
	case AGA_OFF: break;
	}
	return 0;
}

WRITE8_HANDLER ( pc200_videoram_w )
{
	switch (aga.mode) {
	default:
		if (offset>=0x8000)
			pc_video_videoram_w(offset-0x8000, data);
		break;
	case AGA_MONO:
		pc_video_videoram_w(offset,data);
		break;
	}
}

 READ8_HANDLER( pc200_videoram_r )
{
	switch (aga.mode) {
	default: 
		if (offset>=0x8000) return videoram[offset-0x8000];
		return 0;
	case AGA_MONO:
		return videoram[offset];
	}
	return 0;
}



static struct {
	UINT8 port8, portd, porte;
} pc200= { 0 };

// in reality it is of course only 1 graphics adapter,
// but now cga and mda are splitted in mess
WRITE8_HANDLER( pc200_cga_w )
{
	switch(offset) {
	case 4:
		pc200.portd |= 0x20;
		pc_cga8_w(offset,data);
		break;
	case 8:
		pc200.port8 = data;
		pc200.portd |= 0x80;
		pc_cga8_w(offset,data);
		break;
	case 0xe:
		pc200.portd = 0x1f;
		if (data & 0x80)
			pc200.portd |= 0x40;

/* The bottom 3 bits of this port are:
 * Bit 2: Disable AGA
 * Bit 1: Select MDA 
 * Bit 0: Select external display (monitor) rather than internal display
 *       (TV for PC200; LCD for PPC512) */
		if ((pc200.porte & 7) != (data & 7))
		{
			if (data & 4)
				pc_aga_set_mode(AGA_OFF);
			else if (data & 2)
				pc_aga_set_mode(AGA_MONO);
			else
				pc_aga_set_mode(AGA_COLOR);
		}
		pc200.porte = data;
		break;

	default:
		pc_cga8_w(offset,data);
		break;
	}
}

 READ8_HANDLER ( pc200_cga_r )
{
	UINT8 result = 0;

	switch(offset) {
	case 8:
		result = pc200.port8;
		break;

	case 0xd:
		// after writing 0x80 to 0x3de, bits 7..5 of 0x3dd from the 2nd read must be 0
		result=pc200.portd;
		pc200.portd&=0x1f;
		break;

	case 0xe:
		// 0x20 low cga
		// 0x10 low special
		result = input_port_1_r(0)&0x38;
		break;

	default:
		result = pc_cga8_r(offset);
		break;
	}
	return result;
}
