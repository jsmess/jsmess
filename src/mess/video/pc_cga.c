/***************************************************************************

  Color Graphics Adapter (CGA) section


  Notes on Port 3D8
  (http://www.clipx.net/ng/interrupts_and_ports/ng2d045.php)

	Port 3D8  -  Color/VGA Mode control register

			xx1x xxxx  Attribute bit 7. 0=blink, 1=Intesity
			xxx1 xxxx  640x200 mode
			xxxx 1xxx  Enable video signal
			xxxx x1xx  Select B/W mode
			xxxx xx1x  Select graphics
			xxxx xxx1  80x25 text


	The usage of the above control register for various modes is:
			xx10 1100  40x25 alpha B/W
			xx10 1000  40x25 alpha color
			xx10 1101  80x25 alpha B/W
			xx10 1001  80x25 alpha color
			xxx0 1110  320x200 graph B/W
			xxx0 1010  320x200 graph color
			xxx1 1110  640x200 graph B/W


	PC1512 display notes

	The PC1512 built-in display adaptor is an emulation of IBM's CGA.  Unlike a
	real CGA, it is not built around a real MC6845 controller, and so attempts
	to get custom video modes out of it may not work as expected. Its 640x200
	CGA mode can be set up to be a 16-color mode rather than mono.

	If you program it with BIOS calls, the PC1512 behaves just like a real CGA,
	except:

	- The 'greyscale' text modes (0 and 2) behave just like the 'color'
	  ones (1 and 3). On a color monitor both are in color; on a mono
	  monitor both are in greyscale.
	- Mode 5 (the 'greyscale' graphics mode) displays in color, using
	  an alternative color palette: Cyan, Red and White.
	- The undocumented 160x100x16 "graphics" mode works correctly.

	(source John Elliott http://www.seasip.info/AmstradXT/pc1512disp.html)

***************************************************************************/

#include "driver.h"
#include "pc_cga.h"
#include "video/mc6845.h"
#include "video/pc_video.h"
#include "memconv.h"

#define VERBOSE_CGA 0		/* CGA (Color Graphics Adapter) */

#define CGA_LOG(N,M,A) \
	if(VERBOSE_CGA>=N){ if( M )logerror("%11.6f: %-24s",attotime_to_double(timer_get_time()),(char*)M ); logerror A; }

/***************************************************************************

	Static declarations

***************************************************************************/

static VIDEO_START( pc_cga );
static PALETTE_INIT( pc_cga );


/* In cgapal.c; it's quite big */
extern const unsigned char cga_palette[16 * CGA_PALETTE_SETS][3];


static VIDEO_UPDATE( mc6845_cga );
static READ8_HANDLER( pc_cga8_r );
static WRITE8_HANDLER( pc_cga8_w );
static READ16_HANDLER( pc_cga16le_r );
static WRITE16_HANDLER( pc_cga16le_w );
static READ32_HANDLER( pc_cga32le_r );
static WRITE32_HANDLER( pc_cga32le_w );
static MC6845_UPDATE_ROW( cga_update_row );
static MC6845_ON_HSYNC_CHANGED( cga_hsync_changed );
static MC6845_ON_VSYNC_CHANGED( cga_vsync_changed );
static VIDEO_START( pc1512 );
static VIDEO_UPDATE( mc6845_pc1512 );


static const mc6845_interface mc6845_cga_intf = {
	CGA_SCREEN_NAME,	/* screen number */
	XTAL_14_31818MHz/8,	/* clock */
	8,					/* numbers of pixels per video memory address */
	NULL,				/* begin_update */
	cga_update_row,		/* update_row */
	NULL,				/* end_update */
	NULL,				/* on_de_chaged */
	cga_hsync_changed,	/* on_hsync_changed */
	cga_vsync_changed	/* on_vsync_changed */
};


