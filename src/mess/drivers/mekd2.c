/******************************************************************************
    Motorola Evaluation Kit 6800 D2
    MEK6800D2

    system driver

    Juergen Buchmueller <pullmoll@t-online.de>, Jan 2000

    memory map

    range       short   description
    0000-00ff   RAM     256 bytes RAM
    0100-01ff   RAM     optional 256 bytes RAM
    6000-63ff   PROM    optional PROM
    or
    6000-67ff   ROM     optional ROM
    8004-8007   PIA
    8008        ACIA    cassette interface
    8020-8023   PIA     keyboard interface
    a000-a07f   RAM     128 bytes RAM (JBUG scratch)
    c000-c3ff   PROM    optional PROM
    or
    c000-c7ff   ROM     optional ROM
    e000-e3ff   ROM     JBUG monitor program
    e400-ffff   -/-     not used

******************************************************************************/

#include "emu.h"
#include "cpu/m6800/m6800.h"
#include "sound/dac.h"
#include "devices/cartslot.h"
#include "includes/mekd2.h"

#ifdef UNUSED_FUNCTION
static  READ8_HANDLER(mekd2_pia_r) { return 0xff; }
static  READ8_HANDLER(mekd2_cas_r) { return 0xff; }
static  READ8_HANDLER(mekd2_kbd_r) { return 0xff; }
#endif

static UINT8 pia[8];

#ifdef UNUSED_FUNCTION
static WRITE8_HANDLER(mekd2_pia_w) { }
static WRITE8_HANDLER(mekd2_cas_w) { }
#endif

