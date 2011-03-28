/***************************************************************************

    Skeleton driver for Applix 1616 computer
 
    See for docs: http;//www.microbee-mspp.org.au
    You need to sign up and make an introductory thread.
    Then you will be granted permission to visit the repository.
 
	TODO: everything!

****************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"


class applix_state : public driver_device
{
public:
	applix_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static ADDRESS_MAP_START(applix_mem, AS_PROGRAM, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x000000, 0x4fffff) AM_RAM
	AM_RANGE(0x500000, 0x5fffff) AM_ROM
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( applix )
INPUT_PORTS_END


static MACHINE_RESET(applix)
{
	//applix_state *state = machine->driver_data<applix_state>();
	UINT8* RAM = machine->region("maincpu")->base();
	memcpy(RAM, RAM+0x500000, 8);
	//machine->device("maincpu")->reset();
}

static VIDEO_START( applix )
{
}

static SCREEN_UPDATE( applix )
{
	return 0;
}

static MACHINE_CONFIG_START( applix, applix_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68000, 7500000)
	MCFG_CPU_PROGRAM_MAP(applix_mem)

	MCFG_MACHINE_RESET(applix)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_SCREEN_UPDATE(applix)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	MCFG_VIDEO_START(applix)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( applix )
	ROM_REGION16_BE(0x1000000, "maincpu", 0)
	ROM_LOAD16_BYTE( "1616oslv.044", 0x500000, 0x10000, CRC(ef619994) SHA1(ff16fe9e2c99a1ffc855baf89278a97a2a2e881a) )
	ROM_LOAD16_BYTE( "1616oshv.044", 0x500001, 0x10000, CRC(4a1a90d3) SHA1(4df504bbf6fc5dad76c29e9657bfa556500420a6) )
	ROM_LOAD16_BYTE( "1616oslv.045", 0x520000, 0x10000, CRC(951bd441) SHA1(e0a38c8d0d38d84955c1de3f6a7d56ce06b063f6) )
	ROM_LOAD16_BYTE( "1616oshv.045", 0x520001, 0x10000, CRC(9dfb3224) SHA1(5223833a357f90b147f25826c01713269fc1945f) )
	ROM_LOAD( "1616osv.045",  0x540000, 0x20000, CRC(b9f75432) SHA1(278964e2a02b1fe26ff34f09dc040e03c1d81a6d) )
	ROM_LOAD( "1616ssdv.022", 0x560000, 0x08000, CRC(6d8e413a) SHA1(fc27d92c34f231345a387b06670f36f8c1705856) )
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1993, applix,	0,       0, 	applix,	applix,	 0, 	  "Applix Pty Ltd",   "Applix 1616", GAME_NOT_WORKING | GAME_NO_SOUND)