MACHINE_DRIVER_START( pcvideo_cga )
	MDRV_SCREEN_ADD(CGA_SCREEN_NAME, RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_RAW_PARAMS(XTAL_14_31818MHz,912,0,640,262,0,200)
	MDRV_PALETTE_LENGTH( CGA_PALETTE_SETS * 16 )

	MDRV_PALETTE_INIT(pc_cga)

	MDRV_DEVICE_ADD(CGA_MC6845_NAME, MC6845)
	MDRV_DEVICE_CONFIG( mc6845_cga_intf )

	MDRV_VIDEO_START( pc_cga )
	MDRV_VIDEO_UPDATE( mc6845_cga )
MACHINE_DRIVER_END

MACHINE_DRIVER_START( pcvideo_pc1512 )
	MDRV_IMPORT_FROM( pcvideo_cga )
	MDRV_VIDEO_START( pc1512 )
	MDRV_VIDEO_UPDATE( mc6845_pc1512 )
MACHINE_DRIVER_END



/***************************************************************************

	Local variables and declarations

***************************************************************************/

/* CGA dipswitch settings. These have to be kept consistent over all systems
 * that use CGA. */

#define CGA_FONT_0				0
#define CGA_FONT_1				1
#define CGA_FONT_2				2
#define CGA_FONT_3				3

#define CGA_MONITOR		(readinputport(20)&0x1C)
#define CGA_MONITOR_RGB			0x00	/* Colour RGB */
#define CGA_MONITOR_MONO		0x04	/* Greyscale RGB */
#define CGA_MONITOR_COMPOSITE	0x08	/* Colour composite */
#define CGA_MONITOR_TELEVISION	0x0C	/* Television */
#define CGA_MONITOR_LCD			0x10	/* LCD, eg PPC512 */

#define CGA_CHIPSET		(readinputport(20)&0xE0)
#define CGA_CHIPSET_IBM			0x00	/* Original IBM CGA */
#define CGA_CHIPSET_PC1512		0x20	/* PC1512 CGA subset */
#define CGA_CHIPSET_PC200		0x40	/* PC200 in CGA mode */
#define CGA_CHIPSET_ATI			0x60	/* ATI (supports Plantronics) */
#define CGA_CHIPSET_PARADISE	0x80	/* Paradise (used in PC1640) */

static struct
{
	UINT8 mode_control;	/* wo 0x3d8 */
	UINT8 color_select;	/* wo 0x3d9 */
	UINT8 status;		/* ro 0x3da */
	UINT8 plantronics;	/* wo 0x3dd, ATI chipset only */

	UINT8 frame;

	UINT8	*chr_gen;

	mc6845_update_row_func	update_row;
	UINT8	palette_lut_2bpp[4];
	UINT8	vsync;
	UINT8	hsync;
} cga;



/***************************************************************************

	Methods

***************************************************************************/

/* Initialise the cga palette */
static PALETTE_INIT( pc_cga )
{
	int i;

	for ( i = 0; i < CGA_PALETTE_SETS * 16; i++ ) {
		palette_set_color_rgb( machine, i, cga_palette[i][0], cga_palette[i][1], cga_palette[i][2] );
	}
}


static int internal_pc_cga_video_start(int personality)
{
	memset(&cga, 0, sizeof(cga));
	cga.update_row = NULL;

	cga.chr_gen = memory_region(REGION_GFX1) + 0x1000;

	state_save_register_item("pccga", 0, cga.mode_control);
	state_save_register_item("pccga", 0, cga.color_select);
	state_save_register_item("pccga", 0, cga.status);
	state_save_register_item("pccga", 0, cga.plantronics);

	return 0;
}



static VIDEO_START( pc_cga )
{
	int buswidth;

	/* Changed video RAM size to full 32k, for cards which support the
	 * Plantronics chipset.
	 * TODO: Cards which don't support Plantronics should repeat at
	 * BC000h */
	buswidth = cputype_databus_width(machine->config->cpu[0].type, ADDRESS_SPACE_PROGRAM);
	switch(buswidth)
	{
		case 8:
			memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbbfff, 0, 0x0c000, SMH_BANK11 );
			memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbbfff, 0, 0x0c000, pc_video_videoram_w );
			memory_install_read8_handler(0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc_cga8_r );
			memory_install_write8_handler(0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc_cga8_w );
			break;

		case 16:
			memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbbfff, 0, 0x0c000, SMH_BANK11 );
			memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbbfff, 0, 0x0c000, pc_video_videoram16le_w );
			memory_install_read16_handler(0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc_cga16le_r );
			memory_install_write16_handler(0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc_cga16le_w );
			break;

		case 32:
			memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbbfff, 0, 0x0c000, SMH_BANK11 );
			memory_install_write32_handler(0, ADDRESS_SPACE_PROGRAM, 0xb8000, 0xbbfff, 0, 0x0c000, pc_video_videoram32_w );
			memory_install_read32_handler(0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc_cga32le_r );
			memory_install_write32_handler(0, ADDRESS_SPACE_IO, 0x3d0, 0x3df, 0, 0, pc_cga32le_w );
			break;

		default:
			fatalerror("CGA:  Bus width %d not supported\n", buswidth);
			break;
	}

	videoram_size = 0x4000;

	videoram = auto_malloc(videoram_size);

	memory_set_bankptr(11, videoram);

	internal_pc_cga_video_start(M6845_PERSONALITY_GENUINE);
}



static VIDEO_UPDATE( mc6845_cga ) {
	device_config	*devconf = (device_config *) device_list_find_by_tag(screen->machine->config->devicelist, MC6845, CGA_MC6845_NAME);
	mc6845_update( devconf, bitmap, cliprect);

	/* Check for changes in font dipsetting */
	switch ( CGA_FONT & 0x01 ) {
	case 0:
		cga.chr_gen = memory_region(REGION_GFX1) + 0x1800;
		break;
	case 1:
		cga.chr_gen = memory_region(REGION_GFX1) + 0x1000;
		break;
	}
	return 0;
}


/***************************************************************************
  Draw text mode with 40x25 characters (default) with high intensity bg.
  The character cell size is 16x8
***************************************************************************/

static MC6845_UPDATE_ROW( cga_text_inten_update_row ) {
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);
	int i;

	if ( y == 0 ) logerror("cga_text_inten_update_row\n");
	for ( i = 0; i < x_count; i++ ) {
		UINT16 offset = ( ( ma + i ) << 1 ) & 0x3fff;
		UINT8 chr = videoram[ offset ];
		UINT8 attr = videoram[ offset +1 ];
		UINT8 data = cga.chr_gen[ chr * 8 + ra ];
		UINT16 fg = attr & 0x0F;
		UINT16 bg = ( attr >> 4 ) & 0x07;

		if ( i == cursor_x ) {
			data = 0xFF;
		}

		*p = ( data & 0x80 ) ? fg : bg; p++;
		*p = ( data & 0x40 ) ? fg : bg; p++;
		*p = ( data & 0x20 ) ? fg : bg; p++;
		*p = ( data & 0x10 ) ? fg : bg; p++;
		*p = ( data & 0x08 ) ? fg : bg; p++;
		*p = ( data & 0x04 ) ? fg : bg; p++;
		*p = ( data & 0x02 ) ? fg : bg; p++;
		*p = ( data & 0x01 ) ? fg : bg; p++;
	}
}


