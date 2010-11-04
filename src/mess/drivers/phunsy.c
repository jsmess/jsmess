/***************************************************************************
   
        PHUNSY (Philipse Universal System)

        04/11/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/s2650/s2650.h"

static ADDRESS_MAP_START(phunsy_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x07ff) AM_ROM
	AM_RANGE( 0x0800, 0x0fff) AM_RAM
	AM_RANGE( 0x1000, 0x17ff) AM_RAM // Video RAM
	AM_RANGE( 0x1800, 0x1fff) AM_RAM // Banked ROM
	AM_RANGE( 0x4000, 0xffff) AM_RAM // Bankek RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( phunsy_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( phunsy )
INPUT_PORTS_END


static MACHINE_RESET(phunsy) 
{	
}

static VIDEO_START( phunsy )
{
}

static VIDEO_UPDATE( phunsy )
{
    return 0;
}

static MACHINE_CONFIG_START( phunsy, driver_device )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",S2650, XTAL_1MHz)
    MDRV_CPU_PROGRAM_MAP(phunsy_mem)
    MDRV_CPU_IO_MAP(phunsy_io)	

    MDRV_MACHINE_RESET(phunsy)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
	/* Display (page 12 of pdf)
	   - 8Mhz clock
	   - 64 6 pixel characters on a line.
	   - 16us not active, 48us active: ( 64 * 6 ) * 60 / 48 => 480 pixels wide
	   - 313 line display of which 256 are displayed.
	*/
	MDRV_SCREEN_RAW_PARAMS(XTAL_8MHz, 480, 0, 64*6, 313, 0, 256)
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(phunsy)
    MDRV_VIDEO_UPDATE(phunsy)
MACHINE_CONFIG_END


/* ROM definition */
ROM_START( phunsy )
	ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "phunsy_bios.bin", 0x0000, 0x0800, CRC(a789e82e) SHA1(b1c130ab2b3c139fd16ddc5dc7bdcaf7a9957d02))
	ROM_LOAD( "dass.bin",        0x0800, 0x0800, CRC(13380140) SHA1(a999201cb414abbf1e10a7fcc1789e3e000a5ef1))
	ROM_LOAD( "pdcr.bin",        0x1000, 0x0800, CRC(74bf9d0a) SHA1(8d2f673615215947f033571f1221c6aa99c537e9))
	ROM_LOAD( "labhnd.bin",      0x1800, 0x0800, CRC(1d5a106b) SHA1(a20d09e32e21cf14db8254cbdd1d691556b473f0))
	ROM_REGION( 0x0080, "gfx", ROMREGION_ERASEFF )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1980, phunsy,  0,       0, 	phunsy, 	phunsy, 	 0,  	   	 "J.F.P. Philipse",   "PHUNSY",		GAME_NOT_WORKING | GAME_NO_SOUND)

