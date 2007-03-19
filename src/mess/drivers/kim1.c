/******************************************************************************
	KIM-1

	system driver

	Juergen Buchmueller, Oct 1999

    KIM-1 memory map

    range     short     description
    0000-03FF RAM
    1400-16FF ???
    1700-173F 6530-003  see 6530
    1740-177F 6530-002  see 6530
    1780-17BF RAM       internal 6530-003
    17C0-17FF RAM       internal 6530-002
    1800-1BFF ROM       internal 6530-003
    1C00-1FFF ROM       internal 6530-002

    6530
    offset  R/W short   purpose
    0       X   DRA     Data register A
    1       X   DDRA    Data direction register A
    2       X   DRB     Data register B
    3       X   DDRB    Data direction register B
    4       W   CNT1T   Count down from value, divide by 1, disable IRQ
    5       W   CNT8T   Count down from value, divide by 8, disable IRQ
    6       W   CNT64T  Count down from value, divide by 64, disable IRQ
            R   LATCH   Read current counter value, disable IRQ
    7       W   CNT1KT  Count down from value, divide by 1024, disable IRQ
            R   STATUS  Read counter statzs, bit 7 = 1 means counter overflow
    8       X   DRA     Data register A
    9       X   DDRA    Data direction register A
    A       X   DRB     Data register B
    B       X   DDRB    Data direction register B
    C       W   CNT1I   Count down from value, divide by 1, enable IRQ
    D       W   CNT8I   Count down from value, divide by 8, enable IRQ
    E       W   CNT64I  Count down from value, divide by 64, enable IRQ
            R   LATCH   Read current counter value, enable IRQ
    F       W   CNT1KI  Count down from value, divide by 1024, enable IRQ
            R   STATUS  Read counter statzs, bit 7 = 1 means counter overflow

    6530-002 (U2)
        DRA bit write               read
        ---------------------------------------------
            0-6 segments A-G        key columns 1-7
            7   PTR                 KBD

        DRB bit write               read
        ---------------------------------------------
            0   PTR                 -/-
            1-4 dec 1-3 key row 1-3
                    4   RW3
                    5-9 7-seg   1-6
    6530-003 (U3)
        DRA bit write               read
        ---------------------------------------------
            0-7 bus PA0-7           bus PA0-7

        DRB bit write               read
        ---------------------------------------------
            0-7 bus PB0-7           bus PB0-7

******************************************************************************/

#include "driver.h"
#include "includes/kim1.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

#if VERBOSE
#define LOG(x)	if( errorlog ) fprintf x
#else
#define LOG(x)	/* x */
#endif