/***************************************************************************
  Draw text mode with 40x25 characters (default) with high intensity bg.
  The character cell size is 16x8
***************************************************************************/

static MC6845_UPDATE_ROW( cga_text_inten_alt_update_row ) {
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);
	int i;

	if ( y == 0 ) logerror("cga_text_inten_alt_update_row\n");
	for ( i = 0; i < x_count; i++ ) {
		UINT16 offset = ( ( ma + i ) << 1 ) & 0x3fff;
		UINT8 chr = videoram[ offset ];
		UINT8 attr = videoram[ offset +1 ];
		UINT8 data = cga.chr_gen[ chr * 8 + ra ];
		UINT16 fg = attr & 0x0F;

		if ( i == cursor_x ) {
			data = 0xFF;
		}

		*p = ( data & 0x80 ) ? fg : 0; p++;
		*p = ( data & 0x40 ) ? fg : 0; p++;
		*p = ( data & 0x20 ) ? fg : 0; p++;
		*p = ( data & 0x10 ) ? fg : 0; p++;
		*p = ( data & 0x08 ) ? fg : 0; p++;
		*p = ( data & 0x04 ) ? fg : 0; p++;
		*p = ( data & 0x02 ) ? fg : 0; p++;
		*p = ( data & 0x01 ) ? fg : 0; p++;
	}
}


/***************************************************************************
  Draw text mode with 40x25 characters (default) and blinking colors.
  The character cell size is 16x8
***************************************************************************/

static MC6845_UPDATE_ROW( cga_text_blink_update_row ) {
	UINT16	*p = BITMAP_ADDR16(bitmap, y, 0);
	int i;

	for ( i = 0; i < x_count; i++ ) {
		UINT16 offset = ( ( ma + i ) << 1 ) & 0x3fff;
		UINT8 chr = videoram[ offset ];
		UINT8 attr = videoram[ offset +1 ];
		UINT8 data = cga.chr_gen[ chr * 8 + ra ];
		UINT16 fg = attr & 0x0F;
		UINT16 bg = ( attr >> 4 ) & 0x07;

		if ( i == cursor_x ) {
			data = 0xFF;
		} else {
			if ( ( attr & 0x80 ) && ( cga.frame & 0x10 ) ) {
				data = 0x00;
			}
		}

		*p = ( data & 0x80 ) ? fg : bg; p++;
		*p = ( data & 0x40 ) ? fg : bg; p++;
		*p = ( data & 0x20 ) ? fg : bg; p++;
		*p = ( data & 0x10 ) ? fg : bg; p++;
		*p = ( data & 0x08 ) ? fg : bg; p++;
		*p = ( data & 0x04 ) ? fg : bg; p++;
		*p = ( data & 0x02 ) ? fg : bg; p++;
		*p = ( data & 0x01 ) ? fg : bg; p++;
	}
}


/***************************************************************************
  Draw text mode with 40x25 characters (default) and blinking colors.
  The character cell size is 16x8
***************************************************************************/

static MC6845_UPDATE_ROW( cga_text_blink_alt_update_row ) {
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);
	int i;

	if ( y == 0 ) logerror("cga_text_blink_alt_update_row\n");
	for ( i = 0; i < x_count; i++ ) {
		UINT16 offset = ( ( ma + i ) << 1 ) & 0x3fff;
		UINT8 chr = videoram[ offset ];
		UINT8 attr = videoram[ offset +1 ];
		UINT8 data = cga.chr_gen[ chr * 8 + ra ];
		UINT16 fg = attr & 0x07;
		UINT16 bg = 0;

		if ( i == cursor_x ) {
			data = 0xFF;
		} else {
			if ( ( attr & 0x80 ) && ( cga.frame & 0x10 ) ) {
				data = 0x00;
				bg = ( attr >> 4 ) & 0x07;
			}
		}

		*p = ( data & 0x80 ) ? fg : bg; p++;
		*p = ( data & 0x40 ) ? fg : bg; p++;
		*p = ( data & 0x20 ) ? fg : bg; p++;
		*p = ( data & 0x10 ) ? fg : bg; p++;
		*p = ( data & 0x08 ) ? fg : bg; p++;
		*p = ( data & 0x04 ) ? fg : bg; p++;
		*p = ( data & 0x02 ) ? fg : bg; p++;
		*p = ( data & 0x01 ) ? fg : bg; p++;
	}
}


/* The lo-res graphics mode on a colour composite monitor */

static MC6845_UPDATE_ROW( cga_gfx_4bppl_update_row ) {
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);
	int i;

	if ( y == 0 ) logerror("cga_gfx_4bppl_update_row\n");
	for ( i = 0; i < x_count; i++ ) {
		UINT16 offset = ( ( ( ma + i ) << 1 ) & 0x1fff ) | ( ( y & 1 ) << 13 );
		UINT8 data = videoram[ offset ];

		*p = data >> 4; p++;
		*p = data >> 4; p++;
		*p = data & 0x0F; p++;
		*p = data & 0x0F; p++;

		data = videoram[ offset + 1 ];

		*p = data >> 4; p++;
		*p = data >> 4; p++;
		*p = data & 0x0F; p++;
		*p = data & 0x0F; p++;
	}
}


/* The hi-res graphics mode on a colour composite monitor
 *
 * The different scaling factors mean that the '160x200' versions of screens
 * are the same size as the normal colour ones.
 */

