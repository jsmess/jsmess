/***************************************************************************
   
        Bondwell 12/14

        12/05/2009 Skeleton driver.

****************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"

static UINT8 *bw_videoram;

static ADDRESS_MAP_START(bw12_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x1000, 0xf7ff) AM_RAM
	AM_RANGE(0xf800, 0xffff) AM_RAM AM_BASE(&bw_videoram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( bw12_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( bw12 )
INPUT_PORTS_END


static MACHINE_RESET(bw12) 
{	
}

static VIDEO_START( bw12 )
{
}

static VIDEO_UPDATE( bw12 )
{
	UINT8 *gfx = memory_region(screen->machine, "gfx1");
	int x,y,b;

	for(y = 0; y < 25*8; y++ )
	{
		for(x = 0; x < 80; x++ )
		{
			int addr = x + (y / 8)*80;
			UINT8 code = gfx[bw_videoram [addr]*16+ (y % 8)];
			for (b = 7; b >= 0; b--)
			{								
				UINT8 col = (code >> b) & 0x01;
				*BITMAP_ADDR16(bitmap, y, x*8+(7-b)) =  col;
			}
		}
	}

    return 0;
}

static MACHINE_DRIVER_START( bw12 )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(bw12_mem)
    MDRV_CPU_IO_MAP(bw12_io)	

    MDRV_MACHINE_RESET(bw12)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(80*8, 25*8)
    MDRV_SCREEN_VISIBLE_AREA(0, 80*8-1, 0, 25*8-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(bw12)
    MDRV_VIDEO_UPDATE(bw12)
MACHINE_DRIVER_END

static SYSTEM_CONFIG_START(bw12)
	CONFIG_RAM_DEFAULT(64 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START(bw14)
	CONFIG_RAM_DEFAULT(128 * 1024)
SYSTEM_CONFIG_END

/* ROM definition */
ROM_START( bw12 )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "bw14boot.bin", 0x0000, 0x1000, CRC(782fe341) SHA1(eefe5ad6b1ef77a1caf0af743b74de5fa1c4c19d))
	ROM_REGION(0x1000, "gfx1",0)
	ROM_LOAD( "bw14char.bin", 0x0000, 0x1000, CRC(f9dd68b5) SHA1(50132b759a6d84c22c387c39c0f57535cd380411))
ROM_END

#define rom_bw14 rom_bw12
/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    CONFIG COMPANY   FULLNAME       FLAGS */
COMP( 1984, bw12,  0,       0, 	bw12, 	bw12, 	 0,  	  bw12,  	 "Bondwell",   "BW 12",		GAME_NOT_WORKING)
COMP( 1984, bw14,  bw12,    0, 	bw12, 	bw12, 	 0,  	  bw14,  	 "Bondwell",   "BW 14",		GAME_NOT_WORKING)
