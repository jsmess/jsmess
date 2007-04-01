/***************************************************************************

	systems/apple2gs.c
	Apple IIgs
	Driver by Nathan Woods and R. Belmont
	
    TODO:
    - Fix spurious interrupt problem
    - Fix 5.25" disks
    - Optimize video code
    - More RAM configurations

    NOTES:

    Video timing and the h/vcount registers:
    						      VCounts
    HCounts go like this:				      0xfa (start of frame, still in vblank)
    0 0x40 0x41 0x58 (first visible pixel)        0x7f
                 ____________________________________     0x100 (first visible scan line)
                |                                    |
                |                                    |
                |                                    |
                |                                    |
                |                                    |
    HBL region  |                                    |
                |                                    |
                |                                    |
                |                                    |
                |                                    |
                |                                    |   0x1ba (first line of Vblank, c019 and heartbeat trigger here, only true VBL if in A2 classic modes) 
                |                                    |
                 ____________________________________    0x1c8 (actual start of vblank in IIgs modes)
    
    						     0x1ff (end of frame, in vblank)
    
    There are 64 HCounts total, and 704 pixels total, so HCounts do not map to the pixel clock.
    VCounts do map directly to scanlines however, and count 262 of them.

=================================================================

***************************************************************************/

#include "driver.h"
#include "state.h"
#include "inputx.h"
#include "video/generic.h"
#include "includes/apple2.h"
#include "machine/ay3600.h"
#include "devices/mflopimg.h"
#include "formats/ap2_dsk.h"
#include "includes/apple2gs.h"
#include "devices/sonydriv.h"
#include "devices/appldriv.h"
#include "sound/es5503.h"

static gfx_layout apple2gs_text_layout =
{
	14,8,		/* 14*8 characters */
	512,		/* 256 characters */
	1,			/* 1 bits per pixel */
	{ 0 },		/* no bitplanes; 1 bit per pixel */
	{ 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1 },   /* x offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8			/* every char takes 8 bytes */
};

static gfx_layout apple2gs_dbltext_layout =
{
	7,8,		/* 7*8 characters */
	512,		/* 256 characters */
	1,			/* 1 bits per pixel */
	{ 0 },		/* no bitplanes; 1 bit per pixel */
	{ 7, 6, 5, 4, 3, 2, 1 },    /* x offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8			/* every char takes 8 bytes */
};

static gfx_decode apple2gs_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0x0000, &apple2gs_text_layout, 0, 2 },
	{ REGION_GFX1, 0x0000, &apple2gs_dbltext_layout, 0, 2 },
	{ -1 } /* end of array */
};



static const unsigned char apple2gs_palette[] =
{
	0x00, 0x00, 0x00,	/* Black */
	0xD0, 0x00, 0x30,	/* Dark Red */
	0x00, 0x00, 0x90,	/* Dark Blue */
	0xD0, 0x20, 0xD0,	/* Purple */
	0x00, 0x70, 0x20,	/* Dark Green */
	0x50, 0x50, 0x50,	/* Dark Grey */
	0x20, 0x20, 0xF0,	/* Medium Blue */
	0x60, 0xA0, 0xF0,	/* Light Blue */
	0x80, 0x50, 0x00,	/* Brown */
	0xF0, 0x60, 0x00,	/* Orange */
	0xA0, 0xA0, 0xA0,	/* Light Grey */
	0xF0, 0x90, 0x80,	/* Pink */
	0x10, 0xD0, 0x00,	/* Light Green */
	0xF0, 0xF0, 0x00,	/* Yellow */
	0x40, 0xF0, 0x90,	/* Aquamarine */
	0xF0, 0xF0, 0xF0	/* White */
};

UINT8 apple2gs_docram[64*1024];

MACHINE_DRIVER_EXTERN( apple2e );
INPUT_PORTS_EXTERN( apple2ep );

