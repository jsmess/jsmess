/***************************************************************************
   
        Bandai Gundam RX-78

        13/07/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(rx78_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0xffff) AM_RAM AM_REGION("maincpu", 0x2000)
ADDRESS_MAP_END

static ADDRESS_MAP_START( rx78_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( rx78 )
INPUT_PORTS_END


static MACHINE_RESET(rx78) 
{	
}

static VIDEO_START( rx78 )
{
}

static VIDEO_UPDATE( rx78 )
{
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0xe8be,x;
	UINT8 *RAM = memory_region(screen->machine, "maincpu");

	for (y = 0; y < 23; y++)
	{
		for (ra = 0; ra < 8; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma+30; x++)
			{
				chr = RAM[x];

				/* get pattern of pixels for that character scanline */
				gfx = RAM[0x1a27 + (chr<<3) + ra];

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
		}
		ma+=30;
	}
	return 0;
}

/* F4 Character Displayer */
static const gfx_layout rx78_charlayout =
{
	8, 8,					/* 8 x 8 characters */
	187,					/* 187 characters */
	1,					/* 1 bits per pixel */
	{ 0 },					/* no bitplanes */
	/* x offsets */
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	/* y offsets */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8					/* every char takes 8 bytes */
};

static GFXDECODE_START( rx78 )
	GFXDECODE_ENTRY( "maincpu", 0x1a27, rx78_charlayout, 0, 1 )
GFXDECODE_END

static MACHINE_DRIVER_START( rx78 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)	// 4.1mhz
    MDRV_CPU_PROGRAM_MAP(rx78_mem)
    MDRV_CPU_IO_MAP(rx78_io)	

    MDRV_MACHINE_RESET(rx78)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(240, 184)
    MDRV_SCREEN_VISIBLE_AREA(0, 240-1, 0, 184-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)
	MDRV_GFXDECODE(rx78)

    MDRV_VIDEO_START(rx78)
    MDRV_VIDEO_UPDATE(rx78)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( rx78 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "ipl.rom", 0x0000, 0x2000, CRC(a194ea53) SHA1(ba39e73e6eb7cbb8906fff1f81a98964cd62af0d))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1983, rx78,  	0,       0, 		rx78, 	rx78, 	 0,  	  "Bandai",   "Gundam RX-78",		GAME_NOT_WORKING | GAME_NO_SOUND)