static MC6845_UPDATE_ROW( cga_gfx_4bpph_update_row ) {
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);
	int i;

	if ( y == 0 ) logerror("cga_gfx_4bpph_update_row\n");
	for ( i = 0; i < x_count; i++ ) {
		UINT16 offset = ( ( ( ma + i ) << 1 ) & 0x1fff ) | ( ( y & 1 ) << 13 );
		UINT8 data = videoram[ offset ];

		*p = data >> 4; p++;
		*p = data >> 4; p++;
		*p = data & 0x0F; p++;
		*p = data & 0x0F; p++;

		data = videoram[ offset + 1 ];

		*p = data >> 4; p++;
		*p = data >> 4; p++;
		*p = data & 0x0F; p++;
		*p = data & 0x0F; p++;
	}
}


/***************************************************************************
  Draw graphics mode with 320x200 pixels (default) with 2 bits/pixel.
  Even scanlines are from CGA_base + 0x0000, odd from CGA_base + 0x2000
  cga fetches 2 byte per mscrtc6845 access.
***************************************************************************/

static MC6845_UPDATE_ROW( cga_gfx_2bpp_update_row ) {
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);
	int i;

//	if ( y == 0 ) logerror("cga_gfx_2bpp_update_row\n");
	for ( i = 0; i < x_count; i++ ) {
		UINT16 offset = ( ( ( ma + i ) << 1 ) & 0x1fff ) | ( ( y & 1 ) << 13 );
		UINT8 data = videoram[ offset ];

		*p = cga.palette_lut_2bpp[ ( data >> 6 ) & 0x03 ]; p++;
		*p = cga.palette_lut_2bpp[ ( data >> 4 ) & 0x03 ]; p++;
		*p = cga.palette_lut_2bpp[ ( data >> 2 ) & 0x03 ]; p++;
		*p = cga.palette_lut_2bpp[   data        & 0x03 ]; p++;

		data = videoram[ offset+1 ];

		*p = cga.palette_lut_2bpp[ ( data >> 6 ) & 0x03 ]; p++;
		*p = cga.palette_lut_2bpp[ ( data >> 4 ) & 0x03 ]; p++;
		*p = cga.palette_lut_2bpp[ ( data >> 2 ) & 0x03 ]; p++;
		*p = cga.palette_lut_2bpp[   data        & 0x03 ]; p++;
	}
}


/***************************************************************************
  Draw graphics mode with 640x200 pixels (default).
  The cell size is 1x1 (1 scanline is the real default)
  Even scanlines are from CGA_base + 0x0000, odd from CGA_base + 0x2000
***************************************************************************/

static MC6845_UPDATE_ROW( cga_gfx_1bpp_update_row ) {
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);
	UINT8	fg = cga.color_select & 0x0F;
	int i;

	if ( y == 0 ) logerror("cga_gfx_1bpp_update_row\n");
	for ( i = 0; i < x_count; i++ ) {
		UINT16 offset = ( ( ( ma + i ) << 1 ) & 0x1fff ) | ( ( y & 1 ) << 13 );
		UINT8 data = videoram[ offset ];

		*p = ( data & 0x80 ) ? fg : 0; p++;
		*p = ( data & 0x40 ) ? fg : 0; p++;
		*p = ( data & 0x20 ) ? fg : 0; p++;
		*p = ( data & 0x10 ) ? fg : 0; p++;
		*p = ( data & 0x08 ) ? fg : 0; p++;
		*p = ( data & 0x04 ) ? fg : 0; p++;
		*p = ( data & 0x02 ) ? fg : 0; p++;
		*p = ( data & 0x01 ) ? fg : 0; p++;

		data = videoram[ offset + 1 ];

		*p = ( data & 0x80 ) ? fg : 0; p++;
		*p = ( data & 0x80 ) ? fg : 0; p++;
		*p = ( data & 0x80 ) ? fg : 0; p++;
		*p = ( data & 0x80 ) ? fg : 0; p++;
		*p = ( data & 0x80 ) ? fg : 0; p++;
		*p = ( data & 0x80 ) ? fg : 0; p++;
		*p = ( data & 0x80 ) ? fg : 0; p++;
		*p = ( data & 0x80 ) ? fg : 0; p++;
	}
}


static MC6845_UPDATE_ROW( cga_update_row ) {
	if ( cga.update_row ) {
		cga.update_row( device, bitmap, cliprect, ma, ra, y, x_count, cursor_x, param );
	}
}


static MC6845_ON_HSYNC_CHANGED( cga_hsync_changed ) {
	cga.hsync = hsync ? 1 : 0;
}


static MC6845_ON_VSYNC_CHANGED( cga_vsync_changed ) {
	cga.vsync = vsync ? 8 : 0;
	if ( vsync ) {
		cga.frame++;
	}
}