INPUT_PORTS_START( apple2gs )
	PORT_INCLUDE( apple2ep )

	PORT_START_TAG("adb_mouse_x")
	PORT_BIT( 0x7f, 0x00, IPT_MOUSE_X) PORT_SENSITIVITY(100) PORT_KEYDELTA(0)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1) PORT_CODE(MOUSECODE_1_BUTTON2) PORT_NAME("Mouse Button 1")

	PORT_START_TAG("adb_mouse_y")
	PORT_BIT( 0x7f, 0x00, IPT_MOUSE_Y) PORT_SENSITIVITY(100) PORT_KEYDELTA(0)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2) PORT_CODE(MOUSECODE_1_BUTTON1) PORT_NAME("Mouse Button 0")

INPUT_PORTS_END



/* Initialize the palette */
static PALETTE_INIT( apple2gs )
{
	extern PALETTE_INIT( apple2 );
	palette_init_apple2(machine, colortable, color_prom);
	palette_set_colors(machine, 0, apple2gs_palette, sizeof(apple2gs_palette) / 3);
}

static READ8_HANDLER( apple2gs_adc_read )
{
	return 0x80;
}

static struct ES5503interface es5503_interface = 
{
	apple2gs_doc_irq,
	apple2gs_adc_read,
	apple2gs_docram
};

static MACHINE_DRIVER_START( apple2gs )
	MDRV_IMPORT_FROM( apple2e )
	MDRV_CPU_REPLACE("main", G65816, APPLE2GS_14M/5)

	MDRV_SCREEN_SIZE(704, 262)	// 640+32+32 for the borders
	MDRV_SCREEN_VISIBLE_AREA(0,703,0,230)
	MDRV_PALETTE_LENGTH( 16+256 )
	MDRV_GFXDECODE( apple2gs_gfxdecodeinfo )

	MDRV_MACHINE_START( apple2gs )
	MDRV_MACHINE_RESET( apple2gs )

	MDRV_PALETTE_INIT( apple2gs )
	MDRV_VIDEO_START( apple2gs )
	MDRV_VIDEO_UPDATE( apple2gs )

	MDRV_NVRAM_HANDLER( apple2gs )

	MDRV_SOUND_REPLACE("A2DAC", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(ES5503, APPLE2GS_7M)
	MDRV_SOUND_CONFIG(es5503_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START(apple2gs)
	ROM_REGION(0x1000,REGION_GFX1,0)
	ROM_LOAD ( "apple2gs.chr", 0x0000, 0x1000, CRC(91e53cd8) SHA1(34e2443e2ef960a36c047a09ed5a93f471797f89))

	ROM_REGION(0x40000,REGION_CPU1,0)
	ROM_LOAD("rom03", 0x0000, 0x40000, CRC(de7ddf29) SHA1(bc32bc0e8902946663998f56aea52be597d9e361))
ROM_END

ROM_START(apple2g1)
	ROM_REGION(0x1000,REGION_GFX1,0)
	ROM_LOAD ( "apple2gs.chr", 0x0000, 0x1000, CRC(91e53cd8) SHA1(34e2443e2ef960a36c047a09ed5a93f471797f89))

	ROM_REGION(0x20000,REGION_CPU1,0)
	ROM_LOAD("rom01", 0x0000, 0x20000, CRC(42f124b0) SHA1(e4fc7560b69d062cb2da5b1ffbe11cd1ca03cc37))
ROM_END

ROM_START(apple2g0)
	ROM_REGION(0x1000,REGION_GFX1,0)
	ROM_LOAD ( "apple2gs.chr", 0x0000, 0x1000, CRC(91e53cd8) SHA1(34e2443e2ef960a36c047a09ed5a93f471797f89))

	ROM_REGION(0x20000,REGION_CPU1,0)
	ROM_LOAD("rom0a.bin", 0x0000,  0x8000, CRC(9cc78238) SHA1(0ea82e10720a01b68722ab7d9f66efec672a44d3))
	ROM_LOAD("rom0b.bin", 0x8000,  0x8000, CRC(8baf2a79) SHA1(91beeb11827932fe10475252d8036a63a2edbb1c))
	ROM_LOAD("rom0c.bin", 0x10000, 0x8000, CRC(94c32caa) SHA1(4806d50d676b06f5213b181693fc1585956b98bb))
	ROM_LOAD("rom0d.bin", 0x18000, 0x8000, CRC(200a15b8) SHA1(0c2890bb169ead63369738bbd5f33b869f24c42a))
ROM_END



/* -----------------------------------------------------------------------
 * Floppy disk devices
 * ----------------------------------------------------------------------- */

static void apple2gs_floppy35_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* 3.5" floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_SONYDRIV_ALLOWABLE_SIZES:		info->i = SONY_FLOPPY_ALLOW400K | SONY_FLOPPY_ALLOW800K | SONY_FLOPPY_SUPPORT2IMG; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME+0:						strcpy(info->s = device_temp_str(), "slot5disk1"); break;
		case DEVINFO_STR_NAME+1:						strcpy(info->s = device_temp_str(), "slot5disk2"); break;
		case DEVINFO_STR_SHORT_NAME+0:					strcpy(info->s = device_temp_str(), "s5d1"); break;
		case DEVINFO_STR_SHORT_NAME+1:					strcpy(info->s = device_temp_str(), "s5d2"); break;
		case DEVINFO_STR_DESCRIPTION+0:					strcpy(info->s = device_temp_str(), "Slot 5 Disk #1"); break;
		case DEVINFO_STR_DESCRIPTION+1:					strcpy(info->s = device_temp_str(), "Slot 5 Disk #2"); break;

		default:										sonydriv_device_getinfo(devclass, state, info); break;
	}
}



