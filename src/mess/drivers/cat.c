/***************************************************************************
   
        Canon Cat

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/m68000/m68000.h"

UINT16 *cat_video_ram;

static ADDRESS_MAP_START(cat_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x0003ffff) AM_ROM // 256 KB ROM
	AM_RANGE(0x00400000, 0x0045ffff) AM_RAM AM_BASE(&cat_video_ram) // 384 KB RAM
ADDRESS_MAP_END

UINT16 *swyft_video_ram;

static ADDRESS_MAP_START(swyft_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000000, 0x0000ffff) AM_ROM // 64 KB ROM
	AM_RANGE(0x00040000, 0x000fffff) AM_RAM AM_BASE(&swyft_video_ram) 
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( cat )
INPUT_PORTS_END

INPUT_PORTS_START( swyft )
INPUT_PORTS_END

static MACHINE_RESET(cat) 
{	
}

static VIDEO_START( cat )
{
}

static VIDEO_UPDATE( cat )
{
	UINT16 code;
	int y, x, b;
	
	int addr = 0;	
	for (y = 0; y < 344; y++)
	{
		int horpos = 0;
		for (x = 0; x < 42; x++)
		{			
			code = cat_video_ram[addr++];
			for (b = 15; b >= 0; b--)
			{				
				*BITMAP_ADDR16(bitmap, y, horpos++) =  (code >> b) & 0x01;
			}
		}
	}
	return 0;
}

static TIMER_CALLBACK( swyft_reset )
{
	memset (memory_get_read_ptr(cputag_get_address_space(machine, "maincpu", ADDRESS_SPACE_PROGRAM),  0xe2341 ),0xff,1);
}

static MACHINE_RESET(swyft) 
{
	timer_set(machine, ATTOTIME_IN_USEC(10), NULL, 0, swyft_reset);

}

static VIDEO_START( swyft )
{
}

static VIDEO_UPDATE( swyft )
{
	UINT16 code;
	int y, x, b;
	
	int addr = 0;	
	for (y = 0; y < 242; y++)
	{
		int horpos = 0;
		for (x = 0; x < 20; x++)
		{			
			code = swyft_video_ram[addr++];
			for (b = 15; b >= 0; b--)
			{				
				*BITMAP_ADDR16(bitmap, y, horpos++) =  (code >> b) & 0x01;
			}
		}
	}
	return 0;
}


static MACHINE_DRIVER_START( cat )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M68000, XTAL_5MHz)
    MDRV_CPU_PROGRAM_MAP(cat_mem)

    MDRV_MACHINE_RESET(cat)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(672, 344)
    MDRV_SCREEN_VISIBLE_AREA(0, 672-1, 0, 344-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(cat)
    MDRV_VIDEO_UPDATE(cat)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( swyft )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",M68000, XTAL_5MHz)
    MDRV_CPU_PROGRAM_MAP(swyft_mem)

    MDRV_MACHINE_RESET(swyft)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(320, 242)
    MDRV_SCREEN_VISIBLE_AREA(0, 320-1, 0, 242-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(swyft)
    MDRV_VIDEO_UPDATE(swyft)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( swyft )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "infoapp.lo", 0x0000, 0x8000, CRC(52c1bd66) SHA1(b3266d72970f9d64d94d405965b694f5dcb23bca))
	ROM_LOAD( "infoapp.hi", 0x8000, 0x8000, CRC(83505015) SHA1(693c914819dd171114a8c408f399b56b470f6be0))
ROM_END

ROM_START( cat )
    ROM_REGION( 0x40000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD16_BYTE( "r240l0.bin", 0x00001, 0x10000, CRC(1b89bdc4) SHA1(39c639587dc30f9d6636b46d0465f06272838432))
	ROM_LOAD16_BYTE( "r240h0.bin", 0x00000, 0x10000, CRC(94f89b8c) SHA1(6c336bc30636a02c625d31f3057ec86bf4d155fc))
	ROM_LOAD16_BYTE( "r240l1.bin", 0x20001, 0x10000, CRC(1a73be4f) SHA1(e2de2cb485f78963368fb8ceba8fb66ca56dba34))
	ROM_LOAD16_BYTE( "r240h1.bin", 0x20000, 0x10000, CRC(898dd9f6) SHA1(93e791dd4ed7e4afa47a04df6fdde359e41c2075))
ROM_END

/* Driver */

/*    YEAR  NAME  PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1985, swyft,0,       0, 		swyft, 		swyft, 	 0,  	  0,  	 "Information Applicance Inc.",   "Swyft",		GAME_NOT_WORKING)
COMP( 1987, cat,  swyft,   0, 		cat, 		cat, 	 0,  	  0,  	 "Canon",   "Cat",		GAME_NOT_WORKING)

