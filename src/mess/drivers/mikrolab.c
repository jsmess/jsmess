/***************************************************************************
   
        Mikrolab KR580IK80

        10/12/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"

class mikrolab_state : public driver_device
{
public:
	mikrolab_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }
};

static ADDRESS_MAP_START(mikrolab_mem, ADDRESS_SPACE_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x05ff ) AM_ROM	// Monitor is up to 0x2ff
	AM_RANGE( 0x8000, 0x83ff ) AM_RAM	
ADDRESS_MAP_END

static ADDRESS_MAP_START( mikrolab_io , ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( mikrolab )
INPUT_PORTS_END


static MACHINE_RESET(mikrolab) 
{	
}

static VIDEO_START( mikrolab )
{
}

static VIDEO_UPDATE( mikrolab )
{
    return 0;
}

static MACHINE_CONFIG_START( mikrolab, mikrolab_state )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",I8080, XTAL_2MHz)
    MDRV_CPU_PROGRAM_MAP(mikrolab_mem)
    MDRV_CPU_IO_MAP(mikrolab_io)	

    MDRV_MACHINE_RESET(mikrolab)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(mikrolab)
    MDRV_VIDEO_UPDATE(mikrolab)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( mikrolab )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	/* these dumps are taken from PDF so need check with real device */
	ROM_LOAD( "rom-1.bin", 0x0000, 0x0200, BAD_DUMP CRC(eed5f23b) SHA1(c82f7a16ce44c4fcbcb333245555feae1fcdf058))
	ROM_LOAD( "rom-2.bin", 0x0200, 0x0200, BAD_DUMP CRC(726a224f) SHA1(7ed8d2c6dd4fb7836475e207e1972e33a6a91d2f))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT   COMPANY   FULLNAME       FLAGS */
COMP( ????, mikrolab,  0,       0, 	mikrolab, 	mikrolab, 	 0,	 "<unknown>",   "Mikrolab KR580IK80",		GAME_NOT_WORKING | GAME_NO_SOUND)