static void apple2gs_floppy525_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* 5.25" floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_NOT_WORKING:					/* info->i = 1; */ break;
		case DEVINFO_INT_APPLE525_SPINFRACT_DIVIDEND:	info->i = 15; break;
		case DEVINFO_INT_APPLE525_SPINFRACT_DIVISOR:	info->i = 16; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_NAME+0:						strcpy(info->s = device_temp_str(), "slot6disk1"); break;
		case DEVINFO_STR_NAME+1:						strcpy(info->s = device_temp_str(), "slot6disk2"); break;
		case DEVINFO_STR_SHORT_NAME+0:					strcpy(info->s = device_temp_str(), "s6d1"); break;
		case DEVINFO_STR_SHORT_NAME+1:					strcpy(info->s = device_temp_str(), "s6d2"); break;
		case DEVINFO_STR_DESCRIPTION+0:					strcpy(info->s = device_temp_str(), "Slot 6 Disk #1"); break;
		case DEVINFO_STR_DESCRIPTION+1:					strcpy(info->s = device_temp_str(), "Slot 6 Disk #2"); break;

		default:										apple525_device_getinfo(devclass, state, info); break;
	}
}

static DRIVER_INIT(apple2gs)
{
	state_save_register_global_array(apple2gs_docram);
}

/* ----------------------------------------------------------------------- */

SYSTEM_CONFIG_START(apple2gs)
	CONFIG_DEVICE(apple2gs_floppy35_getinfo)
	CONFIG_DEVICE(apple2gs_floppy525_getinfo)

	CONFIG_RAM_DEFAULT			(2 * 1024 * 1024)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT    COMPAT  MACHINE   INPUT       INIT      CONFIG		COMPANY            FULLNAME */
COMP( 1989, apple2gs, 0,        apple2, apple2gs, apple2gs,   apple2gs, apple2gs,	"Apple Computer", "Apple IIgs (ROM03)", 0 )
COMP( 1987, apple2g1, apple2gs, 0,      apple2gs, apple2gs,   apple2gs, apple2gs,	"Apple Computer", "Apple IIgs (ROM01)", 0 )
COMP( 1986, apple2g0, apple2gs, 0,      apple2gs, apple2gs,   apple2gs, apple2gs,	"Apple Computer", "Apple IIgs (ROM00)",			GAME_NOT_WORKING )
