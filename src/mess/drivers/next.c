/***************************************************************************

        NeXT

        05/11/2009 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"


class next_state : public driver_device
{
public:
	next_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};


static ADDRESS_MAP_START(next_mem, AS_PROGRAM, 32)
	AM_RANGE(0x00000000, 0x0001ffff) AM_ROM AM_REGION("user1", 0)
	AM_RANGE(0x01000000, 0x0101ffff) AM_ROM AM_REGION("user1", 0)
	AM_RANGE(0x02000000, 0x0200ffff) AM_RAM
	AM_RANGE(0x0b000000, 0x0b03ffff) AM_RAM
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( next )
INPUT_PORTS_END


static MACHINE_RESET(next)
{
}

static VIDEO_START( next )
{
}

static SCREEN_UPDATE( next )
{
    return 0;
}

static MACHINE_CONFIG_START( next, next_state )
    /* basic machine hardware */
    MCFG_CPU_ADD("maincpu",M68030, XTAL_25MHz)
    MCFG_CPU_PROGRAM_MAP(next_mem)

    MCFG_MACHINE_RESET(next)

    /* video hardware */
    MCFG_SCREEN_ADD("screen", RASTER)
    MCFG_SCREEN_REFRESH_RATE(50)
    MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
    MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
    MCFG_SCREEN_SIZE(640, 480)
    MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
    MCFG_SCREEN_UPDATE(next)

    MCFG_PALETTE_LENGTH(2)
    MCFG_PALETTE_INIT(black_and_white)

    MCFG_VIDEO_START(next)
MACHINE_CONFIG_END

static MACHINE_CONFIG_DERIVED( next040, next )
	MCFG_CPU_REPLACE("maincpu", M68040, XTAL_33MHz)
	MCFG_CPU_PROGRAM_MAP(next_mem)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( next )
    ROM_REGION32_BE( 0x20000, "user1", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "v12", "v1.2" )
	ROMX_LOAD( "rev_1.2.bin",     0x0000, 0x10000, CRC(7070bd78) SHA1(e34418423da61545157e36b084e2068ad41c9e24), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "v10", "v1.0 v41" )
	ROMX_LOAD( "rev_1.0_v41.bin", 0x0000, 0x10000, CRC(54df32b9) SHA1(06e3ecf09ab67a571186efd870e6b44028612371), ROM_BIOS(2))
ROM_END

ROM_START( nextnt )
	ROM_REGION32_BE( 0x20000, "user1", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "v25", "v2.5 v66" )
	ROMX_LOAD( "rev_2.5_v66.bin", 0x0000, 0x20000, CRC(f47e0bfe) SHA1(b3534796abae238a0111299fc406a9349f7fee24), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "v24", "v2.4 v65" )
	ROMX_LOAD( "rev_2.4_v65.bin", 0x0000, 0x20000, CRC(74e9e541) SHA1(67d195351288e90818336c3a84d55e6a070960d2), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "v21", "v2.1 v59" )
	ROMX_LOAD( "rev_2.1_v59.bin", 0x0000, 0x20000, CRC(f20ef956) SHA1(09586c6de1ca73995f8c9b99870ee3cc9990933a), ROM_BIOS(3))
ROM_END

ROM_START( nexttrb )
	ROM_REGION32_BE( 0x20000, "user1", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS( 0, "v33", "v3.3 v74" )
	ROMX_LOAD( "rev_3.3_v74.bin", 0x0000, 0x20000, CRC(fbc3a2cd) SHA1(a9bef655f26f97562de366e4a33bb462e764c929), ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "v32", "v3.2 v72" )
	ROMX_LOAD( "rev_3.2_v72.bin", 0x0000, 0x20000, CRC(e750184f) SHA1(ccebf03ed090a79c36f761265ead6cd66fb04329), ROM_BIOS(2))
	ROM_SYSTEM_BIOS( 2, "v30", "v3.0 v70" )
	ROMX_LOAD( "rev_3.0_v70.bin", 0x0000, 0x20000, CRC(37250453) SHA1(a7e42bd6a25c61903c8ca113d0b9a624325ee6cf), ROM_BIOS(3))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY                 FULLNAME                FLAGS */
COMP( 1987, next,   0,          0,   next,      next,    0,      "Next Software Inc",   "NeXT",				GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1990, nextnt, next,       0,   next040,   next,    0,      "Next Software Inc",   "NeXT (Non Turbo)",	GAME_NOT_WORKING | GAME_NO_SOUND)
COMP( 1992, nexttrb,next,       0,   next040,   next,    0,      "Next Software Inc",   "NeXT (Turbo)",		GAME_NOT_WORKING | GAME_NO_SOUND)

