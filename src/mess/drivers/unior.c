/***************************************************************************

        Unior

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"

static ADDRESS_MAP_START(unior_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000,0xf7ff) AM_RAM
	AM_RANGE(0xf800,0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( unior_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( unior )
INPUT_PORTS_END


static MACHINE_RESET(unior)
{
}

static VIDEO_START( unior )
{
}

static VIDEO_UPDATE( unior )
{
    return 0;
}

/* F4 Character Displayer */
static const gfx_layout unior_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	256,					/* 256 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{  0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static GFXDECODE_START( unior )
	GFXDECODE_ENTRY( "gfx1", 0x0000, unior_charlayout, 0, 1 )
GFXDECODE_END

static MACHINE_CONFIG_START( unior, driver_device )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",I8080, 2222222)
    MDRV_CPU_PROGRAM_MAP(unior_mem)
    MDRV_CPU_IO_MAP(unior_io)

    MDRV_MACHINE_RESET(unior)

    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MDRV_GFXDECODE(unior)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(unior)
    MDRV_VIDEO_UPDATE(unior)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( unior )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "unior.rom", 0xf800, 0x0800, CRC(23a347e8) SHA1(2ef3134e2f4a696c3b52a145fa5a2d4c3487194b))
    ROM_REGION( 0x0840, "gfx1", ROMREGION_ERASEFF )
	ROM_LOAD( "unior.fnt",   0x0000, 0x0800, CRC(4f654828) SHA1(8c0ac11ea9679a439587952e4908940b67c4105e))
	ROM_LOAD( "palette.rom", 0x0800, 0x0040, CRC(b4574ceb) SHA1(f7a82c61ab137de8f6a99b0c5acf3ac79291f26a))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 19??, unior,  radio86,       0,	unior,	unior,	 0, 	 "<unknown>",   "Unior",		GAME_NOT_WORKING | GAME_NO_SOUND)
