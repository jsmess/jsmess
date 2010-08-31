/***************************************************************************
   
        mc-CP/M-Computer

        31/08/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

static UINT8 *mccpm_ram;

static ADDRESS_MAP_START(mccpm_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_RAM AM_BASE(&mccpm_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( mccpm_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	ADDRESS_MAP_GLOBAL_MASK(0xff)	
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( mccpm )
INPUT_PORTS_END


static MACHINE_RESET(mccpm) 
{	
	UINT8* bios = memory_region(machine, "maincpu");

	memcpy(mccpm_ram,bios, 0x1000);
}

static VIDEO_START( mccpm )
{
}

static VIDEO_UPDATE( mccpm )
{
    return 0;
}

static MACHINE_DRIVER_START( mccpm )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(mccpm_mem)
    MDRV_CPU_IO_MAP(mccpm_io)	

    MDRV_MACHINE_RESET(mccpm)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(mccpm)
    MDRV_VIDEO_UPDATE(mccpm)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( mccpm )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "mon36.j15", 0x0000, 0x1000, CRC(9c441537) SHA1(f95bad52d9392b8fc9d9b8779b7b861672a0022b))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1981, mccpm,  0,       0, 		mccpm, 	mccpm, 	 0,  	 "GRAF Elektronik Systeme GmbH",   "mc-CP/M-Computer",		GAME_NOT_WORKING | GAME_NO_SOUND)

