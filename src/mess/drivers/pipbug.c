/***************************************************************************

        pipbug

        08/04/2010 Skeleton driver.

        PIPBUG isn't a computer; it is a the name of the bios used
        in a number of small 2650-based computers from 1976 to 1978.
        Examples include Baby 2650, Eurocard 2650, etc., plus Signetics
        own PC1001 and PC1500 systems. PIPBUG was written by Signetics.

        The sole means of communication is via a serial terminal.
        PIPBUG uses the SENSE and FLAG pins as serial lines, thus
        there is no need for a UART. The baud rate is 110.

        The Baby 2650 (featured in Electronics Australia magazine in
        March 1977) has 256 bytes of RAM.

        TODO
        - Connect to a terminal when MESS has the capability of a
             generic serial terminal.
        - After that, remove all the current video code (only there
             currently to prevent a crash).

****************************************************************************/

#include "emu.h"
#include "cpu/s2650/s2650.h"


class pipbug_state : public driver_device
{
public:
	pipbug_state(running_machine &machine, const driver_device_config_base &config)
		: driver_device(machine, config) { }

};

static WRITE8_HANDLER( pipbug_ctrl_w )
{
// 0x80 is written here - not connected in the baby 2650
}

static ADDRESS_MAP_START(pipbug_mem, AS_PROGRAM, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE( 0x0000, 0x03ff) AM_ROM
	AM_RANGE( 0x0400, 0x7fff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( pipbug_io, AS_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(S2650_CTRL_PORT, S2650_CTRL_PORT) AM_WRITE(pipbug_ctrl_w)
	//AM_RANGE(S2650_SENSE_PORT, S2650_FO_PORT) AM_READWRITE(pipbug_serial_in,pipbug_serial_out)
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( pipbug )
INPUT_PORTS_END


static MACHINE_RESET(pipbug)
{
}

static VIDEO_START( pipbug )
{
}

static SCREEN_UPDATE( pipbug )
{
	return 0;
}

static MACHINE_CONFIG_START( pipbug, pipbug_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu",S2650, XTAL_1MHz)
	MCFG_CPU_PROGRAM_MAP(pipbug_mem)
	MCFG_CPU_IO_MAP(pipbug_io)

	MCFG_MACHINE_RESET(pipbug)

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(50)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MCFG_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MCFG_SCREEN_SIZE(640, 480)
	MCFG_SCREEN_VISIBLE_AREA(0, 640-1, 0, 480-1)
	MCFG_SCREEN_UPDATE(pipbug)

	MCFG_PALETTE_LENGTH(2)
	MCFG_PALETTE_INIT(black_and_white)

	MCFG_VIDEO_START(pipbug)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( pipbug )
	ROM_REGION( 0x8000, "maincpu", ROMREGION_ERASEFF )
	ROM_LOAD( "pipbug.rom", 0x0000, 0x0400, CRC(f242b93e) SHA1(f82857cc882e6b5fc9f00b20b375988024f413ff))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY   FULLNAME       FLAGS */
COMP( 1979, pipbug,  0,       0,	pipbug, 	pipbug, 	 0,  "<unknown>",   "PIPBUG", GAME_NOT_WORKING | GAME_NO_SOUND_HW )

