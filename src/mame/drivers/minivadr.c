/***************************************************************************

Minivader (Space Invaders's mini game)
(c)1990 Taito Corporation

Driver by Takahiro Nogi (nogi@kt.rim.or.jp) 1999/12/19 -

This is a test board sold together with the cabinet (as required by law in
Japan). It has no sound.

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"


WRITE8_HANDLER( minivadr_videoram_w );
VIDEO_UPDATE( minivadr );
PALETTE_INIT( minivadr );


static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xa000, 0xbfff) AM_READ(MRA8_RAM)
	AM_RANGE(0xe008, 0xe008) AM_READ(input_port_0_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xa000, 0xbfff) AM_WRITE(minivadr_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0xe008, 0xe008) AM_WRITE(MWA8_NOP)		// ???
ADDRESS_MAP_END


INPUT_PORTS_START( minivadr )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END


static MACHINE_DRIVER_START( minivadr )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,24000000 / 6)		 /* 4 MHz ? */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 256-1, 16, 240-1)
	MDRV_PALETTE_LENGTH(2)

	MDRV_PALETTE_INIT(minivadr)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(minivadr)

	/* sound hardware */
MACHINE_DRIVER_END


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( minivadr )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "d26-01.bin",	0x0000, 0x2000, CRC(a96c823d) SHA1(aa9969ff80e94b0fff0f3530863f6b300510162e) )
ROM_END


GAME( 1990, minivadr, 0, minivadr, minivadr, 0, ROT0, "Taito Corporation", "Minivader", GAME_SUPPORTS_SAVE )