static void pc_cga_set_palette_luts(void) {
	/* Setup 2bpp palette lookup table */
	if ( cga.mode_control & 0x10 ) {
		cga.palette_lut_2bpp[0] = 0;
	} else {
		cga.palette_lut_2bpp[0] = cga.color_select & 0x0F;
	}
	if ( cga.mode_control & 0x04 ) {
		cga.palette_lut_2bpp[1] = ( ( cga.color_select & 0x10 ) >> 1 ) | 3;
		cga.palette_lut_2bpp[2] = ( ( cga.color_select & 0x10 ) >> 1 ) | 4;
		cga.palette_lut_2bpp[3] = ( ( cga.color_select & 0x10 ) >> 1 ) | 7;
	} else {
		if ( cga.color_select & 0x20 ) {
			cga.palette_lut_2bpp[1] = ( ( cga.color_select & 0x10 ) >> 1 ) | 3;
			cga.palette_lut_2bpp[2] = ( ( cga.color_select & 0x10 ) >> 1 ) | 5;
			cga.palette_lut_2bpp[3] = ( ( cga.color_select & 0x10 ) >> 1 ) | 7;
		} else {
			cga.palette_lut_2bpp[1] = ( ( cga.color_select & 0x10 ) >> 1 ) | 2;
			cga.palette_lut_2bpp[2] = ( ( cga.color_select & 0x10 ) >> 1 ) | 4;
			cga.palette_lut_2bpp[3] = ( ( cga.color_select & 0x10 ) >> 1 ) | 6;
		}
	}
	//logerror("2bpp lut set to %d,%d,%d,%d\n", cga.palette_lut_2bpp[0], cga.palette_lut_2bpp[1], cga.palette_lut_2bpp[2], cga.palette_lut_2bpp[3]);
}

/*
 *	rW	CGA mode control register (see #P138)
 */
static void pc_cga_mode_control_w(int data)
{
	unsigned char mask = 0x3B;

	CGA_LOG(1,"CGA_mode_control_w",("$%02x: columns %d, gfx %d, hires %d, blink %d\n",
		data, (data&1)?80:40, (data>>1)&1, (data>>4)&1, (data>>5)&1));

	/* CGA composite: Switching between mono & colour behaves like a
	 * mode change */
	if(CGA_MONITOR == CGA_MONITOR_COMPOSITE) mask = 0x3F;
	cga.mode_control = data;

	//logerror("mode set to %02X\n", cga.mode_control & 0x3F );
	switch ( cga.mode_control & 0x3F ) {
	case 0x08: case 0x09: case 0x0C: case 0x0D:
		cga.update_row = cga_text_inten_update_row;
		break;
	case 0x0A: case 0x0B: case 0x2A: case 0x2B:
		if ( CGA_MONITOR == CGA_MONITOR_COMPOSITE ) {
			cga.update_row = cga_gfx_4bppl_update_row;
		} else {
			cga.update_row = cga_gfx_2bpp_update_row;
		}
		break;
	case 0x0E: case 0x0F: case 0x2E: case 0x2F:
		cga.update_row = cga_gfx_2bpp_update_row;
		break;
	case 0x18: case 0x19: case 0x1C: case 0x1D:
		cga.update_row = cga_text_inten_alt_update_row;
		break;
	case 0x1A: case 0x1B: case 0x3A: case 0x3B:
		if ( CGA_MONITOR == CGA_MONITOR_COMPOSITE ) {
			cga.update_row = cga_gfx_4bpph_update_row;
		} else {
			cga.update_row = cga_gfx_2bpp_update_row;
		}
		break;
	case 0x1E: case 0x1F: case 0x3E: case 0x3F:
		cga.update_row = cga_gfx_1bpp_update_row;
		break;
	case 0x28: case 0x29: case 0x2C: case 0x2D:
		cga.update_row = cga_text_blink_update_row;
		break;
	case 0x38: case 0x39: case 0x3C: case 0x3D:
		cga.update_row = cga_text_blink_alt_update_row;
		break;
	default:
		cga.update_row = NULL;
		break;
	}

	pc_cga_set_palette_luts();
}



/*
 *	?W	reserved for color select register on color adapter
 */
static void pc_cga_color_select_w(int data)
{
	CGA_LOG(1,"CGA_color_select_w",("$%02x\n", data));
	cga.color_select = data;
	//logerror("color_select_w: %02X\n", data);
	pc_cga_set_palette_luts();
}



/*
 * Select Plantronics modes
 */
static void pc_cga_plantronics_w(int data)
{
	CGA_LOG(1,"CGA_plantronics_w",("$%02x\n", data));

	if (CGA_CHIPSET != CGA_CHIPSET_ATI) return;

	data &= 0x70;	/* Only bits 6-4 are used */
	if (cga.plantronics == data) return;
	cga.plantronics = data;
}



/*************************************************************************
 *
 *		CGA
 *		color graphics adapter
 *
 *************************************************************************/

static READ8_HANDLER( pc_cga8_r )
{
	device_config	*devconf = (device_config *) device_list_find_by_tag(machine->config->devicelist, MC6845, CGA_MC6845_NAME);
	int data = 0xff;
	switch( offset )
	{
		case 0: case 2: case 4: case 6:
			/* return last written mc6845 address value here? */
			break;
		case 1: case 3: case 5: case 7:
			data = mc6845_register_r( devconf, offset );
			break;
		case 10:
			data = cga.vsync | ( ( data & 0x40 ) >> 4 ) | cga.hsync;
			break;
    }
	return data;
}



static WRITE8_HANDLER( pc_cga8_w )
{
	device_config	*devconf;

	switch(offset) {
	case 0: case 2: case 4: case 6:
		devconf = (device_config *) device_list_find_by_tag(machine->config->devicelist, MC6845, CGA_MC6845_NAME);
		mc6845_address_w( devconf, offset, data );
		break;
	case 1: case 3: case 5: case 7:
		devconf = (device_config *) device_list_find_by_tag(machine->config->devicelist, MC6845, CGA_MC6845_NAME);
		mc6845_register_w( devconf, offset, data );
		break;
	case 8:
		pc_cga_mode_control_w(data);
		break;
	case 9:
		pc_cga_color_select_w(data);
		break;
	case 0x0d:
		pc_cga_plantronics_w(data);
		break;
	}
}