static ADDRESS_MAP_START ( kim1_map , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE(0x0000, 0x03ff) AM_MIRROR(0xe000) AM_RAM
	AM_RANGE(0x1700, 0x173f) AM_MIRROR(0xe000) AM_READWRITE( m6530_003_r, m6530_003_w )
	AM_RANGE(0x1740, 0x177f) AM_MIRROR(0xe000) AM_READWRITE( m6530_002_r, m6530_002_w )
	AM_RANGE(0x1780, 0x17bf) AM_MIRROR(0xe000) AM_RAM
	AM_RANGE(0x17c0, 0x17ff) AM_MIRROR(0xe000) AM_RAM
	AM_RANGE(0x1800, 0x1bff) AM_MIRROR(0xe000) AM_ROM
	AM_RANGE(0x1c00, 0x1fff) AM_MIRROR(0xe000) AM_ROM
ADDRESS_MAP_END

INPUT_PORTS_START( kim1 )
	PORT_START			/* IN0 keys row 0 */
	PORT_BIT (0x80, 0x00, IPT_UNUSED )
	PORT_BIT(0x40, 0x40, IPT_KEYBOARD) PORT_NAME("0.6: 0") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x20, 0x20, IPT_KEYBOARD) PORT_NAME("0.5: 1") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x10, 0x10, IPT_KEYBOARD) PORT_NAME("0.4: 2") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x08, 0x08, IPT_KEYBOARD) PORT_NAME("0.3: 3") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x04, 0x04, IPT_KEYBOARD) PORT_NAME("0.2: 4") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x02, 0x02, IPT_KEYBOARD) PORT_NAME("0.1: 5") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x01, 0x01, IPT_KEYBOARD) PORT_NAME("0.0: 6") PORT_CODE(KEYCODE_6)
	PORT_START			/* IN1 keys row 1 */
	PORT_BIT (0x80, 0x00, IPT_UNUSED )
	PORT_BIT(0x40, 0x40, IPT_KEYBOARD) PORT_NAME("1.6: 7") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x20, 0x20, IPT_KEYBOARD) PORT_NAME("1.5: 8") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x10, 0x10, IPT_KEYBOARD) PORT_NAME("1.4: 9") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x08, 0x08, IPT_KEYBOARD) PORT_NAME("1.3: A") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x04, 0x04, IPT_KEYBOARD) PORT_NAME("1.2: B") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x02, 0x02, IPT_KEYBOARD) PORT_NAME("1.1: C") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x01, 0x01, IPT_KEYBOARD) PORT_NAME("1.0: D") PORT_CODE(KEYCODE_D)
	PORT_START			/* IN2 keys row 2 */
	PORT_BIT (0x80, 0x00, IPT_UNUSED )
	PORT_BIT(0x40, 0x40, IPT_KEYBOARD) PORT_NAME("2.6: E") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x20, 0x20, IPT_KEYBOARD) PORT_NAME("2.5: F") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x10, 0x10, IPT_KEYBOARD) PORT_NAME("2.4: AD (F1)") PORT_CODE(KEYCODE_F1)
	PORT_BIT(0x08, 0x08, IPT_KEYBOARD) PORT_NAME("2.3: DA (F2)") PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x04, 0x04, IPT_KEYBOARD) PORT_NAME("2.2: +  (CR)") PORT_CODE(KEYCODE_ENTER)
	PORT_BIT(0x02, 0x02, IPT_KEYBOARD) PORT_NAME("2.1: GO (F5)") PORT_CODE(KEYCODE_F5)
	PORT_BIT(0x01, 0x01, IPT_KEYBOARD) PORT_NAME("2.0: PC (F6)") PORT_CODE(KEYCODE_F6)
	PORT_START			/* IN3 STEP and RESET keys, MODE switch */
	PORT_BIT (0x80, 0x00, IPT_UNUSED )
	PORT_BIT(0x40, 0x40, IPT_KEYBOARD) PORT_NAME("sw1: ST (F7)") PORT_CODE(KEYCODE_F7)
	PORT_BIT(0x20, 0x20, IPT_KEYBOARD) PORT_NAME("sw2: RS (F3)") PORT_CODE(KEYCODE_F3)
	PORT_BIT(0x10, 0x10, IPT_DIPSWITCH_NAME) PORT_NAME("sw3: SS (NumLock)") PORT_CODE(KEYCODE_NUMLOCK) PORT_TOGGLE
	PORT_DIPSETTING( 0x00, "single step")
	PORT_DIPSETTING( 0x10, "run")
	PORT_BIT (0x08, 0x00, IPT_UNUSED )
	PORT_BIT (0x04, 0x00, IPT_UNUSED )
	PORT_BIT (0x02, 0x00, IPT_UNUSED )
	PORT_BIT (0x01, 0x00, IPT_UNUSED )
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
	{ REGION_GFX1, 0, &led_layout, 0, 16 },
	{ REGION_GFX2, 0, &key_layout, 16*2, 2 },
	{ -1 } /* end of array */
};


static MACHINE_DRIVER_START( kim1 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", M6502, 1000000)        /* 1 MHz */
	MDRV_CPU_PROGRAM_MAP(kim1_map, 0)
	MDRV_CPU_VBLANK_INT(kim1_interrupt, 1)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_RESET( kim1 )

	/* video hardware (well, actually there was no video ;) */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(600, 768)
	MDRV_SCREEN_VISIBLE_AREA(0, 600 - 1, 0, 768 - 1)
	MDRV_GFXDECODE( gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(32768+21)
	MDRV_COLORTABLE_LENGTH(256)
	MDRV_PALETTE_INIT( kim1 )

	MDRV_VIDEO_START( kim1 )
	MDRV_VIDEO_UPDATE( kim1 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
MACHINE_DRIVER_END


ROM_START(kim1)
	ROM_REGION(0x10000,REGION_CPU1,0)
		ROM_LOAD("6530-003.bin",    0x1800, 0x0400, CRC(a2a56502) SHA1(60b6e48f35fe4899e29166641bac3e81e3b9d220))
		ROM_LOAD("6530-002.bin",    0x1c00, 0x0400, CRC(2b08e923) SHA1(054f7f6989af3a59462ffb0372b6f56f307b5362))
	ROM_REGION(128 * 24 * 3,REGION_GFX1,0)
		/* space filled with 7segement graphics by kim1_init_driver */
	ROM_REGION( 24 * 18 * 3 * 2,REGION_GFX2,0)
		/* space filled with key icons by kim1_init_driver */
ROM_END

SYSTEM_CONFIG_START(kim1)
	CONFIG_DEVICE(kim1_cassette_getinfo)
SYSTEM_CONFIG_END

/*    YEAR  NAME      PARENT    COMPAT	MACHINE   INPUT     INIT      CONFIG  COMPANY   FULLNAME */
COMP( 1975, kim1,	  0, 		0,		kim1,	  kim1, 	kim1,	  kim1,	  "MOS Technologies",  "KIM-1" , 0)
