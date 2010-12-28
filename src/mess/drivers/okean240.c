/***************************************************************************
   
        Okean-240

        28/12/2011 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"

class okean240_state : public driver_device
{
public:
	okean240_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }
};

static ADDRESS_MAP_START(okean240_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

static ADDRESS_MAP_START( okean240_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( okean240 )
INPUT_PORTS_END


static MACHINE_RESET(okean240) 
{	
}

static VIDEO_START( okean240 )
{
}

static VIDEO_UPDATE( okean240 )
{
    return 0;
}

static MACHINE_CONFIG_START( okean240, okean240_state )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",I8080, XTAL_12MHz / 6)
    MDRV_CPU_PROGRAM_MAP(okean240_mem)
    MDRV_CPU_IO_MAP(okean240_io)	

    MDRV_MACHINE_RESET(okean240)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(okean240)
    MDRV_VIDEO_UPDATE(okean240)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( okean240 )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "monitor.bin", 0xe000, 0x2000, CRC(bcac5ca0) SHA1(602ab824704d3d5d07b3787f6227ff903c33c9d5))
	ROM_LOAD( "cpm80.bin",   0xc000, 0x2000, CRC(b89a7e16) SHA1(b8f937c04f430be18e48f296ed3ef37194171204))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( ????, okean240,  0,       0, 	okean240, 	okean240, 	 0,  "<unknown>",   "Okean-240",		GAME_NOT_WORKING | GAME_NO_SOUND)