static READ16_HANDLER( pc_cga16le_r ) { return read16le_with_read8_handler(pc_cga8_r,machine,  offset, mem_mask); }
static WRITE16_HANDLER( pc_cga16le_w ) { write16le_with_write8_handler(pc_cga8_w, machine, offset, data, mem_mask); }
static READ32_HANDLER( pc_cga32le_r ) { return read32le_with_read8_handler(pc_cga8_r, machine, offset, mem_mask); }
static WRITE32_HANDLER( pc_cga32le_w ) { write32le_with_write8_handler(pc_cga8_w, machine, offset, data, mem_mask); }


/* Old plantronics rendering code, leaving it uncommented until we have re-implemented it */

//
// From choosevideomode:
//
//      /* Plantronics high-res */
//      if ((cga.mode_control & 2) && (cga.plantronics & 0x20))
//          proc = cga_pgfx_2bpp;
//      /* Plantronics low-res */
//      if ((cga.mode_control & 2) && (cga.plantronics & 0x10))
//          proc = cga_pgfx_4bpp;
//

//INLINE void pgfx_plot_unit_4bpp(bitmap_t *bitmap,
//							 int x, int y, int offs)
//{
//	int color, values[2];
//	int i;
//
//	if (cga.plantronics & 0x40)
//	{
//		values[0] = videoram[offs | 0x4000];
//		values[1] = videoram[offs];
//	}
//	else
//	{
//		values[0] = videoram[offs];
//		values[1] = videoram[offs | 0x4000];
//	}
//	for (i=3; i>=0; i--)
//	{
//		color = ((values[0] & 0x3) << 1) |
//			((values[1] & 2)   >> 1) |
//			((values[1] & 1)   << 3);
//		*BITMAP_ADDR16(bitmap, y, x+i) = Machine->pens[color];
//		values[0]>>=2;
//		values[1]>>=2;
//	}
//}
//
//
//
///***************************************************************************
//  Draw graphics mode with 640x200 pixels (default) with 2 bits/pixel.
//  Even scanlines are from CGA_base + 0x0000, odd from CGA_base + 0x2000
//  Second plane at CGA_base + 0x4000 / 0x6000
//***************************************************************************/
//
//static void cga_pgfx_4bpp(bitmap_t *bitmap, struct mscrtc6845 *crtc)
//{
//	int i, sx, sy, sh;
//	int	offs = mscrtc6845_get_start(crtc)*2;
//	int lines = mscrtc6845_get_char_lines(crtc);
//	int height = mscrtc6845_get_char_height(crtc);
//	int columns = mscrtc6845_get_char_columns(crtc)*2;
//
//	for (sy=0; sy<lines; sy++,offs=(offs+columns)&0x1fff)
//	{
//		for (sh=0; sh<height; sh++, offs|=0x2000)
//		{
//			// char line 0 used as a12 line in graphic mode
//			if (!(sh & 1))
//			{
//				for (i=offs, sx=0; sx<columns; sx++, i=(i+1)&0x1fff)
//				{
//					pgfx_plot_unit_4bpp(bitmap, sx*4, sy*height+sh, i);
//				}
//			}
//			else
//			{
//				for (i=offs|0x2000, sx=0; sx<columns; sx++, i=((i+1)&0x1fff)|0x2000)
//				{
//					pgfx_plot_unit_4bpp(bitmap, sx*4, sy*height+sh, i);
//				}
//			}
//		}
//	}
//}
//
//
//
//INLINE void pgfx_plot_unit_2bpp(bitmap_t *bitmap,
//					 int x, int y, const UINT16 *palette, int offs)
//{
//	int i;
//	UINT8 bmap[2], values[2];
//	UINT16 *dest;
//
//	if (cga.plantronics & 0x40)
//	{
//		values[0] = videoram[offs];
//		values[1] = videoram[offs | 0x4000];
//	}
//	else
//	{
//		values[0] = videoram[offs | 0x4000];
//		values[1] = videoram[offs];
//	}
//	bmap[0] = bmap[1] = 0;
//	for (i=3; i>=0; i--)
//	{
//		bmap[0] = bmap[0] << 1; if (values[0] & 0x80) bmap[0] |= 1;
//		bmap[0] = bmap[0] << 1; if (values[1] & 0x80) bmap[0] |= 1;
//		bmap[1] = bmap[1] << 1; if (values[0] & 0x08) bmap[1] |= 1;
//		bmap[1] = bmap[1] << 1; if (values[1] & 0x08) bmap[1] |= 1;
//		values[0] = values[0] << 1;
//		values[1] = values[1] << 1;
//	}
//
//	dest = BITMAP_ADDR16(bitmap, y, x);
//	*(dest++) = palette[(bmap[0] >> 6) & 0x03];
//	*(dest++) = palette[(bmap[0] >> 4) & 0x03];
//	*(dest++) = palette[(bmap[0] >> 2) & 0x03];
//	*(dest++) = palette[(bmap[0] >> 0) & 0x03];
//	*(dest++) = palette[(bmap[1] >> 6) & 0x03];
//	*(dest++) = palette[(bmap[1] >> 4) & 0x03];
//	*(dest++) = palette[(bmap[1] >> 2) & 0x03];
//	*(dest++) = palette[(bmap[1] >> 0) & 0x03];
//}
//
//
//
///***************************************************************************
//  Draw graphics mode with 320x200 pixels (default) with 2 bits/pixel.
//  Even scanlines are from CGA_base + 0x0000, odd from CGA_base + 0x2000
//  cga fetches 2 byte per mscrtc6845 access (not modeled here)!
//***************************************************************************/
//
//static void cga_pgfx_2bpp(bitmap_t *bitmap, struct mscrtc6845 *crtc)
//{
//	int i, sx, sy, sh;
//	int	offs = mscrtc6845_get_start(crtc)*2;
//	int lines = mscrtc6845_get_char_lines(crtc);
//	int height = mscrtc6845_get_char_height(crtc);
//	int columns = mscrtc6845_get_char_columns(crtc)*2;
//	int colorset = cga.color_select & 0x3F;
//	const UINT16 *palette;
//
//	/* Most chipsets use bit 2 of the mode control register to
//	 * access a third palette. But not consistently. */
//	pc_cga_check_palette();
//	switch(CGA_CHIPSET)
//	{
//		/* The IBM Professional Graphics Controller behaves like
//		 * the PC1512, btw. */
//		case CGA_CHIPSET_PC1512:
//		if ((colorset < 32) && (cga.mode_control & 4)) colorset += 64;
//		break;
//
//		case CGA_CHIPSET_IBM:
//		case CGA_CHIPSET_PC200:
//		case CGA_CHIPSET_ATI:
//		case CGA_CHIPSET_PARADISE:
//		if (cga.mode_control & 4) colorset = (colorset & 0x1F) + 64;
//		break;
//	}
//
//
//	/* The fact that our palette is located in cga_colortable is a vestigial
//	 * aspect from when we were doing that ugly trick where drawgfx() would
//	 * handle graphics drawing.  Truthfully, we should probably be using
//	 * palette_set_color_rgb() here and not doing the palette lookup in the loop
//	 */
//	palette = &cga_colortable[256*2 + 16*2] + colorset * 4;
//
//	for (sy=0; sy<lines; sy++,offs=(offs+columns)&0x1fff) {
//
//		for (sh=0; sh<height; sh++)
//		{
//			if (!(sh&1)) { // char line 0 used as a12 line in graphic mode
//				for (i=offs, sx=0; sx<columns; sx++, i=(i+1)&0x1fff)
//				{
//					pgfx_plot_unit_2bpp(bitmap, sx*8, sy*height+sh, palette, i);
//				}
//			}
//			else
//			{
//				for (i=offs|0x2000, sx=0; sx<columns; sx++, i=((i+1)&0x1fff)|0x2000)
//				{
//					pgfx_plot_unit_2bpp(bitmap, sx*8, sy*height+sh, palette, i);
//				}
//			}
//		}
//	}
//}



