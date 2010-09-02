/***************************************************************************

        Cromemco C-10 Personal Computer

        30/08/2010 Skeleton driver

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

static UINT8 *c10_ram;

static ADDRESS_MAP_START(c10_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_RAM AM_BASE(&c10_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( c10_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( c10 )
INPUT_PORTS_END

static MACHINE_RESET(c10)
{
	UINT8* bios = memory_region(machine, "maincpu");

	memcpy(c10_ram,bios, 0x4000);
	memcpy(c10_ram+0x8000,bios, 0x4000);
}

static VIDEO_START( c10 )
{
}

static VIDEO_UPDATE( c10 )
{
	return 0;
}

/* F4 Character Displayer */
static const gfx_layout c10_charlayout =
{
	8, 9,					/* 8 x 9 characters */
	512,					/* 512 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8 },
	8*16					/* every char takes 16 bytes */
};

static GFXDECODE_START( c10 )
	GFXDECODE_ENTRY( "chargen", 0x0000, c10_charlayout, 0, 1 )
GFXDECODE_END


static MACHINE_CONFIG_START( c10, driver_device )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, XTAL_16MHz / 4)
	MDRV_CPU_PROGRAM_MAP(c10_mem)
	MDRV_CPU_IO_MAP(c10_io)

	MDRV_MACHINE_RESET(c10)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 192) /* border size not accurate */
	MDRV_SCREEN_VISIBLE_AREA(0, 256 - 1, 0, 192 - 1)
	MDRV_GFXDECODE(c10)

	MDRV_PALETTE_LENGTH(2)
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(c10)
	MDRV_VIDEO_UPDATE(c10)
MACHINE_CONFIG_END


/* ROM definition */
ROM_START( c10 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "c10_cros.bin", 0x0000, 0x4000, CRC(2ccf5983) SHA1(52f7c497f5284bf5df9eb0d6e9142bb1869d8c24))
	ROM_REGION( 0x2000, "chargen", 0 )
	ROM_LOAD( "c10_char.bin", 0x0000, 0x2000, CRC(cb530b6f) SHA1(95590bbb433db9c4317f535723b29516b9b9fcbf))
ROM_END

/* Driver */

/*   YEAR  NAME    PARENT  COMPAT   MACHINE  INPUT  INIT        COMPANY   FULLNAME       FLAGS */
COMP( 1982, c10,  0,       0,	c10,	c10,	 0, 	  "Cromemco",   "C-10",		GAME_NOT_WORKING | GAME_NO_SOUND)