static WRITE8_HANDLER(mekd2_kbd_w)
{
	UINT8 *videoram = space->machine->generic.videoram.u8;
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
//  AM_RANGE(0x0100, 0x01ff) AM_RAM                     /* optional, set up in mekd2_init_machine */
//  AM_RANGE(0x6000, 0x67ff) AM_ROM                     /* -"- */
//  AM_RANGE(0x8004, 0x8007) AM_READWRITE(mekd2_pia_r, mekd2_pia_w)
//  AM_RANGE(0x8008, 0x8008) AM_READWRITE(mekd2_cas_r, mekd2_cas_w)
	AM_RANGE(0x8020, 0x8023) AM_WRITE(mekd2_kbd_w)		/* mekd2_kbd_r */
	AM_RANGE(0xa000, 0xa07f) AM_RAM
//  AM_RANGE(0xc000, 0xc7ff) AM_RAM                     /* optional, set up in mekd2_init_machine */
	AM_RANGE(0xe000, 0xe3ff) AM_ROM AM_MIRROR(0x1c00)	/* JBUG ROM */
ADDRESS_MAP_END

/* 2008-05 FP: Among the 8 Unknown inputs there are 4 command keys, the other inputs being unused:
  - P : Punch data from memory to magnetic tape
  - L : Load memory from magnetic tape
  - N : Trace one instruction
  - V : Set (and remove) breakpoints

The keys are laid out as:

  P L N V

  7 8 9 A  M
  4 5 6 B  E
  1 2 3 C  R
  0 F E D  G

 */
static INPUT_PORTS_START( mekd2 )
	PORT_START("KEY0")
	PORT_BIT(0x80, 0x80, IPT_UNUSED)
	PORT_BIT(0x40, 0x40, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
	PORT_BIT(0x20, 0x20, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)
	PORT_BIT(0x10, 0x10, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
	PORT_BIT(0x08, 0x08, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)
	PORT_BIT(0x04, 0x04, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)
	PORT_BIT(0x02, 0x02, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
	PORT_BIT(0x01, 0x01, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)

	PORT_START("KEY1")
	PORT_BIT(0x80, 0x80, IPT_UNUSED)
	PORT_BIT(0x40, 0x40, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
	PORT_BIT(0x20, 0x20, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
	PORT_BIT(0x10, 0x10, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
	PORT_BIT(0x08, 0x08, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
	PORT_BIT(0x04, 0x04, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
	PORT_BIT(0x02, 0x02, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)
	PORT_BIT(0x01, 0x01, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)

	PORT_START("KEY2")
	PORT_BIT(0x80, 0x80, IPT_UNUSED)
	PORT_BIT(0x40, 0x40, IPT_KEYBOARD) PORT_NAME("E (hex)") PORT_CODE(KEYCODE_E)
	PORT_BIT(0x20, 0x20, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
	PORT_BIT(0x10, 0x10, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
	PORT_BIT(0x08, 0x08, IPT_KEYBOARD) PORT_NAME("E (escape)") PORT_CODE(KEYCODE_H)
	PORT_BIT(0x04, 0x04, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)
	PORT_BIT(0x02, 0x02, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
	PORT_BIT(0x01, 0x01, IPT_KEYBOARD) PORT_NAME("Unknown 1")

	PORT_START("KEY3")
	PORT_BIT(0x80, 0x80, IPT_UNUSED)
	PORT_BIT(0x40, 0x40, IPT_KEYBOARD) PORT_NAME("Unknown 2")
	PORT_BIT(0x20, 0x20, IPT_KEYBOARD) PORT_NAME("Unknown 3") // this one corresponds to the 'square' key in the current code
	PORT_BIT(0x10, 0x10, IPT_KEYBOARD) PORT_NAME("Unknown 4")
	PORT_BIT(0x08, 0x08, IPT_KEYBOARD) PORT_NAME("Unknown 5")
	PORT_BIT(0x04, 0x04, IPT_KEYBOARD) PORT_NAME("Unknown 6")
	PORT_BIT(0x02, 0x02, IPT_KEYBOARD) PORT_NAME("Unknown 7")
	PORT_BIT(0x01, 0x01, IPT_KEYBOARD) PORT_NAME("Unknown 8")
INPUT_PORTS_END

static const gfx_layout led_layout =
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

static const gfx_layout key_layout =
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

static GFXDECODE_START( mekd2 )
	GFXDECODE_ENTRY( "gfx1", 0, led_layout, 0, 16 )
	GFXDECODE_ENTRY( "gfx2", 0, key_layout, 16*2, 2 )
GFXDECODE_END

static MACHINE_CONFIG_START( mekd2, driver_device )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", M6800, 614400)        /* 614.4 kHz */
	MDRV_CPU_PROGRAM_MAP(mekd2_mem)
	MDRV_QUANTUM_TIME(HZ(60))

    /* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(600, 768)
	MDRV_SCREEN_VISIBLE_AREA(0, 600-1, 0, 768-1)
	MDRV_GFXDECODE( mekd2 )
	MDRV_PALETTE_LENGTH(40)
	MDRV_PALETTE_INIT( mekd2 )

	MDRV_VIDEO_START( mekd2 )
	MDRV_VIDEO_UPDATE( mekd2 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD("dac", DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_CARTSLOT_ADD("cart")
	MDRV_CARTSLOT_EXTENSION_LIST("d2")
	MDRV_CARTSLOT_NOT_MANDATORY
	MDRV_CARTSLOT_LOAD(mekd2_cart)
MACHINE_CONFIG_END


ROM_START(mekd2)
	ROM_REGION(0x10000,"maincpu",0)
		ROM_LOAD("jbug.rom",    0xe000, 0x0400, CRC(a2a56502) SHA1(60b6e48f35fe4899e29166641bac3e81e3b9d220))
	ROM_REGION(128 * 24 * 3,"gfx1",ROMREGION_ERASEFF)
		/* space filled with 7segement graphics by mekd2_init_driver */
	ROM_REGION( 24 * 18 * 3 * 2,"gfx2",ROMREGION_ERASEFF)
		/* space filled with key icons by mekd2_init_driver */
ROM_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME    PARENT  COMPAT  MACHINE   INPUT     INIT    COMPANY     FULLNAME */
CONS( 1977, mekd2,	0,		0,		mekd2,	  mekd2,	mekd2,	"Motorola",	"MEK6800D2" , GAME_NOT_WORKING )
