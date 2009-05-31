/***************************************************************************

        Interact Family Computer

        12/05/2009 Skeleton driver - Micko
	31/05/2009 Added notes and video - Robbbert

	This was made by Interact Company of Ann Arbor, Michigan. However,
	just after launch, the company collapsed. The liquidator, Protecto,
	sold some and MicroVideo sold the rest. MicroVideo continued to
	develop but went under 2 years later. Meanwhile, the French company
	Lambda Systems sold a clone called the Victor Lambda. But, like the
	Americans, Lambda Systems also collapsed. Another French company,
	Micronique, purchased all remaining stock and intellectual rights
	from Lambda Systems, Microvideo and Interact, and the computer becomes
	wholly French. The computer has a name change, becoming the Hector.
	This in turn gets upgraded (2HR, HRX, MX). The line is finally
	retired in about 1985.

	TO DO:
	Keyboard
	Cassette
	SN76477 / Sound
	Colour
	Cartload

****************************************************************************/

#include "driver.h"
#include "cpu/i8085/i8085.h"
//#include "cpu/z80/z80.h"			// later Hector models used Z80A @ 4mhz

static READ8_HANDLER( interact_videoram_r )
{
	return videoram[offset];
}

static WRITE8_HANDLER( interact_videoram_w )
{
	videoram[offset] = data;
}

static ADDRESS_MAP_START(interact_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000,0x0fff) AM_ROM
	AM_RANGE(0x4000,0x47ff) AM_RAM AM_READWRITE(interact_videoram_r,interact_videoram_w) AM_SIZE(&videoram_size) AM_BASE(&videoram)
	AM_RANGE(0x4800,0x7fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( interact_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( interact )
INPUT_PORTS_END


static MACHINE_RESET(interact)
{
}

static VIDEO_START( interact )
{
}

static VIDEO_UPDATE( interact )
{
	UINT8 y,gfx;
	UINT16 sy=0,ma=0,x;

	for (y = 0; y < 64; y++)
	{
		UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

		for (x = ma; x < ma + 32; x++)
		{
			gfx = videoram[x];

			/* Display a scanline of a character (8 pixels) */
			*p = ( gfx & 0x01 ) ? 1 : 0; p++;
			*p = ( gfx & 0x02 ) ? 1 : 0; p++;
			*p = ( gfx & 0x04 ) ? 1 : 0; p++;
			*p = ( gfx & 0x08 ) ? 1 : 0; p++;
			*p = ( gfx & 0x10 ) ? 1 : 0; p++;
			*p = ( gfx & 0x20 ) ? 1 : 0; p++;
			*p = ( gfx & 0x40 ) ? 1 : 0; p++;
			*p = ( gfx & 0x80 ) ? 1 : 0; p++;
		}
		ma+=32;
	}
	return 0;
}

static MACHINE_DRIVER_START( interact )
	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu",8080, XTAL_2MHz)
	MDRV_CPU_PROGRAM_MAP(interact_mem)
	MDRV_CPU_IO_MAP(interact_io)

	MDRV_MACHINE_RESET(interact)

	/* video hardware */
	MDRV_SCREEN_ADD("screen", RASTER)
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(256, 79)
	MDRV_SCREEN_VISIBLE_AREA(0, 224, 0, 78)
	MDRV_PALETTE_LENGTH(2)				// 8 colours, but only 4 at a time
	MDRV_PALETTE_INIT(black_and_white)

	MDRV_VIDEO_START(interact)
	MDRV_VIDEO_UPDATE(interact)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(interact)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( interact )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "interact.rom", 0x0000, 0x0800, CRC(1aa50444) SHA1(405806c97378abcf7c7b0d549430c78c7fc60ba2))
ROM_END

ROM_START( hector1 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "hector1.rom",  0x0000, 0x1000, CRC(3be6628b) SHA1(1c106d6732bed743d8283d39e5b8248271f18c42))
ROM_END


/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG		 COMPANY   FULLNAME       FLAGS */
COMP(1979, interact, 0,        0,       interact, interact, 0,	interact,  	 "Interact",   "Interact Family Computer", GAME_NOT_WORKING)
COMP(????, hector1,  interact, 0, 	interact, interact, 0,  interact,  	 "Micronique", "Hector 1",	GAME_NOT_WORKING)

