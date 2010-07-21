/***************************************************************************
   
        Japan Electronics College MYCOMZ-80A

        21/07/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

static ADDRESS_MAP_START(mycom_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xbfff) AM_RAM
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( mycom )
INPUT_PORTS_END


static MACHINE_RESET(mycom) 
{	
}

static VIDEO_START( mycom )
{
}

static VIDEO_UPDATE( mycom )
{
    return 0;
}

static MACHINE_DRIVER_START( mycom )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",Z80, XTAL_4MHz)
    MDRV_CPU_PROGRAM_MAP(mycom_mem)

    MDRV_MACHINE_RESET(mycom)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(mycom)
    MDRV_VIDEO_UPDATE(mycom)
MACHINE_DRIVER_END

/* ROM definition */
ROM_START( mycom )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )	
	ROM_LOAD( "bios.rom", 0xc000, 0x3000, CRC(e6f50355) SHA1(5d3acea360c0a8ab547db03a43e1bae5125f9c2a))
	ROM_LOAD( "basic.rom",0xf000, 0x1000, CRC(3b077465) SHA1(777427182627f371542c5e0521ed3ca1466a90e1))
	ROM_REGION( 0x0800, "gfx", ROMREGION_ERASEFF )
	ROM_LOAD( "font.rom", 0x0000, 0x0800, CRC(4039bb6f) SHA1(086ad303bf4bcf983fd6472577acbf744875fea8))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1981, mycom,  	0,       0, 		mycom, 	mycom, 	 0,  	   "Japan Electronics College",   "MYCOMZ-80A",		GAME_NOT_WORKING | GAME_NO_SOUND)

