/***************************************************************************
   
        VTA-2000 Terminal
		
			board images : http://fotki.yandex.ru/users/lodedome/album/93699?p=0
			
		BDP-15 board only

        29/11/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"

class vta2000_state : public driver_device
{
public:
	vta2000_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};

static ADDRESS_MAP_START(vta2000_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
    AM_RANGE( 0x0000, 0x5fff ) AM_ROM
    AM_RANGE( 0x6000, 0xffff ) AM_RAM 
ADDRESS_MAP_END

static ADDRESS_MAP_START( vta2000_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( vta2000 )
INPUT_PORTS_END


static MACHINE_RESET(vta2000) 
{	
}

static VIDEO_START( vta2000 )
{
}

static VIDEO_UPDATE( vta2000 )
{
    return 0;
}

static MACHINE_CONFIG_START( vta2000, vta2000_state )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",I8080, XTAL_4MHz / 4)
    MDRV_CPU_PROGRAM_MAP(vta2000_mem)
    MDRV_CPU_IO_MAP(vta2000_io)	

    MDRV_MACHINE_RESET(vta2000)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(vta2000)
    MDRV_VIDEO_UPDATE(vta2000)
MACHINE_CONFIG_END


/* ROM definition */
ROM_START( vta2000 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "bdp-15_11.rom", 0x4000, 0x2000, CRC(d4abe3e9) SHA1(ab1973306e263b0f66f2e1ede50cb5230f8d69d5))
	ROM_LOAD( "bdp-15_12.rom", 0x2000, 0x2000, CRC(4a5fe332) SHA1(f1401c26687236184fec0558cc890e796d7d5c77))
	ROM_LOAD( "bdp-15_13.rom", 0x0000, 0x2000, CRC(b6b89d90) SHA1(0356d7ba77013b8a79986689fb22ef4107ef885b))
	ROM_REGION(0x2000, "gfx1",0)
	ROM_LOAD( "bdp-15_14.rom", 0x0000, 0x2000, CRC(a1dc4f8e) SHA1(873fd211f44713b713d73163de2d8b5db83d2143))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( ????, vta2000,  0,       0, 	vta2000, 	vta2000, 	 0,  	  "<unknown>",   "VTA-2000",		GAME_NOT_WORKING | GAME_NO_SOUND)

