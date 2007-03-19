/******************************************************************************
	Motorola Evaluation Kit 6800 D2
	MEK6800D2

	system driver

	Juergen Buchmueller <pullmoll@t-online.de>, Jan 2000

	memory map

	range		short	description
	0000-00ff	RAM 	256 bytes RAM
	0100-01ff	RAM 	optional 256 bytes RAM
	6000-63ff	PROM	optional PROM
	or
	6000-67ff	ROM 	optional ROM
	8004-8007	PIA
	8008		ACIA	cassette interface
	8020-8023	PIA 	keyboard interface
	a000-a07f	RAM 	128 bytes RAM (JBUG scratch)
	c000-c3ff	PROM	optional PROM
	or
	c000-c7ff	ROM 	optional ROM
	e000-e3ff	ROM 	JBUG monitor program
	e400-ffff	-/- 	not used

******************************************************************************/

#include "driver.h"
#include "video/generic.h"
#include "devices/cartslot.h"
#include "includes/mekd2.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

#if VERBOSE
#define LOG(x)	logerror(x)
#else
#define LOG(x)	/* x */
#endif

static  READ8_HANDLER(mekd2_pia_r) { return 0xff; }
static  READ8_HANDLER(mekd2_cas_r) { return 0xff; }
static  READ8_HANDLER(mekd2_kbd_r) { return 0xff; }

UINT8 pia[8];

static WRITE8_HANDLER(mekd2_pia_w) { }
static WRITE8_HANDLER(mekd2_cas_w) { }

static WRITE8_HANDLER(mekd2_kbd_w)
{
	pia[offset] = data;
	switch( offset )
	{
	case 2:
		if( data & 0x20 )
		{
			videoram[0*2+0] = ~pia[0];
			videoram[0*2+1] = 14;
		}
		if( data & 0x10 )
		{
			videoram[1*2+0] = ~pia[0];
			videoram[1*2+1] = 14;
		}
		if( data & 0x08 )
		{
			videoram[2*2+0] = ~pia[0];
			videoram[2*2+1] = 14;
		}
		if( data & 0x04 )
		{
			videoram[3*2+0] = ~pia[0];
			videoram[3*2+1] = 14;
		}
		if( data & 0x02 )
		{
			videoram[4*2+0] = ~pia[0];
			videoram[4*2+1] = 14;
		}
		if( data & 0x01 )
		{
			videoram[5*2+0] = ~pia[0];
			videoram[5*2+1] = 14;
		}
		break;
	}
}


static ADDRESS_MAP_START( mekd2_mem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x00ff) AM_RAM
//	AM_RANGE(0x0100, 0x01ff) AM_RAM						/* optional, set up in mekd2_init_machine */
//	AM_RANGE(0x6000, 0x67ff) AM_ROM						/* -"- */
//	AM_RANGE(0x8004, 0x8007) AM_READWRITE(mekd2_pia_r, mekd2_pia_w)
//	AM_RANGE(0x8008, 0x8008) AM_READWRITE(mekd2_cas_r, mekd2_cas_w)
	AM_RANGE(0x8020, 0x8023) AM_WRITE(mekd2_kbd_w)		/* mekd2_kbd_r */
	AM_RANGE(0xa000, 0xa07f) AM_RAM
//	AM_RANGE(0xc000, 0xc7ff) AM_RAM						/* optional, set up in mekd2_init_machine */
	AM_RANGE(0xe000, 0xe3ff) AM_ROM AM_MIRROR(0x1c00)	/* JBUG ROM */
ADDRESS_MAP_END

INPUT_PORTS_START( mekd2 )
	PORT_START			/* IN0 keys row 0 */
INPUT_PORTS_END

static gfx_layout led_layout =
{
	18, 24, 	/* 16 x 24 LED 7segment displays */
	128,		/* 128 codes */
	1,			/* 1 bit per pixel */
	{ 0 },		/* no bitplanes */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  8, 9,10,11,12,13,14,15,
	 16,17 },
	{ 0*24, 1*24, 2*24, 3*24,
	  4*24, 5*24, 6*24, 7*24,
	  8*24, 9*24,10*24,11*24,
	 12*24,13*24,14*24,15*24,
	 16*24,17*24,18*24,19*24,
	 20*24,21*24,22*24,23*24,
	 24*24,25*24,26*24,27*24,
	 28*24,29*24,30*24,31*24 },
	24 * 24,	/* every LED code takes 32 times 18 (aligned 24) bit words */
};

static gfx_layout key_layout =
{
	24, 18, 	/* 24 * 18 keyboard icons */
	24, 		/* 24  codes */
	2,			/* 2 bit per pixel */
	{ 0, 1 },	/* two bitplanes */
	{ 0*2, 1*2, 2*2, 3*2, 4*2, 5*2, 6*2, 7*2,
	  8*2, 9*2,10*2,11*2,12*2,13*2,14*2,15*2,
	 16*2,17*2,18*2,19*2,20*2,21*2,22*2,23*2 },
	{ 0*24*2, 1*24*2, 2*24*2, 3*24*2, 4*24*2, 5*24*2, 6*24*2, 7*24*2,
	  8*24*2, 9*24*2,10*24*2,11*24*2,12*24*2,13*24*2,14*24*2,15*24*2,
	 16*24*2,17*24*2 },
	18 * 24 * 2,	/* every icon takes 18 rows of 24 * 2 bits */
};

static gfx_decode gfxdecodeinfo[] =
{
	{ 1, 0, &led_layout, 0, 16 },
	{ 2, 0, &key_layout, 16*2, 2 },
	{ -1 } /* end of array */
};



static MACHINE_DRIVER_START( mekd2 )
	/* basic machine hardware */
	MDRV_CPU_ADD(M6800, 614400)        /* 614.4 kHz */
	MDRV_CPU_PROGRAM_MAP(mekd2_mem, 0)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(600, 768)
	MDRV_SCREEN_VISIBLE_AREA(0, 600-1, 0, 768-1)
	MDRV_GFXDECODE( gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(21 + 32768)
	MDRV_COLORTABLE_LENGTH(256)
	MDRV_PALETTE_INIT( mekd2 )

	MDRV_VIDEO_START( mekd2 )
	MDRV_VIDEO_UPDATE( mekd2 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END


ROM_START(mekd2)
	ROM_REGION(0x10000,REGION_CPU1,0)
		ROM_LOAD("jbug.rom",    0xe000, 0x0400, CRC(a2a56502) SHA1(60b6e48f35fe4899e29166641bac3e81e3b9d220))
	ROM_REGION(128 * 24 * 3,REGION_GFX1,0)
		/* space filled with 7segement graphics by mekd2_init_driver */
	ROM_REGION( 24 * 18 * 3 * 2,REGION_GFX2,0)
		/* space filled with key icons by mekd2_init_driver */
ROM_END

static void mekd2_cartslot_getinfo(const device_class *devclass, UINT32 state, union devinfo *info)
{
	/* cartslot */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case DEVINFO_INT_COUNT:							info->i = 1; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case DEVINFO_PTR_LOAD:							info->load = device_load_mekd2_cart; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "d2"); break;

		default:										cartslot_device_getinfo(devclass, state, info); break;
	}
}

SYSTEM_CONFIG_START(mekd2)
	CONFIG_DEVICE(mekd2_cartslot_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*	  YEAR	NAME	PARENT	COMPAT	MACHINE   INPUT 	INIT	CONFIG	COMPANY		FULLNAME */
CONS( 1977, mekd2,	0,		0,		mekd2,	  mekd2,	mekd2,	mekd2,	"Motorola",	"MEK6800D2" , 0)
