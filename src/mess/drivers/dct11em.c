/***************************************************************************
   
        DEC DCT11-EM

        03/12/2010 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/t11/t11.h"

class dct11em_state : public driver_device
{
public:
	dct11em_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }
};

static ADDRESS_MAP_START(dct11em_mem, ADDRESS_SPACE_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x0fff ) AM_RAM  // RAM
	AM_RANGE( 0x2000, 0x2fff ) AM_RAM  // Optional RAM
	AM_RANGE( 0xa000, 0xdfff ) AM_ROM  // RAM
ADDRESS_MAP_END

/* Input ports */
INPUT_PORTS_START( dct11em )
INPUT_PORTS_END


static MACHINE_RESET(dct11em) 
{	
}

static VIDEO_START( dct11em )
{
}

static VIDEO_UPDATE( dct11em )
{
    return 0;
}

static const struct t11_setup t11_data =
{
	0x1403			/* according to specs */
};

static MACHINE_CONFIG_START( dct11em, dct11em_state )
    /* basic machine hardware */
    MDRV_CPU_ADD("maincpu",T11, 7500000) // 7.5MHz XTAL
	MDRV_CPU_CONFIG(t11_data)
    MDRV_CPU_PROGRAM_MAP(dct11em_mem)

    MDRV_MACHINE_RESET(dct11em)
	
    /* video hardware */
    MDRV_SCREEN_ADD("screen", RASTER)
    MDRV_SCREEN_REFRESH_RATE(50)
    MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MDRV_SCREEN_SIZE(640, 480)
    MDRV_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MDRV_PALETTE_LENGTH(2)
    MDRV_PALETTE_INIT(black_and_white)

    MDRV_VIDEO_START(dct11em)
    MDRV_VIDEO_UPDATE(dct11em)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( dct11em )
    ROM_REGION( 0x10000, "maincpu", ROMREGION_ERASEFF )
	// Highest address line inverted
	ROM_LOAD16_BYTE( "23-213e4.bin", 0x8000, 0x2000, CRC(bdd82f39) SHA1(347deeff77596b67eee27a39a9c40075fcf5c10d))
	ROM_LOAD16_BYTE( "23-214e4.bin", 0x8001, 0x2000, CRC(b523dae8) SHA1(cd1a64a2bce9730f7a9177d391663919c7f56073))
	ROM_COPY("maincpu", 0x8000, 0xc000, 0x2000)
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT     COMPANY   FULLNAME       FLAGS */
COMP( 1983, dct11em,  0,       0, 	dct11em, 	dct11em, 	 0,   "Digital Equipment Corporation",   "DCT11-EM",		GAME_NOT_WORKING | GAME_NO_SOUND)

