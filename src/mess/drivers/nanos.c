/***************************************************************************
   
        Nanos

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static const UINT8 *FNT;

static READ8_HANDLER( nanos_videoram_r )
{
	return videoram[offset];
}

static WRITE8_HANDLER( nanos_videoram_w )
{
	videoram[offset] = data;
}

static ADDRESS_MAP_START(nanos_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x0fff ) AM_ROM
	AM_RANGE( 0x1000, 0xf7ff ) AM_RAM
	AM_RANGE( 0xf800, 0xffcf ) AM_READWRITE(nanos_videoram_r,nanos_videoram_w) AM_SIZE(&videoram_size) AM_BASE(&videoram)
	AM_RANGE( 0xffd0, 0xffff ) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( nanos_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( nanos )
INPUT_PORTS_END


static MACHINE_RESET(nanos) 
{	
}

static VIDEO_START( nanos )
{
	FNT = memory_region(machine, "gfx1");
}

static VIDEO_UPDATE( nanos )
{
//	static UINT8 framecnt=0;
	UINT8 y,ra,chr,gfx;
	UINT16 sy=0,ma=0,x;

//	framecnt++;

	for (y = 0; y < 25; y++)
	{
		for (ra = 0; ra < 10; ra++)
		{
			UINT16  *p = BITMAP_ADDR16(bitmap, sy++, 0);

			for (x = ma; x < ma + 80; x++)
			{
				if (ra < 8)
				{
					chr = videoram[x];

					/* get pattern of pixels for that character scanline */
					gfx = FNT[(chr<<3) | ra ];
				}
				else
					gfx = 0;

				/* Display a scanline of a character (8 pixels) */
				*p = ( gfx & 0x80 ) ? 1 : 0; p++;
				*p = ( gfx & 0x40 ) ? 1 : 0; p++;
				*p = ( gfx & 0x20 ) ? 1 : 0; p++;
				*p = ( gfx & 0x10 ) ? 1 : 0; p++;
				*p = ( gfx & 0x08 ) ? 1 : 0; p++;
				*p = ( gfx & 0x04 ) ? 1 : 0; p++;
				*p = ( gfx & 0x02 ) ? 1 : 0; p++;
				*p = ( gfx & 0x01 ) ? 1 : 0; p++;
			}
		}
		ma+=80;
	}
	return 0;
}

static MACHINE_DRIVER_START( nanos )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(nanos_mem)
    MDRV_CPU_IO_MAP(nanos_io)	

    MDRV_MACHINE_RESET(nanos)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(80*8, 25*10)
	MDRV_SCREEN_VISIBLE_AREA(0,80*8-1,0,25*10-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(nanos)
    MDRV_VIDEO_UPDATE(nanos)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(nanos)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( nanos )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "k7634_1.rom", 0x0000, 0x0800, CRC(8e34e6ac) SHA1(fd342f6effe991823c2a310737fbfcba213c4fe3))
	ROM_LOAD( "k7634_2.rom", 0x0800, 0x0180, CRC(4e01b02b) SHA1(8a279da886555c7470a1afcbb3a99693ea13c237))

	ROM_REGION( 0x0800, "gfx1", 0 )
	ROM_LOAD( "zg_nanos.rom", 0x0000, 0x0800, CRC(5682d3f9) SHA1(5b738972c815757821c050ee38b002654f8da163))

ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( ????, nanos,  0,       0, 	nanos, 	nanos, 	 0,  	  nanos,  	 "Ingenieurhochschule fur Seefahrt Warnemunde/Wustrow",   "Nanos",		GAME_NOT_WORKING)