// amstrad pc1512 video hardware
// mapping of the 4 planes into videoram
// (text data should be readable at videoram+0)
static const int videoram_offset[4]= { 0x0000, 0x4000, 0x8000, 0xC000 };


static const UINT8 mc6845_writeonce_register[31] = {
	1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


static struct
{
	UINT8	write;
	UINT8	read;
	UINT8	mc6845_address;
	UINT8	mc6845_locked_register[31];
} pc1512;


static MC6845_UPDATE_ROW( pc1512_gfx_4bpp_update_row ) {
	UINT16  *p = BITMAP_ADDR16(bitmap, y, 0);
	UINT16	offset_base = ra << 13;
	int i;

	if ( y == 0 ) logerror("pc1512_gfx_4bpp_update_row\n");
	for ( i = 0; i < x_count; i++ ) {
		UINT16 offset = offset_base | ( ( ma + i ) & 0x1FFF );
		UINT16 i = ( cga.color_select & 8 ) ? videoram[ videoram_offset[3] | offset ] << 3 : 0;
		UINT16 r = ( cga.color_select & 4 ) ? videoram[ videoram_offset[2] | offset ] << 2 : 0;
		UINT16 g = ( cga.color_select & 2 ) ? videoram[ videoram_offset[1] | offset ] << 1 : 0;
		UINT16 b = ( cga.color_select & 1 ) ? videoram[ videoram_offset[0] | offset ]      : 0;

		*p = ( ( i & 0x400 ) | ( r & 0x200 ) | ( g & 0x100 ) | ( b & 0x80 ) ) >> 7; p++;
		*p = ( ( i & 0x200 ) | ( r & 0x100 ) | ( g & 0x080 ) | ( b & 0x40 ) ) >> 6; p++;
		*p = ( ( i & 0x100 ) | ( r & 0x080 ) | ( g & 0x040 ) | ( b & 0x20 ) ) >> 5; p++;
		*p = ( ( i & 0x080 ) | ( r & 0x040 ) | ( g & 0x020 ) | ( b & 0x10 ) ) >> 4; p++;
		*p = ( ( i & 0x040 ) | ( r & 0x020 ) | ( g & 0x010 ) | ( b & 0x08 ) ) >> 3; p++;
		*p = ( ( i & 0x020 ) | ( r & 0x010 ) | ( g & 0x008 ) | ( b & 0x04 ) ) >> 2; p++;
		*p = ( ( i & 0x010 ) | ( r & 0x008 ) | ( g & 0x004 ) | ( b & 0x02 ) ) >> 1; p++;
		*p =   ( i & 0x008 ) | ( r & 0x004 ) | ( g & 0x002 ) | ( b & 0x01 )       ; p++;
	}
}


static WRITE8_HANDLER ( pc1512_w )
{
	device_config	*devconf;

	switch (offset) {
	case 0: case 2: case 4: case 6:
		devconf = (device_config *) device_list_find_by_tag(machine->config->devicelist, MC6845, CGA_MC6845_NAME);
		data &= 0x1F;
		mc6845_address_w( devconf, offset, data );
		pc1512.mc6845_address = data;
		break;

	case 1: case 3: case 5: case 7:
		devconf = (device_config *) device_list_find_by_tag(machine->config->devicelist, MC6845, CGA_MC6845_NAME);
		if ( ! pc1512.mc6845_locked_register[pc1512.mc6845_address] ) {
			mc6845_register_w( devconf, offset, data );
			if ( mc6845_writeonce_register[pc1512.mc6845_address] ) {
				pc1512.mc6845_locked_register[pc1512.mc6845_address] = 1;
			}
		}
		break;

	case 0x8:
		/* Check if we're changing to graphics mode 2 */
		if ( ( cga.mode_control & 0x12 ) != 0x12 && ( data & 0x12 ) == 0x12 ) {
			pc1512.write = 0x0F;
		} else {
			memory_set_bankptr(1, videoram + videoram_offset[0]);
		}
		cga.mode_control = data;
		switch( cga.mode_control & 0x3F ) {
		case 0x08: case 0x09: case 0x0C: case 0x0D:
			cga.update_row = cga_text_inten_update_row;
			break;
		case 0x0A: case 0x0B: case 0x2A: case 0x2B:
			if ( CGA_MONITOR == CGA_MONITOR_COMPOSITE ) {
				cga.update_row = cga_gfx_4bppl_update_row;
			} else {
				cga.update_row = cga_gfx_2bpp_update_row;
			}
			break;
		case 0x0E: case 0x0F: case 0x2E: case 0x2F:
			cga.update_row = cga_gfx_2bpp_update_row;
			break;
		case 0x18: case 0x19: case 0x1C: case 0x1D:
			cga.update_row = cga_text_inten_alt_update_row;
			break;
		case 0x1A: case 0x1B: case 0x3A: case 0x3B:
			cga.update_row = pc1512_gfx_4bpp_update_row;
			break;
		case 0x1E: case 0x1F: case 0x3E: case 0x3F:
			cga.update_row = cga_gfx_1bpp_update_row;
			break;
		case 0x28: case 0x29: case 0x2C: case 0x2D:
			cga.update_row = cga_text_blink_update_row;
			break;
		case 0x38: case 0x39: case 0x3C: case 0x3D:
			cga.update_row = cga_text_blink_alt_update_row;
			break;
		default:
			cga.update_row = NULL;
			break;
		}
		break;

	case 0xd:
		pc1512.write = data;
		break;

	case 0xe:
		pc1512.read = data;
		if ( ( cga.mode_control & 0x12 ) == 0x12 ) {
			memory_set_bankptr(1, videoram + videoram_offset[data & 3]);
		}
		break;

	default:
		pc_cga8_w(machine, offset,data);
		break;
	}
}

static READ8_HANDLER ( pc1512_r )
{
	UINT8 data;

	switch (offset) {
	case 0xd:
		data = pc1512.write;
		break;

	case 0xe:
		data = pc1512.read;
		break;

	default:
		data = pc_cga8_r(machine, offset);
		break;
	}
	return data;
}


static WRITE8_HANDLER ( pc1512_videoram_w )
{
	if ( ( cga.mode_control & 0x12 ) == 0x12 ) {
		if (pc1512.write & 1)
			videoram[offset+videoram_offset[0]] = data; /* blue plane */
		if (pc1512.write & 2)
			videoram[offset+videoram_offset[1]] = data; /* green */
		if (pc1512.write & 4)
			videoram[offset+videoram_offset[2]] = data; /* red */
		if (pc1512.write & 8)
			videoram[offset+videoram_offset[3]] = data; /* intensity (text, 4color) */
	} else {
		videoram[offset + videoram_offset[0]] = data;
	}
}



READ16_HANDLER ( pc1512_16le_r ) { return read16le_with_read8_handler(pc1512_r, machine, offset, mem_mask); }
WRITE16_HANDLER ( pc1512_16le_w ) { write16le_with_write8_handler(pc1512_w, machine, offset, data, mem_mask); }
WRITE16_HANDLER ( pc1512_videoram16le_w ) { write16le_with_write8_handler(pc1512_videoram_w, machine, offset, data, mem_mask); }



static VIDEO_START( pc1512 )
{
	videoram_size = 0x10000;
	videoram = auto_malloc( videoram_size );
	memory_set_bankptr(1,videoram + videoram_offset[0]);

	memset( &pc1512, 0, sizeof ( pc1512 ) );
	pc1512.write = 0xf;
	pc1512.read = 0;

	/* PC1512 cut-down 6845 */
	internal_pc_cga_video_start(M6845_PERSONALITY_PC1512);
}


static VIDEO_UPDATE( mc6845_pc1512 ) {
	device_config	*devconf = (device_config *) device_list_find_by_tag(screen->machine->config->devicelist, MC6845, CGA_MC6845_NAME);
	mc6845_update( devconf, bitmap, cliprect);

	/* Check for changes in font dipsetting */
	switch ( CGA_FONT & 0x03 ) {
	case 0:
		cga.chr_gen = memory_region(REGION_GFX1) + 0x0000;
		break;
	case 1:
		cga.chr_gen = memory_region(REGION_GFX1) + 0x0800;
		break;
	case 2:
		cga.chr_gen = memory_region(REGION_GFX1) + 0x1000;
		break;
	case 3:
		cga.chr_gen = memory_region(REGION_GFX1) + 0x1800;
		break;
	}
	return 0;
}


